import os
import sys
import time
import math
import spidev
import evdev
import threading
import RPi.GPIO as GPIO
import usb4vc_ui
from usb4vc_shared import *
import usb4vc_gamepads

SPI_BUF_INDEX_MAGIC = 0
SPI_BUF_INDEX_SEQNUM = 1
SPI_BUF_INDEX_MSG_TYPE = 2

SPI_MOSI_MSG_TYPE_NOP = 0
SPI_MOSI_MSG_TYPE_INFO_REQUEST = 1
SPI_MOSI_MSG_TYPE_SET_PROTOCOL = 2
SPI_MOSI_MSG_TYPE_REQ_ACK = 3

SPI_MOSI_MSG_TYPE_KEYBOARD_EVENT = 8
SPI_MOSI_MSG_TYPE_MOUSE_EVENT = 9
SPI_MOSI_MSG_TYPE_GAMEPAD_EVENT_RAW_UNKNOWN = 10
SPI_MOSI_MSG_TYPE_GAMEPAD_EVENT_MAPPED = 11
SPI_MOSI_MSG_TYPE_GAMEPAD_EVENT_RAW_SUPPORTED = 12

SPI_MISO_MSG_TYPE_NOP = 0
SPI_MISO_MSG_TYPE_INFO_REQUEST = 128
SPI_MISO_MSG_TYPE_KB_LED_REQUEST = 129

SPI_MOSI_MAGIC = 0xde
SPI_MISO_MAGIC = 0xcd

PCARD_BUSY_PIN = 20
SLAVE_REQ_PIN = 16

GPIO.setmode(GPIO.BCM)
GPIO.setup(SLAVE_REQ_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
GPIO.add_event_detect(SLAVE_REQ_PIN, GPIO.RISING)

GPIO.setup(PCARD_BUSY_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

pcard_spi = spidev.SpiDev(0, 0)
pcard_spi.max_speed_hz = 2000000

opened_device_dict = {}

led_device_path = '/sys/class/leds'
input_device_path = '/dev/input/by-path/'


SPI_XFER_TIMEOUT = 0.025
def xfer_when_not_busy(data, drop=False):
    start_ts = time.time()
    while GPIO.input(PCARD_BUSY_PIN):
        # print(time.time(), "P-Card is busy!")
        if drop:
            return None
        if time.time() - start_ts > SPI_XFER_TIMEOUT:
            break
    return pcard_spi.xfer(data)

def is_gamepad_button(event_code):
    name_result = code_value_to_name_lookup.get(event_code)
    if name_result is None:
        return False
    for item in name_result:
        if item in gamepad_event_code_name_list:
            return True
    return False

nop_spi_msg_template = [SPI_MOSI_MAGIC] + [0]*31
info_request_spi_msg_template = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_INFO_REQUEST] + [0]*29
keyboard_event_spi_msg_template = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_KEYBOARD_EVENT] + [0]*29
mouse_event_spi_msg_template = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_MOUSE_EVENT] + [0]*29
gamepad_event_ibm_ggp_spi_msg_template = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_GAMEPAD_EVENT_MAPPED, 0, 0, 0, 0, 0, 127, 127, 127, 127] + [0]*20
raw_usb_unknown_gamepad_event_spi_msg_template = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_GAMEPAD_EVENT_RAW_UNKNOWN] + [0]*7 + [127]*8 + [0]*14
raw_usb_supported_gamepad_event_spi_msg_template = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_GAMEPAD_EVENT_RAW_SUPPORTED] + [0]*7 + [127]*4 + [0, 0, 127, 127] + [0]*14

def make_spi_msg_ack():
    return [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_REQ_ACK] + [0]*29

def make_keyboard_spi_packet(input_data, kbd_id):
    result = list(keyboard_event_spi_msg_template)
    result[3] = kbd_id
    result[4] = input_data[2]
    result[5] = input_data[3]
    result[6] = input_data[4]
    return result

def make_mouse_spi_packet(mouse_dict, mouse_id):
    to_transfer = list(mouse_event_spi_msg_template)
    to_transfer[3] = mouse_id
    to_transfer[4:6] = mouse_dict['x']
    to_transfer[6:8] = mouse_dict['y']
    to_transfer[8] = mouse_dict['scroll']
    to_transfer[9] = mouse_dict['hscroll']
    to_transfer[13] = mouse_dict[BTN_LEFT]
    to_transfer[14] = mouse_dict[BTN_RIGHT]
    to_transfer[15] = mouse_dict[BTN_MIDDLE]
    to_transfer[16] = mouse_dict[BTN_SIDE]
    to_transfer[17] = mouse_dict[BTN_EXTRA]
    return to_transfer

PID_PROTOCOL_OFF = 0
PID_GENERIC_GAMEPORT_GAMEPAD = 7
PID_BBC_MICRO_JOYSTICK = 14
PID_RAW_USB_GAMEPAD = 127

devices = [evdev.InputDevice(path) for path in evdev.list_devices()]

for device in devices:
    print(device.path, device.name, device.phys)

# import asyncio, evdev

# mouse = evdev.InputDevice('/dev/input/event0')
# keybd = evdev.InputDevice('/dev/input/event6')


# async def print_events(device):
#     async for event in device.async_read_loop():
#         empty = list(keyboard_event_spi_msg_template)
#         xfer_when_not_busy(empty)
#         print(device.path, evdev.categorize(event), sep=': ')

# for device in mouse, keybd:
#     asyncio.ensure_future(print_events(device))

# loop = asyncio.get_event_loop()
# loop.run_forever()
import os
import sys
import time
import spidev
import threading
import RPi.GPIO as GPIO

from usb4vc_oled import oled_display_queue

"""
sudo apt install stm32flash

OPTIONAL
git clone https://git.code.sf.net/p/stm32flash/code stm32flash-code
cd stm32flash-code
sudo make install

HAVE TO ASSERT BOOT0 THE WHOLE TIME
/usr/local/bin/stm32flash -r hhh -a 0x3b /dev/i2c-1

"""

SLAVE_REQ_PIN = 16
GPIO.setmode(GPIO.BCM)
GPIO.setup(SLAVE_REQ_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
GPIO.add_event_detect(SLAVE_REQ_PIN, GPIO.RISING)

pcard_spi = spidev.SpiDev(0, 0)
pcard_spi.max_speed_hz = 2000000

keyboard_opened_device_dict = {}
mouse_opened_device_dict = {}
gamepad_opened_device_dict = {}

led_device_path = '/sys/class/leds'
input_device_path = '/dev/input/by-path/'

"""
https://elixir.bootlin.com/linux/latest/source/include/uapi/linux/input-event-codes.h#L38

xbox gamepad:
EVENT TYPE: EV_ABS
dpad: ABS_HAT0X ABS_HAT1X
buttons: https://elixir.bootlin.com/linux/latest/source/include/uapi/linux/input-event-codes.h#L385
sticks: ABS_X, ABS_Y, ABS_RX, ABS_RY
"""

# https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h
EV_SYN = 0
EV_KEY = 1
EV_REL = 2
EV_ABS = 3

SYN_REPORT = 0

REL_X = 0x00
REL_Y = 0x01
REL_WHEEL = 0x08

SPI_BUF_INDEX_MAGIC = 0
SPI_BUF_INDEX_SEQNUM = 1
SPI_BUF_INDEX_MSG_TYPE = 2

SPI_MOSI_MSG_KEYBOARD_EVENT = 1
SPI_MOSI_MSG_MOUSE_EVENT = 2
SPI_MOSI_MSG_GAMEPAD_EVENT = 3
SPI_MOSI_MSG_REQ_ACK = 4

SPI_MISO_MSG_INFO_REPLY = 0
SPI_MISO_MSG_KB_LED_REQ = 1

SPI_MOSI_MAGIC = 0xde
SPI_MISO_MAGIC = 0xcd

keyboard_spi_msg_header = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_KEYBOARD_EVENT, 0]
mouse_spi_msg_template = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_MOUSE_EVENT, 0] + [0]*28

def make_spi_msg_ack():
    return [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_REQ_ACK] + [0]*29

def get_01(value):
    if value:
        return 1
    return 0

def change_kb_led(ps2kb_led_byte):
    led_file_list = os.listdir(led_device_path)
    capslock_list = [os.path.join(led_device_path, x) for x in led_file_list if 'capslock' in x]
    numlock_list = [os.path.join(led_device_path, x) for x in led_file_list if 'numlock' in x]
    scrolllock_list = [os.path.join(led_device_path, x) for x in led_file_list if 'scrolllock' in x]

    for item in scrolllock_list:
        os.system(f"sudo bash -c 'echo {get_01(ps2kb_led_byte & 0x1)} > {os.path.join(item, 'brightness')}'")

    for item in numlock_list:
        os.system(f"sudo bash -c 'echo {get_01(ps2kb_led_byte & 0x2)} > {os.path.join(item, 'brightness')}'")

    for item in capslock_list:
        os.system(f"sudo bash -c 'echo {get_01(ps2kb_led_byte & 0x4)} > {os.path.join(item, 'brightness')}'")

def raw_input_event_worker():
    mouse_spi_packet_dict = {}
    mouse_button_state_list = [0] * 5
    print("raw_input_event_worker started")
    while 1:
        for key in list(keyboard_opened_device_dict):
            try:
                data = keyboard_opened_device_dict[key][0].read(16)
            except OSError:
                keyboard_opened_device_dict[key][0].close()
                del keyboard_opened_device_dict[key]
                print("keyboard disappeared:", key)
                continue
            if data is None:
                continue
            data = list(data[8:])
            if data[0] == EV_KEY:
                to_transfer = keyboard_spi_msg_header + data + [0]*20
                to_transfer[3] = keyboard_opened_device_dict[key][1]
                pcard_spi.xfer(to_transfer)

        for key in list(mouse_opened_device_dict):
            try:
                data = mouse_opened_device_dict[key][0].read(16)
            except OSError:
                mouse_opened_device_dict[key][0].close()
                del mouse_opened_device_dict[key]
                print("mouse disappeared:", key)
                continue
            if data is None:
                continue
            data = list(data[8:])
            # print(data)
            if data[0] == EV_REL:
                if data[2] == REL_X:
                    mouse_spi_packet_dict["x"] = data[4:6]
                if data[2] == REL_Y:
                    mouse_spi_packet_dict["y"] = data[4:6]
                if data[2] == REL_WHEEL:
                    mouse_spi_packet_dict["scroll"] = data[4:6]
            if data[0] == EV_KEY:
                mouse_button_state_list[data[2]-16] = data[4]
                to_transfer = list(mouse_spi_msg_template)
                to_transfer[10:13] = data[2:5]
                to_transfer[13:18] = mouse_button_state_list[:]
                to_transfer[3] = mouse_opened_device_dict[key][1]
                pcard_spi.xfer(to_transfer)

            if data[0] == EV_SYN and data[2] == SYN_REPORT and len(mouse_spi_packet_dict) > 0:
                to_transfer = list(mouse_spi_msg_template)
                if 'x' in mouse_spi_packet_dict:
                    to_transfer[4:6] = mouse_spi_packet_dict['x']
                if 'y' in mouse_spi_packet_dict:
                    to_transfer[6:8] = mouse_spi_packet_dict['y']
                if 'scroll' in mouse_spi_packet_dict:
                    to_transfer[8:10] = mouse_spi_packet_dict['scroll']
                to_transfer[13:18] = mouse_button_state_list[:]
                to_transfer[3] = mouse_opened_device_dict[key][1]
                mouse_spi_packet_dict.clear()
                pcard_spi.xfer(to_transfer)

        if GPIO.event_detected(SLAVE_REQ_PIN):
            slave_result = None
            for x in range(2):
                slave_result = pcard_spi.xfer(make_spi_msg_ack())
            print(slave_result)
            if slave_result[SPI_BUF_INDEX_MAGIC] == SPI_MISO_MAGIC and slave_result[SPI_BUF_INDEX_MSG_TYPE] == SPI_MISO_MSG_KB_LED_REQ:
                change_kb_led(slave_result[3])
                change_kb_led(slave_result[3])

def usb_device_scan_worker():
    print("usb_device_scan_worker started")
    while 1:
        time.sleep(0.75)
        try:
            device_file_list = os.listdir(input_device_path)
        except FileNotFoundError:
            print("No input devices found")
            oled_display_queue.put((0, 'no input'))
            continue
        except Exception as e:
            print('list input device exception:', e)
            continue
        mouse_list = [os.path.join(input_device_path, x) for x in device_file_list if 'event-mouse' in x]
        keyboard_list = [os.path.join(input_device_path, x) for x in device_file_list if 'event-kbd' in x]
        gamepad_list = [os.path.join(input_device_path, x) for x in device_file_list if 'event-joystick' in x]

        for item in keyboard_list:
            if item not in keyboard_opened_device_dict:
                try:
                    this_file = open(item, "rb")
                    os.set_blocking(this_file.fileno(), False)
                    device_index = 0
                    try:
                        device_index = sum([int(x) for x in item.split(':')[1].split('.')][:2])
                    except:
                        pass
                    keyboard_opened_device_dict[item] = (this_file, device_index)
                    print("opened keyboard", keyboard_opened_device_dict[item][1], ':' , item)
                except Exception as e:
                    print("keyboard open exception:", e)
                    continue

        for item in mouse_list:
            if item not in mouse_opened_device_dict:
                try:
                    this_file = open(item, "rb")
                    os.set_blocking(this_file.fileno(), False)
                    device_index = 0
                    try:
                        device_index = sum([int(x) for x in item.split(':')[1].split('.')][:2])
                    except:
                        pass
                    mouse_opened_device_dict[item] = (this_file, device_index)
                    print("opened mouse", mouse_opened_device_dict[item][1], ':' , item)
                except Exception as e:
                    print("mouse open exception:", e)
                    continue

raw_input_event_parser_thread = threading.Thread(target=raw_input_event_worker, daemon=True)
usb_device_scan_thread = threading.Thread(target=usb_device_scan_worker, daemon=True)

# raw_input_event_parser_thread.start()
# usb_device_scan_thread.start()

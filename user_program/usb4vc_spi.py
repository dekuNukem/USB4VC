import os
import sys
import time
import spidev
import RPi.GPIO as GPIO
from usb4vc_shared import *

PCARD_BUSY_PIN = 20
SLAVE_REQ_PIN = 16
pcard_spi = None

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

set_protocl_spi_msg_template = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_SET_PROTOCOL] + [0]*29

nop_spi_msg_template = [SPI_MOSI_MAGIC] + [0]*31
info_request_spi_msg_template = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_INFO_REQUEST] + [0]*29
keyboard_event_spi_msg_template = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_KEYBOARD_EVENT] + [0]*29
mouse_event_spi_msg_template = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_MOUSE_EVENT] + [0]*29
gamepad_event_ibm_ggp_spi_msg_template = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_GAMEPAD_EVENT_MAPPED, 0, 0, 0, 0, 0, 127, 127, 127, 127] + [0]*20
raw_usb_unknown_gamepad_event_spi_msg_template = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_GAMEPAD_EVENT_RAW_UNKNOWN] + [0]*7 + [127]*8 + [0]*14
raw_usb_supported_gamepad_event_spi_msg_template = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_GAMEPAD_EVENT_RAW_SUPPORTED] + [0]*7 + [127]*4 + [0, 0, 127, 127] + [0]*14

def spi_init():
    global pcard_spi
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(SLAVE_REQ_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
    GPIO.add_event_detect(SLAVE_REQ_PIN, GPIO.RISING)
    GPIO.setup(PCARD_BUSY_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
    pcard_spi = spidev.SpiDev(0, 0)
    pcard_spi.max_speed_hz = 2000000

SPI_XFER_TIMEOUT = 0.025
def xfer_when_not_busy(data, dont_wait=False, timeout_sec=SPI_XFER_TIMEOUT):
    start_ts = time.time()
    while GPIO.input(PCARD_BUSY_PIN):
        # print(time.time(), "P-Card is busy!")
        if dont_wait:
            return None
        if time.time() - start_ts > timeout_sec:
            break
    return pcard_spi.xfer(data)

def get_pboard_info():
    # send request
    this_msg = list(info_request_spi_msg_template)
    this_msg[3] = RPI_APP_VERSION_TUPLE[0]
    this_msg[4] = RPI_APP_VERSION_TUPLE[1]
    this_msg[5] = RPI_APP_VERSION_TUPLE[2]
    xfer_when_not_busy(this_msg)

    time.sleep(0.05)
    # send an empty message to allow response to be shifted into RPi
    response = xfer_when_not_busy(list(nop_spi_msg_template))
    time.sleep(0.05)
    response = xfer_when_not_busy(list(nop_spi_msg_template))
    return response

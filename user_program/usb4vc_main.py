import os
import sys
import time
import RPi.GPIO as GPIO

GPIO.setmode(GPIO.BCM)

import usb4vc_shared
import usb4vc_usb_scan
import usb4vc_ui

PBOARD_RESET_PIN = 25
PBOARD_DFU_PIN = 12

def reset_pboard():
    print("resetting protocol board...")
    GPIO.setup(PBOARD_RESET_PIN, GPIO.OUT)
    GPIO.output(PBOARD_RESET_PIN, GPIO.LOW)
    time.sleep(0.05)
    GPIO.setup(PBOARD_RESET_PIN, GPIO.IN)
    time.sleep(0.05)
    print("done")

GPIO.setup(PBOARD_RESET_PIN, GPIO.IN)
GPIO.setup(PBOARD_DFU_PIN, GPIO.OUT)
GPIO.output(PBOARD_DFU_PIN, GPIO.LOW)

reset_pboard()

usb4vc_ui.ui_init()

usb4vc_ui.ui_worker.start()

usb4vc_usb_scan.usb_device_scan_thread.start()
usb4vc_usb_scan.raw_input_event_parser_thread.start()

while 1:
    time.sleep(10)
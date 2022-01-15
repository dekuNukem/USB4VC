import os
import sys
import time
import RPi.GPIO as GPIO
import usb4vc_shared
import usb4vc_usb_scan
import usb4vc_ui

# usb4vc_ui.reset_pboard()

os.system('sudo bash -c "echo 1 > /sys/module/bluetooth/parameters/disable_ertm"')

usb4vc_ui.ui_init()
usb4vc_ui.ui_thread.start()

usb4vc_usb_scan.usb_device_scan_thread.start()
usb4vc_usb_scan.raw_input_event_parser_thread.start()

while 1:
    time.sleep(10)
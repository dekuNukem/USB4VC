import os
import sys
import time

import usb4vc_usb_scan
# import usb4vc_oled

usb4vc_usb_scan.usb_device_scan_thread.start()
usb4vc_usb_scan.raw_input_event_parser_thread.start()

while 1:
    time.sleep(10)
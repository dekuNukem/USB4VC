import os
import sys
import time

import usb4vc_usb_scan
import usb4vc_oled

usb4vc_oled.print_welcome_screen((0, 2, 3))
time.sleep(1)
usb4vc_oled.oled_clear()

usb4vc_usb_scan.usb_device_scan_thread.start()
usb4vc_usb_scan.raw_input_event_parser_thread.start()

while 1:
    time.sleep(10)
import os
import sys
import time

import usb4vc_usb_scan
import usb4vc_oled

RPI_APP_VERSION_TUPLE = (0, 0, 1)

usb4vc_oled.print_welcome_screen(RPI_APP_VERSION_TUPLE)
# time.sleep(5)
# usb4vc_oled.oled_clear()

usb4vc_oled.oled_queue_worker.start()
usb4vc_usb_scan.usb_device_scan_thread.start()
usb4vc_usb_scan.raw_input_event_parser_thread.start()

while 1:
    time.sleep(10)
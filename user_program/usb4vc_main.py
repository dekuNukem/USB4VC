import os
import sys
import time
import RPi.GPIO as GPIO
import usb4vc_shared
import usb4vc_usb_scan
import usb4vc_ui
import subprocess 

# usb4vc_ui.reset_pboard()

usb4vc_ui.ui_init()
usb4vc_ui.ui_thread.start()

usb4vc_usb_scan.usb_device_scan_thread.start()
usb4vc_usb_scan.raw_input_event_parser_thread.start()

while 1:
    ertm_status = subprocess.getoutput(f"cat /sys/module/bluetooth/parameters/disable_ertm").strip().replace('\n', '').replace('\r', '')
    if ertm_status != 'Y':
        print('ertm_status:', ertm_status)
        print("Disabling ERTM....")
        subprocess.call('echo 1 > /sys/module/bluetooth/parameters/disable_ertm', shell=True)
        print("DONE")
    time.sleep(2)
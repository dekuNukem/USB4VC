import os
import sys
import time
import serial

# import usb4vc_shared
# import usb4vc_usb_scan
# import usb4vc_ui

ser = serial.Serial('/dev/serial0', 115200)

while 1:
    to_send = f"hello {int(time.time())}"
    print(to_send)
    ser.write(to_send.encode())
    time.sleep(0.5)

import os
import sys
import time
import serial
import subprocess

# import usb4vc_shared
# import usb4vc_usb_scan
# import usb4vc_ui
# subprocess.getoutput(f"timeout 5 bluetoothctl --agent NoInputNoOutput paired-devices")

ser = serial.Serial('/dev/serial0', 115200)

while 1:
    received = ser.readline().decode().replace('\r', '')
    if len(received) <= 0:
        continue
    print("I received:", received)
    print(subprocess.getoutput(f"timeout 1 {received}"))
    # to_send = f"hello {int(time.time())}"
    # print(to_send)
    # ser.write(to_send.encode())
    # time.sleep(0.5)

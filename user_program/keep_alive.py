import os
import time

while 1:
    exit_code = os.system("cd /home/pi/usb4vc/rpi_app; python3 -u usb4vc_main.py") >> 8
    print("App died! Exit code:", exit_code)
    if exit_code == 169:
        exit()
    time.sleep(0.5)
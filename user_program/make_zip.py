import usb4vc_shared
import os
import sys
import time

os.system("rm -rfv ./rpi_app")
os.system("sleep 0.1")
os.system("rm -fv ./*.zip")
os.system("sleep 0.1")
os.system("mkdir rpi_app")
os.system("sleep 0.1")
os.system("cp -v ./*.py rpi_app/")
os.system("cp -v ./*.ttf rpi_app/")

filename = f'usb4vc_src_{usb4vc_shared.RPI_APP_VERSION_TUPLE[0]}.{usb4vc_shared.RPI_APP_VERSION_TUPLE[1]}.{usb4vc_shared.RPI_APP_VERSION_TUPLE[2]}.zip'
os.system(f"7z a {filename} rpi_app")
print(filename)
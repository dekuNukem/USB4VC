import os
from usb4vc_shared import RPI_APP_VERSION_TUPLE as v

os.system("rm -fv ./*.zip")
os.system("rm -rfv ./rpi_app")
os.system("mkdir rpi_app")
os.system("cp ./*.py rpi_app")
os.system("cp ./*.ttf rpi_app")

zip_file_name = f"usb4vc_src_{v[0]}.{v[1]}.{v[2]}.zip"
os.system(f"7z.exe a {zip_file_name} ./rpi_app")

os.system("rm -rfv ./rpi_app")

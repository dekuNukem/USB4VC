import os
import sys
import time
import json
import RPi.GPIO as GPIO
import usb4vc_usb_scan
import usb4vc_ui
import subprocess 

from usb4vc_shared import xpadneo_source_path, rpi_model_save_file_path

# usb4vc_ui.reset_pboard()

def get_kernel_version():
    return int(subprocess.getoutput("uname -r | awk -F. '{ printf \"%03d%03d\",$1,$2 }'").strip())

def get_current_rpi_model():
    current_model = 'Unknown'
    try:
        current_model = subprocess.getoutput("cat /proc/device-tree/model").replace('\n', '').replace('\r', '').strip()
    except Exception as e:
        print('get_current_rpi_model:', e)
    return current_model

def get_stored_rpi_model():
    stored_model = 'Unknown'

    # migrate rpi_model.txt from old location for compat
    old_rpi_model_save_file_path = "/home/pi/rpi_model.txt"
    if os.path.exists(old_rpi_model_save_file_path):
        os.system(f"mv {old_rpi_model_save_file_path} {rpi_model_save_file_path}")

    try:
        with open(rpi_model_save_file_path, 'r') as file:
            stored_model = file.read().replace('\n', '').replace('\r', '').strip()
    except Exception as e:
        print('get_stored_rpi_model:', e)
    return stored_model

def save_rpi_model(model_str):
    try:
        with open(rpi_model_save_file_path, 'w') as file:
            file.write(model_str)
    except Exception as e:
        print('save_rpi_model:', e)

def check_rpi_model():
    current_model = get_current_rpi_model()
    stored_model = get_stored_rpi_model()
    print('current_model', current_model)
    print('stored_model', stored_model)
    if current_model != stored_model:
        usb4vc_ui.oled_print_model_changed()
        for x in range(20):
            print("!!!!!!!!!! DO NOT UNPLUG UNTIL I REBOOT !!!!!!!!!!")
            print("!!!!!!!!!! DO NOT UNPLUG UNTIL I REBOOT !!!!!!!!!!")
            print("!!!!!!!!!! DO NOT UNPLUG UNTIL I REBOOT !!!!!!!!!!")
            time.sleep(0.1)
        os.system(f"cd {xpadneo_source_path}; sudo ./uninstall.sh")
        usb4vc_ui.oled_print_oneline("Recompiling...")
        os.system(f"cd {xpadneo_source_path}; sudo ./install.sh")
        usb4vc_ui.oled_print_reboot()
        save_rpi_model(current_model)
        time.sleep(2)
        os.system('sudo reboot')
        time.sleep(10)

check_rpi_model()

try:
    boot_config = os.popen('cat /boot/cmdline.txt').read()
    if "usbhid.mousepoll".lower() not in boot_config.lower():
        print("@@@@@@@@@@@@ writing usbhid.mousepoll to /boot/cmdline.txt @@@@@@@@@@@@")
        new_config = boot_config.replace('\r', '').replace('\n', '').strip() + ' usbhid.mousepoll=0\n'
        with open('/boot/cmdline.txt', 'w') as ffff:
            ffff.write(new_config)
except Exception as e:
    print('usbhid.mousepoll exception:', e)

try:
    ertm_requires_disable = get_kernel_version() < 6000

    usb4vc_ui.ui_init()
    usb4vc_ui.ui_thread.start()

    usb4vc_usb_scan.usb_device_scan_thread.start()
    usb4vc_usb_scan.raw_input_event_parser_thread.start()

    while 1:
        time.sleep(2)
        if ertm_requires_disable and os.path.exists("/sys/module/bluetooth/parameters/disable_ertm"):
            try:
                ertm_status = subprocess.getoutput("cat /sys/module/bluetooth/parameters/disable_ertm").replace('\n', '').replace('\r', '').strip()
                if ertm_status != 'Y':
                    print('ertm_status:', ertm_status)
                    print("Disabling ERTM....")
                    subprocess.call('echo 1 > /sys/module/bluetooth/parameters/disable_ertm')
                    print("DONE")
            except Exception:
                continue
finally:
    GPIO.cleanup()



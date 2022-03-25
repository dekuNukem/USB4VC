import os
import sys
import time
import json
import RPi.GPIO as GPIO
import usb4vc_usb_scan
import usb4vc_ui
import subprocess 

# usb4vc_ui.reset_pboard()

def get_current_rpi_model():
    current_model = 'Unknown'
    try:
        current_model = subprocess.getoutput("cat /proc/device-tree/model").replace('\n', '').replace('\r', '').strip()
    except Exception as e:
        print('get_current_rpi_model:', e)
    return current_model

rpi_model_save_file = '/home/pi/rpi_model.txt'

def get_stored_rpi_model():
    stored_model = 'Unknown'
    try:
        with open(rpi_model_save_file, 'r') as file:
            stored_model = file.read().replace('\n', '').replace('\r', '').strip()
    except Exception as e:
        print('get_stored_rpi_model:', e)
    return stored_model

def save_rpi_model(model_str):
    try:
        with open(rpi_model_save_file, 'w') as file:
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
        for x in range(50):
            print("!!!!!!!!!! DO NOT UNPLUG UNTIL I REBOOT !!!!!!!!!!")
            print("!!!!!!!!!! DO NOT UNPLUG UNTIL I REBOOT !!!!!!!!!!")
            print("!!!!!!!!!! DO NOT UNPLUG UNTIL I REBOOT !!!!!!!!!!")
            time.sleep(0.1)
        os.system("cd /home/pi/xpadneo/; sudo ./uninstall.sh")
        usb4vc_ui.oled_print_oneline("Recompiling...")
        os.system("cd /home/pi/xpadneo/; sudo ./install.sh")
        usb4vc_ui.oled_print_oneline("Done! Rebooting..")
        save_rpi_model(current_model)
        time.sleep(2)
        os.system('sudo reboot')
        time.sleep(10)

def update_wifi_credentials():
    config_file_path = '/etc/wpa_supplicant/wpa_supplicant.conf'
    if os.path.isfile(config_file_path) is False:
        print('update_wifi_credentials: system config file not found')
        return
    new_wifi_info_path = "/home/pi/usb4vc/config/wifi_info.json"
    if os.path.isfile(new_wifi_info_path) is False:
        print('update_wifi_credentials: user wifi info file not found')
        return

    with open(new_wifi_info_path) as json_file:
        wifi_dict = json.load(json_file)

    config_str = f"""
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1
country={wifi_dict['wifi_country_code']}

network={{
        ssid="{wifi_dict['wifi_name']}"
        psk="{wifi_dict['wifi_password']}"
}}
"""

    config_str_unsecured = f"""
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1
country={wifi_dict['wifi_country_code']}

network={{
        ssid="{wifi_dict['wifi_name']}"
        key_mgmt=NONE
}}
"""
    to_write = config_str
    if len(wifi_dict['wifi_password']) > 0:
        to_write = config_str_unsecured
    with open(config_file_path, 'w') as wifi_config_file:
        wifi_config_file.write(to_write)

check_rpi_model()

try:
    update_wifi_credentials()
except Exception as e:
    print('update_wifi_credentials exception:', e)

usb4vc_ui.ui_init()
usb4vc_ui.ui_thread.start()

usb4vc_usb_scan.usb_device_scan_thread.start()
usb4vc_usb_scan.raw_input_event_parser_thread.start()

while 1:
    time.sleep(2)
    try:
        ertm_status = subprocess.getoutput("cat /sys/module/bluetooth/parameters/disable_ertm").replace('\n', '').replace('\r', '').strip()
        if ertm_status != 'Y':
            print('ertm_status:', ertm_status)
            print("Disabling ERTM....")
            subprocess.call('echo 1 > /sys/module/bluetooth/parameters/disable_ertm')
            print("DONE")
    except Exception:
        continue
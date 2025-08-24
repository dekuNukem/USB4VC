# https://luma-oled.readthedocs.io/en/latest/software.html

import os
import sys
import time
import threading
import usb4vc_oled
from luma.core.render import canvas
import RPi.GPIO as GPIO
import usb4vc_usb_scan
import usb4vc_show_ev
import usb4vc_check_update
import json
import subprocess
from subprocess import Popen, PIPE

from usb4vc_shared import *

config_file_path = os.path.join(config_dir_path, 'config.json')

ensure_dir(this_app_dir_path)
ensure_dir(config_dir_path)
ensure_dir(firmware_dir_path)
ensure_dir(temp_dir_path)

PLUS_BUTTON_PIN = 27
MINUS_BUTTON_PIN = 19
ENTER_BUTTON_PIN = 22
SHUTDOWN_BUTTON_PIN = 21

PBOARD_RESET_PIN = 25
PBOARD_BOOT0_PIN = 12
SLEEP_LED_PIN = 26

GPIO.setmode(GPIO.BCM)
GPIO.setup(PBOARD_RESET_PIN, GPIO.IN)
GPIO.setup(PBOARD_BOOT0_PIN, GPIO.IN)
GPIO.setup(SLEEP_LED_PIN, GPIO.OUT)
GPIO.output(SLEEP_LED_PIN, GPIO.LOW)

SPI_MOSI_MAGIC = 0xde
SPI_MOSI_MSG_TYPE_SET_PROTOCOL = 2
set_protocl_spi_msg_template = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_SET_PROTOCOL] + [0]*29

class my_button(object):
    def __init__(self, bcm_pin):
        super(my_button, self).__init__()
        self.pin_number = bcm_pin
        GPIO.setup(self.pin_number, GPIO.IN, pull_up_down=GPIO.PUD_UP)
        self.prev_state = GPIO.input(self.pin_number)

    def is_pressed(self):
        result = False
        current_state = GPIO.input(self.pin_number)
        if self.prev_state == 1 and current_state == 0:
            result = True
        self.prev_state = current_state
        return result


pboard_info_spi_msg = [0] * 32
this_pboard_id = PBOARD_ID_UNKNOWN

USBGP_BTN_SOUTH = 0x130
USBGP_BTN_EAST = 0x131
USBGP_BTN_C = 0x132
USBGP_BTN_NORTH = 0x133
USBGP_BTN_WEST = 0x134
USBGP_BTN_Z = 0x135
USBGP_BTN_TL = 0x136
USBGP_BTN_TR = 0x137
USBGP_BTN_TL2 = 0x138
USBGP_BTN_TR2 = 0x139
USBGP_BTN_SELECT = 0x13a
USBGP_BTN_START = 0x13b
USBGP_BTN_MODE = 0x13c
USBGP_BTN_THUMBL = 0x13d
USBGP_BTN_THUMBR = 0x13e

USBGP_BTN_A = USBGP_BTN_SOUTH
USBGP_BTN_B = USBGP_BTN_EAST
USBGP_BTN_X = USBGP_BTN_NORTH
USBGP_BTN_Y = USBGP_BTN_WEST

USBGP_ABS_X = 0x00 # left stick X
USBGP_ABS_Y = 0x01 # left stick Y
USBGP_ABS_Z = 0x02 # left analog trigger
USBGP_ABS_RX = 0x03 # right stick X
USBGP_ABS_RY = 0x04 # right stick Y
USBGP_ABS_RZ = 0x05 # right analog trigger
USBGP_ABS_HAT0X = 0x10 # D-pad X
USBGP_ABS_HAT0Y = 0x11 # D-pad Y

GENERIC_USB_GAMEPAD_TO_MOUSE_KB_DEAULT_MAPPING = {
    "MAPPING_TYPE": "DEFAULT_MOUSE_KB",
    'BTN_TL': {'code': 'BTN_LEFT'},
    'BTN_TR': {'code': 'BTN_RIGHT'},
    'BTN_TL2': {'code': 'BTN_LEFT'},
    'BTN_TR2': {'code': 'BTN_RIGHT'},
    'ABS_X': {'code': 'REL_X'},
    'ABS_Y': {'code': 'REL_Y'},
    'ABS_HAT0X': {'code': 'KEY_RIGHT', 'code_neg': 'KEY_LEFT'},
    'ABS_HAT0Y': {'code': 'KEY_DOWN', 'code_neg': 'KEY_UP'}
}

IBM_GENERIC_USB_GAMEPAD_TO_15PIN_GAMEPORT_GAMEPAD_DEAULT_MAPPING = {
    "MAPPING_TYPE": "DEFAULT_15PIN",
    # buttons to buttons
    'BTN_SOUTH': {'code':'IBM_GGP_BTN_1'},
    'BTN_NORTH': {'code':'IBM_GGP_BTN_2'},
    'BTN_EAST': {'code':'IBM_GGP_BTN_3'},
    'BTN_WEST': {'code':'IBM_GGP_BTN_4'},
    
    'BTN_TL': {'code':'IBM_GGP_BTN_1'},
    'BTN_TR': {'code':'IBM_GGP_BTN_2'},
    'BTN_Z': {'code':'IBM_GGP_BTN_3'},
    'BTN_C': {'code':'IBM_GGP_BTN_4'},
    
    'BTN_TL2': {'code':'IBM_GGP_BTN_1'},
    'BTN_TR2': {'code':'IBM_GGP_BTN_2'},

    # analog axes to analog axes
    'ABS_X': {'code':'IBM_GGP_JS1_X'},
    'ABS_Y': {'code':'IBM_GGP_JS1_Y'},
    'ABS_HAT0X': {'code':'IBM_GGP_JS1_X'},
    'ABS_HAT0Y': {'code':'IBM_GGP_JS1_Y'},
    'ABS_RX': {'code':'IBM_GGP_JS2_X'},
    'ABS_RY': {'code':'IBM_GGP_JS2_Y'},
}

PROTOCOL_OFF = {'pid':0, 'display_name':"OFF"}
PROTOCOL_AT_PS2_KB = {'pid':1, 'display_name':"AT/PS2"}
PROTOCOL_XT_KB = {'pid':2, 'display_name':"PC XT"}
PROTOCOL_ADB_KB = {'pid':3, 'display_name':"ADB"}
PROTOCOL_PS2_MOUSE_NORMAL = {'pid':4, 'display_name':"PS/2"}
PROTOCOL_MICROSOFT_SERIAL_MOUSE = {'pid':5, 'display_name':"Microsft Serial"}
PROTOCOL_ADB_MOUSE = {'pid':6, 'display_name':"ADB"}
PROTOCOL_15PIN_GAMEPORT_GAMEPAD = {'pid':7, 'display_name':"Generic 15-Pin", 'mapping':IBM_GENERIC_USB_GAMEPAD_TO_15PIN_GAMEPORT_GAMEPAD_DEAULT_MAPPING}
PROTOCOL_MOUSESYSTEMS_SERIAL_MOUSE = {'pid':8, 'display_name':"MouseSys Serial"}
PROTOCOL_USB_GP_TO_MOUSE_KB = {'pid':0, 'display_name':'Mouse & KB', 'mapping':GENERIC_USB_GAMEPAD_TO_MOUSE_KB_DEAULT_MAPPING}
PROTOCOL_RAW_KEYBOARD = {'pid':125, 'display_name':"Raw data"}
PROTOCOL_RAW_MOUSE = {'pid':126, 'display_name':"Raw data"}
PROTOCOL_RAW_GAMEPAD = {'pid':127, 'display_name':"Raw data"}

PROTOCOL_APPLE_QUAD_MOUSE = {'pid':9, 'display_name':"Lisa/EarlyMac"}
PROTOCOL_M0110_KEYBOARD = {'pid':10, 'display_name':"EarlyMac"}
PROTOCOL_LISA_KEYBOARD = {'pid':11, 'display_name':"Lisa"}

custom_profile_list = []

try:
    onlyfiles = [f for f in os.listdir(config_dir_path) if os.path.isfile(os.path.join(config_dir_path, f))]
    json_map_files = [os.path.join(config_dir_path, x) for x in onlyfiles if x.lower().startswith('usb4vc_map') and x.lower().endswith(".json")]
    for item in json_map_files:
        print('loading json file:', item)
        with open(item) as json_file:
            custom_profile_list.append(json.load(json_file))
except Exception as e:
    print('exception json load:', e)

def get_list_of_usb_drive():
    usb_drive_set = set()
    try:
        usb_drive_path = subprocess.getoutput(f"timeout 2 df -h | grep -i usb").replace('\r', '').split('\n')
        for item in [x for x in usb_drive_path if len(x) > 2]:
            usb_drive_set.add(os.path.join(item.split(' ')[-1], 'usb4vc'))
    except Exception as e:
        print("exception get_list_of_usb_drive:", e)
    return usb_drive_set

def copy_debug_log():
    usb_drive_set = get_list_of_usb_drive()
    if len(usb_drive_set) == 0:
        return False
    for this_path in usb_drive_set:
        if os.path.isdir(this_path):
            print('copying debug log to', this_path)
            os.system(f'sudo cp -v /home/pi/usb4vc/usb4vc_debug_log.txt {this_path}')
    return True

def check_usb_drive():
    usb_drive_set = get_list_of_usb_drive()

    if len(usb_drive_set) == 0:
        return False, 'USB Drive Not Found'

    for this_path in usb_drive_set:
        usb_config_path = os.path.join(this_path, 'config')
        if not os.path.isdir(usb_config_path):
            usb_config_path = None

        if usb_config_path is not None:
            return True, usb_config_path

    return False, 'No Update Data Found'

def get_pbid_and_version(dfu_file_name):
    pbid = None
    try:
        pbid = int(dfu_file_name.split('PBID')[-1].split('_')[0])
    except Exception as e:
        print("exception fw pbid parse:", e)
    fw_ver_tuple = None
    try:
        fw_ver = dfu_file_name.lower().split('_v')[-1].split('.')[0].split('_')
        fw_ver_tuple = (int(fw_ver[0]), int(fw_ver[1]), int(fw_ver[2]))
    except Exception as e:
        print('exception fw ver parse:', e)
    return pbid, fw_ver_tuple

def reset_pboard():
    print("resetting protocol board...")
    GPIO.setup(PBOARD_BOOT0_PIN, GPIO.IN)
    GPIO.setup(PBOARD_RESET_PIN, GPIO.OUT)
    GPIO.output(PBOARD_RESET_PIN, GPIO.LOW)
    time.sleep(0.05)
    GPIO.setup(PBOARD_RESET_PIN, GPIO.IN)
    time.sleep(0.05)
    print("done")

def enter_dfu():
    # RESET LOW: Enter reset
    GPIO.setup(PBOARD_RESET_PIN, GPIO.OUT)
    GPIO.output(PBOARD_RESET_PIN, GPIO.LOW)
    time.sleep(0.05)
    # BOOT0 HIGH: Boot into DFU mode
    GPIO.setup(PBOARD_BOOT0_PIN, GPIO.OUT)
    GPIO.output(PBOARD_BOOT0_PIN, GPIO.HIGH)
    time.sleep(0.05)
    # Release RESET, BOOT0 still HIGH, STM32 now in DFU mode
    GPIO.setup(PBOARD_RESET_PIN, GPIO.IN)
    time.sleep(1.5)

def exit_dfu():
    # Release BOOT0
    GPIO.setup(PBOARD_BOOT0_PIN, GPIO.IN)
    # Activate RESET
    GPIO.setup(PBOARD_RESET_PIN, GPIO.OUT)
    GPIO.output(PBOARD_RESET_PIN, GPIO.LOW)
    time.sleep(0.05)
    # Release RESET, BOOT0 is LOW, STM32 boots in normal mode
    GPIO.setup(PBOARD_RESET_PIN, GPIO.IN)
    time.sleep(1.5)

def fw_update(fw_path, pbid):
    is_updated = False
    if pbid in i2c_bootloader_pbid and fw_path.lower().endswith('.hex'):
        enter_dfu()
        os.system(f'sudo stm32flash -w {fw_path} -a 0x3b /dev/i2c-1')
        is_updated = True
    elif pbid in usb_bootloader_pbid and fw_path.lower().endswith('.dfu'):
        enter_dfu()
        lsusb_str = subprocess.getoutput("lsusb")
        if 'in DFU'.lower() not in lsusb_str.lower():
            with canvas(usb4vc_oled.oled_device) as draw:
                usb4vc_oled.oled_print_centered("Connect a USB cable", usb4vc_oled.font_regular, 0, draw)
                usb4vc_oled.oled_print_centered("from P-Card to RPi", usb4vc_oled.font_regular, 10, draw)
                usb4vc_oled.oled_print_centered("and try again", usb4vc_oled.font_regular, 20, draw)
            time.sleep(4)
        else:
            os.system(f'sudo dfu-util --device ,0483:df11 -a 0 -D {fw_path}')
            is_updated = True
    exit_dfu()
    return is_updated
    
def update_pboard_firmware(this_pid):
    onlyfiles = [f for f in os.listdir(firmware_dir_path) if os.path.isfile(os.path.join(firmware_dir_path, f))]
    firmware_files = [x for x in onlyfiles if x.startswith("PBFW_") and (x.lower().endswith(".dfu") or x.lower().endswith(".hex")) and "PBID" in x]
    this_pboard_version_tuple = (pboard_info_spi_msg[5], pboard_info_spi_msg[6], pboard_info_spi_msg[7])
    for item in firmware_files:
        pbid, fw_ver_tuple = get_pbid_and_version(item)
        if pbid is None or fw_ver_tuple is None:
            continue
        print('update_pboard_firmware:', this_pid, this_pboard_version_tuple, fw_ver_tuple)
        if pbid == this_pid and fw_ver_tuple > this_pboard_version_tuple:
            print("DOING IT NOW")
            with canvas(usb4vc_oled.oled_device) as draw:
                usb4vc_oled.oled_print_centered("Loading Firmware:", usb4vc_oled.font_medium, 0, draw)
                usb4vc_oled.oled_print_centered(item.strip("PBFW_").strip(".dfu").strip(".hex"), usb4vc_oled.font_regular, 16, draw)
            if fw_update(os.path.join(firmware_dir_path, item), this_pid):
                return True
    return False

def update_from_usb(usb_config_path):
    if usb_config_path is not None:
        os.system(f'cp -v /home/pi/usb4vc/config/config.json {usb_config_path}')
        os.system('mv -v /home/pi/usb4vc/config/config.json /home/pi/usb4vc/config.json')
        os.system('rm -rfv /home/pi/usb4vc/config/*')
        os.system(f"cp -v {os.path.join(usb_config_path, '*')} /home/pi/usb4vc/config")
        os.system("mv -v /home/pi/usb4vc/config.json /home/pi/usb4vc/config/config.json")

ibmpc_keyboard_protocols = [PROTOCOL_OFF, PROTOCOL_AT_PS2_KB, PROTOCOL_XT_KB]
ibmpc_mouse_protocols = [PROTOCOL_OFF, PROTOCOL_PS2_MOUSE_NORMAL, PROTOCOL_MICROSOFT_SERIAL_MOUSE, PROTOCOL_MOUSESYSTEMS_SERIAL_MOUSE]
ibmpc_gamepad_protocols = [PROTOCOL_OFF, PROTOCOL_15PIN_GAMEPORT_GAMEPAD, PROTOCOL_USB_GP_TO_MOUSE_KB]

adb_keyboard_protocols = [PROTOCOL_OFF, PROTOCOL_ADB_KB]
adb_mouse_protocols = [PROTOCOL_OFF, PROTOCOL_ADB_MOUSE]
adb_gamepad_protocols = [PROTOCOL_OFF, PROTOCOL_USB_GP_TO_MOUSE_KB]

raw_keyboard_protocols = [PROTOCOL_OFF, PROTOCOL_RAW_KEYBOARD]
raw_mouse_protocols = [PROTOCOL_OFF, PROTOCOL_RAW_MOUSE]
raw_gamepad_protocols = [PROTOCOL_OFF, PROTOCOL_RAW_GAMEPAD]

lisa_mac_adb_keyboard_protocols = [PROTOCOL_OFF, PROTOCOL_LISA_KEYBOARD, PROTOCOL_M0110_KEYBOARD, PROTOCOL_ADB_KB]
lisa_mac_adb_mouse_protocols = [PROTOCOL_OFF, PROTOCOL_APPLE_QUAD_MOUSE, PROTOCOL_ADB_MOUSE]
lisa_mac_adb_gamepad_protocols = [PROTOCOL_OFF, PROTOCOL_USB_GP_TO_MOUSE_KB]

mouse_sensitivity_list = [1, 1.25, 1.5, 1.75, 0.25, 0.5, 0.75]

"""
key is protocol card ID
conf_dict[pbid]:
hw revision
current keyboard protocol
current mouse protocol
current gamepad procotol
mouse sensitivity
"""

configuration_dict = {}

LINUX_EXIT_CODE_TIMEOUT = 124

def bt_setup():
    rfkill_str = subprocess.getoutput("/usr/sbin/rfkill -n")
    if 'bluetooth' not in rfkill_str:
        return 1, "no BT receiver found"
    os.system('/usr/sbin/rfkill unblock bluetooth')
    time.sleep(0.1)
    exit_code = os.system('timeout 1 bluetoothctl agent NoInputNoOutput') >> 8
    if exit_code == LINUX_EXIT_CODE_TIMEOUT:
        return 2, 'bluetoothctl stuck'
    return 0, ''

def scan_bt_devices(timeout_sec = 5):
    exit_code = os.system(f"timeout {timeout_sec} bluetoothctl --agent NoInputNoOutput scan on") >> 8
    if exit_code != LINUX_EXIT_CODE_TIMEOUT:
        return None, 'scan error'
    device_str = subprocess.getoutput("bluetoothctl --agent NoInputNoOutput devices")
    dev_list = []
    for line in device_str.replace('\r', '').split('\n'):
        if 'device' not in line.lower():
            continue
        line_split = line.split(' ', maxsplit=2)
        # skip if device has no name
        if len(line_split) < 3 or line_split[2].count('-') == 5:
            continue
        dev_list.append((line_split[1], line_split[2]))
    return dev_list, ''

def pair_device(mac_addr):
    is_ready = False
    is_sent = False
    fail_phrases = ['fail', 'error', 'not available', 'excep']
    with Popen(["bluetoothctl", "--agent", "NoInputNoOutput"], stdout=PIPE, stdin=PIPE, bufsize=1,
               universal_newlines=True, shell=True) as p:
        for line in p.stdout:
            print(line, end='')
            line_lo = line.lower()
            if 'registered' in line_lo:
                is_ready = True
            if is_ready is False:
                continue
            if '#' in line_lo and is_sent == False:
                p.stdin.write(f'pair {mac_addr}\n')
                is_sent = True
            if 'PIN code:' in line:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered("Enter PIN code:", usb4vc_oled.font_medium, 0, draw)
                    usb4vc_oled.oled_print_centered(line.split('PIN code:')[-1], usb4vc_oled.font_medium, 15, draw)
            if '(yes/no)' in line:
                p.stdin.write('yes\n')
            if 'number in 0-999999' in line:
                return False, "Error: Passkey needed"
            if 'successful' in line_lo:
                p.stdin.write('exit\n')
                return True, 'Success!'
            for item in fail_phrases:
                if item in line_lo:
                    p.stdin.write('exit\n')
                    return False, line
    return False, "wtf"

def get_paired_devices():
    dev_set = set()
    try:
        device_str = subprocess.getoutput(f"timeout 5 bluetoothctl --agent NoInputNoOutput devices Paired")
        for line in device_str.replace('\r', '').split('\n'):
            if 'device' not in line.lower():
                continue
            line_split = line.split(' ', maxsplit=2)
            # skip if device has no name
            if len(line_split) < 3 or line_split[2].count('-') == 5:
                continue
            dev_set.add((line_split[1], line_split[2]))
    except Exception as e:
        print('exception get_paired_devices:', e)
    return dev_set

def load_config():
    global configuration_dict
    try:
        with open(config_file_path) as json_file:
            temp_dict = json.load(json_file)
            # json dump all keys as strings, need to convert them back to ints
            for key in temp_dict:
                if key.isdigit():
                    configuration_dict[int(key)] = temp_dict[key]
                else:
                    configuration_dict[key] = temp_dict[key]
    except Exception as e:
        print("exception config load failed!", e)

def get_ip_address():
    ip_str = subprocess.getoutput("timeout 1 hostname -I")
    ip_list = [x for x in ip_str.split(' ') if '.' in x]
    if len(ip_list) == 0:
        return "Offline"
    return f'{ip_list[0]}'

def save_config():
    try:
        with open(config_file_path, 'w', encoding='utf-8') as save_file:
           save_file.write(json.dumps(configuration_dict))
    except Exception as e:
        print("exception config save failed!", e)

class usb4vc_menu(object):
    def cap_index(self, index, list_size):
        if index >= list_size:
            return 0
        return index
    def __init__(self, pboard, conf_dict):
        super(usb4vc_menu, self).__init__()
        self.current_level = 0
        self.current_page = 0
        self.level_size = 6
        self.page_size = [7, 5, 4, 1, 1, 5]
        self.kb_protocol_list = list(pboard['protocol_list_keyboard'])
        self.mouse_protocol_list = list(pboard['protocol_list_mouse'])
        self.gamepad_protocol_list = list(pboard['protocol_list_gamepad'])
        self.pb_info = dict(pboard)
        self.current_keyboard_protocol_index =  self.cap_index(conf_dict.get('keyboard_protocol_index', 0), len(self.kb_protocol_list))
        self.current_mouse_protocol_index =  self.cap_index(conf_dict.get("mouse_protocol_index", 0), len(self.mouse_protocol_list))
        self.current_mouse_sensitivity_offset_index =  self.cap_index(conf_dict.get("mouse_sensitivity_index", 0), len(mouse_sensitivity_list))
        self.current_gamepad_protocol_index =  self.cap_index(conf_dict.get("gamepad_protocol_index", 0), len(self.gamepad_protocol_list))
        self.current_keyboard_protocol = self.kb_protocol_list[self.current_keyboard_protocol_index]
        self.current_mouse_protocol = self.mouse_protocol_list[self.current_mouse_protocol_index]
        self.current_gamepad_protocol = self.gamepad_protocol_list[self.current_gamepad_protocol_index]
        self.last_spi_message = []
        self.bluetooth_device_list = None
        self.error_message = ''
        self.pairing_result = ''
        self.bt_scan_timeout_sec = 10
        self.paired_devices_list = []
        self.send_protocol_set_spi_msg()

    def switch_page(self, amount):
        self.current_page = (self.current_page + amount) % self.page_size[self.current_level]

    def goto_page(self, new_page):
        if new_page < self.page_size[self.current_level]:
            self.current_page = new_page

    def goto_level(self, new_level):
        if new_level < self.level_size:
            self.current_level = new_level
            self.current_page = 0

    def display_page(self, level, page):
        if level == 0:
            if page == 0:
                with canvas(usb4vc_oled.oled_device) as draw:
                    mouse_count, kb_count, gp_count = usb4vc_usb_scan.get_device_count()
                    draw.text((0, 0),  f"KBD {kb_count} {self.current_keyboard_protocol['display_name']}", font=usb4vc_oled.font_regular, fill="white")
                    draw.text((0, 10), f"MOS {mouse_count} {self.current_mouse_protocol['display_name']}", font=usb4vc_oled.font_regular, fill="white")
                    draw.text((0, 20), f"GPD {gp_count} {self.current_gamepad_protocol['display_name']}", font=usb4vc_oled.font_regular, fill="white")
            if page == 1:
                with canvas(usb4vc_oled.oled_device) as draw:
                    if 'Unknown' in self.pb_info['full_name']:
                        draw.text((0, 0), f"{self.pb_info['full_name']} PID {this_pboard_id}", font=usb4vc_oled.font_regular, fill="white")
                    else:
                        draw.text((0, 0), f"{self.pb_info['full_name']}", font=usb4vc_oled.font_regular, fill="white")
                    draw.text((0, 10), f"PB {self.pb_info['fw_ver'][0]}.{self.pb_info['fw_ver'][1]}.{self.pb_info['fw_ver'][2]}  RPi {RPI_APP_VERSION_TUPLE[0]}.{RPI_APP_VERSION_TUPLE[1]}.{RPI_APP_VERSION_TUPLE[2]}", font=usb4vc_oled.font_regular, fill="white")
                    draw.text((0, 20), f"IP: {get_ip_address()}", font=usb4vc_oled.font_regular, fill="white")
            if page == 2:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered("Load Custom", usb4vc_oled.font_medium, 0, draw)
                    usb4vc_oled.oled_print_centered("Config from USB", usb4vc_oled.font_medium, 16, draw)
            if page == 3:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered("Internet Update", usb4vc_oled.font_medium, 10, draw)
            if page == 4:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered("Show Event Codes", usb4vc_oled.font_medium, 0, draw)
                    usb4vc_oled.oled_print_centered("(experimental)", usb4vc_oled.font_regular, 20, draw)
            if page == 5:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered("Remove BT Device", usb4vc_oled.font_medium, 10, draw)
            if page == 6:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered("Pair Bluetooth", usb4vc_oled.font_medium, 10, draw)
        if level == 1:
            if page == 0:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered("Keyboard Protocol", usb4vc_oled.font_medium, 0, draw)
                    usb4vc_oled.oled_print_centered(self.kb_protocol_list[self.current_keyboard_protocol_index]['display_name'], usb4vc_oled.font_medium, 15, draw)
            if page == 1:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered("Mouse Protocol", usb4vc_oled.font_medium, 0, draw)
                    usb4vc_oled.oled_print_centered(self.mouse_protocol_list[self.current_mouse_protocol_index]['display_name'], usb4vc_oled.font_medium, 15, draw)
            if page == 2:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered("Gamepad Protocol", usb4vc_oled.font_medium, 0, draw)
                    usb4vc_oled.oled_print_centered(self.gamepad_protocol_list[self.current_gamepad_protocol_index]['display_name'], usb4vc_oled.font_medium, 15, draw)
            if page == 3:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered("Mouse Sensitivity", usb4vc_oled.font_medium, 0, draw)
                    usb4vc_oled.oled_print_centered(f"{mouse_sensitivity_list[self.current_mouse_sensitivity_offset_index]}", usb4vc_oled.font_medium, 15, draw)
            if page == 4:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered("Save & Quit", usb4vc_oled.font_medium, 10, draw)
        if level == 2:
            if page == 0:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered("Put your device in", usb4vc_oled.font_regular, 0, draw)
                    usb4vc_oled.oled_print_centered("pairing mode now.", usb4vc_oled.font_regular, 10, draw)
                    usb4vc_oled.oled_print_centered("Press enter to start", usb4vc_oled.font_regular, 20, draw)
            if page == 1:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered("Scanning...", usb4vc_oled.font_medium, 0, draw)
                    usb4vc_oled.oled_print_centered("Please wait", usb4vc_oled.font_medium, 15, draw)
                result, self.error_message = bt_setup()
                if result != 0:
                    self.goto_page(3)
                    self.display_curent_page()
                    return
                paired_devices_set = get_paired_devices()
                self.bluetooth_device_list, self.error_message = scan_bt_devices(self.bt_scan_timeout_sec)
                self.bluetooth_device_list = list(set(self.bluetooth_device_list) - paired_devices_set)
                if len(self.bluetooth_device_list) == 0:
                    self.error_message = "Nothing was found"
                    self.goto_page(3)
                    self.display_curent_page()
                    return
                print("BT LIST:", self.bluetooth_device_list)
                # set up level 3 menu structure
                self.page_size[3] = len(self.bluetooth_device_list) + 1
                self.goto_level(3)
                self.display_curent_page()
            if page == 2:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered("Pairing result:", usb4vc_oled.font_medium, 0, draw)
                    usb4vc_oled.oled_print_centered(self.pairing_result, usb4vc_oled.font_regular, 20, draw)
            if page == 3:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered("Bluetooth Error!", usb4vc_oled.font_medium, 0, draw)
                    usb4vc_oled.oled_print_centered(self.error_message, usb4vc_oled.font_regular, 20, draw)
        if level == 3:
            if page == self.page_size[3] - 1:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered("Exit", usb4vc_oled.font_medium, 10, draw)
            else:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered(f"Found {len(self.bluetooth_device_list)}. Pair this?", usb4vc_oled.font_regular, 0, draw)
                    usb4vc_oled.oled_print_centered(f"{self.bluetooth_device_list[page][1]}", usb4vc_oled.font_regular, 10, draw)
                    usb4vc_oled.oled_print_centered(f"{self.bluetooth_device_list[page][0]}", usb4vc_oled.font_regular, 20, draw)
        if level == 4:
            if page == self.page_size[4] - 1:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered("Exit", usb4vc_oled.font_medium, 10, draw)
            else:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered(f"Remove this?", usb4vc_oled.font_regular, 0, draw)
                    usb4vc_oled.oled_print_centered(f"{self.paired_devices_list[page][1]}", usb4vc_oled.font_regular, 10, draw)
                    usb4vc_oled.oled_print_centered(f"{self.paired_devices_list[page][0]}", usb4vc_oled.font_regular, 20, draw)
        if level == 5:
            if page == 0:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered("Power Down", usb4vc_oled.font_medium, 10, draw)
            if page == 1:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered("Relaunch", usb4vc_oled.font_medium, 10, draw)
            if page == 2:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered("Reboot", usb4vc_oled.font_medium, 10, draw)
            if page == 3:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered("Exit to Linux", usb4vc_oled.font_medium, 10, draw)
            if page == 4:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered("Cancel", usb4vc_oled.font_medium, 10, draw)

    def send_protocol_set_spi_msg(self):
        status_dict = {}
        for index, item in enumerate(self.kb_protocol_list):
            if item['pid'] & 0x7f in status_dict and status_dict[item['pid'] & 0x7f] == 1:
                continue
            status_dict[item['pid'] & 0x7f] = 0
            if index == self.current_keyboard_protocol_index:
                status_dict[item['pid'] & 0x7f] = 1

        for index, item in enumerate(self.mouse_protocol_list):
            if item['pid'] & 0x7f in status_dict and status_dict[item['pid'] & 0x7f] == 1:
                continue
            status_dict[item['pid'] & 0x7f] = 0
            if index == self.current_mouse_protocol_index:
                status_dict[item['pid'] & 0x7f] = 1

        for index, item in enumerate(self.gamepad_protocol_list):
            if item['pid'] & 0x7f in status_dict and status_dict[item['pid'] & 0x7f] == 1:
                continue
            status_dict[item['pid'] & 0x7f] = 0
            if index == self.current_gamepad_protocol_index:
                status_dict[item['pid'] & 0x7f] = 1

        protocol_bytes = []
        for key in status_dict:
            if key == PROTOCOL_OFF['pid']:
                continue
            if status_dict[key]:
                protocol_bytes.append(key | 0x80)
            else:
                protocol_bytes.append(key)

        this_msg = list(set_protocl_spi_msg_template)
        this_msg[3:3+len(protocol_bytes)] = protocol_bytes

        self.current_keyboard_protocol = self.kb_protocol_list[self.current_keyboard_protocol_index]
        self.current_mouse_protocol = self.mouse_protocol_list[self.current_mouse_protocol_index]
        self.current_gamepad_protocol = self.gamepad_protocol_list[self.current_gamepad_protocol_index]

        if this_msg == self.last_spi_message:
            print("SPI: no need to send")
            return
        print("set_protocol:", [hex(x) for x in this_msg])
        usb4vc_usb_scan.set_protocol(this_msg)
        print('new status:', [hex(x) for x in usb4vc_usb_scan.get_pboard_info()])
        self.last_spi_message = list(this_msg)

    def action(self, level, page):
        if level == 0:
            if page == 2:
                usb_present, config_path = check_usb_drive()
                if usb_present is False:
                    with canvas(usb4vc_oled.oled_device) as draw:
                        usb4vc_oled.oled_print_centered("Error:", usb4vc_oled.font_medium, 0, draw)
                        usb4vc_oled.oled_print_centered(str(config_path), usb4vc_oled.font_regular, 16, draw)
                    time.sleep(3)
                    self.goto_level(0)
                else:
                    with canvas(usb4vc_oled.oled_device) as draw:
                        usb4vc_oled.oled_print_centered("Copying", usb4vc_oled.font_medium, 0, draw)
                        usb4vc_oled.oled_print_centered("Debug Log...", usb4vc_oled.font_medium, 16, draw)
                    copy_debug_log()
                    time.sleep(2)
                    with canvas(usb4vc_oled.oled_device) as draw:
                        usb4vc_oled.oled_print_centered("Copying custom", usb4vc_oled.font_medium, 0, draw)
                        usb4vc_oled.oled_print_centered("mapping...", usb4vc_oled.font_medium, 16, draw)
                    time.sleep(2)
                    update_from_usb(config_path)
                    with canvas(usb4vc_oled.oled_device) as draw:
                        usb4vc_oled.oled_print_centered("Update complete!", usb4vc_oled.font_medium, 0, draw)
                        usb4vc_oled.oled_print_centered("Relaunching...", usb4vc_oled.font_medium, 16, draw)
                    time.sleep(3)
                    usb4vc_oled.oled_device.clear()
                    os._exit(0)
                self.goto_level(0)
            elif page == 3:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered("Updating...", usb4vc_oled.font_medium, 10, draw)
                fffff = usb4vc_check_update.download_latest_firmware(this_pboard_id)
                if fffff != 0:
                    with canvas(usb4vc_oled.oled_device) as draw:
                        usb4vc_oled.oled_print_centered("Unable to download", usb4vc_oled.font_medium, 0, draw)
                        usb4vc_oled.oled_print_centered(f"firmware: {fffff}", usb4vc_oled.font_medium, 16, draw)
                elif update_pboard_firmware(this_pboard_id):
                        with canvas(usb4vc_oled.oled_device) as draw:
                            usb4vc_oled.oled_print_centered("Firmware updated!", usb4vc_oled.font_medium, 10, draw)
                else:
                    with canvas(usb4vc_oled.oled_device) as draw:
                        usb4vc_oled.oled_print_centered("FW update ERR or", usb4vc_oled.font_medium, 0, draw)
                        usb4vc_oled.oled_print_centered("already newest", usb4vc_oled.font_medium, 15, draw)
                time.sleep(3)
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered("Updating code...", usb4vc_oled.font_medium, 10, draw)
                time.sleep(1)
                update_result = usb4vc_check_update.update(temp_dir_path)
                if update_result[0] == 0:
                    with canvas(usb4vc_oled.oled_device) as draw:
                        usb4vc_oled.oled_print_centered("Update complete!", usb4vc_oled.font_medium, 0, draw)
                        usb4vc_oled.oled_print_centered("Relaunching...", usb4vc_oled.font_medium, 16, draw)
                else:
                    with canvas(usb4vc_oled.oled_device) as draw:
                        usb4vc_oled.oled_print_centered("Update failed:", usb4vc_oled.font_medium, 0, draw)
                        usb4vc_oled.oled_print_centered(f"{update_result[-1]} {update_result[0]}", usb4vc_oled.font_regular, 16, draw)
                time.sleep(4)
                usb4vc_oled.oled_device.clear()
                os._exit(0)
            elif page == 4:
                try:
                    usb4vc_show_ev.ev_loop([plus_button, minus_button, enter_button])
                except Exception as e:
                    print('exception ev_loop:', e)
                self.goto_level(0)
            elif page == 5:
                self.paired_devices_list = list(get_paired_devices())
                self.page_size[4] = len(self.paired_devices_list) + 1
                self.goto_level(4)
            elif page == 6:
                self.goto_level(2)
            else:
                self.goto_level(1)
        if level == 1:
            if page == 0:
                self.current_keyboard_protocol_index = (self.current_keyboard_protocol_index + 1) % len(self.kb_protocol_list)
            if page == 1:
                self.current_mouse_protocol_index = (self.current_mouse_protocol_index + 1) % len(self.mouse_protocol_list)
            if page == 2:
                self.current_gamepad_protocol_index = (self.current_gamepad_protocol_index + 1) % len(self.gamepad_protocol_list)
            if page == 3:
                self.current_mouse_sensitivity_offset_index = (self.current_mouse_sensitivity_offset_index + 1) % len(mouse_sensitivity_list)
            if page == 4:
                configuration_dict[this_pboard_id]["keyboard_protocol_index"] = self.current_keyboard_protocol_index
                configuration_dict[this_pboard_id]["mouse_protocol_index"] = self.current_mouse_protocol_index
                configuration_dict[this_pboard_id]["mouse_sensitivity_index"] = self.current_mouse_sensitivity_offset_index
                configuration_dict[this_pboard_id]["gamepad_protocol_index"] = self.current_gamepad_protocol_index
                save_config()
                self.send_protocol_set_spi_msg()
                self.goto_level(0)
        if level == 2:
            if page == 0:  
                self.switch_page(1)
            if page == 2:
                self.goto_level(0)
            if page == 3:
                self.goto_level(0)
        if level == 3:
            if page == self.page_size[3] - 1:
                self.goto_level(0)
            else:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered("Pairing...", usb4vc_oled.font_medium, 0, draw)
                    usb4vc_oled.oled_print_centered("Please wait", usb4vc_oled.font_medium, 15, draw)
                print("pairing", self.bluetooth_device_list[page])
                bt_mac_addr = self.bluetooth_device_list[page][0]
                is_successful, result_message = pair_device(bt_mac_addr)
                self.pairing_result = result_message.split('.')[-1].strip()[-22:]
                if is_successful:
                    os.system(f'timeout {self.bt_scan_timeout_sec} bluetoothctl --agent NoInputNoOutput trust {bt_mac_addr}')
                    os.system(f'timeout {self.bt_scan_timeout_sec} bluetoothctl --agent NoInputNoOutput connect {bt_mac_addr}')
                self.goto_level(2)
                self.goto_page(2)
        if level == 4:
            if page == self.page_size[4] - 1:
                self.goto_level(0)
            else:
                os.system(f'timeout 5 bluetoothctl --agent NoInputNoOutput untrust {self.paired_devices_list[page][0]}')
                os.system(f'timeout 5 bluetoothctl --agent NoInputNoOutput remove {self.paired_devices_list[page][0]}')
                self.goto_level(0)
        if level == 5:
            if page == 0:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered("Wait Until Green", usb4vc_oled.font_medium, 0, draw)
                    usb4vc_oled.oled_print_centered("LED Stops Blinking", usb4vc_oled.font_medium, 15, draw)
                time.sleep(2)
                os.system("sudo halt")
                while 1:
                    time.sleep(1)
            if page == 1:
                usb4vc_oled.oled_device.clear()
                os._exit(0)
            if page == 2:
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered("Rebooting...", usb4vc_oled.font_medium, 0, draw)
                    usb4vc_oled.oled_print_centered("Unplug if stuck >10s", usb4vc_oled.font_regular, 16, draw)
                os.system("sudo reboot")
                while 1:
                    time.sleep(1)
            if page == 3:
                usb4vc_oled.oled_device.clear()
                os._exit(169)
            if page == 4:
                self.goto_level(0)
        self.display_curent_page()

    def action_current_page(self):
        self.action(self.current_level, self.current_page)

    def display_curent_page(self):
        self.display_page(self.current_level, self.current_page)

    def update_usb_status(self):
        if self.current_level == 0 and self.current_page == 0:
            self.display_page(0, 0)

    def update_board_status(self):
        if self.current_level == 0 and self.current_page == 1:
            self.display_page(0, 1)

pboard_database = {
    PBOARD_ID_UNKNOWN:{'author':'Unknown', 'fw_ver':(0,0,0), 'full_name':'Unknown', 'hw_rev':0, 'protocol_list_keyboard':raw_keyboard_protocols, 'protocol_list_mouse':raw_mouse_protocols, 'protocol_list_gamepad':raw_gamepad_protocols},
    PBOARD_ID_IBMPC:{'author':'dekuNukem', 'fw_ver':(0,0,0), 'full_name':'IBM PC Compatible', 'hw_rev':0, 'protocol_list_keyboard':ibmpc_keyboard_protocols, 'protocol_list_mouse':ibmpc_mouse_protocols, 'protocol_list_gamepad':ibmpc_gamepad_protocols},
    PBOARD_ID_ADB:{'author':'dekuNukem', 'fw_ver':(0,0,0), 'full_name':'Apple Desktop Bus', 'hw_rev':0, 'protocol_list_keyboard':adb_keyboard_protocols, 'protocol_list_mouse':adb_mouse_protocols, 'protocol_list_gamepad':adb_gamepad_protocols},
    PBOARD_ID_APPLE_LISA_MAC_ADB:{'author':'dekuNukem', 'fw_ver':(0,0,0), 'full_name':'Apple Lisa/Mac/ADB', 'hw_rev':0, 'protocol_list_keyboard':lisa_mac_adb_keyboard_protocols, 'protocol_list_mouse':lisa_mac_adb_mouse_protocols, 'protocol_list_gamepad':lisa_mac_adb_gamepad_protocols},
}

def get_pboard_dict(pid):
    if pid not in pboard_database:
        pid = 0
    return pboard_database[pid]

def get_mouse_sensitivity():
    return mouse_sensitivity_list[configuration_dict[this_pboard_id]["mouse_sensitivity_index"]]

def ui_init():
    global pboard_info_spi_msg
    global this_pboard_id
    load_config()
    pboard_info_spi_msg = usb4vc_usb_scan.get_pboard_info()
    print("PB INFO:", pboard_info_spi_msg)
    this_pboard_id = pboard_info_spi_msg[3]
    if this_pboard_id in pboard_database:
        # load custom profile mapping into protocol list
        for item in custom_profile_list:
            this_mapping_bid = board_id_lookup.get(item['protocol_board'], 0)
            if this_mapping_bid == this_pboard_id and item['device_type'] in pboard_database[this_pboard_id]:
                this_mapping_pid = protocol_id_lookup.get(item['protocol_name'])
                item['pid'] = this_mapping_pid
                pboard_database[this_pboard_id][item['device_type']].append(item)
        pboard_database[this_pboard_id]['hw_rev'] = pboard_info_spi_msg[4]
        pboard_database[this_pboard_id]['fw_ver'] = (pboard_info_spi_msg[5], pboard_info_spi_msg[6], pboard_info_spi_msg[7])
    if 'rpi_app_ver' not in configuration_dict:
        configuration_dict['rpi_app_ver'] = RPI_APP_VERSION_TUPLE
    if this_pboard_id not in configuration_dict:
        configuration_dict[this_pboard_id] = {"keyboard_protocol_index":1, "mouse_protocol_index":1, "mouse_sensitivity_index":0, "gamepad_protocol_index":1}

plus_button = my_button(PLUS_BUTTON_PIN)
minus_button = my_button(MINUS_BUTTON_PIN)
enter_button = my_button(ENTER_BUTTON_PIN)
shutdown_button = my_button(SHUTDOWN_BUTTON_PIN)

class oled_sleep_control(object):
    def __init__(self):
        super(oled_sleep_control, self).__init__()
        self.is_sleeping = False
        self.last_input_event = time.time()
        self.ui_loop_count = 0
    def sleep(self):
        if self.is_sleeping is False:
            print("sleeping!")
            usb4vc_oled.oled_device.clear()
            self.is_sleeping = True
            # GPIO.output(SLEEP_LED_PIN, GPIO.HIGH)
    def wakeup(self):
        if self.is_sleeping:
            print("waking up!")
            my_menu.display_curent_page()
            self.last_input_event = time.time()
            self.is_sleeping = False
            # GPIO.output(SLEEP_LED_PIN, GPIO.LOW)
    def check_sleep(self):
        # time.time() might jump ahead a lot when RPi gets its time from network
        # this ensures OLED won't go to sleep too early
        if self.ui_loop_count <= 1500:
            return
        if time.time() - self.last_input_event > 180:
            self.sleep()
        else:
            self.wakeup()
    def kick(self):
        self.last_input_event = time.time()

my_oled = oled_sleep_control()
my_menu = None
def ui_worker():
    global my_menu
    print(configuration_dict)
    print("ui_worker started")
    my_menu = usb4vc_menu(get_pboard_dict(this_pboard_id), configuration_dict[this_pboard_id])
    my_menu.display_page(0, 0)
    for x in range(2):
        GPIO.output(SLEEP_LED_PIN, GPIO.HIGH)
        time.sleep(0.2)
        GPIO.output(SLEEP_LED_PIN, GPIO.LOW)
        time.sleep(0.2)
    while 1: 
        time.sleep(0.1)
        my_oled.ui_loop_count += 1
        if my_oled.is_sleeping is False and my_oled.ui_loop_count % 5 == 0:
            my_menu.update_usb_status()
            my_menu.update_board_status()

        if plus_button.is_pressed():
            my_oled.kick()
            if my_oled.is_sleeping:
                my_oled.wakeup()
            elif my_menu.current_level != 2:
                my_menu.switch_page(1)
                my_menu.display_curent_page()

        if minus_button.is_pressed():
            my_oled.kick()
            if my_oled.is_sleeping:
                my_oled.wakeup()
            elif my_menu.current_level != 2:
                my_menu.switch_page(-1)
                my_menu.display_curent_page()

        if enter_button.is_pressed():
            my_oled.kick()
            if my_oled.is_sleeping:
                my_oled.wakeup()
            else:
                my_menu.action_current_page()

        if shutdown_button.is_pressed():
            my_oled.kick()
            if my_oled.is_sleeping:
                my_oled.wakeup()
            else:
                my_menu.goto_level(5)
                my_menu.display_curent_page()
        my_oled.check_sleep()

def get_gamepad_protocol():
    return my_menu.current_gamepad_protocol

def oled_print_model_changed():
    with canvas(usb4vc_oled.oled_device) as draw:
        usb4vc_oled.oled_print_centered("RPi Model Changed!", usb4vc_oled.font_regular, 0, draw)
        usb4vc_oled.oled_print_centered("Recompiling BT Driver", usb4vc_oled.font_regular, 10, draw)
        usb4vc_oled.oled_print_centered("Might take a while...", usb4vc_oled.font_regular, 20, draw)

def oled_print_oneline(msg):
    with canvas(usb4vc_oled.oled_device) as draw:
        usb4vc_oled.oled_print_centered(msg, usb4vc_oled.font_medium, 10, draw)

def oled_print_reboot():
    with canvas(usb4vc_oled.oled_device) as draw:
        usb4vc_oled.oled_print_centered("Done! Rebooting..", usb4vc_oled.font_medium, 0, draw)
        usb4vc_oled.oled_print_centered("Unplug if stuck >10s", usb4vc_oled.font_regular, 16, draw)

ui_thread = threading.Thread(target=ui_worker, daemon=True)

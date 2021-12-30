# https://luma-oled.readthedocs.io/en/latest/software.html

import os
import sys
import time
import threading
from demo_opts import get_device
from luma.core.render import canvas
from PIL import ImageFont
import RPi.GPIO as GPIO
import usb4vc_usb_scan
import usb4vc_shared
import json
import subprocess

data_dir_path = os.path.join(os.path.expanduser('~'), 'usb4vc_data')
config_file_path = os.path.join(data_dir_path, 'config.json')

def ensure_dir(dir_path):
    print('ensure_dir', dir_path)
    if not os.path.exists(dir_path):
        os.makedirs(dir_path)

ensure_dir(data_dir_path)

PLUS_BUTTON_PIN = 27
MINUS_BUTTON_PIN = 19
ENTER_BUTTON_PIN = 22
SHUTDOWN_BUTTON_PIN = 21

GPIO.setmode(GPIO.BCM)

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
        

OLED_WIDTH = 128
OLED_HEIGHT = 32

my_arg = ['--display', 'ssd1306', '--interface', 'spi', '--spi-port', '0', '--spi-device', '1', '--gpio-reset', '6', '--gpio-data-command', '5', '--width', str(OLED_WIDTH), '--height', str(OLED_HEIGHT), '--spi-bus-speed', '2000000']
device = get_device(my_arg)

PBOARD_ID_UNKNOWN = 0
PBOARD_ID_IBMPC = 1
PBOARD_ID_ADB = 2
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

IBMPC_GGP_DEAULT_MAPPING = {
    # buttons to buttons
    USBGP_BTN_X: {'code':usb4vc_shared.IBMPC_GGP_BTN_2[0], 'type':usb4vc_shared.IBMPC_GGP_BTN_2[1]},
    USBGP_BTN_B: {'code':usb4vc_shared.IBMPC_GGP_BTN_3[0], 'type':usb4vc_shared.IBMPC_GGP_BTN_3[1]},
    USBGP_BTN_Y: {'code':usb4vc_shared.IBMPC_GGP_BTN_4[0], 'type':usb4vc_shared.IBMPC_GGP_BTN_4[1]},
    USBGP_BTN_A: {'code':usb4vc_shared.IBMPC_GGP_BTN_1[0], 'type':usb4vc_shared.IBMPC_GGP_BTN_1[1]},
    USBGP_BTN_TL: {'code':usb4vc_shared.IBMPC_GGP_BTN_1[0], 'type':usb4vc_shared.IBMPC_GGP_BTN_1[1]},
    USBGP_BTN_TR: {'code':usb4vc_shared.IBMPC_GGP_BTN_2[0], 'type':usb4vc_shared.IBMPC_GGP_BTN_2[1]},
    # analog stick to analog stick
    USBGP_ABS_X: {'code':usb4vc_shared.IBMPC_GGP_JS1_X[0], 'type':usb4vc_shared.IBMPC_GGP_JS1_X[1]},
    USBGP_ABS_Y: {'code':usb4vc_shared.IBMPC_GGP_JS1_Y[0], 'type':usb4vc_shared.IBMPC_GGP_JS1_Y[1]},
    USBGP_ABS_HAT0X: {'code':usb4vc_shared.IBMPC_GGP_JS1_X[0], 'type':usb4vc_shared.IBMPC_GGP_JS1_X[1]},
    USBGP_ABS_HAT0Y: {'code':usb4vc_shared.IBMPC_GGP_JS1_Y[0], 'type':usb4vc_shared.IBMPC_GGP_JS1_Y[1]},
    USBGP_ABS_RX: {'code':usb4vc_shared.IBMPC_GGP_JS2_X[0], 'type':usb4vc_shared.IBMPC_GGP_JS2_X[1]},
    USBGP_ABS_RY: {'code':usb4vc_shared.IBMPC_GGP_JS2_Y[0], 'type':usb4vc_shared.IBMPC_GGP_JS2_Y[1]},
}

PROTOCOL_OFF = {'pid':0, 'display_name':"OFF"}
PROTOCOL_AT_PS2_KB = {'pid':1, 'display_name':"AT/PS2"}
PROTOCOL_XT_KB = {'pid':2, 'display_name':"PC XT"}
PROTOCOL_ADB_KB = {'pid':3, 'display_name':"ADB"}
PROTOCOL_PS2_MOUSE = {'pid':4, 'display_name':"PS/2"}
PROTOCOL_MICROSOFT_SERIAL_MOUSE = {'pid':5, 'display_name':"MS Serial"}
PROTOCOL_ADB_MOUSE = {'pid':6, 'display_name':"ADB"}
PROTOCOL_GENERIC_GAMEPORT_GAMEPAD = {'pid':7, 'display_name':"Generic PC", 'mapping':IBMPC_GGP_DEAULT_MAPPING}
PROTOCOL_GAMEPORT_GRAVIS_GAMEPAD = {'pid':8, 'display_name':"Gravis Pro"}
PROTOCOL_GAMEPORT_MICROSOFT_SIDEWINDER = {'pid':9, 'display_name':"MS Sidewinder"}
PROTOCOL_RAW_KEYBOARD = {'pid':125, 'display_name':"Raw data"}
PROTOCOL_RAW_MOUSE = {'pid':126, 'display_name':"Raw data"}
PROTOCOL_RAW_GAMEPAD = {'pid':127, 'display_name':"Raw data"}

custom_profile_1 = {
    'name':'test',
    'type':'plist_gamepad',
    'bid':PBOARD_ID_IBMPC,
    'pid':PROTOCOL_GENERIC_GAMEPORT_GAMEPAD['pid'],
    'mapping':
    {
        # buttons to buttons
        USBGP_BTN_X: {'code':usb4vc_shared.MOUSE_BTN_LEFT[0], 'type':usb4vc_shared.MOUSE_BTN_LEFT[1]},
        USBGP_BTN_B: {'code':usb4vc_shared.MOUSE_BTN_RIGHT[0], 'type':usb4vc_shared.MOUSE_BTN_RIGHT[1]},
        USBGP_BTN_Y: {'code':usb4vc_shared.MOUSE_BTN_MIDDLE[0], 'type':usb4vc_shared.MOUSE_BTN_MIDDLE[1]},
        USBGP_BTN_A: {'code':usb4vc_shared.MOUSE_BTN_MIDDLE[0], 'type':usb4vc_shared.MOUSE_BTN_MIDDLE[1]},
        # analog stick to analog stick
        USBGP_ABS_X: {'code':usb4vc_shared.MOUSE_X[0], 'type':usb4vc_shared.MOUSE_X[1], 'deadzone_percent':15},
        USBGP_ABS_Y: {'code':usb4vc_shared.MOUSE_Y[0], 'type':usb4vc_shared.MOUSE_Y[1], 'deadzone_percent':15},
        USBGP_ABS_HAT0X: {'code':usb4vc_shared.IBMPC_GGP_JS1_X[0], 'type':usb4vc_shared.IBMPC_GGP_JS1_X[1]},
        USBGP_ABS_HAT0Y: {'code':usb4vc_shared.IBMPC_GGP_JS1_Y[0], 'type':usb4vc_shared.IBMPC_GGP_JS1_Y[1]},
        # buttons to analog stick
        USBGP_BTN_TL: {'code':usb4vc_shared.IBMPC_GGP_BTN_1[0], 'type':usb4vc_shared.IBMPC_GGP_BTN_1[1]},
        USBGP_BTN_TR: {'code':usb4vc_shared.IBMPC_GGP_BTN_2[0], 'type':usb4vc_shared.IBMPC_GGP_BTN_2[1]},
        # buttons to keyboard key
        USBGP_BTN_START: {'code':usb4vc_shared.KB_KEY_A[0], 'type':usb4vc_shared.KB_KEY_A[1]},
        USBGP_BTN_SELECT: {'code':usb4vc_shared.KB_KEY_B[0], 'type':usb4vc_shared.KB_KEY_B[1]},
        USBGP_BTN_TL: {'code':usb4vc_shared.IBMPC_GGP_JS1_X_NEG[0], 'type':usb4vc_shared.IBMPC_GGP_JS1_X_NEG[1]},
        USBGP_BTN_TR: {'code':usb4vc_shared.IBMPC_GGP_JS1_X_POS[0], 'type':usb4vc_shared.IBMPC_GGP_JS1_X_POS[1]},
        # analog stick to keyboard key
        USBGP_ABS_RX: {'type':'pb_kb', 'code':usb4vc_shared.KB_KEY_RIGHT[0], 'code_neg':usb4vc_shared.KB_KEY_LEFT[0], 'deadzone_percent':15},
        USBGP_ABS_RY: {'type':'pb_kb', 'code':usb4vc_shared.KB_KEY_DOWN[0], 'code_neg':usb4vc_shared.KB_KEY_UP[0], 'deadzone_percent':15},
    }
}

custom_profile_2 = {
    'name':'map2kb',
    'type':'plist_gamepad',
    'bid':PBOARD_ID_IBMPC,
    'pid':PROTOCOL_GENERIC_GAMEPORT_GAMEPAD['pid'],
    'mapping':
    {
        # USBGP_BTN_X: KB_KEY_LEFT,
        # USBGP_BTN_B: KB_KEY_RIGHT,
        # USBGP_BTN_Y: KB_KEY_UP,
        # USBGP_BTN_A: KB_KEY_DOWN,
        # USBGP_BTN_TL: KB_KEY_LCTRL,
        # USBGP_BTN_TR: KB_KEY_LCTRL,
    }
}

custom_profile_list = [custom_profile_1, custom_profile_2]

"""
OLED for USB4VC
128*32

3 font sizes available, regular/large: ProggyTiny.ttf, medium: ChiKareGo2.ttf.

regular ProggyTiny:
16 point, 3 lines, y=0, 10, 20

large type ProggyTiny:
32 point, 2 lines (1 large + 1 regular), y=0, 20

medium type ChiKareGo2:
16 point, 2 lines, y=0, 15 

characters per line:
regular font: 21 
medium font: 16
large font: 11

int(sys.argv[1])
sh sync.sh; ssh -t pi@192.168.1.56 "pkill python3;cd ~/usb4vc;python3 usb4vc_main.py"
"""

font_regular = ImageFont.truetype("ProggyTiny.ttf", 16)
font_medium = ImageFont.truetype("ChiKareGo2.ttf", 16)
font_large = ImageFont.truetype("ProggyTiny.ttf", 32)

max_char_per_line = {font_regular:21, font_medium:17, font_large:11}
width_per_char = {font_regular:6, font_medium:8, font_large:12}

def oled_print_centered(text, font, y, this_canvas):
    text = text.strip()[:max_char_per_line[font]]
    start_x = int((OLED_WIDTH - (len(text) * width_per_char[font]))/2)
    if start_x < 0:
        start_x = 0
    this_canvas.text((start_x, y), text, font=font, fill="white")

keyboard_protocol_options_ibmpc = [PROTOCOL_OFF, PROTOCOL_AT_PS2_KB, PROTOCOL_XT_KB]
mouse_protocol_options_ibmpc = [PROTOCOL_OFF, PROTOCOL_PS2_MOUSE, PROTOCOL_MICROSOFT_SERIAL_MOUSE]
gamepad_protocol_options_ibmpc = [PROTOCOL_OFF, PROTOCOL_GENERIC_GAMEPORT_GAMEPAD, PROTOCOL_GAMEPORT_GRAVIS_GAMEPAD, PROTOCOL_GAMEPORT_MICROSOFT_SIDEWINDER]

keyboard_protocol_options_adb = [PROTOCOL_OFF, PROTOCOL_ADB_KB]
mouse_protocol_options_adb = [PROTOCOL_OFF, PROTOCOL_ADB_MOUSE]
gamepad_protocol_options_adb = [PROTOCOL_OFF]

keyboard_protocol_options_raw = [PROTOCOL_OFF, PROTOCOL_RAW_KEYBOARD]
mouse_protocol_options_raw = [PROTOCOL_OFF, PROTOCOL_RAW_MOUSE]
gamepad_protocol_options_raw = [PROTOCOL_OFF, PROTOCOL_RAW_GAMEPAD]

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
    exit_code = os.system('timeout 1 bluetoothctl agent on')
    if exit_code == LINUX_EXIT_CODE_TIMEOUT:
        return 2, 'bluetoothctl stuck'
    return 0, ''

def scan_bt_devices(timeout_sec = 5):
    exit_code = os.system(f"timeout {timeout_sec} bluetoothctl scan on") >> 8
    if exit_code != LINUX_EXIT_CODE_TIMEOUT:
        return None, 'scan error'
    device_str = subprocess.getoutput("bluetoothctl devices")
    dev_list = []
    for line in device_str.replace('\r', '').split('\n'):
        if 'device' not in line.lower():
            continue
        line_split = line.split(' ', maxsplit=2)
        # skip if device has no name
        if len(line_split) < 3 or line_split[2].count('-') == 5:
            continue
        dev_list.append((line_split[1], line_split[2]))
    if len(dev_list) == 0:
        return None, 'Nothing was found'
    return dev_list, ''

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
        print("config load failed!", e)

def save_config():
    try:
        with open(config_file_path, 'w', encoding='utf8') as save_file:
           save_file.write(json.dumps(configuration_dict))
    except Exception as e:
        print("config save failed!", e)

class usb4vc_menu(object):
    def cap_index(self, index, list_size):
        if index >= list_size:
            return 0
        return index
    def __init__(self, pboard, conf_dict):
        super(usb4vc_menu, self).__init__()
        self.current_level = 0
        self.current_page = 0
        self.level_size = 4
        self.page_size = [3, 5, 4, 1]
        self.kb_opts = list(pboard['plist_keyboard'])
        self.mouse_opts = list(pboard['plist_mouse'])
        self.gamepad_opts = list(pboard['plist_gamepad'])
        self.pb_info = dict(pboard)
        self.current_keyboard_protocol_index = self.cap_index(conf_dict['keyboard_protocol_index'], len(self.kb_opts))
        self.current_mouse_protocol_index = self.cap_index(conf_dict["mouse_protocol_index"], len(self.mouse_opts))
        self.current_mouse_sensitivity_offset_index = self.cap_index(conf_dict["mouse_sensitivity_index"], len(mouse_sensitivity_list))
        self.current_gamepad_protocol_index = self.cap_index(conf_dict["gamepad_protocol_index"], len(self.gamepad_opts))
        self.current_keyboard_protocol = self.kb_opts[self.current_keyboard_protocol_index]
        self.current_mouse_protocol = self.mouse_opts[self.current_mouse_protocol_index]
        self.current_gamepad_protocol = self.gamepad_opts[self.current_gamepad_protocol_index]
        self.last_spi_message = []
        self.bluetooth_device_list = None
        self.error_message = ''
        self.pairing_result = ''
        self.send_spi()

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
                with canvas(device) as draw:
                    mouse_count, kb_count, gp_count = usb4vc_usb_scan.get_device_count()
                    draw.text((0, 0),  f"KBD {kb_count} {self.current_keyboard_protocol['display_name']}", font=font_regular, fill="white")
                    draw.text((0, 10), f"MOS {mouse_count} {self.current_mouse_protocol['display_name']}", font=font_regular, fill="white")
                    draw.text((0, 20), f"GPD {gp_count} {self.current_gamepad_protocol['display_name']}", font=font_regular, fill="white")
            if page == 1:
                with canvas(device) as draw:
                    draw.text((0, 0), f"PB:{self.pb_info['full_name']}", font=font_regular, fill="white")
                    draw.text((0, 10), f"FW:{self.pb_info['fw_ver'][0]}.{self.pb_info['fw_ver'][1]}.{self.pb_info['fw_ver'][2]}  REV:{self.pb_info['hw_rev']}", font=font_regular, fill="white")
                    draw.text((0, 20), f"IP: 192.168.231.12", font=font_regular, fill="white")
            if page == 2:
                with canvas(device) as draw:
                    oled_print_centered("Pair Bluetooth", font_medium, 10, draw)

        if level == 1:
            if page == 0:
                with canvas(device) as draw:
                    oled_print_centered("Keyboard Protocol", font_medium, 0, draw)
                    oled_print_centered(self.kb_opts[self.current_keyboard_protocol_index]['display_name'], font_medium, 15, draw)
            if page == 1:
                with canvas(device) as draw:
                    oled_print_centered("Mouse Protocol", font_medium, 0, draw)
                    oled_print_centered(self.mouse_opts[self.current_mouse_protocol_index]['display_name'], font_medium, 15, draw)
            if page == 2:
                with canvas(device) as draw:
                    oled_print_centered("Gamepad Protocol", font_medium, 0, draw)
                    oled_print_centered(self.gamepad_opts[self.current_gamepad_protocol_index]['display_name'], font_medium, 15, draw)
            if page == 3:
                with canvas(device) as draw:
                    oled_print_centered("Mouse Sensitivity", font_medium, 0, draw)
                    oled_print_centered(f"{mouse_sensitivity_list[self.current_mouse_sensitivity_offset_index]}", font_medium, 15, draw)
            if page == 4:
                with canvas(device) as draw:
                    oled_print_centered("Save & Quit", font_medium, 10, draw)
        if level == 2:
            if page == 0:
                with canvas(device) as draw:
                    oled_print_centered("Put your device in", font_regular, 0, draw)
                    oled_print_centered("pairing mode now.", font_regular, 10, draw)
                    oled_print_centered("Ready to start?", font_regular, 20, draw)
            if page == 1:
                with canvas(device) as draw:
                    oled_print_centered("Scanning for new", font_regular, 0, draw)
                    oled_print_centered("Bluetooth devices...", font_regular, 10, draw)
                    oled_print_centered("One moment please", font_regular, 20, draw)
                result, self.error_message = bt_setup()
                if result != 0:
                    self.goto_page(3)
                    self.display_curent_page()
                    return
                self.bluetooth_device_list, self.error_message = scan_bt_devices(10)
                if self.bluetooth_device_list is None:
                    self.goto_page(3)
                    self.display_curent_page()
                    return
                print("BT LIST:", self.bluetooth_device_list)
                # set up level 3 menu structure
                self.page_size[3] = len(self.bluetooth_device_list) + 1
                self.goto_level(3)
                self.display_curent_page()
            if page == 2:
                with canvas(device) as draw:
                    oled_print_centered("Pairing result:", font_medium, 0, draw)
                    oled_print_centered(self.pairing_result, font_regular, 15, draw)
            if page == 3:
                with canvas(device) as draw:
                    oled_print_centered("Bluetooth Error!", font_medium, 0, draw)
                    oled_print_centered(self.error_message, font_regular, 15, draw)
        if level == 3:
            if page == self.page_size[3] - 1:
                with canvas(device) as draw:
                    oled_print_centered("Exit", font_medium, 10, draw)
            else:
                with canvas(device) as draw:
                    oled_print_centered("Pair this device?", font_medium, 0, draw)
                    oled_print_centered(f"{self.bluetooth_device_list[page][1]}", font_medium, 15, draw)

    def send_spi(self):
        status_dict = {}
        for index, item in enumerate(self.kb_opts):
            if item['pid'] & 0x7f in status_dict and status_dict[item['pid'] & 0x7f] == 1:
                continue
            status_dict[item['pid'] & 0x7f] = 0
            if index == self.current_keyboard_protocol_index:
                status_dict[item['pid'] & 0x7f] = 1

        for index, item in enumerate(self.mouse_opts):
            if item['pid'] & 0x7f in status_dict and status_dict[item['pid'] & 0x7f] == 1:
                continue
            status_dict[item['pid'] & 0x7f] = 0
            if index == self.current_mouse_protocol_index:
                status_dict[item['pid'] & 0x7f] = 1

        for index, item in enumerate(self.gamepad_opts):
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

        this_msg = list(usb4vc_shared.set_protocl_spi_msg_template)
        this_msg[3:3+len(protocol_bytes)] = protocol_bytes

        self.current_keyboard_protocol = self.kb_opts[self.current_keyboard_protocol_index]
        self.current_mouse_protocol = self.mouse_opts[self.current_mouse_protocol_index]
        self.current_gamepad_protocol = self.gamepad_opts[self.current_gamepad_protocol_index]

        print('this', this_msg)
        print('last', self.last_spi_message)
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
                self.goto_level(2)
            else:
                self.goto_level(1)
        if level == 1:
            if page == 0:
                self.current_keyboard_protocol_index = (self.current_keyboard_protocol_index + 1) % len(self.kb_opts)
            if page == 1:
                self.current_mouse_protocol_index = (self.current_mouse_protocol_index + 1) % len(self.mouse_opts)
            if page == 2:
                self.current_gamepad_protocol_index = (self.current_gamepad_protocol_index + 1) % len(self.gamepad_opts)
            if page == 3:
                self.current_mouse_sensitivity_offset_index = (self.current_mouse_sensitivity_offset_index + 1) % len(mouse_sensitivity_list)
            if page == 4:
                configuration_dict[this_pboard_id]["keyboard_protocol_index"] = self.current_keyboard_protocol_index
                configuration_dict[this_pboard_id]["mouse_protocol_index"] = self.current_mouse_protocol_index
                configuration_dict[this_pboard_id]["mouse_sensitivity_index"] = self.current_mouse_sensitivity_offset_index
                configuration_dict[this_pboard_id]["gamepad_protocol_index"] = self.current_gamepad_protocol_index
                print(configuration_dict)
                save_config()
                self.send_spi()
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
                print("pairing", self.bluetooth_device_list[page])
                pair_timeout_sec = 15
                pair_result = subprocess.getoutput(f"timeout {pair_timeout_sec} bluetoothctl pair {self.bluetooth_device_list[page][0]}")
                print('-------', pair_result, '----------')
                self.pairing_result = 'Unknown'
                try:
                    self.pairing_result = [x for x in pair_result.split('\n') if '[' not in x][-1][-22:].split('.')[-1].strip()
                except Exception as e:
                    print('pair:', e)
                self.goto_level(2)
                self.goto_page(2)
        self.display_curent_page()

    def action_current_page(self):
        self.action(self.current_level, self.current_page);

    def display_curent_page(self):
        self.display_page(self.current_level, self.current_page)

    def update_usb_status(self):
        if self.current_level == 0 and self.current_page == 0:
            self.display_page(0, 0)

pboard_database = {
    PBOARD_ID_UNKNOWN:{'author':'Unknown', 'fw_ver':(0,0,0), 'full_name':'Unknown/Unplugged', 'hw_rev':0, 'plist_keyboard':keyboard_protocol_options_raw, 'plist_mouse':mouse_protocol_options_raw, 'plist_gamepad':gamepad_protocol_options_raw},
    PBOARD_ID_IBMPC:{'author':'dekuNukem', 'fw_ver':(0,0,0), 'full_name':'IBM PC Compatible', 'hw_rev':0, 'plist_keyboard':keyboard_protocol_options_ibmpc, 'plist_mouse':mouse_protocol_options_ibmpc, 'plist_gamepad':gamepad_protocol_options_ibmpc},
    PBOARD_ID_ADB:{'author':'dekuNukem', 'fw_ver':(0,0,0), 'full_name':'Apple Desktop Bus', 'hw_rev':0, 'plist_keyboard':keyboard_protocol_options_adb, 'plist_mouse':mouse_protocol_options_adb, 'plist_gamepad':gamepad_protocol_options_adb},
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
    this_pboard_id = pboard_info_spi_msg[3]
    if this_pboard_id in pboard_database:
        # load custom profile mapping into protocol list
        for item in custom_profile_list:
            if item['bid'] == this_pboard_id and item['type'] in pboard_database[this_pboard_id]:
                this_profile = {'pid':item['pid'], 'display_name':item['name'], 'mapping':item['mapping']}
                pboard_database[this_pboard_id][item['type']].append(this_profile)
        pboard_database[this_pboard_id]['hw_rev'] = pboard_info_spi_msg[4]
        pboard_database[this_pboard_id]['fw_ver'] = (pboard_info_spi_msg[5], pboard_info_spi_msg[6], pboard_info_spi_msg[7])
    if 'rpi_app_ver' not in configuration_dict:
        configuration_dict['rpi_app_ver'] = usb4vc_shared.RPI_APP_VERSION_TUPLE
    if this_pboard_id not in configuration_dict:
        configuration_dict[this_pboard_id] = {"keyboard_protocol_index":1, "mouse_protocol_index":1, "mouse_sensitivity_index":0, "gamepad_protocol_index":1}

plus_button = my_button(PLUS_BUTTON_PIN)
minus_button = my_button(MINUS_BUTTON_PIN)
enter_button = my_button(ENTER_BUTTON_PIN)
shutdown_button = my_button(SHUTDOWN_BUTTON_PIN)

my_menu = None

def ui_worker():
    global my_menu
    print("ui_worker started")
    my_menu = usb4vc_menu(get_pboard_dict(this_pboard_id), configuration_dict[this_pboard_id])
    my_menu.display_page(0, 0)
    print(configuration_dict)
    while 1: 
        time.sleep(0.1)
        my_menu.update_usb_status();

        if plus_button.is_pressed():
            print(time.time(), "PLUS_BUTTON pressed!")
            if my_menu.current_level != 2:
                my_menu.switch_page(1)
                my_menu.display_curent_page()

        if minus_button.is_pressed():
            print(time.time(), "MINUS_BUTTON pressed!")
            if my_menu.current_level != 2:
                my_menu.switch_page(-1)
                my_menu.display_curent_page()

        if enter_button.is_pressed():
            print(time.time(), "ENTER_BUTTON pressed!")
            my_menu.action_current_page()

        if shutdown_button.is_pressed():
            print(time.time(), "SHUTDOWN_BUTTON pressed!")

def get_gamepad_protocol():
    return my_menu.current_gamepad_protocol

ui_worker = threading.Thread(target=ui_worker, daemon=True)


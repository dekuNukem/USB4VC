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

"""
oled display object:

text
size
font
x, y
duration
afterwards

"""

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

GPIO.setup(PLUS_BUTTON_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.add_event_detect(PLUS_BUTTON_PIN, GPIO.FALLING, bouncetime=200)

GPIO.setup(MINUS_BUTTON_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.add_event_detect(MINUS_BUTTON_PIN, GPIO.FALLING, bouncetime=200)

GPIO.setup(ENTER_BUTTON_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.add_event_detect(ENTER_BUTTON_PIN, GPIO.FALLING, bouncetime=200)

GPIO.setup(SHUTDOWN_BUTTON_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.add_event_detect(SHUTDOWN_BUTTON_PIN, GPIO.FALLING, bouncetime=200)

OLED_WIDTH = 128
OLED_HEIGHT = 32

my_arg = ['--display', 'ssd1306', '--interface', 'spi', '--spi-port', '0', '--spi-device', '1', '--gpio-reset', '6', '--gpio-data-command', '5', '--width', str(OLED_WIDTH), '--height', str(OLED_HEIGHT), '--spi-bus-speed', '2000000']
device = get_device(my_arg)

pboard_info_spi_msg = [0] * 32
this_pboard_id = 0

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

PROTOCOL_OFF = {'pid':0, 'display_name':"OFF"}
PROTOCOL_AT_PS2_KB = {'pid':1, 'display_name':"AT/PS2"}
PROTOCOL_XT_KB = {'pid':2, 'display_name':"PC XT"}
PROTOCOL_ADB_KB = {'pid':3, 'display_name':"ADB"}
PROTOCOL_PS2_MOUSE = {'pid':4, 'display_name':"PS/2"}
PROTOCOL_MICROSOFT_SERIAL_MOUSE = {'pid':5, 'display_name':"MS Serial"}
PROTOCOL_ADB_MOUSE = {'pid':6, 'display_name':"ADB"}
PROTOCOL_GENERIC_GAMEPORT_GAMEPAD = {'pid':7, 'display_name':"Generic PC"}
PROTOCOL_GAMEPORT_GRAVIS_GAMEPAD = {'pid':8, 'display_name':"Gravis Pro"}
PROTOCOL_GAMEPORT_MICROSOFT_SIDEWINDER = {'pid':9, 'display_name':"MS Sidewinder"}
PROTOCOL_RAW_KEYBOARD = {'pid':125, 'display_name':"Raw data"}
PROTOCOL_RAW_MOUSE = {'pid':126, 'display_name':"Raw data"}
PROTOCOL_RAW_GAMEPAD = {'pid':127, 'display_name':"Raw data"}

keyboard_protocol_options_ibmpc = [PROTOCOL_AT_PS2_KB, PROTOCOL_XT_KB, PROTOCOL_OFF]
mouse_protocol_options_ibmpc = [PROTOCOL_PS2_MOUSE, PROTOCOL_MICROSOFT_SERIAL_MOUSE, PROTOCOL_OFF]
gamepad_protocol_options_ibmpc = [PROTOCOL_GENERIC_GAMEPORT_GAMEPAD, PROTOCOL_GAMEPORT_GRAVIS_GAMEPAD, PROTOCOL_GAMEPORT_MICROSOFT_SIDEWINDER, PROTOCOL_OFF]

keyboard_protocol_options_adb = [PROTOCOL_ADB_KB, PROTOCOL_OFF]
mouse_protocol_options_adb = [PROTOCOL_ADB_MOUSE, PROTOCOL_OFF]
gamepad_protocol_options_adb = [PROTOCOL_OFF]

keyboard_protocol_options_raw = [PROTOCOL_RAW_KEYBOARD, PROTOCOL_OFF]
mouse_protocol_options_raw = [PROTOCOL_RAW_MOUSE, PROTOCOL_OFF]
gamepad_protocol_options_raw = [PROTOCOL_RAW_GAMEPAD, PROTOCOL_OFF]

mouse_sensitivity_list = [1, 1.25, 1.5, 1.75, 0.25, 0.5, 0.75]

configuration_dict = {}

"""
key is protocol card ID
conf_dict[pbid]:
hw revision
current keyboard protocol
current mouse protocol
current gamepad procotol
mouse sensitivity
"""

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
    def __init__(self, pboard, conf_dict):
        super(usb4vc_menu, self).__init__()
        self.current_level = 0
        self.current_page = 0
        self.level_size = 2
        self.page_size = [4, 5]
        self.last_refresh = 0
        self.kb_opts = list(pboard['kp'])
        self.mouse_opts = list(pboard['mp'])
        self.gamepad_opts = list(pboard['gp'])
        self.pb_info = dict(pboard)
        self.current_keyboard_protocol_index = conf_dict['keyboard_protocol_index'] % len(self.kb_opts)
        self.current_mouse_protocol_index = conf_dict["mouse_protocol_index"] % len(self.mouse_opts)
        self.current_mouse_sensitivity_offset_index = conf_dict["mouse_sensitivity_index"] % len(mouse_sensitivity_list)
        self.current_gamepad_protocol_index = conf_dict["gamepad_protocol_index"] % len(self.gamepad_opts)
        self.last_spi_message = []
        self.send_spi()

    def switch_page(self, amount):
        self.current_page = (self.current_page + amount) % self.page_size[self.current_level]

    def switch_level(self, amount):
        new_level = self.current_level + amount
        if new_level < self.level_size:
            self.current_level = new_level
            self.current_page = 0

    def display_page(self, level, page):
        if level == 0:
            if page == 0:
                with canvas(device) as draw:
                    draw.text((0, 0),  f"KeyBoard:{self.kb_opts[self.current_keyboard_protocol_index]['display_name']}", font=font_regular, fill="white")
                    draw.text((0, 10), f"Mouse:   {self.mouse_opts[self.current_mouse_protocol_index]['display_name']}", font=font_regular, fill="white")
                    draw.text((0, 20), f"GamePad: {self.gamepad_opts[self.current_gamepad_protocol_index]['display_name']}", font=font_regular, fill="white")
            if page == 1:
                with canvas(device) as draw:
                    draw.text((0, 0), f"USB Keyboard: {len(usb4vc_usb_scan.keyboard_opened_device_dict)}", font=font_regular, fill="white")
                    draw.text((0, 10), f"USB Mouse:    {len(usb4vc_usb_scan.mouse_opened_device_dict)}", font=font_regular, fill="white")
                    draw.text((0, 20), f"USB Gamepad:  {len(usb4vc_usb_scan.gamepad_opened_device_dict)}", font=font_regular, fill="white")
            if page == 2:
                with canvas(device) as draw:
                    draw.text((0, 0), f"PB:{self.pb_info['full_name']}", font=font_regular, fill="white")
                    draw.text((0, 10), f"FW:{self.pb_info['fw_ver'][0]}.{self.pb_info['fw_ver'][1]}.{self.pb_info['fw_ver'][2]}  REV:{self.pb_info['hw_rev']}", font=font_regular, fill="white")
                    draw.text((0, 20), f"IP: 192.168.231.12", font=font_regular, fill="white")
            if page == 3:
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

    def send_spi(self):
        protocol_bytes = []
        for index, item in enumerate(self.kb_opts):
            if item == PROTOCOL_OFF:
                continue
            protocol_bytes.append(item['pid'] & 0x7f)
            if index == self.current_keyboard_protocol_index:
                protocol_bytes[-1] |= 0x80

        for index, item in enumerate(self.mouse_opts):
            if item == PROTOCOL_OFF:
                continue
            protocol_bytes.append(item['pid'] & 0x7f)
            if index == self.current_mouse_protocol_index:
                protocol_bytes[-1] |= 0x80

        for index, item in enumerate(self.gamepad_opts):
            if item == PROTOCOL_OFF:
                continue
            protocol_bytes.append(item['pid'] & 0x7f)
            if index == self.current_gamepad_protocol_index:
                protocol_bytes[-1] |= 0x80

        this_msg = list(usb4vc_shared.set_protocl_spi_msg_template)
        this_msg[3:3+len(protocol_bytes)] = protocol_bytes

        if this_msg == self.last_spi_message:
            print("SPI: no need to send")
            return
        print("set_protocol:", this_msg)
        usb4vc_usb_scan.set_protocol(this_msg)
        print('new status:', usb4vc_usb_scan.get_pboard_info())
        self.last_spi_message = list(this_msg)

    def action(self, level, page):
        if level == 0:
            self.switch_level(1)
            self.display_curent_page()
        if level == 1:
            if page == 0:
                self.current_keyboard_protocol_index = (self.current_keyboard_protocol_index + 1) % len(self.kb_opts)
                self.display_curent_page()
            if page == 1:
                self.current_mouse_protocol_index = (self.current_mouse_protocol_index + 1) % len(self.mouse_opts)
                self.display_curent_page()
            if page == 2:
                self.current_gamepad_protocol_index = (self.current_gamepad_protocol_index + 1) % len(self.gamepad_opts)
                self.display_curent_page()
            if page == 3:
                self.current_mouse_sensitivity_offset_index = (self.current_mouse_sensitivity_offset_index + 1) % len(mouse_sensitivity_list)
                self.display_curent_page()
            if page == 4:
                configuration_dict[this_pboard_id]["keyboard_protocol_index"] = self.current_keyboard_protocol_index
                configuration_dict[this_pboard_id]["mouse_protocol_index"] = self.current_mouse_protocol_index
                configuration_dict[this_pboard_id]["mouse_sensitivity_index"] = self.current_mouse_sensitivity_offset_index
                configuration_dict[this_pboard_id]["gamepad_protocol_index"] = self.current_gamepad_protocol_index
                print(configuration_dict)
                save_config()
                self.send_spi()
                self.switch_level(-1)
                self.display_curent_page()

    def action_current_page(self):
        self.action(self.current_level, self.current_page);

    def display_curent_page(self):
        self.display_page(self.current_level, self.current_page)

    def update_usb_status(self):
        if self.current_level == 0 and self.current_page == 1 and time.time() - self.last_refresh > 0.2:
            self.display_page(0, 1)
            self.last_refresh = time.time()

pboard_database = {
    0:{'author':'Unknown', 'fw_ver':(0,0,0), 'full_name':'Unknown/Unplugged', 'hw_rev':0, 'kp':keyboard_protocol_options_raw, 'mp':mouse_protocol_options_raw, 'gp':gamepad_protocol_options_raw},
    1:{'author':'dekuNukem', 'fw_ver':(0,0,0), 'full_name':'IBM PC Compatible', 'hw_rev':0, 'kp':keyboard_protocol_options_ibmpc, 'mp':mouse_protocol_options_ibmpc, 'gp':gamepad_protocol_options_ibmpc},
    2:{'author':'dekuNukem', 'fw_ver':(0,0,0), 'full_name':'Apple Desktop Bus', 'hw_rev':0, 'kp':keyboard_protocol_options_adb, 'mp':mouse_protocol_options_adb, 'gp':gamepad_protocol_options_adb},
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
        pboard_database[this_pboard_id]['hw_rev'] = pboard_info_spi_msg[4]
        pboard_database[this_pboard_id]['fw_ver'] = (pboard_info_spi_msg[5], pboard_info_spi_msg[6], pboard_info_spi_msg[7])
    if 'rpi_app_ver' not in configuration_dict:
        configuration_dict['rpi_app_ver'] = usb4vc_shared.RPI_APP_VERSION_TUPLE
    if this_pboard_id not in configuration_dict:
        configuration_dict[this_pboard_id] = {"keyboard_protocol_index":0, "mouse_protocol_index":0, "mouse_sensitivity_index":0, "gamepad_protocol_index":0}

def ui_worker():
    print("ui_worker started")
    my_menu = usb4vc_menu(get_pboard_dict(this_pboard_id), configuration_dict[this_pboard_id])
    my_menu.display_page(0, 0)
    print(configuration_dict)
    while 1: 
        time.sleep(0.1)
        my_menu.update_usb_status();

        if GPIO.event_detected(PLUS_BUTTON_PIN):
            print(time.time(), "PLUS_BUTTON pressed!")
            my_menu.switch_page(1)
            my_menu.display_curent_page()

        if GPIO.event_detected(MINUS_BUTTON_PIN):
            print(time.time(), "MINUS_BUTTON pressed!")
            my_menu.switch_page(-1)
            my_menu.display_curent_page()

        if GPIO.event_detected(ENTER_BUTTON_PIN):
            print(time.time(), "ENTER_BUTTON pressed!")
            my_menu.action_current_page()

        if GPIO.event_detected(SHUTDOWN_BUTTON_PIN):
            print(time.time(), "SHUTDOWN_BUTTON pressed!")

ui_worker = threading.Thread(target=ui_worker, daemon=True)



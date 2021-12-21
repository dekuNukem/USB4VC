# https://luma-oled.readthedocs.io/en/latest/software.html

import sys
import time
import threading
from demo_opts import get_device
from luma.core.render import canvas
from PIL import ImageFont
import RPi.GPIO as GPIO
import usb4vc_usb_scan
import usb4vc_shared

"""
oled display object:

text
size
font
x, y
duration
afterwards

"""

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

keyboard_protocol_options_ibmpc = [PROTOCOL_OFF, PROTOCOL_AT_PS2_KB, PROTOCOL_XT_KB]
mouse_protocol_options_ibmpc = [PROTOCOL_OFF, PROTOCOL_PS2_MOUSE, PROTOCOL_MICROSOFT_SERIAL_MOUSE]
gamepad_protocol_options_ibmpc = [PROTOCOL_OFF, PROTOCOL_GENERIC_GAMEPORT_GAMEPAD, PROTOCOL_GAMEPORT_GRAVIS_GAMEPAD, PROTOCOL_GAMEPORT_MICROSOFT_SIDEWINDER]

keyboard_protocol_options_adb = [PROTOCOL_OFF, PROTOCOL_ADB_KB]
mouse_protocol_options_adb = [PROTOCOL_OFF, PROTOCOL_ADB_MOUSE]
gamepad_protocol_options_adb = [PROTOCOL_OFF]

keyboard_protocol_options_raw = [PROTOCOL_OFF, PROTOCOL_RAW_KEYBOARD]
mouse_protocol_options_raw = [PROTOCOL_OFF, PROTOCOL_RAW_MOUSE]
gamepad_protocol_options_raw = [PROTOCOL_OFF, PROTOCOL_RAW_GAMEPAD]

mouse_sensitivity_offset_list = [0, 0.2, 0.4, 0.6, -0.6, -0.4, -0.2]

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

def save_config():
    pass


class usb4vc_menu(object):
    def __init__(self, pboard):
        super(usb4vc_menu, self).__init__()
        self.current_level = 0
        self.current_page = 0
        self.level_size = 2
        self.page_size = [4, 5]
        self.last_refresh = 0
        self.kb_opts = list(pboard['kbp'])
        self.mouse_opts = list(pboard['mp'])
        self.gamepad_opts = list(pboard['gpp'])
        self.pb_info = dict(pboard)
        self.current_keyboard_protocol_index = 0
        self.current_mouse_protocol_index = 0
        self.current_mouse_sensitivity_offset_index = 0
        self.current_gamepad_protocol_index = 0

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
                    draw.text((0, 0),  f"KBD: {self.kb_opts[self.current_keyboard_protocol_index]['display_name']}", font=font_regular, fill="white")
                    draw.text((0, 10), f"MOS: {self.mouse_opts[self.current_mouse_protocol_index]['display_name']}", font=font_regular, fill="white")
                    draw.text((0, 20), f"GPD: {self.gamepad_opts[self.current_gamepad_protocol_index]['display_name']}", font=font_regular, fill="white")
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
                    oled_print_centered(f"{1 + mouse_sensitivity_offset_list[self.current_mouse_sensitivity_offset_index]}", font_medium, 15, draw)
            if page == 4:
                with canvas(device) as draw:
                    oled_print_centered("Back", font_medium, 10, draw)

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
                self.current_mouse_sensitivity_offset_index = (self.current_mouse_sensitivity_offset_index + 1) % len(mouse_sensitivity_offset_list)
                self.display_curent_page()
            if page == 4:
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

pboard_info_spi_msg = [0] * 32
pboard_database = {
    0:{'author':'Unknown', 'fw_ver':(0,0,0), 'full_name':'Unknown/Unplugged', 'hw_rev':0, 'kbp':keyboard_protocol_options_raw, 'mp':mouse_protocol_options_raw, 'gpp':gamepad_protocol_options_raw},
    1:{'author':'dekuNukem', 'fw_ver':(0,0,0), 'full_name':'IBM PC Compatible', 'hw_rev':0, 'kbp':keyboard_protocol_options_ibmpc, 'mp':mouse_protocol_options_ibmpc, 'gpp':gamepad_protocol_options_ibmpc},
    2:{'author':'dekuNukem', 'fw_ver':(0,0,0), 'full_name':'Apple Desktop Bus', 'hw_rev':0, , 'kbp':keyboard_protocol_options_adb, 'mp':mouse_protocol_options_adb, 'gpp':gamepad_protocol_options_adb},
}

def get_pboard_dict(pid):
    if pid not in pboard_database:
        pid = 0
    return pboard_database[pid]

def ui_init():
    global pboard_info_spi_msg
    pboard_info_spi_msg = usb4vc_usb_scan.get_pboard_info()
    if pboard_info_spi_msg[3] in pboard_database:
        pboard_database[pboard_info_spi_msg[3]]['hw_rev'] = pboard_info_spi_msg[4]
        pboard_database[pboard_info_spi_msg[3]]['fw_ver'] = (pboard_info_spi_msg[5], pboard_info_spi_msg[6], pboard_info_spi_msg[7])

def ui_worker():
    print("ui_worker started")
    my_menu = usb4vc_menu(get_pboard_dict(pboard_info_spi_msg[3]))
    my_menu.display_page(0, 0)
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



        
# https://luma-oled.readthedocs.io/en/latest/software.html

import sys
import time
import queue
import threading
from demo_opts import get_device
from luma.core.render import canvas
from PIL import ImageFont
import RPi.GPIO as GPIO
import usb4vc_usb_scan

"""
oled display object:

text
size
font
x, y
duration
afterwards

"""
oled_display_queue = queue.PriorityQueue()


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
$ sh sync.sh; ssh -t pi@192.168.1.56 "pkill python3;cd ~/usb4vc;python3 usb4vc_ui.py"
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

# persistent canvas, will need to clear areas before drawing new stuff
# oled_canvas = canvas(device)

# def print_welcome_screen(version_tuple):
#     with oled_canvas as ocan:
#         oled_print_centered("USB4VC", font_large, 0, ocan)
#         oled_print_centered(f"V{version_tuple[0]}.{version_tuple[1]}.{version_tuple[2]} dekuNukem", font_regular, 20, ocan)
#     device.contrast(1)

def oled_clear():
    device.clear()

"""
(level, page)

(0, 0) 
(0, 1) 
(0, 2) 
"""

class usb4vc_menu(object):
    def __init__(self):
        super(usb4vc_menu, self).__init__()
        self.current_level = 0
        self.current_page = 0
        self.level_size = 2
        self.page_size = [3, 6]
        self.last_refresh = 0

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
                    draw.text((0, 0), "Keyboard: PS/2", font=font_regular, fill="white")
                    draw.text((0, 10), "Mouse: Serial", font=font_regular, fill="white")
                    draw.text((0, 20), "Gamepad: Generic", font=font_regular, fill="white")
            if page == 1:
                with canvas(device) as draw:
                    draw.text((0, 0), f"USB Keyboard: {len(usb4vc_usb_scan.keyboard_opened_device_dict)}", font=font_regular, fill="white")
                    draw.text((0, 10), f"USB Mouse:    {len(usb4vc_usb_scan.mouse_opened_device_dict)}", font=font_regular, fill="white")
                    draw.text((0, 20), f"USB Gamepad:  {len(usb4vc_usb_scan.gamepad_opened_device_dict)}", font=font_regular, fill="white")
            if page == 2:
                with canvas(device) as draw:
                    draw.text((0, 0), "PBoard: IBM PC Compat", font=font_regular, fill="white")
                    draw.text((0, 10), "PBoard FW: 0.1.1", font=font_regular, fill="white")
                    draw.text((0, 20), "IP: 192.168.231.12", font=font_regular, fill="white")

        if level == 1:
            if page == 0:
                with canvas(device) as draw:
                    oled_print_centered("Keyboard Protocol", font_medium, 0, draw)
                    oled_print_centered("PS/2", font_medium, 15, draw)
            if page == 1:
                with canvas(device) as draw:
                    oled_print_centered("Mouse Protocol", font_medium, 0, draw)
                    oled_print_centered("Serial", font_medium, 15, draw)
            if page == 2:
                with canvas(device) as draw:
                    oled_print_centered("Mouse Sensitivity", font_medium, 0, draw)
                    oled_print_centered("1.0", font_medium, 15, draw)
            if page == 3:
                with canvas(device) as draw:
                    oled_print_centered("Gamepad Protocol", font_medium, 0, draw)
                    oled_print_centered("MS Sidewinder", font_medium, 15, draw)
            if page == 4:
                with canvas(device) as draw:
                    oled_print_centered("Pair Bluetooth", font_medium, 10, draw)
            if page == 5:
                with canvas(device) as draw:
                    oled_print_centered("Back", font_medium, 10, draw)

    def action(self, level, page):
        if level == 0:
            self.switch_level(1)
            self.display_curent_page()
        if level == 1:
            if page == 5:
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

my_menu = usb4vc_menu()

def ui_worker():
    print("ui_worker started")
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



        
# https://luma-oled.readthedocs.io/en/latest/software.html

import sys
import time
import queue
import threading
from demo_opts import get_device
from luma.core.render import canvas
from PIL import ImageFont

oled_display_queue = queue.PriorityQueue()

"""
oled display object:

text
size
font
x, y
duration
afterwards

"""

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
$ sh sync.sh; ssh -t pi@192.168.1.56 "pkill python3;cd ~/usb4vc;python3 usb4vc_oled.py"
"""

font_regular = ImageFont.truetype("ProggyTiny.ttf", 16)
font_medium = ImageFont.truetype("ChiKareGo2.ttf", 16)
font_large = ImageFont.truetype("ProggyTiny.ttf", 32)

max_char_per_line = {font_regular:21, font_medium:16, font_large:11}
width_per_char = {font_regular:6, font_medium:8, font_large:12}

def oled_print_centered(text, font, y, this_canvas):
    text = text.strip()[:max_char_per_line[font]]
    start_x = int((OLED_WIDTH - (len(text) * width_per_char[font]))/2)
    if start_x < 0:
        start_x = 0
    this_canvas.text((start_x, y), text, font=font, fill="white")

# persistent canvas, will need to clear areas before drawing new stuff
oled_canvas = canvas(device)

def print_welcome_screen(version_tuple):
    with oled_canvas as ocan:
        oled_print_centered("USB4VC", font_large, 0, ocan)
        oled_print_centered(f"V{version_tuple[0]}.{version_tuple[1]}.{version_tuple[2]} dekuNukem", font_regular, 20, ocan)

def oled_clear():
    device.clear()

def oled_queue_worker():
    print("oled_queue_worker started")
    while 1:
        # print(oled_display_queue.qsize())
        time.sleep(0.75)


oled_queue_worker = threading.Thread(target=oled_queue_worker, daemon=True)


# while 1:
#   with oled_canvas as ocan:
#       ocan.rectangle((0, 0, device.width-1, device.height-1), outline=0, fill=0)
#       oled_print_centered(f"{int(time.time())}", font_large, 0, ocan)
#   time.sleep(1)


# while 1:
#     with canvas(device) as draw:
#         # draw.rectangle(device.bounding_box, outline="white", fill="black")
        
#         # regular font test
#         draw.text((0, 0), "123456789012345678901234567890", font=font_regular, fill="white")
#         draw.text((0, 10), "ABCDEFGHIJ", font=font_regular, fill="white")
#         draw.text((0, 20), "abcdefghij", font=font_regular, fill="white")

#         # medium font test
#         # draw.text((0, 0), "123456789012345678901234567890", font=font_medium, fill="white")
#         # draw.text((0, 15), "123456789012345678901234567890", font=font_medium, fill="white")

#         # large font test
#         # draw.text((0, 0), "123456789012345678901234567890", font=font_large, fill="white")
#         # draw.text((0, 20), "123456789012345678901234567890", font=font_regular, fill="white")
#     time.sleep(1)

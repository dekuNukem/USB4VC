# https://luma-oled.readthedocs.io/en/latest/software.html

import sys
import time
from demo_opts import get_device
from luma.core.render import canvas
from PIL import ImageFont

my_arg = ['--display', 'sh1106', '--interface', 'spi', '--spi-port', '0', '--spi-device', '1', '--gpio-reset', '6', '--gpio-data-command', '5', '--width', '128', '--height', '32']
device = get_device(my_arg)

"""
OLED for USB4VC
128*32
command&conquer ttf font, 12 point
3 lines, y=0, 10, 20
up to 20 characters per line
"""

font_path = "cc.ttf"
font2 = ImageFont.truetype(font_path, 24)

while 1:
	with canvas(device) as draw:
		draw.rectangle(device.bounding_box, outline="white", fill="black")
		draw.text((0, 0), "USB4VC", font=font2, fill="white")
		# draw.text((0, 0), "888888888877777777776666666666", font=font2, fill="white")
		# draw.text((0, 10), "ABCDEFGHIJ", font=font2, fill="white")
		# draw.text((0, 20), "abcdefghij", font=font2, fill="white")
	time.sleep(1)
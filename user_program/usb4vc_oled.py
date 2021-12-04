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
sh sync.sh; ssh pi@192.168.1.56 "pkill python3;cd ~/usb4vc;python3 usb4vc_oled.py 16"

"""

font_regular = ImageFont.truetype("ProggyTiny.ttf", 16)
font_medium = ImageFont.truetype("ChiKareGo2.ttf", int(sys.argv[1]))
font_large = ImageFont.truetype("ProggyTiny.ttf", 32)

while 1:
	with canvas(device) as draw:
		# draw.rectangle(device.bounding_box, outline="white", fill="black")
		
		# regular font test
		# draw.text((0, 0), "123456789012345678901234567890", font=font_regular, fill="white")
		# draw.text((0, 10), "ABCDEFGHIJ", font=font_regular, fill="white")
		# draw.text((0, 20), "abcdefghij", font=font_regular, fill="white")

		# medium font test
		# draw.text((0, 0), "123456789012345678901234567890", font=font_medium, fill="white")
		# draw.text((0, 15), "123456789012345678901234567890", font=font_medium, fill="white")

		# large font test
		# draw.text((0, 0), "123456789012345678901234567890", font=font_large, fill="white")
		# draw.text((0, 20), "123456789012345678901234567890", font=font_regular, fill="white")
	time.sleep(1)
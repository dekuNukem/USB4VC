# https://luma-oled.readthedocs.io/en/latest/software.html

import os
import sys
import time
from demo_opts import get_device
from luma.core.render import canvas
from PIL import ImageFont
import json

OLED_WIDTH = 128
OLED_HEIGHT = 32

my_arg = ['--display', 'ssd1306', '--interface', 'spi', '--spi-port', '0', '--spi-device', '1', '--gpio-reset', '6', '--gpio-data-command', '5', '--width', str(OLED_WIDTH), '--height', str(OLED_HEIGHT), '--spi-bus-speed', '2000000']
oled_device = get_device(my_arg)


font_regular = ImageFont.truetype("ProggyTiny.ttf", 16)
font_medium = ImageFont.truetype("ChiKareGo2.ttf", 16)
font_large = ImageFont.truetype("ProggyTiny.ttf", 32)

max_char_per_line = {font_regular:21, font_medium:17, font_large:11}
width_per_char = {font_regular:6, font_medium:7, font_large:12}

def oled_print_centered(text, font, y, this_canvas):
    text = text.strip()[:max_char_per_line[font]]
    start_x = int((OLED_WIDTH - (len(text) * width_per_char[font]))/2)
    if start_x < 0:
        start_x = 0
    this_canvas.text((start_x, y), text, font=font, fill="white")

curve_vertial_axis_x_pos = 80
curve_horizontal_axis_width = 32

linear_curve = {0: 0, 1: 1, 2: 2, 3: 3, 4: 4, 5: 5, 6: 6, 7: 7, 8: 8, 9: 9, 10: 10, 11: 11, 12: 12, 13: 13, 14: 14, 15: 15, 16: 16, 17: 17, 18: 18, 19: 19, 20: 20, 21: 21, 22: 22, 23: 23, 24: 24, 25: 25, 26: 26, 27: 27, 28: 28, 29: 29, 30: 30, 31: 31, 32: 32, 33: 33, 34: 34, 35: 35, 36: 36, 37: 37, 38: 38, 39: 39, 40: 40, 41: 41, 42: 42, 43: 43, 44: 44, 45: 45, 46: 46, 47: 47, 48: 48, 49: 49, 50: 50, 51: 51, 52: 52, 53: 53, 54: 54, 55: 55, 56: 56, 57: 57, 58: 58, 59: 59, 60: 60, 61: 61, 62: 62, 63: 63, 64: 64, 65: 65, 66: 66, 67: 67, 68: 68, 69: 69, 70: 70, 71: 71, 72: 72, 73: 73, 74: 74, 75: 75, 76: 76, 77: 77, 78: 78, 79: 79, 80: 80, 81: 81, 82: 82, 83: 83, 84: 84, 85: 85, 86: 86, 87: 87, 88: 88, 89: 89, 90: 90, 91: 91, 92: 92, 93: 93, 94: 94, 95: 95, 96: 96, 97: 97, 98: 98, 99: 99, 100: 100, 101: 101, 102: 102, 103: 103, 104: 104, 105: 105, 106: 106, 107: 107, 108: 108, 109: 109, 110: 110, 111: 111, 112: 112, 113: 113, 114: 114, 115: 115, 116: 116, 117: 117, 118: 118, 119: 119, 120: 120, 121: 121, 122: 122, 123: 123, 124: 124, 125: 125, 126: 126, 127: 127}
mid_curve = {0: 1, 1: 1, 2: 1, 3: 1, 4: 1, 5: 1, 6: 1, 7: 1, 8: 1, 9: 2, 10: 2, 11: 2, 12: 2, 13: 2, 14: 3, 15: 3, 16: 3, 17: 3, 18: 3, 19: 4, 20: 4, 21: 4, 22: 4, 23: 5, 24: 5, 25: 5, 26: 5, 27: 6, 28: 6, 29: 6, 30: 6, 31: 7, 32: 7, 33: 7, 34: 8, 35: 8, 36: 8, 37: 9, 38: 9, 39: 9, 40: 10, 41: 10, 42: 10, 43: 11, 44: 11, 45: 12, 46: 12, 47: 12, 48: 13, 49: 13, 50: 14, 51: 14, 52: 14, 53: 15, 54: 15, 55: 16, 56: 16, 57: 17, 58: 17, 59: 18, 60: 18, 61: 19, 62: 20, 63: 20, 64: 21, 65: 21, 66: 22, 67: 23, 68: 23, 69: 24, 70: 25, 71: 25, 72: 26, 73: 27, 74: 28, 75: 29, 76: 30, 77: 31, 78: 32, 79: 33, 80: 34, 81: 35, 82: 36, 83: 37, 84: 38, 85: 39, 86: 40, 87: 42, 88: 43, 89: 44, 90: 45, 91: 47, 92: 48, 93: 50, 94: 51, 95: 52, 96: 54, 97: 56, 98: 57, 99: 59, 100: 60, 101: 62, 102: 64, 103: 66, 104: 67, 105: 69, 106: 71, 107: 73, 108: 75, 109: 77, 110: 79, 111: 81, 112: 83, 113: 86, 114: 88, 115: 90, 116: 93, 117: 95, 118: 98, 119: 101, 120: 103, 121: 106, 122: 110, 123: 112, 124: 116, 125: 119, 126: 122, 127: 126}
curve3 = {0: 1, 1: 1, 2: 1, 3: 1, 4: 1, 5: 1, 6: 1, 7: 1, 8: 1, 9: 1, 10: 1, 11: 1, 12: 1, 13: 1, 14: 1, 15: 1, 16: 1, 17: 1, 18: 2, 19: 2, 20: 2, 21: 2, 22: 2, 23: 2, 24: 2, 25: 2, 26: 2, 27: 3, 28: 3, 29: 3, 30: 3, 31: 3, 32: 3, 33: 4, 34: 4, 35: 4, 36: 4, 37: 4, 38: 5, 39: 5, 40: 5, 41: 5, 42: 5, 43: 6, 44: 6, 45: 6, 46: 6, 47: 7, 48: 7, 49: 7, 50: 8, 51: 8, 52: 8, 53: 8, 54: 9, 55: 9, 56: 10, 57: 10, 58: 10, 59: 11, 60: 11, 61: 11, 62: 12, 63: 12, 64: 13, 65: 13, 66: 14, 67: 14, 68: 15, 69: 15, 70: 16, 71: 16, 72: 17, 73: 17, 74: 18, 75: 19, 76: 19, 77: 20, 78: 21, 79: 22, 80: 23, 81: 23, 82: 24, 83: 25, 84: 26, 85: 27, 86: 29, 87: 30, 88: 31, 89: 32, 90: 33, 91: 34, 92: 36, 93: 37, 94: 38, 95: 40, 96: 41, 97: 43, 98: 44, 99: 46, 100: 48, 101: 49, 102: 51, 103: 53, 104: 55, 105: 57, 106: 59, 107: 61, 108: 63, 109: 65, 110: 67, 111: 69, 112: 72, 113: 74, 114: 77, 115: 79, 116: 82, 117: 85, 118: 88, 119: 92, 120: 94, 121: 99, 122: 101, 123: 105, 124: 110, 125: 114, 126: 118, 127: 126}

joystick_curve_list = [linear_curve, mid_curve, curve3]

with canvas(oled_device) as draw:
    draw.text((0, 0), "Joystick", font=font_medium, fill="white")
    draw.text((0, 15), "Curve", font=font_medium, fill="white")
    draw.line((curve_vertial_axis_x_pos, 0, curve_vertial_axis_x_pos, curve_vertial_axis_x_pos), fill="white")
    draw.line((curve_vertial_axis_x_pos, 31, curve_vertial_axis_x_pos+curve_horizontal_axis_width, 31), fill="white")
    for xxx in range(curve_horizontal_axis_width):
        dict_key = xxx*4
        this_point_x = xxx + curve_vertial_axis_x_pos
        this_point_y = OLED_HEIGHT - linear_curve[dict_key]//4 - 1
        print(this_point_x, this_point_y)
        draw.line((this_point_x,this_point_y,this_point_x,this_point_y), fill="white")
while 1:
    time.sleep(1)
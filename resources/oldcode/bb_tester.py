import os
import sys
import time
import RPi.GPIO as GPIO
from demo_opts import get_device
from luma.core.render import canvas
from PIL import ImageFont

OLED_WIDTH = 128
OLED_HEIGHT = 32

my_arg = ['--display', 'ssd1306', '--interface', 'spi', '--spi-port', '0', '--spi-device', '1', '--gpio-reset', '6', '--gpio-data-command', '5', '--width', str(OLED_WIDTH), '--height', str(OLED_HEIGHT), '--spi-bus-speed', '2000000']
oled_device = get_device(my_arg)
time.sleep(0.5)
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

PLUS_BUTTON_PIN = 27
MINUS_BUTTON_PIN = 19
ENTER_BUTTON_PIN = 22
SHUTDOWN_BUTTON_PIN = 21
PCARD_CS_PIN = 8
SLEEP_LED_PIN = 26

GPIO.setup(SLEEP_LED_PIN, GPIO.OUT)
GPIO.output(SLEEP_LED_PIN, GPIO.LOW)
GPIO.setup(PCARD_CS_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

plus_button = my_button(PLUS_BUTTON_PIN)
minus_button = my_button(MINUS_BUTTON_PIN)
enter_button = my_button(ENTER_BUTTON_PIN)
shutdown_button = my_button(SHUTDOWN_BUTTON_PIN)

loop_count = 0
def print_pattern():
    global loop_count
    loop_count = (loop_count + 1) % 2
    oled_device = get_device(my_arg)
    if loop_count:
        GPIO.output(SLEEP_LED_PIN, GPIO.HIGH)
        with canvas(oled_device) as draw:
            draw.text((0, 0), "ABCDEFGHIJKLMNO", font=font_medium, fill="white")
            draw.text((0, 15), "PQRSTUVWXYZ0123", font=font_medium, fill="white")
    else:
        GPIO.output(SLEEP_LED_PIN, GPIO.LOW)
        with canvas(oled_device) as draw:
            draw.text((0, 0), "===================", font=font_medium, fill="white")
            draw.text((0, 15), "===================", font=font_medium, fill="white")

print_pattern()

while 1:
    time.sleep(0.05)
    if GPIO.input(PCARD_CS_PIN) == 0:
        GPIO.output(SLEEP_LED_PIN, GPIO.LOW)
        exit()
    if plus_button.is_pressed() or \
        minus_button.is_pressed() or \
        enter_button.is_pressed() or \
        shutdown_button.is_pressed():
        print_pattern()

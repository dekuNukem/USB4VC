import time
import sys
import RPi.GPIO as GPIO

BUTTON_GPIO = 16

def button_pressed_callback(channel):
    print(time.time(), "Button pressed!")

GPIO.setmode(GPIO.BCM)
GPIO.setup(BUTTON_GPIO, GPIO.IN, pull_up_down=GPIO.PUD_UP)
# https://sourceforge.net/p/raspberry-gpio-python/wiki/Inputs/
GPIO.add_event_detect(BUTTON_GPIO, GPIO.FALLING, callback=button_pressed_callback, bouncetime=100)

while 1:
    time.sleep(1)
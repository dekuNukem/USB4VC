import time
import sys
import RPi.GPIO as GPIO

PLUS_BUTTON_PIN = 27
MINUS_BUTTON_PIN = 19
ENTER_BUTTON_PIN = 22
SHUTDOWN_BUTTON_PIN = 21

GPIO.setmode(GPIO.BCM)

GPIO.setup(PLUS_BUTTON_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.add_event_detect(PLUS_BUTTON_PIN, GPIO.FALLING, bouncetime=250)

GPIO.setup(MINUS_BUTTON_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.add_event_detect(MINUS_BUTTON_PIN, GPIO.FALLING, bouncetime=250)

GPIO.setup(ENTER_BUTTON_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.add_event_detect(ENTER_BUTTON_PIN, GPIO.FALLING, bouncetime=250)

GPIO.setup(SHUTDOWN_BUTTON_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.add_event_detect(SHUTDOWN_BUTTON_PIN, GPIO.FALLING, bouncetime=250)

while 1:
	time.sleep(0.1)
	if GPIO.event_detected(PLUS_BUTTON_PIN):
		print(time.time(), "PLUS_BUTTON pressed!")

	if GPIO.event_detected(MINUS_BUTTON_PIN):
		print(time.time(), "MINUS_BUTTON pressed!")

	if GPIO.event_detected(ENTER_BUTTON_PIN):
		print(time.time(), "ENTER_BUTTON pressed!")

	if GPIO.event_detected(SHUTDOWN_BUTTON_PIN):
		print(time.time(), "SHUTDOWN_BUTTON pressed!")
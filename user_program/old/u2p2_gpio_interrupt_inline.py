import time
import sys
import RPi.GPIO as GPIO

SLAVE_REQ_PIN = 16

GPIO.setmode(GPIO.BCM)
GPIO.setup(SLAVE_REQ_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)

GPIO.add_event_detect(SLAVE_REQ_PIN, GPIO.FALLING, bouncetime=100)

while 1:
	time.sleep(0.1)
	if GPIO.event_detected(SLAVE_REQ_PIN):
		print(time.time(), "Button pressed!")
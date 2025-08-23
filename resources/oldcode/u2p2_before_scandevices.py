import sys
import time
import spidev
import threading

is_on_raspberry_pi = False
with open('/etc/os-release') as os_version_file:
    is_on_raspberry_pi = 'raspbian' in os_version_file.read().lower()

spi = None
if is_on_raspberry_pi:
    spi = spidev.SpiDev(0, 0) # rasp
    print("I'm on Raspberry Pi!")
else:
    spi = spidev.SpiDev(1, 0) # lichee
    print("I'm on custom board!")

spi.max_speed_hz = 2000000

fff = open(sys.argv[1], "rb" )


"""
https://elixir.bootlin.com/linux/latest/source/include/uapi/linux/input-event-codes.h#L38

xbox gamepad:
EVENT TYPE: EV_ABS
dpad: ABS_HAT0X ABS_HAT1X
buttons: https://elixir.bootlin.com/linux/latest/source/include/uapi/linux/input-event-codes.h#L385
sticks: ABS_X, ABS_Y, ABS_RX, ABS_RY
"""

EV_KEY = 0x01
EV_REL = 0x02
EV_ABS = 0x03

SPI_MSG_KEYBOARD_EVENT = 1
SPI_MSG_MOUSE_EVENT = 2
SPI_MSG_GAMEPAD_EVENT = 3

KEYBOARD_ID_PLACEHOLDER = 0

keyboard_spi_msg_header = [0xde, 0, SPI_MSG_KEYBOARD_EVENT, KEYBOARD_ID_PLACEHOLDER]

def keyboard_worker():
    print("keyboard_thread started")
    while 1:
        data = list(fff.read(16)[8:])
        if data[0] == EV_KEY:
            spi.xfer(keyboard_spi_msg_header + data)
            # print(data)
            # print('----')

def mouse_worker():
    print("mouse_thread started")
    while 1:
        time.sleep(0.2)

keyboard_thread = threading.Thread(target=keyboard_worker, daemon=True)
keyboard_thread.start()

mouse_thread = threading.Thread(target=mouse_worker, daemon=True)
mouse_thread.start()

while 1:
    # print("main loop")
    time.sleep(1)
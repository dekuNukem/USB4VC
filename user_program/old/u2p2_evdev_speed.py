import sys
import time
import libevdev
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

# fff = open("/dev/input/event1", "rb" )
fff = open(sys.argv[1], "rb" )
this_device = libevdev.Device(fff)
# this_device.disable(libevdev.EV_MSC)

def keyboard_worker():
    print("keyboard_thread started")
    while 1:
        for e in this_device.events():
            spi.xfer([0x5]*32)
            print(e)

def mouse_worker():
    print("mouse_thread started")
    while 1:
        time.sleep(0.2)
        # print("sent")

keyboard_thread = threading.Thread(target=keyboard_worker, daemon=True)
keyboard_thread.start()

mouse_thread = threading.Thread(target=mouse_worker, daemon=True)
mouse_thread.start()

while 1:
    # print("main loop")
    time.sleep(1)
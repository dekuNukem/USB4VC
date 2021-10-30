import sys
import spidev
import time

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

EV_SYN = 0
EV_KEY = 1
EV_REL = 2
EV_ABS = 3

fff = open(sys.argv[1], "rb" )
while 1:
    data = list(fff.read(16))
    print(data)

    
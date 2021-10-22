import spidev

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

spi.max_speed_hz = 500000

fff = open("/dev/input/event0", "rb" )
while 1:
    data = fff.read(24)
    data = list(data)
    print(data)
    spi.xfer(data)
    
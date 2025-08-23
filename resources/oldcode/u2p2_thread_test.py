# import sys
import time
import queue
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

spi.max_speed_hz = 500000
MAX_SPI_QUEUE_SIZE = 32
spi_out_queue = queue.Queue(maxsize=MAX_SPI_QUEUE_SIZE)

# sys.setswitchinterval(0.005)

fff = open("/dev/input/event0", "rb" )


def keyboard_worker():
    print("keyboard_thread started")
    while 1:
        data = fff.read(24)
        data = list(data)
        try:
            spi_out_queue.put(data, block=False)
        except Exception:
            continue

def spi_worker():
    print("spi_thread started")
    while 1:
        spi.xfer(spi_out_queue.get())
        # print("sent")

keyboard_thread = threading.Thread(target=keyboard_worker, daemon=True)
keyboard_thread.start()

spi_thread = threading.Thread(target=spi_worker, daemon=True)
spi_thread.start()

while 1:
    # print("main loop")
    time.sleep(1)
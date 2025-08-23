import sys
import libevdev

fd = open(sys.argv[1], "rb" )

d = libevdev.Device(fd)

while True:
    for e in d.events():
        print(e)

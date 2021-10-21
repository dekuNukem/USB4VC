import libevdev

fd = open("/dev/input/event0", "rb" )

d = libevdev.Device(fd)

while True:
    for e in d.events():
        print(e)

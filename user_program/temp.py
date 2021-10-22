while current_line_num < max_lines:
	this_line = file_content[current_line_num]

	print(current_line_num, len(this_line))
	print(this_line, end='')


	if this_line.startswith('I: '):
		while len(this_line) != 1:
			print(this_line)


	current_line_num += 1

FORMAT = '4IHHI'
EVENT_SIZE = struct.calcsize(FORMAT)
print(EVENT_SIZE)

def keyboard_worker():
    print("keyboard_thread started")
    while 1:
        data = fff.read(24)
        # (tv_sec, tv_usec, type, code, value) = struct.unpack('4IHHI', data)
        print(struct.unpack('4IHHI', data))

        # data = list(data)
        # spi.xfer(data)
 

 import sys
import libevdev

fd = open("/dev/input/event0", "rb" )

d = libevdev.Device(fd)
if not d.has(libevdev.EV_KEY.BTN_LEFT):
     print('This does not look like a mouse device')
     sys.exit(0)


# Loop indefinitely while pulling the currently available events off
# the file descriptor
while True:
    for e in d.events():
        if not e.matches(libevdev.EV_KEY):
            continue

        if e.matches(libevdev.EV_KEY.BTN_LEFT):
            print('Left button event')
        elif e.matches(libevdev.EV_KEY.BTN_RIGHT):
            print('Right button event')

print(struct.unpack('4IHHI', data))


data = list(fff.read(16)[8:])
        # https://elixir.bootlin.com/linux/latest/source/include/uapi/linux/input-event-codes.h#L38
        if data[0] == 1: # Keys
            spi.xfer(data)
            # print(data)
            # print('----')
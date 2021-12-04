           # print(data)
            # if data[0] == EV_KEY or data[0] == EV_REL:
            #     to_transfer = mouse_spi_msg_template + data + [0]*20
            #     to_transfer[3] = mouse_opened_device_dict[key][1]
            #     spi.xfer(to_transfer)


                # print(time.time_ns(), 'sent')
                # print(key)
                # print(to_transfer)
                # print('----')
                # print(time.time_ns(), 'sent')
                # print(key)
                # print(to_transfer)
                # print('----')
last_spi_tx = 0
def spi_xfer_with_speedlimit(data):
    spi.xfer(data)
    # global last_spi_tx
    # while time.time_ns() - last_spi_tx <= 5000000:
    #     time.sleep(0.001)
    # spi.xfer(data)
    # last_spi_tx = time.time_ns()

while 1:
    data = list(fff.read(48))
    # mouse_movement_x = None
    # mouse_movement_y = None
    # mouse_wheel_movement_regular = None
    # mouse_wheel_movement_hi_res = None
    # mouse_button_event = None
    # packet1 = data[:16]
    # packet2 = data[16:32]
    # packet3 = data[32:]
    # print(len(packet1), len(packet2), len(packet3))

    for x in range(3):
        this_packet = data[x*16:x*16+16]
        print(this_packet[8:])
    print('----')
    # data = list(data[8:])
    # if data[0] == EV_KEY:
    #     print(time.time(), data)
    #     print('--------')
    # if data[0] == EV_REL:
    #     print(time.time(), data)
    #     print('--------')
    # spi.xfer(data)
    spi.xfer(data)

    # if data[0] == EV_KEY or data[0] == EV_REL:
    #     spi.xfer(data)
    

fff = open(sys.argv[1], "rb" )
while 1:
    data = fff.read(48)
    # data = list(data[8:])
    # if data[0] == EV_KEY:
    #     print(time.time(), data)
    #     print('--------')
    # if data[0] == EV_REL:
    #     print(time.time(), data)
    #     print('--------')
    # spi.xfer(data)
    spi.xfer(data)

    # if data[0] == EV_KEY or data[0] == EV_REL:
    #     spi.xfer(data)
    


    
    for item in scrolllock_list:
        if ps2kb_led_byte & 0x1:
            os.system("sudo bash -c 'echo 1 > " + os.path.join(item, 'brightness') + "'")
        else:
            os.system("sudo bash -c 'echo 0 > " + os.path.join(item, 'brightness') + "'")

    for item in numlock_list:
        if ps2kb_led_byte & 0x2:
            os.system("sudo bash -c 'echo 1 > " + os.path.join(item, 'brightness') + "'")
        else:
            os.system("sudo bash -c 'echo 0 > " + os.path.join(item, 'brightness') + "'")

    for item in capslock_list:
        if ps2kb_led_byte & 0x4:
            os.system("sudo bash -c 'echo 1 > " + os.path.join(item, 'brightness') + "'")
        else:
            os.system("sudo bash -c 'echo 0 > " + os.path.join(item, 'brightness') + "'")

if slave_result[SPI_BUF_INDEX_MAGIC] == SPI_MISO_MAGIC and slave_result[SPI_BUF_INDEX_MSG_TYPE] == SPI_MISO_MSG_KB_LED_REQ:
                    print("good!", slave_result[3])
                    for x in range(2):
                        change_kb_led(slave_result[3])
                        # time.sleep(0.1)
filler = [0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3]
                # testttt = [0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f]
                # time.sleep(0.01)

                    # change_kb_led(slave_result[3])
                    # if slave_result[3] & 0x1:
                    #     print("SCROLL LOCK ON")
                    # if slave_result[3] & 0x2:
                    #     print("NUM LOCK ON")
                    # if slave_result[3] & 0x4:
                    #     print("CAP LOCK ON")
                    # print('----')
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

# example /dev/input/by-path/platform-3f980000.usb-usb-0:1.2.2:1.0-event-kbd
                    # kb_index = int(item.split(':')[1].split('.')[1])


while 1:
    with canvas(device) as draw:
        # draw.rectangle(device.bounding_box, outline="white", fill="black")
        
        # regular font test
        # draw.text((0, 0), "123456789012345678901234567890", font=font_regular, fill="white")
        # draw.text((0, 10), "ABCDEFGHIJ", font=font_regular, fill="white")
        # draw.text((0, 20), "abcdefghij", font=font_regular, fill="white")

        # medium font test
        # draw.text((0, 0), "123456789012345678901234567890", font=font_medium, fill="white")
        # draw.text((0, 15), "123456789012345678901234567890", font=font_medium, fill="white")

        # large font test
        # draw.text((0, 0), "123456789012345678901234567890", font=font_large, fill="white")
        # draw.text((0, 20), "123456789012345678901234567890", font=font_regular, fill="white")
    time.sleep(1)
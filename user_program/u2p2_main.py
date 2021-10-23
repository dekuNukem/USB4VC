import os
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

keyboard_opened_device_dict = {}
mouse_opened_device_dict = {}
gamepad_opened_device_dict = {}

"""
def send kb

def send mouse

def send js

read data first, if exception, skip it
"""

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

keyboard_spi_msg_header = [0xde, 0, SPI_MSG_KEYBOARD_EVENT, 0]

def raw_input_event_worker():
    print("raw_input_event_parser_thread started")
    to_delete = []
    while 1:
        for key in list(keyboard_opened_device_dict):
            try:
                data = keyboard_opened_device_dict[key][0].read(16)
            except OSError:
                keyboard_opened_device_dict[key][0].close()
                del keyboard_opened_device_dict[key]
                print("device disappeared:", key)
            if data is None:
                continue
            data = list(data[8:])
            if data[0] == EV_KEY:
                to_transfer = keyboard_spi_msg_header + data
                to_transfer[3] = keyboard_opened_device_dict[key][1]
                # spi.xfer(to_transfer)
                spi.xfer([0x15]*32)
                print('sent')
                # print(key)
                # print(to_transfer)
                # print('----')

raw_input_event_parser_thread = threading.Thread(target=raw_input_event_worker, daemon=True)
raw_input_event_parser_thread.start()

input_device_path = '/dev/input/by-path/'

while 1:
    try:
        device_file_list = os.listdir(input_device_path)
    except Exception as e:
        print('list input device exception:', e)
        time.sleep(0.75)
        continue
    mouse_list = [os.path.join(input_device_path, x) for x in device_file_list if 'event-mouse' in x]
    keyboard_list = [os.path.join(input_device_path, x) for x in device_file_list if 'event-kbd' in x]
    gamepad_list = [os.path.join(input_device_path, x) for x in device_file_list if 'event-joystick' in x]
    # print(mouse_list)
    # print(keyboard_list)

    for item in keyboard_list:
        if item not in keyboard_opened_device_dict:
            try:
                this_file = open(item, "rb")
                os.set_blocking(this_file.fileno(), False)
                kb_index = 0
                try:
                    kb_index = sum([int(x) for x in item.split(':')[1].split('.')][:2])
                except:
                    pass
                keyboard_opened_device_dict[item] = (this_file, kb_index)
                print("opened keyboard", keyboard_opened_device_dict[item][1], ':' , item)
            except Exception as e:
                print("keyboard open exception:", e)
                continue

    time.sleep(0.75)
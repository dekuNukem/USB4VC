import os
import sys
import time
import spidev
import threading
import subprocess
import RPi.GPIO as GPIO
import usb4vc_ui
import usb4vc_shared

"""
sudo apt install stm32flash

OPTIONAL
git clone https://git.code.sf.net/p/stm32flash/code stm32flash-code
cd stm32flash-code
sudo make install

HAVE TO ASSERT BOOT0 THE WHOLE TIME
/usr/local/bin/stm32flash -r hhh -a 0x3b /dev/i2c-1

"""

SLAVE_REQ_PIN = 16
GPIO.setmode(GPIO.BCM)
GPIO.setup(SLAVE_REQ_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
GPIO.add_event_detect(SLAVE_REQ_PIN, GPIO.RISING)

pcard_spi = spidev.SpiDev(0, 0)
pcard_spi.max_speed_hz = 2000000

keyboard_opened_device_dict = {}
mouse_opened_device_dict = {}
gamepad_opened_device_dict = {}

led_device_path = '/sys/class/leds'
input_device_path = '/dev/input/by-path/'

"""
https://elixir.bootlin.com/linux/latest/source/include/uapi/linux/input-event-codes.h#L38

xbox gamepad:
EVENT TYPE: EV_ABS
dpad: ABS_HAT0X ABS_HAT1X
buttons: https://elixir.bootlin.com/linux/latest/source/include/uapi/linux/input-event-codes.h#L385
sticks: ABS_X, ABS_Y, ABS_RX, ABS_RY
"""

# https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h
EV_SYN = 0
EV_KEY = 1
EV_REL = 2
EV_ABS = 3

SYN_REPORT = 0

REL_X = 0x00
REL_Y = 0x01
REL_WHEEL = 0x08

SPI_BUF_INDEX_MAGIC = 0
SPI_BUF_INDEX_SEQNUM = 1
SPI_BUF_INDEX_MSG_TYPE = 2

SPI_MOSI_MSG_TYPE_NOP = 0
SPI_MOSI_MSG_TYPE_INFO_REQUEST = 1
SPI_MOSI_MSG_TYPE_SET_PROTOCOL = 2
SPI_MOSI_MSG_TYPE_REQ_ACK = 3

SPI_MOSI_MSG_TYPE_KEYBOARD_EVENT = 8
SPI_MOSI_MSG_TYPE_MOUSE_EVENT = 9
SPI_MOSI_MSG_TYPE_GAMEPAD_EVENT_RAW = 10
SPI_MOSI_MSG_TYPE_GAMEPAD_EVENT_MAPPED = 11

SPI_MISO_MSG_TYPE_NOP = 0
SPI_MISO_MSG_TYPE_INFO_REQUEST = 128
SPI_MISO_MSG_TYPE_KB_LED_REQUEST = 129

SPI_MOSI_MAGIC = 0xde
SPI_MISO_MAGIC = 0xcd

GP_BTN_SOUTH = 0x130
GP_BTN_EAST = 0x131
GP_BTN_C = 0x132
GP_BTN_NORTH = 0x133
GP_BTN_WEST = 0x134
GP_BTN_Z = 0x135
GP_BTN_TL = 0x136
GP_BTN_TR = 0x137
GP_BTN_TL2 = 0x138
GP_BTN_TR2 = 0x139
GP_BTN_SELECT = 0x13a
GP_BTN_START = 0x13b
GP_BTN_MODE = 0x13c
GP_BTN_THUMBL = 0x13d
GP_BTN_THUMBR = 0x13e

ABS_X = 0x00
ABS_Y = 0x01
ABS_Z = 0x02
ABS_RX = 0x03
ABS_RY = 0x04
ABS_RZ = 0x05
ABS_THROTTLE = 0x06
ABS_RUDDER = 0x07
ABS_WHEEL = 0x08
ABS_GAS = 0x09
ABS_BRAKE = 0x0a
ABS_HAT0X = 0x10
ABS_HAT0Y = 0x11
ABS_HAT1X = 0x12
ABS_HAT1Y = 0x13
ABS_HAT2X = 0x14
ABS_HAT2Y = 0x15
ABS_HAT3X = 0x16
ABS_HAT3Y = 0x17

BTN_LEFT = 0x110
BTN_RIGHT = 0x111
BTN_MIDDLE = 0x112
BTN_SIDE = 0x113
BTN_EXTRA = 0x114
BTN_FORWARD = 0x115
BTN_BACK = 0x116
BTN_TASK = 0x117

nop_spi_msg_template = [SPI_MOSI_MAGIC] + [0]*31
info_request_spi_msg_template = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_INFO_REQUEST] + [0]*29
keyboard_event_spi_msg_template = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_KEYBOARD_EVENT] + [0]*29
mouse_event_spi_msg_template = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_MOUSE_EVENT] + [0]*29
gamepad_event_mapped_spi_msg_template = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_GAMEPAD_EVENT_MAPPED] + [0]*29

def make_spi_msg_ack():
    return [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_REQ_ACK] + [0]*29

def make_keyboard_spi_packet(input_data, kbd_id):
    result = list(keyboard_event_spi_msg_template)
    result[3] = kbd_id
    result[4] = input_data[2]
    result[5] = input_data[3]
    result[6] = input_data[4]
    return result

def dict_get_if_exist(this_dict, this_key):
    if this_key not in this_dict:
        return 0;
    return this_dict[this_key];

def make_mouse_spi_packet(mouse_dict, mouse_id):
    to_transfer = list(mouse_event_spi_msg_template)
    to_transfer[3] = mouse_id
    if 'x' in mouse_dict:
        to_transfer[4:6] = mouse_dict['x']
    if 'y' in mouse_dict:
        to_transfer[6:8] = mouse_dict['y']
    if 'scroll' in mouse_dict:
        to_transfer[8:10] = mouse_dict['scroll']
    to_transfer[13] = dict_get_if_exist(mouse_dict, BTN_LEFT)
    to_transfer[14] = dict_get_if_exist(mouse_dict, BTN_RIGHT)
    to_transfer[15] = dict_get_if_exist(mouse_dict, BTN_MIDDLE)
    to_transfer[16] = dict_get_if_exist(mouse_dict, BTN_SIDE)
    to_transfer[17] = dict_get_if_exist(mouse_dict, BTN_EXTRA)
    return to_transfer

def make_gamepad_spi_packet(gp_status_dict, gp_id, axes_info):
    result = list(gamepad_event_mapped_spi_msg_template)

    result[3] = gp_id;
    result[4] = dict_get_if_exist(gp_status_dict[gp_id], GP_BTN_SOUTH)
    result[5] = dict_get_if_exist(gp_status_dict[gp_id], GP_BTN_EAST)
    result[6] = dict_get_if_exist(gp_status_dict[gp_id], GP_BTN_WEST)
    result[7] = dict_get_if_exist(gp_status_dict[gp_id], GP_BTN_NORTH)

    result[8] = dict_get_if_exist(gp_status_dict[gp_id], ABS_X)
    if ABS_X in axes_info and 'max' in axes_info[ABS_X]:
        result[8] = int(result[8] / (axes_info[ABS_X]['max'] / 127)) + 127

    result[9] = dict_get_if_exist(gp_status_dict[gp_id], ABS_Y)
    if ABS_Y in axes_info and 'max' in axes_info[ABS_Y]:
        result[9] = int(result[9] / (axes_info[ABS_Y]['max'] / 127)) + 127

    result[10] = dict_get_if_exist(gp_status_dict[gp_id], ABS_RX)
    if ABS_RX in axes_info and 'max' in axes_info[ABS_RX]:
        result[10] = int(result[10] / (axes_info[ABS_RX]['max'] / 127)) + 127

    result[11] = dict_get_if_exist(gp_status_dict[gp_id], ABS_RY)
    if ABS_RY in axes_info and 'max' in axes_info[ABS_RY]:
        result[11] = int(result[11] / (axes_info[ABS_RY]['max'] / 127)) + 127
    return result

def change_kb_led(scrolllock, numlock, capslock):
    led_file_list = os.listdir(led_device_path)
    capslock_list = [os.path.join(led_device_path, x) for x in led_file_list if 'capslock' in x]
    numlock_list = [os.path.join(led_device_path, x) for x in led_file_list if 'numlock' in x]
    scrolllock_list = [os.path.join(led_device_path, x) for x in led_file_list if 'scrolllock' in x]

    for item in scrolllock_list:
        with open(os.path.join(item, 'brightness'), 'w') as led_file:
            led_file.write(str(scrolllock))

    for item in numlock_list:
        with open(os.path.join(item, 'brightness'), 'w') as led_file:
            led_file.write(str(numlock))

    for item in capslock_list:
        with open(os.path.join(item, 'brightness'), 'w') as led_file:
            led_file.write(str(capslock))


def raw_input_event_worker():
    mouse_status_dict = {}
    gamepad_status_dict = {}
    print("raw_input_event_worker started")
    while 1:
# ----------------- KEYBOARD PARSING -----------------
        for key in list(keyboard_opened_device_dict):
            try:
                data = keyboard_opened_device_dict[key][0].read(16)
            except OSError:
                keyboard_opened_device_dict[key][0].close()
                del keyboard_opened_device_dict[key]
                print("keyboard disappeared:", key)
                continue
            if data is None:
                continue
            data = list(data[8:])
            if data[0] == EV_KEY:
                pcard_spi.xfer(make_keyboard_spi_packet(data, keyboard_opened_device_dict[key][1]))

# ----------------- MOUSE PARSING -----------------

        for key in list(mouse_opened_device_dict):
            try:
                data = mouse_opened_device_dict[key][0].read(16)
            except OSError:
                mouse_opened_device_dict[key][0].close()
                del mouse_opened_device_dict[key]
                print("mouse disappeared:", key)
                continue
            if data is None:
                continue
            """
            0 - 1 event_type
            2 - 3 key code
            4 - 7 key status
            """
            data = list(data[8:])
            # print(usb4vc_ui.get_mouse_sensitivity())
            # mouse movement and scrolling
            # buffer those values until a SYNC event
            if data[0] == EV_REL:
                if data[2] == REL_X:
                    rawx = int.from_bytes(data[4:6], byteorder='little', signed=True)
                    rawx = int(rawx * usb4vc_ui.get_mouse_sensitivity()) & 0xffff
                    mouse_status_dict["x"] = list(rawx.to_bytes(2, byteorder='little'))
                if data[2] == REL_Y:
                    rawy = int.from_bytes(data[4:6], byteorder='little', signed=True)
                    rawy = int(rawy * usb4vc_ui.get_mouse_sensitivity()) & 0xffff
                    mouse_status_dict["y"] = list(rawy.to_bytes(2, byteorder='little'))
                if data[2] == REL_WHEEL:
                    mouse_status_dict["scroll"] = data[4:6]
                # print(usb4vc_ui.get_mouse_sensitivity(), mouse_status_dict)

            # mouse button pressed, send it out immediately
            if data[0] == EV_KEY:
                key_code = data[3] * 256 + data[2]
                if 0x110 <= key_code <= 0x117:
                    mouse_status_dict[key_code] = data[4]
                    mouse_status_dict['x'] = [0, 0]
                    mouse_status_dict['y'] = [0, 0]
                    mouse_status_dict['scroll'] = [0, 0]
                    to_transfer = make_mouse_spi_packet(mouse_status_dict, mouse_opened_device_dict[key][1])
                    pcard_spi.xfer(to_transfer)
                """
                Logitech unifying receiver identifies itself as a mouse,
                so need to handle keyboard presses here too for compatibility
                """
                if 0x1 <= key_code <= 127:
                    pcard_spi.xfer(make_keyboard_spi_packet(data, mouse_opened_device_dict[key][1]))

            # SYNC event happened, send out an update
            if data[0] == EV_SYN and data[2] == SYN_REPORT and len(mouse_status_dict) > 0:
                to_transfer = make_mouse_spi_packet(mouse_status_dict, mouse_opened_device_dict[key][1])
                pcard_spi.xfer(to_transfer)
                mouse_status_dict['scroll'] = [0, 0]

# ----------------- GAMEPAD PARSING -----------------
        for key in list(gamepad_opened_device_dict):
            try:
                data = gamepad_opened_device_dict[key][0].read(16)
            except OSError:
                gamepad_opened_device_dict[key][0].close()
                del gamepad_opened_device_dict[key]
                print("gamepad disappeared:", key)
                continue
            if data is None:
                continue
            data = list(data[8:])
            gamepad_id = gamepad_opened_device_dict[key][1]
            if gamepad_id not in gamepad_status_dict:
                gamepad_status_dict[gamepad_id] = {}
            # gamepad button presses, send out immediately
            if data[0] == EV_KEY:
                key_code = data[3] * 256 + data[2]
                if not (GP_BTN_SOUTH <= key_code <= GP_BTN_THUMBR):
                    continue
                gamepad_status_dict[gamepad_id][key_code] = data[4]
                to_transfer = make_gamepad_spi_packet(gamepad_status_dict, gamepad_id, gamepad_opened_device_dict[key][2])
                pcard_spi.xfer(to_transfer)
                continue
            # joystick / analogue trigger movements, cache until next SYNC event
            if data[0] == EV_ABS:
                abs_axes = data[3] * 256 + data[2]
                abs_value = int.from_bytes(data[4:8], byteorder='little', signed=True)
                gamepad_status_dict[gamepad_id][abs_axes] = abs_value
            # SYNC report, update now
            if data[0] == EV_SYN and data[2] == SYN_REPORT:
                to_transfer = make_gamepad_spi_packet(gamepad_status_dict, gamepad_id, gamepad_opened_device_dict[key][2])
                pcard_spi.xfer(to_transfer)

# ----------------- PBOARD INTERRUPT -----------------
        if GPIO.event_detected(SLAVE_REQ_PIN):
            slave_result = None
            for x in range(2):
                slave_result = pcard_spi.xfer(make_spi_msg_ack())
            print(int(time.time()), slave_result)
            if slave_result[SPI_BUF_INDEX_MAGIC] == SPI_MISO_MAGIC and slave_result[SPI_BUF_INDEX_MSG_TYPE] == SPI_MISO_MSG_TYPE_KB_LED_REQUEST:
                try:
                    change_kb_led(slave_result[3], slave_result[4], slave_result[5])
                    change_kb_led(slave_result[3], slave_result[4], slave_result[5])
                except Exception as e:
                    print('change_kb_led exception:', e)

def usb_device_scan_worker():
    print("usb_device_scan_worker started")
    while 1:
        time.sleep(0.75)
        try:
            device_file_list = os.listdir(input_device_path)
        except FileNotFoundError:
            print("No input devices found")
            continue
        except Exception as e:
            print('list input device exception:', e)
            continue
        mouse_list = [os.path.join(input_device_path, x) for x in device_file_list if 'event-mouse' in x]
        keyboard_list = [os.path.join(input_device_path, x) for x in device_file_list if 'event-kbd' in x]
        gamepad_list = [os.path.join(input_device_path, x) for x in device_file_list if 'event-joystick' in x]

        for item in keyboard_list:
            if item not in keyboard_opened_device_dict:
                try:
                    this_file = open(item, "rb")
                    os.set_blocking(this_file.fileno(), False)
                    device_index = 0
                    try:
                        device_index = sum([int(x) for x in item.split(':')[1].split('.')][:2])
                    except:
                        pass
                    keyboard_opened_device_dict[item] = (this_file, device_index)
                    print("opened keyboard", keyboard_opened_device_dict[item][1], ':' , item)
                except Exception as e:
                    print("keyboard open exception:", e)
                    continue

        for item in mouse_list:
            if item not in mouse_opened_device_dict:
                try:
                    this_file = open(item, "rb")
                    os.set_blocking(this_file.fileno(), False)
                    device_index = 0
                    try:
                        device_index = sum([int(x) for x in item.split(':')[1].split('.')][:2])
                    except:
                        pass
                    mouse_opened_device_dict[item] = (this_file, device_index)
                    print("opened mouse", mouse_opened_device_dict[item][1], ':' , item)
                except Exception as e:
                    print("mouse open exception:", e)
                    continue

        for item in gamepad_list:
            if item not in gamepad_opened_device_dict:
                try:
                    this_file = open(item, "rb")
                    os.set_blocking(this_file.fileno(), False)
                    device_index = 0
                    try:
                        device_index = sum([int(x) for x in item.split(':')[1].split('.')][:2])
                    except:
                        pass
                    gamepad_info = subprocess.getoutput("timeout 0.5 evtest " + str(item))
                    gamepad_opened_device_dict[item] = (this_file, device_index, parse_evtest_info(gamepad_info))
                    print("opened gamepad", gamepad_opened_device_dict[item][1], ':' , item)
                except Exception as e:
                    print("gamepad open exception:", e)
                    continue

def parse_evtest_info(input_str):
    input_str = input_str.replace('\r', '').split('Event type 3 (EV_ABS)\n')[-1].split('Event type')[0]
    current_axis_code = None
    info_dict = {}
    for line in input_str.split('\n'):
        line = line.strip().replace('\t', '')
        if " (ABS_" in line:
            current_axis_code = int(line.split('Event code ')[1].split(' (ABS_')[0])
            info_dict[current_axis_code] = {}
            continue
        if current_axis_code is None or len(line) < 3:
            continue
        line_split = line.split(' ')
        info_dict[current_axis_code][line_split[0].lower()] = int(line_split[-1])
    return info_dict

raw_input_event_parser_thread = threading.Thread(target=raw_input_event_worker, daemon=True)
usb_device_scan_thread = threading.Thread(target=usb_device_scan_worker, daemon=True)

# raw_input_event_parser_thread.start()
# usb_device_scan_thread.start()

def get_pboard_info():
    # send request
    this_msg = list(info_request_spi_msg_template)
    this_msg[3] = usb4vc_shared.RPI_APP_VERSION_TUPLE[0]
    this_msg[4] = usb4vc_shared.RPI_APP_VERSION_TUPLE[1]
    this_msg[5] = usb4vc_shared.RPI_APP_VERSION_TUPLE[2]
    pcard_spi.xfer(this_msg)

    time.sleep(0.01)
    # send an empty message to allow response to be shifted into RPi
    response = pcard_spi.xfer(list(nop_spi_msg_template))
    time.sleep(0.01)
    response = pcard_spi.xfer(list(nop_spi_msg_template))
    return response

def set_protocol(raw_msg):
    pcard_spi.xfer(list(raw_msg))

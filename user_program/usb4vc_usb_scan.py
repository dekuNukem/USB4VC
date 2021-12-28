import os
import sys
import time
import spidev
import evdev
import threading
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

ooopened_device_dict = {}

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
ABS_HAT3Y = 0x16
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
gamepad_event_ibm_ggp_spi_msg_template = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_GAMEPAD_EVENT_MAPPED, 0, 0, 0, 0, 0, 127, 127, 127, 127] + [0]*20

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

PID_GENERIC_GAMEPORT_GAMEPAD = 7

def clamp_to_8bit(value, axes_dict, axis_key):
    if axis_key not in axes_dict or 'max' not in axes_dict[axis_key]:
        return value % 256
    return int(value / (axes_dict[axis_key]['max'] / 127)) + 127

IBMPC_GGP_SPI_LOOKUP = {
    'IBMGGP_BTN_1':4,
    'IBMGGP_BTN_2':5,
    'IBMGGP_BTN_3':6,
    'IBMGGP_BTN_4':7,
    'IBMGGP_JS1_X':8,
    'IBMGGP_JS1_Y':9,
    'IBMGGP_JS2_X':10,
    'IBMGGP_JS2_Y':11,
}

MOUSE_SPI_LOOKUP = {
    'MOUSE_BTN_LEFT':13,
    'MOUSE_BTN_RIGHT':14,
    'MOUSE_BTN_MIDDLE':15,
    'MOUSE_BTN_SIDE':16,
    'MOUSE_BTN_EXTRA':17,
    'MOUSE_X':4,
    'MOUSE_Y':6,
    'MOUSE_SCROLL':8,
}

def find_keycode_in_mapping(key_code, mapping_dict):
    if ABS_X <= key_code <= ABS_HAT3Y:
        code_type = 'usb_gp_axes'
    if GP_BTN_SOUTH <= key_code <= GP_BTN_THUMBR:
        code_type = 'usb_gp_btn'
    result = mapping_dict.get((key_code, code_type))
    if type(result) is tuple:
        return code_type, result[0], result[1]
    if type(result) is dict:
        return code_type, result, None
    return None, None, None

def find_furthest_from_midpoint(this_set):
    curr_best = None
    diff_max = 0
    for item in this_set:
        this_diff = abs(item - 127)
        if this_diff >= diff_max:
            diff_max = this_diff
            curr_best = item
    return curr_best

prev_gp_output = {}
prev_kb_output = {}
def make_generic_gamepad_spi_packet(gp_status_dict, gp_id, axes_info, mapping_info):
    global prev_gp_output
    global prev_kb_output
    this_gp_dict = gp_status_dict[gp_id]
    curr_gp_output = {
        'IBMPC_GGP_BTN_1':set([0]),
        'IBMPC_GGP_BTN_2':set([0]),
        'IBMPC_GGP_BTN_3':set([0]),
        'IBMPC_GGP_BTN_4':set([0]),
        'IBMPC_GGP_JS1_X':set([127]),
        'IBMPC_GGP_JS1_Y':set([127]),
        'IBMPC_GGP_JS2_X':set([127]),
        'IBMPC_GGP_JS2_Y':set([127]),
    }
    curr_kb_output = {}
    curr_mouse_output = {
        'IS_MODIFIED':False,
        'MOUSE_BTN_LEFT':set([0]),
        'MOUSE_BTN_MIDDLE':set([0]),
        'MOUSE_BTN_RIGHT':set([0]),
        'MOUSE_BTN_SIDE':set([0]),
        'MOUSE_BTN_EXTRA':set([0]),
        'MOUSE_X':0,
        'MOUSE_Y':0,
        'MOUSE_SCROLL':0,
    }
    
    for from_code in this_gp_dict:
        from_type, to_code, to_type = find_keycode_in_mapping(from_code, mapping_info['mapping'])
        if from_type is None:
            continue
        # button to button
        if from_type == 'usb_gp_btn' and to_type == 'pb_gp_btn' and to_code in curr_gp_output:
            curr_gp_output[to_code].add(this_gp_dict[from_code])
        # analog to analog
        if from_type == 'usb_gp_axes' and to_type == 'pb_gp_axes' and to_code in curr_gp_output:
            curr_gp_output[to_code].add(clamp_to_8bit(this_gp_dict[from_code], axes_info, from_code))
        # button to analog
        if from_type == 'usb_gp_btn' and to_type == 'pb_gp_half_axes' and to_code[:-1] in curr_gp_output and this_gp_dict[from_code]:
            if to_code.endswith('P'):
                axis_value = 255
            elif to_code.endswith('N'):
                axis_value = 0
            curr_gp_output[to_code[:-1]].add(axis_value)
        # button to keyboard key
        if from_type == 'usb_gp_btn' and to_type == 'pb_kb':
            if to_code not in curr_kb_output:
                curr_kb_output[to_code] = set()
            curr_kb_output[to_code].add(this_gp_dict[from_code])
        # analog to keyboard key
        if from_type == 'usb_gp_axes' and type(to_code) is dict and to_code['type'] == 'pb_kb':
            if to_code['pos_key'] not in curr_kb_output:
                curr_kb_output[to_code['pos_key']] = set()
            is_activated = 0
            if clamp_to_8bit(this_gp_dict[from_code], axes_info, from_code) > 127+64:
                is_activated = 1
            curr_kb_output[to_code['pos_key']].add(is_activated)

            if to_code['neg_key'] not in curr_kb_output:
                curr_kb_output[to_code['neg_key']] = set()
            is_activated = 0
            if clamp_to_8bit(this_gp_dict[from_code], axes_info, from_code) < 127-64:
                is_activated = 1
            curr_kb_output[to_code['neg_key']].add(is_activated)
        # button to mouse buttons
        if from_type == 'usb_gp_btn' and to_type == 'mouse_btn' and to_code in curr_mouse_output:
            curr_mouse_output[to_code].add(this_gp_dict[from_code])
            curr_mouse_output['IS_MODIFIED'] = True
        # analog to mouse axes
        if from_type == 'usb_gp_axes' and to_type == 'mouse_axes' and to_code in curr_mouse_output:
            amount = clamp_to_8bit(this_gp_dict[from_code], axes_info, from_code) - 127
            if abs(amount) <= 20:
                amount = 0
            curr_mouse_output[to_code] = int(amount / 15) & 0xffff
            curr_mouse_output['IS_MODIFIED'] = True
    
    for key in curr_kb_output:
        key_status_set = curr_kb_output[key]
        curr_kb_output[key] = 0
        if 1 in key_status_set:
            curr_kb_output[key] = 1

    for key in curr_mouse_output:
        if "MOUSE_BTN_" in key:
            mouse_button_status_set = curr_mouse_output[key]
            curr_mouse_output[key] = 0
            if 1 in mouse_button_status_set:
                curr_mouse_output[key] = 1

    # if curr axes set is more than prev, apply the new one, else apply the one furest from midpoint
    gp_spi_msg = list(gamepad_event_ibm_ggp_spi_msg_template)
    gp_spi_msg[3] = gp_id;
    for key in curr_gp_output:
        if "IBMGGP_BTN_" in key:
            this_value = 0
            if 1 in curr_gp_output[key]:
                this_value = 1
            gp_spi_msg[IBMPC_GGP_SPI_LOOKUP[key]] = this_value
        if 'IBMGGP_JS' in key:
            if key in prev_gp_output:
                new_value_set = curr_gp_output[key] - prev_gp_output[key]
                if len(new_value_set) > 0:
                    gp_spi_msg[IBMPC_GGP_SPI_LOOKUP[key]] = list(new_value_set)[0]
                else:
                    gp_spi_msg[IBMPC_GGP_SPI_LOOKUP[key]] = find_furthest_from_midpoint(curr_gp_output[key])
            else:
                gp_spi_msg[IBMPC_GGP_SPI_LOOKUP[key]] = find_furthest_from_midpoint(curr_gp_output[key])

    kb_spi_msg = None
    new_keys = curr_kb_output.keys() - prev_kb_output.keys()
    if len(new_keys) > 0:
        this_key = list(new_keys)[0]
        kb_spi_msg = list(keyboard_event_spi_msg_template)
        kb_spi_msg[4] = this_key
        kb_spi_msg[6] = curr_kb_output[this_key]
    for key in curr_kb_output:
        if key in prev_kb_output and curr_kb_output[key] != prev_kb_output[key]:
            kb_spi_msg = list(keyboard_event_spi_msg_template)
            kb_spi_msg[4] = key
            kb_spi_msg[6] = curr_kb_output[key]

    mouse_spi_msg = None
    if curr_mouse_output['IS_MODIFIED']:
        mouse_spi_msg = list(mouse_event_spi_msg_template)
        for fuck in curr_mouse_output:
            if "MOUSE_BTN_" in fuck:
                mouse_spi_msg[MOUSE_SPI_LOOKUP[fuck]] = curr_mouse_output[fuck]
            elif "MOUSE_" in fuck:
                mouse_spi_msg[MOUSE_SPI_LOOKUP[fuck]:MOUSE_SPI_LOOKUP[fuck]+2] = list(curr_mouse_output[fuck].to_bytes(2, byteorder='little'))

    prev_gp_output = curr_gp_output
    prev_kb_output = curr_kb_output
    return gp_spi_msg, kb_spi_msg, mouse_spi_msg

def make_gamepad_spi_packet(gp_status_dict, gp_id, axes_info):
    current_protocol = usb4vc_ui.get_gamepad_protocol()
    if current_protocol['pid'] == PID_GENERIC_GAMEPORT_GAMEPAD and current_protocol['is_custom']:
        return make_generic_gamepad_spi_packet(gp_status_dict, gp_id, axes_info, current_protocol)
    return list(nop_spi_msg_template), None, None

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
        for key in list(ooopened_device_dict):
            try:
                data = ooopened_device_dict[key]['file'].read(16)
            except OSError:
                print("Device disappeared:", ooopened_device_dict[key]['name'])
                ooopened_device_dict[key]['file'].close()
                del ooopened_device_dict[key]
                continue
            if data is None:
                continue
            data = list(data[8:])
            print(data)
            # event is a key press
            if data[0] == EV_KEY:
                key_code = data[3] * 256 + data[2]
                print('EV_KEY:', key_code)
                # keyboard keys
                if 0x1 <= key_code <= 248:
                    pcard_spi.xfer(make_keyboard_spi_packet(data, ooopened_device_dict[key]['id']))
                # Mouse buttons
                elif 0x110 <= key_code <= 0x117:
                    mouse_status_dict[key_code] = data[4]
                    mouse_status_dict['x'] = [0, 0]
                    mouse_status_dict['y'] = [0, 0]
                    mouse_status_dict['scroll'] = [0, 0]
                    pcard_spi.xfer(make_mouse_spi_packet(mouse_status_dict, ooopened_device_dict[key]['id']))

def get_input_devices():
    result = []
    available_devices = [evdev.InputDevice(path) for path in evdev.list_devices()]
    for this_device in available_devices:
        dev_dict = {
            'path':this_device.path,
            'name':this_device.name,
            'axes_info':{},
            'is_kb':False,
            'is_mouse':False,
            'is_gp':False,
        }
        cap_str = str(this_device.capabilities(verbose=True))
        if 'BTN_LEFT' in cap_str and "EV_REL" in cap_str:
            dev_dict['is_mouse'] = True
        if 'KEY_ENTER' in cap_str and "KEY_Y" in cap_str:
            dev_dict['is_kb'] = True
        if 'EV_ABS' in cap_str and "BTN_SOUTH" in cap_str:
            dev_dict['is_gp'] = True
            try:
                for item in this_device.capabilities()[EV_ABS]:
                    dev_dict['axes_info'][item[0]] = item[1]._asdict()
            except Exception as e:
                print('get_input_devices exception:', e)
        result.append(dev_dict)
    return result

def usb_device_scan_worker():
    print("usb_device_scan_worker started")
    while 1:
        time.sleep(0.75)
        device_list = get_input_devices()
        if len(device_list) == 0:
            print('No input devices found')
            continue

        for item in device_list:
            if item['path'] in ooopened_device_dict:
                continue
            try:
                this_file = open(item['path'], "rb")
                os.set_blocking(this_file.fileno(), False)
                item['file'] = this_file
                try:
                    item['id'] = int(item['path'][len(item['path'].rstrip('0123456789')):])
                except:
                    item['id'] = 255
                ooopened_device_dict[item['path']] = item
                print("opened device:", item['name'])
            except Exception as e:
                print("Device open exception:", e, item)

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

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

opened_device_dict = {}

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

BTN_SOUTH = 0x130
BTN_EAST = 0x131
BTN_C = 0x132
BTN_NORTH = 0x133
BTN_WEST = 0x134
BTN_Z = 0x135
BTN_TL = 0x136
BTN_TR = 0x137
BTN_TL2 = 0x138
BTN_TR2 = 0x139
BTN_SELECT = 0x13a
BTN_START = 0x13b
BTN_MODE = 0x13c
BTN_THUMBL = 0x13d
BTN_THUMBR = 0x13e

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
    to_transfer[4:6] = mouse_dict['x']
    to_transfer[6:8] = mouse_dict['y']
    to_transfer[8:10] = mouse_dict['scroll']
    to_transfer[13] = mouse_dict[BTN_LEFT]
    to_transfer[14] = mouse_dict[BTN_RIGHT]
    to_transfer[15] = mouse_dict[BTN_MIDDLE]
    to_transfer[16] = mouse_dict[BTN_SIDE]
    to_transfer[17] = mouse_dict[BTN_EXTRA]
    return to_transfer

PID_GENERIC_GAMEPORT_GAMEPAD = 7

def clamp_to_8bit(value, axes_dict, axis_key):
    try:
        range_max = axes_dict[axis_key]['max'] - axes_dict[axis_key]['min']
        return int((value / range_max) * 255) + 127
    except Exception as e:
        print('clamp_to_8bit:', e)
    return 127

IBMPC_GGP_SPI_LOOKUP = {
    'IBMGBTN_1':4,
    'IBMGBTN_2':5,
    'IBMGBTN_3':6,
    'IBMGBTN_4':7,
    'IBMGGP_JS1_X':8,
    'IBMGGP_JS1_Y':9,
    'IBMGGP_JS2_X':10,
    'IBMGGP_JS2_Y':11,
}

MOUSE_SPI_LOOKUP = {
    BTN_LEFT:13,
    BTN_RIGHT:14,
    BTN_MIDDLE:15,
    BTN_SIDE:16,
    BTN_EXTRA:17,
    REL_X:4,
    REL_Y:6,
    REL_WHEEL:8,
}

def find_keycode_in_mapping(key_code, mapping_dict):
    code_type = None
    if ABS_X <= key_code <= ABS_HAT3Y:
        code_type = 'usb_gp_axes'
    elif BTN_SOUTH <= key_code <= BTN_THUMBR:
        code_type = 'usb_gp_btn'
    result = mapping_dict.get(key_code)
    if code_type is None or result is None:
        return None, None
    return code_type, result

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
        'IBMPC_GBTN_1':set([0]),
        'IBMPC_GBTN_2':set([0]),
        'IBMPC_GBTN_3':set([0]),
        'IBMPC_GBTN_4':set([0]),
        'IBMPC_GGP_JS1_X':set([127]),
        'IBMPC_GGP_JS1_Y':set([127]),
        'IBMPC_GGP_JS2_X':set([127]),
        'IBMPC_GGP_JS2_Y':set([127]),
    }
    curr_kb_output = {}
    curr_mouse_output = {
        'IS_MODIFIED':False,
        BTN_LEFT:set([0]),
        BTN_MIDDLE:set([0]),
        BTN_RIGHT:set([0]),
        BTN_SIDE:set([0]),
        BTN_EXTRA:set([0]),
        REL_X:0,
        REL_Y:0,
        REL_WHEEL:0,
    }
    
    for source_code in this_gp_dict:
        source_type, target_info = find_keycode_in_mapping(source_code, mapping_info['mapping'])
        if target_info is None:
            continue
        target_code = target_info['code']
        target_type = target_info['type']
        
        # button to button
        if source_type == 'usb_gp_btn' and target_type == 'pb_gp_btn' and target_code in curr_gp_output:
            curr_gp_output[target_code].add(this_gp_dict[source_code])
        # analog to analog
        if source_type == 'usb_gp_axes' and target_type == 'pb_gp_axes' and target_code in curr_gp_output:
            curr_gp_output[target_code].add(clamp_to_8bit(this_gp_dict[source_code], axes_info, source_code))
        # button to analog
        if source_type == 'usb_gp_btn' and target_type == 'pb_gp_half_axes' and target_code[:-1] in curr_gp_output and this_gp_dict[source_code]:
            if target_code.endswith('P'):
                axis_value = 255
            elif target_code.endswith('N'):
                axis_value = 0
            curr_gp_output[target_code[:-1]].add(axis_value)
        # button to keyboard key
        if source_type == 'usb_gp_btn' and target_type == 'pb_kb':
            if target_code not in curr_kb_output:
                curr_kb_output[target_code] = set()
            curr_kb_output[target_code].add(this_gp_dict[source_code])
        # analog to keyboard key
        if source_type == 'usb_gp_axes' and target_type == 'pb_kb':
            if target_code not in curr_kb_output:
                curr_kb_output[target_code] = set()
            is_activated = 0
            if clamp_to_8bit(this_gp_dict[source_code], axes_info, source_code) > 127 + target_info['deadzone']:
                is_activated = 1
            curr_kb_output[target_code].add(is_activated)

            if target_info['code_neg'] not in curr_kb_output:
                curr_kb_output[target_info['code_neg']] = set()
            is_activated = 0
            if clamp_to_8bit(this_gp_dict[source_code], axes_info, source_code) < 127 - target_info['deadzone']:
                is_activated = 1
            curr_kb_output[target_info['code_neg']].add(is_activated)
        # button to mouse buttons
        if source_type == 'usb_gp_btn' and target_type == 'mouse_btn' and target_code in curr_mouse_output:
            curr_mouse_output[target_code].add(this_gp_dict[source_code])
            curr_mouse_output['IS_MODIFIED'] = True
        # analog to mouse axes
        if source_type == 'usb_gp_axes' and target_type == 'mouse_axes' and target_code in curr_mouse_output:
            amount = clamp_to_8bit(this_gp_dict[source_code], axes_info, source_code) - 127
            if abs(amount) <= target_info['deadzone']:
                amount = 0
            curr_mouse_output[target_code] = int(amount / 15) & 0xffff
            curr_mouse_output['IS_MODIFIED'] = True
    
    for key in curr_kb_output:
        key_status_set = curr_kb_output[key]
        curr_kb_output[key] = 0
        if 1 in key_status_set:
            curr_kb_output[key] = 1

    for key in curr_mouse_output:
        if type(key) is int and BTN_LEFT <= key <= BTN_TASK:
            mouse_button_status_set = curr_mouse_output[key]
            curr_mouse_output[key] = 0
            if 1 in mouse_button_status_set:
                curr_mouse_output[key] = 1

    # if curr axes set is more than prev, apply the new one, else apply the one furest from midpoint
    gp_spi_msg = list(gamepad_event_ibm_ggp_spi_msg_template)
    gp_spi_msg[3] = gp_id;
    for key in curr_gp_output:
        if "IBMGBTN_" in key:
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
        for mcode in curr_mouse_output:
            if type(mcode) is int:
                if BTN_LEFT <= mcode <= BTN_TASK:
                    mouse_spi_msg[MOUSE_SPI_LOOKUP[mcode]] = curr_mouse_output[mcode]
                elif REL_X <= mcode <= 0x08:
                    mouse_spi_msg[MOUSE_SPI_LOOKUP[mcode]:MOUSE_SPI_LOOKUP[mcode]+2] = list(curr_mouse_output[mcode].to_bytes(2, byteorder='little'))

    prev_gp_output = curr_gp_output
    prev_kb_output = curr_kb_output
    return gp_spi_msg, kb_spi_msg, mouse_spi_msg

def make_gamepad_spi_packet(gp_status_dict, gp_id, axes_info):
    current_protocol = usb4vc_ui.get_gamepad_protocol()
    if current_protocol['pid'] == PID_GENERIC_GAMEPORT_GAMEPAD:
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

def clear_mouse_movement(mdict):
    mdict['x'] = [0, 0]
    mdict['y'] = [0, 0]
    mdict['scroll'] = [0, 0]

def raw_input_event_worker():
    mouse_status_dict = {'x': [0, 0], 'y': [0, 0], 'scroll': [0, 0], BTN_LEFT:0, BTN_RIGHT:0, BTN_MIDDLE:0, BTN_SIDE:0, BTN_EXTRA:0, BTN_FORWARD:0, BTN_BACK:0, BTN_TASK:0}
    gamepad_status_dict = {}
    print("raw_input_event_worker started")
    while 1:
        for key in list(opened_device_dict):
            this_device = opened_device_dict[key]
            this_id = this_device['id']
            try:
                data = this_device['file'].read(16)
            except OSError:
                print("Device disappeared:", this_device['name'])
                this_device['file'].close()
                del opened_device_dict[key]
                continue
            if data is None:
                continue

            if this_device['is_gp'] and this_id not in gamepad_status_dict:
                gamepad_status_dict[this_id] = {}

            data = list(data[8:])
            event_code = data[3] * 256 + data[2]

            # event is a key press
            if data[0] == EV_KEY:
                # keyboard keys
                if 0x1 <= event_code <= 248:
                    pcard_spi.xfer(make_keyboard_spi_packet(data, this_id))
                # Mouse buttons
                elif 0x110 <= event_code <= 0x117:
                    mouse_status_dict[event_code] = data[4]
                    clear_mouse_movement(mouse_status_dict)
                    pcard_spi.xfer(make_mouse_spi_packet(mouse_status_dict, this_id))
                # Gamepad buttons
                elif BTN_SOUTH <= event_code <= BTN_THUMBR:
                    gamepad_status_dict[this_id][event_code] = data[4]

            # event is relative axes AKA mouse
            elif data[0] == EV_REL and event_code == REL_X:
                rawx = int.from_bytes(data[4:6], byteorder='little', signed=True)
                rawx = int(rawx * usb4vc_ui.get_mouse_sensitivity()) & 0xffff
                mouse_status_dict["x"] = list(rawx.to_bytes(2, byteorder='little'))
            elif data[0] == EV_REL and event_code == REL_Y:
                rawy = int.from_bytes(data[4:6], byteorder='little', signed=True)
                rawy = int(rawy * usb4vc_ui.get_mouse_sensitivity()) & 0xffff
                mouse_status_dict["y"] = list(rawy.to_bytes(2, byteorder='little'))
            elif data[0] == EV_REL and event_code == REL_WHEEL:
                mouse_status_dict["scroll"] = data[4:6]

            # event is absolute axes AKA joystick
            elif data[0] == EV_ABS:
                abs_value = int.from_bytes(data[4:8], byteorder='little', signed=True)
                gamepad_status_dict[this_id][event_code] = abs_value

            # SYNC event
            elif data[0] == EV_SYN and event_code == SYN_REPORT:
                if this_device['is_mouse']:
                    pcard_spi.xfer(make_mouse_spi_packet(mouse_status_dict, this_id))
                    clear_mouse_movement(mouse_status_dict)
                if this_device['is_gp']:
                    gp_to_transfer, kb_to_transfer, mouse_to_transfer = make_gamepad_spi_packet(gamepad_status_dict, this_id, this_device['axes_info'])
                    pcard_spi.xfer(gp_to_transfer)
                    if kb_to_transfer is not None:
                        time.sleep(0.001)
                        pcard_spi.xfer(kb_to_transfer)
                    if mouse_to_transfer is not None:
                        time.sleep(0.001)
                        pcard_spi.xfer(mouse_to_transfer)

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

def get_device_count():
    mouse_count = 0
    kb_count = 0
    gp_count = 0
    for key in opened_device_dict:
        if opened_device_dict[key]['is_mouse']:
            mouse_count += 1
        elif opened_device_dict[key]['is_kb']:
            kb_count += 1
        elif opened_device_dict[key]['is_gp']:
            gp_count += 1
    return (mouse_count, kb_count, gp_count)

def usb_device_scan_worker():
    print("usb_device_scan_worker started")
    while 1:
        time.sleep(0.75)
        device_list = get_input_devices()
        if len(device_list) == 0:
            print('No input devices found')
            continue

        for item in device_list:
            if item['path'] in opened_device_dict:
                continue
            try:
                this_file = open(item['path'], "rb")
                os.set_blocking(this_file.fileno(), False)
                item['file'] = this_file
                try:
                    item['id'] = int(item['path'][len(item['path'].rstrip('0123456789')):])
                except:
                    item['id'] = 255
                opened_device_dict[item['path']] = item
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

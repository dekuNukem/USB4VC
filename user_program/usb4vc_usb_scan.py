import os
import sys
import time
import math
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
REL_HWHEEL = 0x06
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
BTN_THUMBR = 0x13e

ABS_X = 0x00
ABS_HAT3Y = 0x17

BTN_LEFT = 0x110
BTN_RIGHT = 0x111
BTN_MIDDLE = 0x112
BTN_SIDE = 0x113
BTN_EXTRA = 0x114
BTN_FORWARD = 0x115
BTN_BACK = 0x116
BTN_TASK = 0x117

# some gamepad buttons report as keyboard keys when connected via bluetooth, need to account for those
gamepad_buttons_as_kb_codes = {
    158, # KEY_BACK, share button, xbox one controller when connected via bluetooth
    172, # KEY_HOMEPAGE, xbox logo button, xbox one controller when connected via bluetooth
    }

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
    to_transfer[8] = mouse_dict['scroll']
    to_transfer[9] = mouse_dict['hscroll']
    to_transfer[13] = mouse_dict[BTN_LEFT]
    to_transfer[14] = mouse_dict[BTN_RIGHT]
    to_transfer[15] = mouse_dict[BTN_MIDDLE]
    to_transfer[16] = mouse_dict[BTN_SIDE]
    to_transfer[17] = mouse_dict[BTN_EXTRA]
    return to_transfer

PID_GENERIC_GAMEPORT_GAMEPAD = 7
PID_PROTOCOL_OFF = 0

def get_range_max_and_midpoint(axes_dict, axis_key):
    try:
        return axes_dict[axis_key]['max'] - axes_dict[axis_key]['min'], int((axes_dict[axis_key]['max'] + axes_dict[axis_key]['min']) / 2)
    except Exception as e:
        print('get_range_max_and_midpoint:', e)
    return None, None

def convert_to_8bit_midpoint127(value, axes_dict, axis_key):
    rmax, rmid = get_range_max_and_midpoint(axes_dict, axis_key)
    if rmax is None:
        return 127
    value -= rmid
    return int((value / rmax) * 255) + 127

def scale_to_8bit(value, axes_dict, axis_key):
    rmax, rmid = get_range_max_and_midpoint(axes_dict, axis_key)
    if rmax is None:
        return value % 256
    ratio = rmax / 255
    return int(value/ratio)

IBMPC_GGP_SPI_LOOKUP = {
    'IBM_GGP_BTN_1':4,
    'IBM_GGP_BTN_2':5,
    'IBM_GGP_BTN_3':6,
    'IBM_GGP_BTN_4':7,
    'IBM_GGP_JS1_X':8,
    'IBM_GGP_JS1_Y':9,
    'IBM_GGP_JS2_X':10,
    'IBM_GGP_JS2_Y':11,
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

xbox_one_bluetooth_to_linux_ev_code_dict = {
    "XB1_A":"BTN_SOUTH",
    "XB1_B":"BTN_EAST",
    "XB1_X":"BTN_NORTH",
    "XB1_Y":"BTN_WEST",
    "XB1_LSB":"BTN_THUMBL",
    "XB1_RSB":"BTN_THUMBR",
    "XB1_LB":"BTN_TL",
    "XB1_RB":"BTN_TR",
    "XB1_VIEW":"KEY_BACK",
    "XB1_MENU":"BTN_START",
    "XB1_LOGO":"KEY_HOMEPAGE",
    "XB1_LSX":"ABS_X",
    "XB1_LSY":"ABS_Y",
    "XB1_RSX":"ABS_Z",
    "XB1_RSY":"ABS_RZ",
    "XB1_LT":"ABS_BRAKE",
    "XB1_RT":"ABS_GAS",
    "XB1_DPX":"ABS_HAT0X",
    "XB1_DPY":"ABS_HAT0Y",
}

xbox_one_wired_to_linux_ev_code_dict = {
    "XB1_A":"BTN_SOUTH",
    "XB1_B":"BTN_EAST",
    "XB1_X":"BTN_NORTH",
    "XB1_Y":"BTN_WEST",
    "XB1_LSB":"BTN_THUMBL",
    "XB1_RSB":"BTN_THUMBR",
    "XB1_LB":"BTN_TL",
    "XB1_RB":"BTN_TR",
    "XB1_VIEW":"BTN_SELECT",
    "XB1_MENU":"BTN_START",
    "XB1_LOGO":"BTN_MODE",
    "XB1_LSX":"ABS_X",
    "XB1_LSY":"ABS_Y",
    "XB1_RSX":"ABS_RX",
    "XB1_RSY":"ABS_RY",
    "XB1_LT":"ABS_Z",
    "XB1_RT":"ABS_RZ",
    "XB1_DPX":"ABS_HAT0X",
    "XB1_DPY":"ABS_HAT0Y",
}

def translate_dict(old_mapping_dict, lookup_dict):
    translated_map_dict = {}
    for key in old_mapping_dict:
        lookup_result = lookup_dict.get(key)
        if lookup_result is not None:
            translated_map_dict[lookup_result] = old_mapping_dict[key]
    return translated_map_dict

def find_keycode_in_mapping(source_code, mapping_dict, usb_gamepad_type):
    source_name = usb4vc_shared.code_value_to_name_lookup.get(source_code)
    if source_name is None:
        return None, None
    if usb_gamepad_type == "Xbox One Bluetooth":
        mapping_dict = translate_dict(mapping_dict, xbox_one_bluetooth_to_linux_ev_code_dict)
    elif usb_gamepad_type == "Xbox One Wired":
        mapping_dict = translate_dict(mapping_dict, xbox_one_wired_to_linux_ev_code_dict)
    target_info = None
    for item in source_name:
        if item in mapping_dict:
            target_info = mapping_dict[item]
    if target_info is None or 'code' not in target_info:
        return None, None
    target_info = dict(target_info) # make a copy so the lookup table itself won't get modified

    lookup_result = usb4vc_shared.code_name_to_value_lookup.get(target_info['code'])
    if lookup_result is None:
        return None, None
    source_type = None
    if ABS_X <= source_code <= ABS_HAT3Y:
        source_type = 'usb_abs_axis'
    elif BTN_SOUTH <= source_code <= BTN_THUMBR or source_code in gamepad_buttons_as_kb_codes:
        source_type = 'usb_gp_btn'
    if source_type is None:
        return None, None
    target_info['code'] = lookup_result[0]
    if 'code_neg' in target_info:
        target_info['code_neg'] = usb4vc_shared.code_name_to_value_lookup.get(target_info['code_neg'])[0]
    target_info['type'] = lookup_result[1]
    return source_type, target_info

def find_furthest_from_midpoint(this_set):
    curr_best = None
    diff_max = 0
    for item in this_set:
        this_diff = abs(item - 127)
        if this_diff >= diff_max:
            diff_max = this_diff
            curr_best = item
    return curr_best

def apply_curve(x_0_255, y_0_255):
    x_neg127_127 = x_0_255 - 127
    y_neg127_127 = y_0_255 - 127
    try:
        real_amount = int(math.sqrt(abs(x_neg127_127) ** 2 + abs(y_neg127_127) ** 2))
        if real_amount > 127:
            real_amount = 127
        lookup_result = usb4vc_ui.get_joystick_curve().get(real_amount, real_amount)
        ratio = real_amount / lookup_result
        xxxxx = int(x_neg127_127 / ratio)
        yyyyy = int(y_neg127_127 / ratio)
        return xxxxx + 127, yyyyy + 127
    except Exception as e:
        # print('apply_curve:', e)
        pass
    return x_0_255, y_0_255

ABS_Z = 0x02
ABS_RZ = 0x05
ABS_GAS = 0x09
ABS_BRAKE = 0x0a
xbox_one_wired_analog_trigger_codes = {ABS_Z, ABS_RZ}
xbox_one_bluetooth_analog_trigger_codes = {ABS_GAS, ABS_BRAKE}

def is_analog_trigger(ev_code, gamepad_type):
    if gamepad_type == "Xbox One Bluetooth" and ev_code in xbox_one_bluetooth_analog_trigger_codes:
        return True
    if gamepad_type == "Xbox One Wired" and ev_code in xbox_one_wired_analog_trigger_codes:
        return True
    return False

prev_gp_output = {}
prev_kb_output = {}
curr_mouse_output = {}
def make_generic_gamepad_spi_packet(gp_status_dict, gp_id, axes_info, mapping_info):
    global prev_gp_output
    global prev_kb_output
    global curr_mouse_output
    this_gp_dict = gp_status_dict[gp_id]
    curr_gp_output = {
        'IBM_GGP_BTN_1':set([0]),
        'IBM_GGP_BTN_2':set([0]),
        'IBM_GGP_BTN_3':set([0]),
        'IBM_GGP_BTN_4':set([0]),
        'IBM_GGP_JS1_X':set([127]),
        'IBM_GGP_JS1_Y':set([127]),
        'IBM_GGP_JS2_X':set([127]),
        'IBM_GGP_JS2_Y':set([127]),
    }
    curr_kb_output = {}
    curr_mouse_output = {
        'mouse_id':gp_id,
        'is_modified':False,
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
        usb_gamepad_type = mapping_info.get('usb_gamepad_type', 'Generic USB')
        source_type, target_info = find_keycode_in_mapping(source_code, mapping_info['mapping'], usb_gamepad_type)
        if target_info is None:
            continue
        target_code = target_info['code']
        target_type = target_info['type']
        # print(source_code, source_type, target_code, target_type)
        # print("----------")
        # usb gamepad button to generic gamepad button
        if source_type == 'usb_gp_btn' and target_type == 'ibm_ggp_btn' and target_code in curr_gp_output:
            curr_gp_output[target_code].add(this_gp_dict[source_code])
        # usb gamepad analog axes to generic gamepad analog axes
        if source_type == 'usb_abs_axis' and target_type == 'ibm_ggp_axis' and target_code in curr_gp_output:
            curr_gp_output[target_code].add(convert_to_8bit_midpoint127(this_gp_dict[source_code], axes_info, source_code))
        # usb gamepad analog axes to generic gamepad analog half axes
        # only use to map USB analog trigger to generic gameport gamepad half axes
        if source_type == 'usb_abs_axis' and target_type == 'ibm_ggp_half_axis' and target_code[:-1] in curr_gp_output:
            source_value = convert_to_8bit_midpoint127(this_gp_dict[source_code], axes_info, source_code)//2
            axis_value = 127
            if target_code.endswith('P'):
                axis_value += abs(source_value)
            elif target_code.endswith('N'):
                axis_value -= abs(source_value)
            curr_gp_output[target_code[:-1]].add(axis_value)
        # usb gamepad button to generic gamepad analog axes
        if source_type == 'usb_gp_btn' and target_type == 'ibm_ggp_half_axis' and target_code[:-1] in curr_gp_output and this_gp_dict[source_code]:
            if target_code.endswith('P'):
                axis_value = 255
            elif target_code.endswith('N'):
                axis_value = 0
            curr_gp_output[target_code[:-1]].add(axis_value)
        # usb gamepad button to keyboard key
        if source_type == 'usb_gp_btn' and target_type == 'kb_key':
            if target_code not in curr_kb_output:
                curr_kb_output[target_code] = set()
            curr_kb_output[target_code].add(this_gp_dict[source_code])
        # usb gamepad analog axes to keyboard key
        if source_type == 'usb_abs_axis' and target_type == 'kb_key':
            if target_code not in curr_kb_output:
                curr_kb_output[target_code] = set()
            is_activated = 0
            deadzone_amount = 50
            try:
                deadzone_amount = int(127 * target_info['deadzone_percent'] / 100)
            except Exception:
                pass
            if is_analog_trigger(source_code, usb_gamepad_type):
                scaled_value = scale_to_8bit(this_gp_dict[source_code], axes_info, source_code)
                if scaled_value > deadzone_amount:
                    is_activated = 1
                curr_kb_output[target_code].add(is_activated)
            else:
                if convert_to_8bit_midpoint127(this_gp_dict[source_code], axes_info, source_code) > 127 + deadzone_amount:
                    is_activated = 1
                curr_kb_output[target_code].add(is_activated)
                if target_info['code_neg'] not in curr_kb_output:
                    curr_kb_output[target_info['code_neg']] = set()
                is_activated = 0
                if convert_to_8bit_midpoint127(this_gp_dict[source_code], axes_info, source_code) < 127 - deadzone_amount:
                    is_activated = 1
                curr_kb_output[target_info['code_neg']].add(is_activated)

        # usb gamepad button to mouse buttons
        if source_type == 'usb_gp_btn' and target_type == 'mouse_btn' and target_code in curr_mouse_output:
            curr_mouse_output[target_code].add(this_gp_dict[source_code])
            curr_mouse_output['is_modified'] = True
        # usb gamepad analog axes to mouse axes
        if source_type == 'usb_abs_axis' and target_type == 'usb_rel_axis' and target_code in curr_mouse_output:
            if is_analog_trigger(source_code, usb_gamepad_type):
                pass
            else:
                movement = convert_to_8bit_midpoint127(this_gp_dict[source_code], axes_info, source_code) - 127
                deadzone_amount = 20
                try:
                    deadzone_amount = int(127 * target_info['deadzone_percent'] / 100)
                except Exception:
                    pass
                if abs(movement) <= deadzone_amount:
                    movement = 0
                joystick_to_mouse_slowdown = 17
                curr_mouse_output[target_code] = int(movement / joystick_to_mouse_slowdown)
                curr_mouse_output['is_modified'] = True

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
        if "IBM_GGP_BTN_" in key:
            this_value = 0
            if 1 in curr_gp_output[key]:
                this_value = 1
            gp_spi_msg[IBMPC_GGP_SPI_LOOKUP[key]] = this_value
        if 'IBM_GGP_JS' in key:
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
        kb_spi_msg[3] = gp_id
        kb_spi_msg[4] = this_key
        kb_spi_msg[6] = curr_kb_output[this_key]
    for key in curr_kb_output:
        if key in prev_kb_output and curr_kb_output[key] != prev_kb_output[key]:
            kb_spi_msg = list(keyboard_event_spi_msg_template)
            kb_spi_msg[3] = gp_id
            kb_spi_msg[4] = key
            kb_spi_msg[6] = curr_kb_output[key]

    mouse_spi_msg = None
    if curr_mouse_output['is_modified']:
        mouse_spi_msg = list(mouse_event_spi_msg_template)
        mouse_spi_msg[3] = gp_id
        for mcode in curr_mouse_output:
            if type(mcode) is int:
                if BTN_LEFT <= mcode <= BTN_TASK:
                    mouse_spi_msg[MOUSE_SPI_LOOKUP[mcode]] = curr_mouse_output[mcode]
                elif REL_X <= mcode <= REL_WHEEL:
                    mouse_spi_msg[MOUSE_SPI_LOOKUP[mcode]:MOUSE_SPI_LOOKUP[mcode]+2] = list((curr_mouse_output[mcode] & 0xffff).to_bytes(2, byteorder='little'))

    prev_gp_output = curr_gp_output
    prev_kb_output = curr_kb_output
    gp_spi_msg[8], gp_spi_msg[9] = apply_curve(gp_spi_msg[8], gp_spi_msg[9])
    gp_spi_msg[10], gp_spi_msg[11] = apply_curve(gp_spi_msg[10], gp_spi_msg[11])
    return gp_spi_msg, kb_spi_msg, mouse_spi_msg

def make_gamepad_spi_packet(gp_status_dict, gp_id, axes_info):
    current_protocol = usb4vc_ui.get_gamepad_protocol()
    try:
        if current_protocol['pid'] in [PID_GENERIC_GAMEPORT_GAMEPAD, PID_PROTOCOL_OFF]:
            return make_generic_gamepad_spi_packet(gp_status_dict, gp_id, axes_info, current_protocol)
    except Exception as e:
        print("make_generic_gamepad_spi_packet:", e)
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
    mdict['scroll'] = 0
    mdict['hscroll'] = 0

def joystick_hold_update():
    needs_update = 0
    abs_value_multiplier = 1.5
    for key in curr_mouse_output:
        if type(key) is int and REL_X <= key <= REL_WHEEL and curr_mouse_output[key]:
            needs_update = 1
    if needs_update and curr_mouse_output['is_modified']:
        mouse_spi_msg = list(mouse_event_spi_msg_template)
        mouse_spi_msg[3] = curr_mouse_output['mouse_id']
        for mcode in curr_mouse_output:
            if type(mcode) is int:
                if BTN_LEFT <= mcode <= BTN_TASK:
                    mouse_spi_msg[MOUSE_SPI_LOOKUP[mcode]] = curr_mouse_output[mcode]
                elif REL_X <= mcode <= 0x08:
                    movement_value = int(curr_mouse_output[mcode] * abs_value_multiplier) & 0xffff
                    mouse_spi_msg[MOUSE_SPI_LOOKUP[mcode]:MOUSE_SPI_LOOKUP[mcode]+2] = list(movement_value.to_bytes(2, byteorder='little'))
        pcard_spi.xfer(mouse_spi_msg)

def multiply_round_up_0(number, multi):
    new_number = number * multi
    if 0 < new_number < 1:
        new_number = 1
    elif -1 < new_number < 0:
        new_number = -1
    return int(new_number)

gamepad_hold_check_interval = 0.02
def raw_input_event_worker():
    last_usb_event = 0
    mouse_status_dict = {'x': [0, 0], 'y': [0, 0], 'scroll': 0, 'hscroll': 0, BTN_LEFT:0, BTN_RIGHT:0, BTN_MIDDLE:0, BTN_SIDE:0, BTN_EXTRA:0, BTN_FORWARD:0, BTN_BACK:0, BTN_TASK:0}
    gamepad_status_dict = {}
    next_gamepad_hold_check = time.time() + gamepad_hold_check_interval
    last_mouse_msg = []
    print("raw_input_event_worker started")
    while 1:
        now = time.time()
        if now > next_gamepad_hold_check:
            joystick_hold_update()
            next_gamepad_hold_check = now + gamepad_hold_check_interval

        # give other threads some breathing room if 
        # no USB input events
        if now - last_usb_event > 1:
            time.sleep(0.005)

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

            usb4vc_ui.my_oled.kick()
            last_usb_event = now

            if this_device['is_gp'] and this_id not in gamepad_status_dict:
                gamepad_status_dict[this_id] = {}

            data = list(data[8:])
            event_code = data[3] * 256 + data[2]

            # event is a key press
            if data[0] == EV_KEY:
                # keyboard keys
                if 0x1 <= event_code <= 248 and event_code not in gamepad_buttons_as_kb_codes:
                    pcard_spi.xfer(make_keyboard_spi_packet(data, this_id))
                # Mouse buttons
                elif 0x110 <= event_code <= 0x117:
                    mouse_status_dict[event_code] = data[4]
                    next_gamepad_hold_check = now + gamepad_hold_check_interval
                    if data[4] != 2:
                        clear_mouse_movement(mouse_status_dict)
                        pcard_spi.xfer(make_mouse_spi_packet(mouse_status_dict, this_id))
                # Gamepad buttons
                elif BTN_SOUTH <= event_code <= BTN_THUMBR or event_code in gamepad_buttons_as_kb_codes:
                    gamepad_status_dict[this_id][event_code] = data[4]

            # event is relative axes AKA mouse
            elif data[0] == EV_REL and event_code == REL_X:
                rawx = int.from_bytes(data[4:6], byteorder='little', signed=True)
                rawx = multiply_round_up_0(rawx, usb4vc_ui.get_mouse_sensitivity()) & 0xffff
                mouse_status_dict["x"] = list(rawx.to_bytes(2, byteorder='little'))
            elif data[0] == EV_REL and event_code == REL_Y:
                rawy = int.from_bytes(data[4:6], byteorder='little', signed=True)
                rawy = multiply_round_up_0(rawy, usb4vc_ui.get_mouse_sensitivity()) & 0xffff
                mouse_status_dict["y"] = list(rawy.to_bytes(2, byteorder='little'))
            elif data[0] == EV_REL and event_code == REL_WHEEL:
                mouse_status_dict['scroll'] = data[4]
            elif data[0] == EV_REL and event_code == REL_HWHEEL:
                mouse_status_dict['hscroll'] = data[4]

            # event is absolute axes AKA joystick
            elif this_device['is_gp'] and data[0] == EV_ABS:
                abs_value = int.from_bytes(data[4:8], byteorder='little', signed=True)
                gamepad_status_dict[this_id][event_code] = abs_value

            # SYNC event
            elif data[0] == EV_SYN and event_code == SYN_REPORT:
                if this_device['is_mouse']:
                    this_mouse_msg = make_mouse_spi_packet(mouse_status_dict, this_id)
                    # send spi mouse message if there is moment, or the button is not typematic
                    if (max(this_mouse_msg[13:18]) != 2 or sum(this_mouse_msg[4:10]) != 0) and (this_mouse_msg[4:] != last_mouse_msg[4:] or sum(this_mouse_msg[4:]) != 0):
                        pcard_spi.xfer(list(this_mouse_msg))
                        next_gamepad_hold_check = now + gamepad_hold_check_interval
                        clear_mouse_movement(mouse_status_dict)
                    last_mouse_msg = list(this_mouse_msg)
                if this_device['is_gp']:
                    gp_to_transfer, kb_to_transfer, mouse_to_transfer = make_gamepad_spi_packet(gamepad_status_dict, this_id, this_device['axes_info'])
                    pcard_spi.xfer(gp_to_transfer)
                    if kb_to_transfer is not None:
                        time.sleep(0.001)
                        pcard_spi.xfer(kb_to_transfer)
                    if mouse_to_transfer is not None:
                        time.sleep(0.001)
                        pcard_spi.xfer(mouse_to_transfer)
                        next_gamepad_hold_check = now + gamepad_hold_check_interval
            
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
        try:
            device_list = get_input_devices()
        except Exception as e:
            print('exception at get_input_devices:', e)
            continue
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

def get_pboard_info():
    # send request
    this_msg = list(info_request_spi_msg_template)
    this_msg[3] = usb4vc_shared.RPI_APP_VERSION_TUPLE[0]
    this_msg[4] = usb4vc_shared.RPI_APP_VERSION_TUPLE[1]
    this_msg[5] = usb4vc_shared.RPI_APP_VERSION_TUPLE[2]
    pcard_spi.xfer(this_msg)

    time.sleep(0.1)
    # send an empty message to allow response to be shifted into RPi
    response = pcard_spi.xfer(list(nop_spi_msg_template))
    time.sleep(0.01)
    response = pcard_spi.xfer(list(nop_spi_msg_template))
    return response

def set_protocol(raw_msg):
    time.sleep(0.05)
    pcard_spi.xfer(list(raw_msg))
    time.sleep(0.05)

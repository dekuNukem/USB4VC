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
import usb4vc_gamepads

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
BTN_TR = 0x137
BTN_TL2 = 0x138
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
    373, # KEY_MODE, Xbox logo button, xbox one controller when connected via bluetooth on xpadneo
    }

nop_spi_msg_template = [SPI_MOSI_MAGIC] + [0]*31
info_request_spi_msg_template = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_INFO_REQUEST] + [0]*29
keyboard_event_spi_msg_template = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_KEYBOARD_EVENT] + [0]*29
mouse_event_spi_msg_template = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_MOUSE_EVENT] + [0]*29
gamepad_event_ibm_ggp_spi_msg_template = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_GAMEPAD_EVENT_MAPPED, 0, 0, 0, 0, 0, 127, 127, 127, 127] + [0]*20
raw_usb_gamepad_event_spi_msg_template = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_GAMEPAD_EVENT_RAW] + [0]*7 + [127]*8 + [0]*14

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

PID_PROTOCOL_OFF = 0
PID_GENERIC_GAMEPORT_GAMEPAD = 7
PID_RAW_USB_GAMEPAD = 127

def get_range_max_and_midpoint(axes_dict, axis_key):
    try:
        return axes_dict[axis_key]['max'] - axes_dict[axis_key]['min'], int((axes_dict[axis_key]['max'] + axes_dict[axis_key]['min']) / 2)
    except Exception as e:
        print('exception get_range_max_and_midpoint:', e)
    return None, None

def convert_to_8bit_midpoint127(value, axes_dict, axis_key):
    rmax, rmid = get_range_max_and_midpoint(axes_dict, axis_key)
    if rmax is None:
        return 127
    value -= rmid
    result = int((value / rmax) * 255) + 127
    if result == 254:
        result = 255
    return result

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

def translate_dict(old_mapping_dict, lookup_dict):
    translated_map_dict = dict(old_mapping_dict)
    for key in old_mapping_dict:
        lookup_result = lookup_dict.get(key)
        if lookup_result is not None:
            translated_map_dict[lookup_result] = old_mapping_dict[key]
            del translated_map_dict[key]
    return translated_map_dict

def find_keycode_in_mapping(source_code, mapping_dict, usb_gamepad_type):
    # print(source_code, mapping_dict, usb_gamepad_type)
    map_dict_copy = dict(mapping_dict)
    if 'DualSense' in usb_gamepad_type and map_dict_copy.get("MAPPING_TYPE") == "DEFAULT_15PIN":
        map_dict_copy = usb4vc_gamepads.PS5_DEFAULT_MAPPING
    elif 'DualShock 4' in usb_gamepad_type and map_dict_copy.get("MAPPING_TYPE") == "DEFAULT_15PIN":
        map_dict_copy = usb4vc_gamepads.PS4_DEFAULT_MAPPING
    elif 'Xbox' in usb_gamepad_type and map_dict_copy.get("MAPPING_TYPE") == "DEFAULT_15PIN":
        map_dict_copy = usb4vc_gamepads.XBOX_DEFAULT_MAPPING
    elif 'DualSense' in usb_gamepad_type and map_dict_copy.get("MAPPING_TYPE") == "DEFAULT_MOUSE_KB":
        map_dict_copy = usb4vc_gamepads.PS5_DEFAULT_KB_MOUSE_MAPPING
    elif 'DualShock 4' in usb_gamepad_type and map_dict_copy.get("MAPPING_TYPE") == "DEFAULT_MOUSE_KB":
        map_dict_copy = usb4vc_gamepads.PS4_DEFAULT_KB_MOUSE_MAPPING
    elif 'Xbox' in usb_gamepad_type and map_dict_copy.get("MAPPING_TYPE") == "DEFAULT_MOUSE_KB":
        map_dict_copy = usb4vc_gamepads.XBOX_DEFAULT_KB_MOUSE_MAPPING

    source_name = usb4vc_shared.code_value_to_name_lookup.get(source_code)
    if source_name is None:
        return None, None
    if 'Xbox' in usb_gamepad_type:
        map_dict_copy = translate_dict(map_dict_copy, usb4vc_gamepads.xbox_one_to_linux_ev_code_dict)
    elif 'DualSense' in usb_gamepad_type:
        map_dict_copy = translate_dict(map_dict_copy, usb4vc_gamepads.ps5_to_linux_ev_code_dict)
    elif 'DualShock 4' in usb_gamepad_type:
        map_dict_copy = translate_dict(map_dict_copy, usb4vc_gamepads.ps4_to_linux_ev_code_dict)
    target_info = None
    for item in source_name:
        if item in map_dict_copy:
            target_info = map_dict_copy[item]
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
        # print('exception apply_curve:', e)
        pass
    return x_0_255, y_0_255

prev_gp_output = {}
prev_kb_output = {}
curr_mouse_output = {}
def make_15pin_gamepad_spi_packet(gp_status_dict, this_device_info, mapping_info):
    global prev_gp_output
    global prev_kb_output
    global curr_mouse_output

    axes_info = this_device_info['axes_info']
    this_gamepad_name = this_device_info.get('name', '').lower().strip()
    usb_gamepad_type = this_device_info['gamepad_type']
    gp_id = this_device_info['id']
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
        source_type, target_info = find_keycode_in_mapping(source_code, mapping_info['mapping'], usb_gamepad_type)
        if target_info is None:
            continue
        source_value = this_gp_dict[source_code]
        target_code = target_info['code']
        target_type = target_info['type']
        # print(source_code, source_type, target_code, target_type)
        # print("----------")
        # usb gamepad button to generic gamepad button
        if source_type == 'usb_gp_btn' and target_type == 'ibm_ggp_btn' and target_code in curr_gp_output:
            curr_gp_output[target_code].add(source_value)
        # usb gamepad analog axes to generic gamepad analog axes
        if source_type == 'usb_abs_axis' and target_type == 'ibm_ggp_axis' and target_code in curr_gp_output and usb4vc_gamepads.is_analog_trigger(source_code, usb_gamepad_type) is False:
            curr_gp_output[target_code].add(source_value)
        # usb gamepad analog axes to generic gamepad analog half axes
        # only use to map USB analog trigger to generic gameport gamepad half axes
        if source_type == 'usb_abs_axis' and target_type == 'ibm_ggp_half_axis' and target_code[:-1] in curr_gp_output:
            axis_value = 127
            if target_code.endswith('P'):
                axis_value += abs(source_value)//2
            elif target_code.endswith('N'):
                axis_value -= abs(source_value)//2
            curr_gp_output[target_code[:-1]].add(axis_value)
        # usb gamepad button to generic gamepad analog axes
        if source_type == 'usb_gp_btn' and target_type == 'ibm_ggp_half_axis' and target_code[:-1] in curr_gp_output and source_value:
            if target_code.endswith('P'):
                axis_value = 255
            elif target_code.endswith('N'):
                axis_value = 0
            curr_gp_output[target_code[:-1]].add(axis_value)
        # usb gamepad button to keyboard key
        if source_type == 'usb_gp_btn' and target_type == 'kb_key':
            if target_code not in curr_kb_output:
                curr_kb_output[target_code] = set()
            curr_kb_output[target_code].add(source_value)
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
            if usb4vc_gamepads.is_analog_trigger(source_code, usb_gamepad_type):
                if source_value > deadzone_amount:
                    is_activated = 1
                curr_kb_output[target_code].add(is_activated)
            else:
                if source_value > 127 + deadzone_amount:
                    is_activated = 1
                curr_kb_output[target_code].add(is_activated)
                if target_info['code_neg'] not in curr_kb_output:
                    curr_kb_output[target_info['code_neg']] = set()
                is_activated = 0
                if source_value < 127 - deadzone_amount:
                    is_activated = 1
                curr_kb_output[target_info['code_neg']].add(is_activated)

        # usb gamepad button to mouse buttons
        if source_type == 'usb_gp_btn' and target_type == 'mouse_btn' and target_code in curr_mouse_output:
            curr_mouse_output[target_code].add(source_value)
            curr_mouse_output['is_modified'] = True
        # usb gamepad analog axes to mouse axes
        if source_type == 'usb_abs_axis' and target_type == 'usb_rel_axis' and target_code in curr_mouse_output and usb4vc_gamepads.is_analog_trigger(source_code, usb_gamepad_type) is False:
            movement = source_value - 127
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
                if 0 and len(new_value_set) > 0:
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


def make_unsupported_raw_gamepad_spi_packet(gp_status_dict, this_device_info):
    gp_id = this_device_info['id']
    this_msg = list(raw_usb_gamepad_event_spi_msg_template)
    this_msg[3] = gp_id
    this_msg[4:6] = this_device_info['vendor_id'].to_bytes(2, byteorder='little')
    this_msg[6:8] = this_device_info['product_id'].to_bytes(2, byteorder='little')

    for event_code in gp_status_dict[gp_id]:
        if BTN_SOUTH <= event_code <= BTN_TR:
            this_msg[8] |= (gp_status_dict[gp_id][event_code] << (event_code - BTN_SOUTH))
        elif BTN_TL2 <= event_code <= BTN_THUMBR:
            this_msg[9] |= (gp_status_dict[gp_id][event_code] << (event_code - BTN_TL2))
        elif ABS_X <= event_code <= ABS_HAT3Y:
            msg_index = usb4vc_gamepads.raw_usb_gamepad_abs_axes_to_spi_msg_index_lookup.get(event_code)
            if msg_index is not None:
                this_msg[msg_index] = gp_status_dict[gp_id][event_code]
    # print('{:08b}'.format(this_msg[9]))
    return this_msg, None, None

def xbname_to_ev_codename(xb_btn):
    ev_name = usb4vc_gamepads.xbox_one_to_linux_ev_code_dict.get(xb_btn)
    if ev_name is None:
        return None
    return usb4vc_shared.code_name_to_value_lookup[ev_name][0]

xbox_to_generic_dict = {
    xbname_to_ev_codename('XB_A'):'FACE_BUTTON_SOUTH',
    xbname_to_ev_codename('XB_B'):'FACE_BUTTON_EAST',
    xbname_to_ev_codename('XB_X'):'FACE_BUTTON_WEST',
    xbname_to_ev_codename('XB_Y'):'FACE_BUTTON_NORTH',
    xbname_to_ev_codename('XB_LB'):'SHOULDER_BUTTON_LEFT',
    xbname_to_ev_codename('XB_RB'):'SHOULDER_BUTTON_RIGHT',
    xbname_to_ev_codename('XB_LSB'):'STICK_BUTTON_LEFT',
    xbname_to_ev_codename('XB_RSB'):'STICK_BUTTON_RIGHT',
    xbname_to_ev_codename('XB_VIEW'):'MISC_BUTTON_1',
    xbname_to_ev_codename('XB_MENU'):'MISC_BUTTON_2',
    xbname_to_ev_codename('XB_LOGO'):'LOGO_BUTTON',
}

def make_supported_raw_gamepad_spi_packet(gp_status_dict, this_device_info):
    gp_id = this_device_info['id']
    this_gp_status = gp_status_dict[gp_id]
    print(this_gp_status)
    generic_button_dict = {
        'FACE_BUTTON_SOUTH':0,
        'FACE_BUTTON_EAST':0,
        'FACE_BUTTON_WEST':0,
        'FACE_BUTTON_NORTH':0,
        'SHOULDER_BUTTON_LEFT':0,
        'SHOULDER_BUTTON_RIGHT':0,
        'BUMPER_BUTTON_LEFT':0,
        'BUMPER_BUTTON_RIGHT':0,
        'STICK_BUTTON_LEFT':0,
        'STICK_BUTTON_RIGHT':0,
        'LOGO_BUTTON':0,
        'MISC_BUTTON_1':0,
        'MISC_BUTTON_2':0,
        'MISC_BUTTON_3':0,
        'MISC_BUTTON_4':0,
        'MISC_BUTTON_5':0,
    }
    if this_device_info['gamepad_type'] == 'Xbox':
        for item in this_gp_status:
            generic_name = xbox_to_generic_dict.get(item)
            if generic_name is not None:
                generic_button_dict[generic_name] = this_gp_status[item]
        print(generic_button_dict)
    return list(nop_spi_msg_template), None, None

def make_gamepad_spi_packet(gp_status_dict, this_device_info):
    current_protocol = usb4vc_ui.get_gamepad_protocol()
    try:
        if current_protocol['pid'] in [PID_GENERIC_GAMEPORT_GAMEPAD, PID_PROTOCOL_OFF]:
            return make_15pin_gamepad_spi_packet(gp_status_dict, this_device_info, current_protocol)
        elif current_protocol['pid'] == PID_RAW_USB_GAMEPAD:
            if this_device_info['gamepad_type'] == 'Generic': 
                return make_unsupported_raw_gamepad_spi_packet(gp_status_dict, this_device_info)
            else:
                return make_supported_raw_gamepad_spi_packet(gp_status_dict, this_device_info)
    except Exception as e:
        print("exception make_15pin_gamepad_spi_packet:", e)
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

def get_stick_axes(this_device):
    if 'DualSense' in this_device['gamepad_type']:
        return ["ABS_X", "ABS_Y", "ABS_Z", "ABS_RZ"]
    return ["ABS_X", "ABS_Y", "ABS_RX", "ABS_RY"]

gamepad_status_dict = {}
gamepad_hold_check_interval = 0.02
def raw_input_event_worker():
    last_usb_event = 0
    mouse_status_dict = {'x': [0, 0], 'y': [0, 0], 'scroll': 0, 'hscroll': 0, BTN_LEFT:0, BTN_RIGHT:0, BTN_MIDDLE:0, BTN_SIDE:0, BTN_EXTRA:0, BTN_FORWARD:0, BTN_BACK:0, BTN_TASK:0}
    next_gamepad_hold_check = time.time() + gamepad_hold_check_interval
    last_mouse_msg = []
    last_gamepad_msg = None
    in_deadzone_list = []
    last_mouse_button_msg = None
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
                        last_mouse_button_msg = make_mouse_spi_packet(mouse_status_dict, this_id)
                        pcard_spi.xfer(list(last_mouse_button_msg))
                # Gamepad buttons
                elif BTN_SOUTH <= event_code <= BTN_THUMBR or event_code in gamepad_buttons_as_kb_codes:
                    this_btn_status = data[4]
                    if this_btn_status != 0:
                        this_btn_status = 1
                    gamepad_status_dict[this_id][event_code] = this_btn_status

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
                abs_value_midpoint127 = convert_to_8bit_midpoint127(abs_value, this_device['axes_info'], event_code)
                gamepad_status_dict[this_id][event_code] = abs_value_midpoint127

            # SYNC event
            elif data[0] == EV_SYN and event_code == SYN_REPORT:
                if this_device['is_mouse']:
                    this_mouse_msg = make_mouse_spi_packet(mouse_status_dict, this_id)
                    if this_mouse_msg == last_mouse_button_msg:
                        pass
                    # send spi mouse message if there is moment, or the button is not typematic
                    elif (max(this_mouse_msg[13:18]) != 2 or sum(this_mouse_msg[4:10]) != 0) and (this_mouse_msg[4:] != last_mouse_msg[4:] or sum(this_mouse_msg[4:]) != 0):
                        pcard_spi.xfer(list(this_mouse_msg))
                        next_gamepad_hold_check = now + gamepad_hold_check_interval
                        clear_mouse_movement(mouse_status_dict)
                        last_mouse_msg = list(this_mouse_msg)
                if this_device['is_gp']:
                    for stick_axes_name in get_stick_axes(this_device):
                        axes_code = usb4vc_shared.code_name_to_value_lookup.get(stick_axes_name)[0]
                        this_gp_dict = gamepad_status_dict[this_device['id']]
                        if axes_code in this_gp_dict and 127 - 12 <= this_gp_dict[axes_code] <= 127 + 12:
                            this_gp_dict[axes_code] = 127
                    gamepad_output = make_gamepad_spi_packet(gamepad_status_dict, this_device)
                    # print(gamepad_output[0])
                    if gamepad_output != last_gamepad_msg:
                        gp_to_transfer, kb_to_transfer, mouse_to_transfer = gamepad_output
                        pcard_spi.xfer(list(gp_to_transfer))
                        if kb_to_transfer is not None:
                            time.sleep(0.001)
                            pcard_spi.xfer(list(kb_to_transfer))
                        if mouse_to_transfer is not None:
                            time.sleep(0.001)
                            pcard_spi.xfer(list(mouse_to_transfer))
                            next_gamepad_hold_check = now + gamepad_hold_check_interval
                        last_gamepad_msg = gamepad_output
            
        # ----------------- PBOARD INTERRUPT -----------------
        if GPIO.event_detected(SLAVE_REQ_PIN):
            # send out ACK to turn off P-Card interrupt
            slave_result = pcard_spi.xfer(make_spi_msg_ack())
            time.sleep(0.001)
            # send another to shift response into RPi
            slave_result = pcard_spi.xfer(make_spi_msg_ack())
            print(int(time.time()), slave_result)
            if slave_result[SPI_BUF_INDEX_MAGIC] == SPI_MISO_MAGIC and slave_result[SPI_BUF_INDEX_MSG_TYPE] == SPI_MISO_MSG_TYPE_KB_LED_REQUEST:
                try:
                    change_kb_led(slave_result[3], slave_result[4], slave_result[5])
                    change_kb_led(slave_result[3], slave_result[4], slave_result[5])
                except Exception as e:
                    print('exception change_kb_led:', e)

def get_input_devices():
    result = []
    available_devices = [evdev.InputDevice(path) for path in evdev.list_devices()]
    for this_device in available_devices:
        if 'motion' in this_device.name.lower():
            continue
        dev_dict = {
            'path':this_device.path,
            'name':this_device.name,
            'vendor_id':this_device.info.vendor,
            'product_id':this_device.info.product,
            'axes_info':{},
            'gamepad_type':'Generic',
            'is_kb':False,
            'is_mouse':False,
            'is_gp':False,
        }
        cap_str = str(this_device.capabilities(verbose=True))
        if 'BTN_LEFT' in cap_str and "EV_REL" in cap_str:
            dev_dict['is_mouse'] = True
        if 'KEY_ENTER' in cap_str and "KEY_Y" in cap_str:
            dev_dict['is_kb'] = True
        if 'KEY_MODE' in cap_str:
            dev_dict['is_gp'] = True
        if 'EV_ABS' in cap_str and "BTN_SOUTH" in cap_str:
            dev_dict['is_gp'] = True
            try:
                for item in this_device.capabilities()[EV_ABS]:
                    dev_dict['axes_info'][item[0]] = item[1]._asdict()
            except Exception as e:
                print('exception get_input_devices:', e)
            dev_dict['gamepad_type'] = usb4vc_gamepads.gamepad_type_lookup(this_device.info.vendor, this_device.info.product)
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
            print('exception get_input_devices:', e)
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
                print("opened device:", hex(item['vendor_id']), hex(item['product_id']), item['name'])
            except Exception as e:
                print("exception Device open:", e, item)

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

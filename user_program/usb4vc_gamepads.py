GAMEPAD_TYPE_XBOX = 'Xbox'
GAMEPAD_TYPE_PS4_GEN1 = "DualShock 4 [CUH-ZCT1x]"
GAMEPAD_TYPE_PS4_GEN2 = "DualShock 4 [CUH-ZCT2x]"
GAMEPAD_TYPE_PS5_GEN1 = "DualSense wireless controller"
GAMEPAD_TYPE_SWITCH_PRO = "Switch Pro Controller"
GAMEPAD_TYPE_UNKNOWN = "Generic"

def gamepad_type_lookup(vendor_id, product_id):
    # https://the-sz.com/products/usbid/index.php?v=0x54c&p=&n=
    if vendor_id == 0x054c and product_id == 0x09cc:
        return GAMEPAD_TYPE_PS4_GEN2
    if vendor_id == 0x054c and product_id == 0x05c4:
        return GAMEPAD_TYPE_PS4_GEN1
    if vendor_id == 0x054c and product_id == 0x0ce6:
        return GAMEPAD_TYPE_PS5_GEN1
    if vendor_id == 0x057e and product_id == 0x2009:
        return GAMEPAD_TYPE_SWITCH_PRO
    if vendor_id == 0x045e:
        return GAMEPAD_TYPE_XBOX
    return GAMEPAD_TYPE_UNKNOWN

ABS_Z = 0x02
ABS_RX = 0x03
ABS_RY = 0x04
ABS_RZ = 0x05
xbox_one_analog_trigger_codes = {ABS_Z, ABS_RZ}
ps4_analog_trigger_codes = {ABS_Z, ABS_RZ}
ps5_analog_trigger_codes = {ABS_RX, ABS_RY}

def is_analog_trigger(ev_code, gamepad_type):
    if "Xbox" in gamepad_type and ev_code in xbox_one_analog_trigger_codes:
        return True
    if 'DualSense' in gamepad_type and ev_code in ps5_analog_trigger_codes:
        return True
    if 'DualShock' in gamepad_type and ev_code in ps4_analog_trigger_codes:
        return True
    return False

raw_usb_gamepad_abs_axes_to_spi_msg_index_lookup = {
    0x00:10, #ABS_X
    0x01:11, #ABS_Y
    0x02:12, #ABS_Z
    0x03:13, #ABS_RX
    0x04:14, #ABS_RY
    0x05:15, #ABS_RZ
    0x06:None, #ABS_THROTTLE
    0x07:None, #ABS_RUDDER
    0x08:None, #ABS_WHEEL
    0x09:None, #ABS_GAS
    0x0a:None, #ABS_BRAKE
    0x10:16, #ABS_HAT0X
    0x11:17, #ABS_HAT0Y
    0x12:None, #ABS_HAT1X
    0x13:None, #ABS_HAT1Y
    0x14:None, #ABS_HAT2X
    0x15:None, #ABS_HAT2Y
    0x16:None, #ABS_HAT3X
    0x17:None, #ABS_HAT3Y
    0x18:None, #ABS_PRESSURE
    0x19:None, #ABS_DISTANCE
    0x1a:None, #ABS_TILT_X
    0x1b:None, #ABS_TILT_Y
    0x1c:None, #ABS_TOOL_WIDTH
}

xbox_one_to_linux_ev_code_dict = {
    "XB_A":"BTN_SOUTH",
    "XB_B":"BTN_EAST",
    "XB_X":"BTN_NORTH",
    "XB_Y":"BTN_WEST",
    "XB_LSB":"BTN_THUMBL",
    "XB_RSB":"BTN_THUMBR",
    "XB_LB":"BTN_TL",
    "XB_RB":"BTN_TR",
    "XB_VIEW":"BTN_SELECT",
    "XB_MENU":"BTN_START",
    "XB_LOGO":"BTN_MODE",
    "XB_LSX":"ABS_X",
    "XB_LSY":"ABS_Y",
    "XB_RSX":"ABS_RX",
    "XB_RSY":"ABS_RY",
    "XB_LT":"ABS_Z",
    "XB_RT":"ABS_RZ",
    "XB_DPX":"ABS_HAT0X",
    "XB_DPY":"ABS_HAT0Y",
}

ps5_to_linux_ev_code_dict = {
    "PS_CROSS":"BTN_EAST",
    "PS_CIRCLE":"BTN_C",
    "PS_SQUARE":"BTN_SOUTH",
    "PS_TRIANGLE":"BTN_NORTH",
    "PS_L1":"BTN_WEST",
    "PS_R1":"BTN_Z",
    "PS_CREATE":"BTN_TL2",
    "PS_OPTION":"BTN_TR2",
    "PS_LOGO":"BTN_MODE",
    "PS_L2_BUTTON":"BTN_TL",
    "PS_R2_BUTTON":"BTN_TR",
    "PS_MUTE":"BTN_THUMBR",
    "PS_TOUCHPAD_BUTTON":"BTN_THUMBL",
    "PS_LSB":"BTN_SELECT",
    "PS_RSB":"BTN_START",
    "PS_LSX":"ABS_X",
    "PS_LSY":"ABS_Y",
    "PS_RSX":"ABS_Z",
    "PS_RSY":"ABS_RZ",
    "PS_L2_ANALOG":"ABS_RX",
    "PS_R2_ANALOG":"ABS_RY",
    "PS_DPX":"ABS_HAT0X",
    "PS_DPY":"ABS_HAT0Y",
}

ps4_to_linux_ev_code_dict = {
    "PS_CROSS":"BTN_SOUTH",
    "PS_CIRCLE":"BTN_EAST",
    "PS_SQUARE":"BTN_WEST",
    "PS_TRIANGLE":"BTN_NORTH",
    "PS_L1":"BTN_TL",
    "PS_R1":"BTN_TR",
    "PS_SHARE":"BTN_SELECT",
    "PS_OPTION":"BTN_START",
    "PS_LOGO":"BTN_MODE",
    "PS_L2_BUTTON":"BTN_TL2",
    "PS_R2_BUTTON":"BTN_TR2",
    "PS_LSB":"BTN_THUMBL",
    "PS_RSB":"BTN_THUMBR",
    "PS_LSX":"ABS_X",
    "PS_LSY":"ABS_Y",
    "PS_RSX":"ABS_RX",
    "PS_RSY":"ABS_RY",
    "PS_L2_ANALOG":"ABS_Z",
    "PS_R2_ANALOG":"ABS_RZ",
    "PS_DPX":"ABS_HAT0X",
    "PS_DPY":"ABS_HAT0Y",
}

XBOX_DEFAULT_KB_MOUSE_MAPPING = {
    'XB_A': {'code': 'BTN_LEFT'},
    'XB_B': {'code': 'BTN_RIGHT'},
    'XB_LSX': {'code': 'REL_X'},
    'XB_LSY': {'code': 'REL_Y'},
    'XB_LB': {'code': 'BTN_LEFT'},
    'XB_RB': {'code': 'BTN_RIGHT'},
    'XB_RSX': {'code': 'KEY_RIGHT', 'code_neg': 'KEY_LEFT'},
    'XB_RSY': {'code': 'KEY_DOWN', 'code_neg': 'KEY_UP'},
    'XB_DPX': {'code': 'KEY_RIGHT', 'code_neg': 'KEY_LEFT'},
    'XB_DPY': {'code': 'KEY_DOWN', 'code_neg': 'KEY_UP'},
    'XB_X': {'code': 'BTN_LEFT'},
    'XB_Y': {'code': 'BTN_RIGHT'},
    'XB_MENU': {'code': 'KEY_ESC'},
    'XB_VIEW': {'code': 'KEY_ESC'}
}

PS5_DEFAULT_KB_MOUSE_MAPPING = {
    'PS_CROSS': {'code': 'BTN_LEFT'},
    'PS_SQUARE': {'code': 'BTN_LEFT'},
    'PS_L1': {'code': 'BTN_LEFT'},
    'PS_L2_BUTTON': {'code': 'BTN_LEFT'},

    'PS_CIRCLE': {'code': 'BTN_RIGHT'},
    'PS_TRIANGLE': {'code': 'BTN_RIGHT'},
    'PS_R1': {'code': 'BTN_RIGHT'},
    'PS_R2_BUTTON': {'code': 'BTN_RIGHT'},

    'PS_LSX': {'code': 'REL_X'},
    'PS_LSY': {'code': 'REL_Y'},
    
    'PS_RSX': {'code': 'KEY_RIGHT', 'code_neg': 'KEY_LEFT'},
    'PS_RSY': {'code': 'KEY_DOWN', 'code_neg': 'KEY_UP'},
    'PS_DPX': {'code': 'KEY_RIGHT', 'code_neg': 'KEY_LEFT'},
    'PS_DPY': {'code': 'KEY_DOWN', 'code_neg': 'KEY_UP'},
    
    'PS_CREATE': {'code': 'KEY_ESC'},
    'PS_OPTION': {'code': 'KEY_ESC'}
}

PS4_DEFAULT_KB_MOUSE_MAPPING = {
    'PS_CROSS': {'code': 'BTN_LEFT'},
    'PS_SQUARE': {'code': 'BTN_LEFT'},
    'PS_L1': {'code': 'BTN_LEFT'},
    'PS_L2_BUTTON': {'code': 'BTN_LEFT'},

    'PS_CIRCLE': {'code': 'BTN_RIGHT'},
    'PS_TRIANGLE': {'code': 'BTN_RIGHT'},
    'PS_R1': {'code': 'BTN_RIGHT'},
    'PS_R2_BUTTON': {'code': 'BTN_RIGHT'},

    'PS_LSX': {'code': 'REL_X'},
    'PS_LSY': {'code': 'REL_Y'},
    
    'PS_RSX': {'code': 'KEY_RIGHT', 'code_neg': 'KEY_LEFT'},
    'PS_RSY': {'code': 'KEY_DOWN', 'code_neg': 'KEY_UP'},
    'PS_DPX': {'code': 'KEY_RIGHT', 'code_neg': 'KEY_LEFT'},
    'PS_DPY': {'code': 'KEY_DOWN', 'code_neg': 'KEY_UP'},
    
    'PS_SHARE': {'code': 'KEY_ESC'},
    'PS_OPTION': {'code': 'KEY_ESC'}
}

XBOX_DEFAULT_MAPPING = {
    # buttons to buttons
    'BTN_A': {'code':'IBM_GGP_BTN_1'},
    'BTN_X': {'code':'IBM_GGP_BTN_2'},
    'BTN_B': {'code':'IBM_GGP_BTN_3'},
    'BTN_Y': {'code':'IBM_GGP_BTN_4'},
    
    'BTN_TL': {'code':'IBM_GGP_BTN_3'},
    'BTN_TR': {'code':'IBM_GGP_BTN_4'},
    # buttons to keyboard keys
    'BTN_SELECT': {'code':'KEY_ESC'},
    'BTN_START': {'code':'KEY_ENTER'},
    # analog axes to analog axes
    'ABS_X': {'code':'IBM_GGP_JS1_X'},
    'ABS_Y': {'code':'IBM_GGP_JS1_Y'},
    'ABS_HAT0X': {'code':'IBM_GGP_JS1_X'},
    'ABS_HAT0Y': {'code':'IBM_GGP_JS1_Y'},
    'ABS_RX': {'code':'IBM_GGP_JS2_X'},
    'ABS_RY': {'code':'IBM_GGP_JS2_Y'},
    'ABS_Z':{'code':'IBM_GGP_JS2_YP'},
    'ABS_RZ':{'code':'IBM_GGP_JS2_YN'},
}

PS5_DEFAULT_MAPPING = {
    # buttons to buttons
    'BTN_EAST': {'code':'IBM_GGP_BTN_1'},
    'BTN_SOUTH': {'code':'IBM_GGP_BTN_2'},
    'BTN_C': {'code':'IBM_GGP_BTN_3'},
    'BTN_NORTH': {'code':'IBM_GGP_BTN_4'},
    
    'BTN_WEST': {'code':'IBM_GGP_BTN_3'},
    'BTN_Z': {'code':'IBM_GGP_BTN_4'},

    # 'BTN_TL': {'code':'IBM_GGP_BTN_3'},
    # 'BTN_TR': {'code':'IBM_GGP_BTN_4'},

    # analog axes to analog axes
    'ABS_X': {'code':'IBM_GGP_JS1_X'},
    'ABS_Y': {'code':'IBM_GGP_JS1_Y'},
    'ABS_HAT0X': {'code':'IBM_GGP_JS1_X'},
    'ABS_HAT0Y': {'code':'IBM_GGP_JS1_Y'},
    'ABS_Z': {'code':'IBM_GGP_JS2_X'},
    'ABS_RZ': {'code':'IBM_GGP_JS2_Y'},

    'ABS_RX':{'code':'IBM_GGP_JS2_YP'},
    'ABS_RY':{'code':'IBM_GGP_JS2_YN'},
}

PS4_DEFAULT_MAPPING = {
    # buttons to buttons
    'PS_CROSS': {'code':'IBM_GGP_BTN_1'},
    'PS_SQUARE': {'code':'IBM_GGP_BTN_2'},
    'PS_CIRCLE': {'code':'IBM_GGP_BTN_3'},
    'PS_TRIANGLE': {'code':'IBM_GGP_BTN_4'},
    
    'PS_L1': {'code':'IBM_GGP_BTN_3'},
    'PS_R1': {'code':'IBM_GGP_BTN_4'},

    # 'PS_L2_BUTTON': {'code':'IBM_GGP_BTN_3'},
    # 'PS_R2_BUTTON': {'code':'IBM_GGP_BTN_4'},

    # analog axes to analog axes
    'PS_LSX': {'code':'IBM_GGP_JS1_X'},
    'PS_LSY': {'code':'IBM_GGP_JS1_Y'},
    'PS_DPX': {'code':'IBM_GGP_JS1_X'},
    'PS_DPY': {'code':'IBM_GGP_JS1_Y'},
    'PS_RSX': {'code':'IBM_GGP_JS2_X'},
    'PS_RSY': {'code':'IBM_GGP_JS2_Y'},
    'PS_L2_ANALOG':{'code':'IBM_GGP_JS2_YP'},
    'PS_R2_ANALOG':{'code':'IBM_GGP_JS2_YN'},
}

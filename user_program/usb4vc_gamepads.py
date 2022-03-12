def gamepad_type_lookup(vendor_id, product_id):
    # https://the-sz.com/products/usbid/index.php?v=0x54c&p=&n=
    if vendor_id == 0x054c and product_id == 0x09cc:
        return "DualShock 4 [CUH-ZCT2x]"
    if vendor_id == 0x054c and product_id == 0x05c4:
        return "DualShock 4 [CUH-ZCT1x]"
    if vendor_id == 0x054c and product_id == 0x0ce6:
        return "DualSense wireless controller"
    if vendor_id == 0x057e and product_id == 0x2009:
        return "Switch Pro Controller"
    if vendor_id == 0x045e:
        return "Xbox"
    return "Generic"

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

ps5_to_linux_ev_code_dict = {
    "PS5_CROSS":"BTN_EAST",
    "PS5_CIRCLE":"BTN_C",
    "PS5_SQUARE":"BTN_SOUTH",
    "PS5_TRIANGLE":"BTN_NORTH",
    "PS5_L1":"BTN_WEST",
    "PS5_R1":"BTN_Z",
    "PS5_CREATE":"BTN_TL2",
    "PS5_OPTION":"BTN_TR2",
    "PS5_LOGO":"BTN_MODE",
    "PS5_L2_BUTTON":"BTN_TL",
    "PS5_R2_BUTTON":"BTN_TR",
    "PS5_MUTE":"BTN_THUMBR",
    "PS5_TOUCHPAD_BUTTON":"BTN_THUMBL",
    "PS5_LSB":"BTN_SELECT",
    "PS5_RSB":"BTN_START",
    "PS5_LSX":"ABS_X",
    "PS5_LSY":"ABS_Y",
    "PS5_RSX":"ABS_Z",
    "PS5_RSY":"ABS_RZ",
    "PS5_L2_ANALOG":"ABS_RX",
    "PS5_R2_ANALOG":"ABS_RY",
    "PS5_DPX":"ABS_HAT0X",
    "PS5_DPY":"ABS_HAT0Y",
}

ps4_to_linux_ev_code_dict = {
    "PS4_CROSS":"BTN_SOUTH",
    "PS4_CIRCLE":"BTN_EAST",
    "PS4_SQUARE":"BTN_WEST",
    "PS4_TRIANGLE":"BTN_NORTH",
    "PS4_L1":"BTN_TL",
    "PS4_R1":"BTN_TR",
    "PS4_SHARE":"BTN_SELECT",
    "PS4_OPTION":"BTN_START",
    "PS4_LOGO":"BTN_MODE",
    "PS4_L2_BUTTON":"BTN_TL2",
    "PS4_R2_BUTTON":"BTN_TR2",
    "PS4_LSB":"BTN_THUMBL",
    "PS4_RSB":"BTN_THUMBR",
    "PS4_LSX":"ABS_X",
    "PS4_LSY":"ABS_Y",
    "PS4_RSX":"ABS_RX",
    "PS4_RSY":"ABS_RY",
    "PS4_L2_ANALOG":"ABS_Z",
    "PS4_R2_ANALOG":"ABS_RZ",
    "PS4_DPX":"ABS_HAT0X",
    "PS4_DPY":"ABS_HAT0Y",
}

XBOX_DEFAULT_KB_MOUSE_MAPPING = {
    'XB1_A': {'code': 'BTN_LEFT'},
    'XB1_B': {'code': 'BTN_RIGHT'},
    'XB1_LSX': {'code': 'REL_X'},
    'XB1_LSY': {'code': 'REL_Y'},
    'XB1_LB': {'code': 'BTN_LEFT'},
    'XB1_RB': {'code': 'BTN_RIGHT'},
    'XB1_RSX': {'code': 'KEY_RIGHT', 'code_neg': 'KEY_LEFT'},
    'XB1_RSY': {'code': 'KEY_DOWN', 'code_neg': 'KEY_UP'},
    'XB1_DPX': {'code': 'KEY_RIGHT', 'code_neg': 'KEY_LEFT'},
    'XB1_DPY': {'code': 'KEY_DOWN', 'code_neg': 'KEY_UP'},
    'XB1_X': {'code': 'BTN_LEFT'},
    'XB1_Y': {'code': 'BTN_RIGHT'},
    'XB1_MENU': {'code': 'KEY_ESC'},
    'XB1_VIEW': {'code': 'KEY_ESC'}
}

PS5_DEFAULT_KB_MOUSE_MAPPING = {
    'PS5_CROSS': {'code': 'BTN_LEFT'},
    'PS5_SQUARE': {'code': 'BTN_LEFT'},
    'PS5_L1': {'code': 'BTN_LEFT'},
    'PS5_L2_BUTTON': {'code': 'BTN_LEFT'},

    'PS5_CIRCLE': {'code': 'BTN_RIGHT'},
    'PS5_TRIANGLE': {'code': 'BTN_RIGHT'},
    'PS5_R1': {'code': 'BTN_RIGHT'},
    'PS5_R2_BUTTON': {'code': 'BTN_RIGHT'},

    'PS5_LSX': {'code': 'REL_X'},
    'PS5_LSY': {'code': 'REL_Y'},
    
    'PS5_RSX': {'code': 'KEY_RIGHT', 'code_neg': 'KEY_LEFT'},
    'PS5_RSY': {'code': 'KEY_DOWN', 'code_neg': 'KEY_UP'},
    'PS5_DPX': {'code': 'KEY_RIGHT', 'code_neg': 'KEY_LEFT'},
    'PS5_DPY': {'code': 'KEY_DOWN', 'code_neg': 'KEY_UP'},
    
    'PS5_CREATE': {'code': 'KEY_ESC'},
    'PS5_OPTION': {'code': 'KEY_ESC'}
}

PS4_DEFAULT_KB_MOUSE_MAPPING = {
    'PS4_CROSS': {'code': 'BTN_LEFT'},
    'PS4_SQUARE': {'code': 'BTN_LEFT'},
    'PS4_L1': {'code': 'BTN_LEFT'},
    'PS4_L2_BUTTON': {'code': 'BTN_LEFT'},

    'PS4_CIRCLE': {'code': 'BTN_RIGHT'},
    'PS4_TRIANGLE': {'code': 'BTN_RIGHT'},
    'PS4_R1': {'code': 'BTN_RIGHT'},
    'PS4_R2_BUTTON': {'code': 'BTN_RIGHT'},

    'PS4_LSX': {'code': 'REL_X'},
    'PS4_LSY': {'code': 'REL_Y'},
    
    'PS4_RSX': {'code': 'KEY_RIGHT', 'code_neg': 'KEY_LEFT'},
    'PS4_RSY': {'code': 'KEY_DOWN', 'code_neg': 'KEY_UP'},
    'PS4_DPX': {'code': 'KEY_RIGHT', 'code_neg': 'KEY_LEFT'},
    'PS4_DPY': {'code': 'KEY_DOWN', 'code_neg': 'KEY_UP'},
    
    'PS4_SHARE': {'code': 'KEY_ESC'},
    'PS4_OPTION': {'code': 'KEY_ESC'}
}

XBOX_DEFAULT_MAPPING = {
    # buttons to buttons
    'BTN_A': {'code':'IBM_GGP_BTN_1'},
    'BTN_X': {'code':'IBM_GGP_BTN_2'},
    'BTN_B': {'code':'IBM_GGP_BTN_3'},
    'BTN_Y': {'code':'IBM_GGP_BTN_4'},
    
    'BTN_TL': {'code':'IBM_GGP_BTN_1'},
    'BTN_TR': {'code':'IBM_GGP_BTN_2'},
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
    
    'BTN_WEST': {'code':'IBM_GGP_BTN_1'},
    'BTN_Z': {'code':'IBM_GGP_BTN_2'},

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
    'PS4_CROSS': {'code':'IBM_GGP_BTN_1'},
    'PS4_SQUARE': {'code':'IBM_GGP_BTN_2'},
    'PS4_CIRCLE': {'code':'IBM_GGP_BTN_3'},
    'PS4_TRIANGLE': {'code':'IBM_GGP_BTN_4'},
    
    'PS4_L1': {'code':'IBM_GGP_BTN_1'},
    'PS4_R1': {'code':'IBM_GGP_BTN_2'},

    # 'PS4_L2_BUTTON': {'code':'IBM_GGP_BTN_3'},
    # 'PS4_R2_BUTTON': {'code':'IBM_GGP_BTN_4'},

    # analog axes to analog axes
    'PS4_LSX': {'code':'IBM_GGP_JS1_X'},
    'PS4_LSY': {'code':'IBM_GGP_JS1_Y'},
    'PS4_DPX': {'code':'IBM_GGP_JS1_X'},
    'PS4_DPY': {'code':'IBM_GGP_JS1_Y'},
    'PS4_RSX': {'code':'IBM_GGP_JS2_X'},
    'PS4_RSY': {'code':'IBM_GGP_JS2_Y'},
    'PS4_L2_ANALOG':{'code':'IBM_GGP_JS2_YP'},
    'PS4_R2_ANALOG':{'code':'IBM_GGP_JS2_YN'},
}

import time
import os
import usb4vc_oled
from luma.core.render import canvas
import usb4vc_usb_scan
import evdev
import selectors

def ev_loop(button_list):
    selector = selectors.DefaultSelector()
    opened_dict = {}
    last_value = 127
    with canvas(usb4vc_oled.oled_device) as draw:
        usb4vc_oled.oled_print_centered("To exit, HOLD", usb4vc_oled.font_medium, 0, draw)
        usb4vc_oled.oled_print_centered("+ during input", usb4vc_oled.font_medium, 15, draw)
    time.sleep(0.5)
    while 1:
        for button in button_list:
            if button.is_pressed():
                return
        for dev_path in usb4vc_usb_scan.opened_device_dict:
            if dev_path not in opened_dict:
                opened_dict[dev_path] = evdev.InputDevice(dev_path)
                selector.register(opened_dict[dev_path], selectors.EVENT_READ)
        for key, mask in selector.select():
            device = key.fileobj
            for event in device.read():
                cat = evdev.categorize(event)
                bottom_line = None
                ev_str = str(cat)
                if 'axis event' in ev_str.lower():
                    name = '???'
                    try:
                        name = ev_str.strip().split(' ')[-1]
                        if len(name) <= 2:
                            name = cat.event.code
                    except:
                        pass
                    value = '???'
                    try:
                        value = cat.event.value
                    except:
                        pass
                    try:
                        gamepad_status = usb4vc_usb_scan.gamepad_status_dict.get(usb4vc_usb_scan.opened_device_dict[device.path]['id'])
                        value = gamepad_status.get(cat.event.code)
                        if 127 - 20 < value < 127 + 20:
                            value = 127
                    except:
                        pass
                    if value == last_value:
                        continue
                    last_value = value
                    bottom_line = f'{name}, {value}'
                elif 'key event' in ev_str.lower():
                    keycode = '???'
                    try:
                        if isinstance(cat.keycode, str):
                            keycode = cat.keycode
                        elif isinstance(cat.keycode, list):
                            keycode = cat.keycode[0]
                        else:
                            keycode = cat.scancode
                    except:
                        pass
                    bottom_line = f'{keycode} {cat.keystate}'
                if bottom_line is None:
                    continue

                top_line = str(device.name)
                middle_line = 'Unknown PID and VID'
                try:
                    dev_dict = usb4vc_usb_scan.opened_device_dict.get(device.path)
                    middle_line = f"VID:{hex(dev_dict['vendor_id'])} PID:{hex(dev_dict['product_id'])}"
                except Exception:
                    pass
                with canvas(usb4vc_oled.oled_device) as draw:
                    usb4vc_oled.oled_print_centered(top_line, usb4vc_oled.font_regular, 0, draw)
                    usb4vc_oled.oled_print_centered(middle_line, usb4vc_oled.font_regular, 10, draw)
                    usb4vc_oled.oled_print_centered(bottom_line, usb4vc_oled.font_regular, 20, draw)

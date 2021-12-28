import time
import evdev

# sh sync.sh; ssh -t pi@192.168.1.56 "pkill python3;cd ~/usb4vc;python3 evdev_test.py"
# sh sync.sh; ssh -t pi@192.168.1.56 "pkill python3;cd ~/usb4vc;python3 usb4vc_main.py"

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
    
start = time.time_ns()
get_input_devices()
print((time.time_ns() - start) / 1000000)
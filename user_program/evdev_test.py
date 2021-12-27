import time
import evdev

# sh sync.sh; ssh -t pi@192.168.1.56 "pkill python3;cd ~/usb4vc;python3 evdev_test.py"

start = time.time_ns()

keyboard_devices = []
mouse_devices = []
gamepad_devices = []

available_devices = [evdev.InputDevice(path) for path in evdev.list_devices()]
for this_device in available_devices:
    print(this_device.path, this_device.name)
    this_capability = str(this_device.capabilities(verbose=True))
    if 'BTN_LEFT' in this_capability and "EV_REL" in this_capability:
        print('this is a mouse')
        mouse_devices.append(this_device)
    elif 'KEY_ENTER' in this_capability and "KEY_Y" in this_capability:
        print('this is a keyboard')
        keyboard_devices.append(this_device)
    elif 'EV_ABS' in this_capability and "BTN_SOUTH" in this_capability:
        print('this is a gamepad')
        gamepad_devices.append(this_device)
    print("-----")

print(time.time_ns() - start)
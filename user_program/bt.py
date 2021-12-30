import time
import os
import sys
import subprocess
from subprocess import Popen, PIPE

LINUX_EXIT_CODE_TIMEOUT = 124

def bt_setup():
    rfkill_str = subprocess.getoutput("/usr/sbin/rfkill -n")
    if 'bluetooth' not in rfkill_str:
        return 1, "no BT receiver found"
    os.system('/usr/sbin/rfkill unblock bluetooth')
    time.sleep(0.1)
    exit_code = os.system('timeout 1 bluetoothctl agent NoInputNoOutput')
    if exit_code == LINUX_EXIT_CODE_TIMEOUT:
        return 2, 'bluetoothctl stuck'
    return 0, ''

def scan_bt_devices(timeout_sec = 5):
    exit_code = os.system(f"timeout {timeout_sec} bluetoothctl --agent NoInputNoOutput scan on") >> 8
    if exit_code != LINUX_EXIT_CODE_TIMEOUT:
        return None, 'scan error'
    device_str = subprocess.getoutput("bluetoothctl --agent NoInputNoOutput devices")
    dev_list = []
    for line in device_str.replace('\r', '').split('\n'):
        if 'device' not in line.lower():
            continue
        line_split = line.split(' ', maxsplit=2)
        # skip if device has no name
        if len(line_split) < 3 or line_split[2].count('-') == 5:
            continue
        dev_list.append((line_split[1], line_split[2]))
    if len(dev_list) == 0:
        return None, 'Nothing was found'
    return dev_list, ''

def delete_all_devices():
    dev_set = set()
    device_str = subprocess.getoutput(f"bluetoothctl --agent NoInputNoOutput devices")
    device_str2 = subprocess.getoutput(f"bluetoothctl --agent NoInputNoOutput paired-devices")

    for line in (device_str + '\n' + device_str2).replace('\r', '').split('\n'):
        if 'device' not in line.lower():
            continue
        line_split = line.split(' ', maxsplit=2)
        # skip if device has no name
        if len(line_split) < 3 or line_split[2].count('-') == 5:
            continue
        dev_set.add((line_split[1], line_split[2]))

    print('vvvvv')
    print(device_str)
    print(device_str2)
    print(dev_set)
    print('^^^^^')

    for item in dev_set:
        os.system(f'bluetoothctl --agent NoInputNoOutput block {item[0]}')
        os.system(f'bluetoothctl --agent NoInputNoOutput remove {item[0]}')
        os.system(f'bluetoothctl --agent NoInputNoOutput unblock {item[0]}')
    print('done')


def pair_device(mac_addr):
    is_ready = False
    is_sent = False
    fail_phrases = ['fail', 'error', 'not available', 'excep']
    with Popen(["bluetoothctl", "--agent", "NoInputNoOutput"], stdout=PIPE, stdin=PIPE, bufsize=1,
               universal_newlines=True, shell=True) as p:
        for line in p.stdout:
            print(line, end='')
            line_lo = line.lower()
            if 'registered' in line_lo:
                is_ready = True
            if is_ready is False:
                continue
            if '#' in line_lo and is_sent == False:
                p.stdin.write(f'pair {mac_addr}\n')
                is_sent = True
            if 'successful' in line_lo:
                p.stdin.write('exit\n')
                return True, 'Success!'
            for item in fail_phrases:
                if item in line_lo:
                    p.stdin.write('exit\n')
                    return False, line
    return False, "wtf"

if len(sys.argv) > 1:
    print("deleting all devices..")
    delete_all_devices()
else:
    print(pair_device('20:20:01:01:3C:0C'))
    # [agent] PIN code: 027004


    # [agent] Enter passkey (number in 0-999999):
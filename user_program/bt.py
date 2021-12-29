import os
import subprocess

def scan_bt_devices():
	LINUX_EXIT_CODE_TIMEOUT = 124
	os.system('bluetoothctl agent on')
	result = os.system("timeout 10 bluetoothctl scan on") >> 8
	if result != LINUX_EXIT_CODE_TIMEOUT:
		print('bluetoothctl scan error')
		return None
	device_str = subprocess.getoutput("bluetoothctl devices")
	print(device_str)
	bt_dev_list = []
	for line in device_str.replace('\r', '').split('\n'):
		if 'device' not in line.lower() or 'yun' in line.lower():
			continue
		line_split = line.split(' ', maxsplit=2)
		if len(line_split) < 3 or line_split[2].count('-') == 5:
			continue
		bt_dev_list.append((line_split[1], line_split[2]))
	return bt_dev_list

def pair_bt_device(mac_addr):
	result_str = subprocess.getoutput(f"timeout 10 bluetoothctl pair {mac_addr}")
	print(result_str)

dev_list = scan_bt_devices()
pair_bt_device(dev_list[0][0])
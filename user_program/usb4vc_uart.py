import os
import sys
import time
import serial
import subprocess
import base64
import threading

# import usb4vc_shared
# import usb4vc_usb_scan
# import usb4vc_ui
# subprocess.getoutput(f"timeout 5 bluetoothctl --agent NoInputNoOutput paired-devices")

ser = None
try:
    ser = serial.Serial('/dev/serial0', 115200)
except Exception as e:
    print("SERIAL OPEN EXCEPTION:", e)

def uart_worker():
    if ser is None:
        return
    while 1:
        received = ser.readline().decode().replace('\r', '').replace('\n', '')
        print("I received:", received)
        line_split = received.split(' ', maxsplit=2)
        if len(line_split) < 2:
            continue
        magic = line_split[0]
        cmd_type = line_split[1]
        payload = line_split[-1]

        if magic != 'U4':
            continue
        if cmd_type == "QUIT":
            exit()

        # read file
        if cmd_type == 'RF':
            file_path = line_split[-1].strip()
            content = None
            try:
                with open(file_path, 'rb') as myfile:
                    content = myfile.read()
            except Exception as e:
                ser.write(f"U4 ERR {e}\n".encode('utf-8'))
                continue
            base64_bytes = base64.standard_b64encode(content)
            base64_message = base64_bytes.decode('utf-8')
            ser.write(f"U4 OK {base64_message}\n".encode('utf-8'))

        # write file
        if cmd_type == 'WF':
            wf_args = payload.split(' ', maxsplit=2)
            if len(wf_args) < 2:
                ser.write("U4 ERR missing file content\n".encode('utf-8'))
                continue
            file_path = wf_args[0].strip()
            base64_message = wf_args[1].strip()
            try:
                base64_bytes = base64.standard_b64decode(base64_message)
            except Exception as e:
                ser.write(f"U4 ERR {e}\n".encode('utf-8'))
                continue
            try:
                with open(file_path, 'wb') as myfile:
                    myfile.write(base64_bytes)
                ser.write("U4 OK\n".encode('utf-8'))
            except Exception as e:
                ser.write(f"U4 ERR {e}\n".encode('utf-8'))
                continue

        # delete file
        if cmd_type == 'DF':
            file_path = line_split[-1].strip()
            try:
                os.remove(file_path)
            except Exception as e:
                ser.write(f"U4 ERR {e}\n".encode('utf-8'))
                continue
            ser.write("U4 OK\n".encode('utf-8'))

# ui_worker = threading.Thread(target=ui_worker, daemon=True)

import os
import sys
import time
import serial
import subprocess
import base64
import threading

import usb4vc_shared
# import usb4vc_usb_scan
# import usb4vc_ui

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
        # print("I received:", received)
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

        if cmd_type == 'SC':
            try:
                shell_result = subprocess.getoutput(payload).strip()
                base64_bytes = base64.standard_b64encode(shell_result.encode('utf-8'))
                base64_message = base64_bytes.decode('utf-8')
                ser.write(f"U4 OK {base64_message}\n".encode('utf-8'))
            except Exception as e:
                ser.write(f"U4 ERR {e}\n".encode('utf-8'))
                continue

        if cmd_type == 'INFO':
            try:
                ser.write(f"U4 OK USB4VC RPi App v{usb4vc_shared.RPI_APP_VERSION_TUPLE[0]}.{usb4vc_shared.RPI_APP_VERSION_TUPLE[1]}.{usb4vc_shared.RPI_APP_VERSION_TUPLE[2]} dekuNukem 2022\n".encode('utf-8'))
            except Exception as e:
                ser.write(f"U4 ERR {e}\n".encode('utf-8'))
                continue

uart_thread = threading.Thread(target=uart_worker, daemon=True)

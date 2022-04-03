import os
import time
import json
import socket
import urllib.request
import requests
import zipfile
import shutil

from usb4vc_shared import RPI_APP_VERSION_TUPLE
from usb4vc_shared import this_app_dir_path
from usb4vc_shared import config_dir_path
from usb4vc_shared import firmware_dir_path
from usb4vc_shared import temp_dir_path
from usb4vc_shared import ensure_dir
from usb4vc_shared import i2c_bootloader_pbid
from usb4vc_shared import usb_bootloader_pbid

usb4vc_release_url = "https://api.github.com/repos/dekuNukem/usb4vc/releases/latest"

def is_internet_available():
    try:
        socket.create_connection(("www.google.com", 80), timeout=1)
        return True
    except OSError:
        pass
    return False

def versiontuple(v):
    return tuple(map(int, (v.strip('v').split("."))))

def get_remote_tag_version():
    try:
        if is_internet_available() is False:
            return 1, 'Internet Unavailable'
        result_dict = json.loads(urllib.request.urlopen(usb4vc_release_url).read())
        return 0, versiontuple(result_dict['tag_name'])
    except Exception as e:
        return 2, f'exception: {e}'
    return 3, 'Unknown'

"""
0 success
>0 fail
"""
def download_latest_usb4vc_release(save_path):
    try:
        if is_internet_available() is False:
            return 1, 'Internet Unavailable'
        result_dict = json.loads(urllib.request.urlopen(usb4vc_release_url).read())
        header = {'User-Agent': 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_13_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/68.0.3440.106 Safari/537.36',}
        for item in result_dict['assets']:
            if item['name'].lower().startswith('usb4vc_src') and item['name'].lower().endswith('.zip'):
                zip_path = os.path.join(save_path, item['name'])
                with open(zip_path, 'wb') as out_file:
                    content = requests.get(item['browser_download_url'], headers=header, timeout=5).content
                    out_file.write(content)
                return 0, zip_path
        return 2, 'No Update Found'
    except Exception as e:
        return 3, f'exception: {e}'
    return 4, 'Unknown'

def unzip_file(zip_path, extract_path):
    try:
        with zipfile.ZipFile(zip_path, 'r') as zip_ref:
            zip_ref.extractall(extract_path)
    except Exception as e:
        return 5, str(e) 
    return 0, 'Success!'

def get_usb4vc_update(temp_path):
    os.system(f'rm -rfv {os.path.join(temp_path, "*")}')
    time.sleep(0.1)
    rcode, msg = download_latest_usb4vc_release(temp_path)
    if rcode != 0:
        return rcode, msg
    rcode, msg = unzip_file(msg, temp_path)
    return rcode, msg

def update(temp_path):
    rcode, item = get_remote_tag_version()
    if rcode != 0:
        return 1, "Unknown error"
    if item < RPI_APP_VERSION_TUPLE:
        return 2, 'Local code is newer'
    rcode, item = get_usb4vc_update(temp_path)
    if rcode != 0:
        return 3, 'Download failed'
    try:
        src_code_path = os.path.join(temp_path, 'rpi_app')
        if len(os.listdir(src_code_path)) <= 5:
            return 4, 'Too few files'
    except Exception as e:
        return 5, f'Unknown error: {e}'
    os.system(f'rm -rfv {os.path.join(this_app_dir_path, "*")}')
    os.system(f'cp -fv {os.path.join(src_code_path, "*")} {this_app_dir_path}')
    os.system(f'chown pi {os.path.join(this_app_dir_path, "*")}') # change owner from root back to pi for easy editing
    return 0, 'Success'

firmware_url = 'https://api.github.com/repos/dekuNukem/USB4VC/contents/firmware/releases?ref=master'

# version number most recent to least recent
# dont forget to check extension
def get_firmware_list(pcard_id):
    try:
        file_list = json.loads(urllib.request.urlopen(firmware_url).read())
        fw_list = [x['name'] for x in file_list if 'name' in x and 'type' in x and x['type'] == 'file']
        fw_list = [d for d in fw_list if d.startswith('PBFW') and d.lower() and f"PBID{pcard_id}" in d]
        fw_list.sort(key=lambda s: list(map(int, s.lower().split('_v')[1].split('.')[0].replace('_', '.').split('.'))), reverse=True)
    except Exception as e:
        print('get_firmware_list:', e)
        return []
    return fw_list

def download_latest_firmware(pcard_id):
    fw_list = get_firmware_list(pcard_id)
    if pcard_id in usb_bootloader_pbid:
        fw_list = [x for x in fw_list if x.lower().endswith('.dfu')]
    else:
        fw_list = [x for x in fw_list if x.lower().endswith('.hex')]
    if len(fw_list) == 0:
        return 1
    fw_filename = fw_list[0]
    fw_download_url = f"https://github.com/dekuNukem/USB4VC/raw/master/firmware/releases/{fw_filename}"
    ensure_dir(firmware_dir_path)
    os.system(f'rm -rfv {os.path.join(firmware_dir_path, "*")}')
    print("downloading", fw_download_url)
    fw_download_path = os.path.join(firmware_dir_path, fw_filename)
    try:
        with open(fw_download_path, 'wb') as out_file:
            content = requests.get(fw_download_url, timeout=5).content
            out_file.write(content)
    except Exception as e:
        print('exception download_latest_firmware:', e)
        return 2
    return 0

# print(get_remote_tag_version()[1], RPI_APP_VERSION_TUPLE)
# print(get_remote_tag_version()[1] > RPI_APP_VERSION_TUPLE)
# print(download_latest_firmware(1))
# print(update(temp_dir_path))
# print(get_usb4vc_update(temp_dir_path))
# print(get_remote_tag_version() >= RPI_APP_VERSION_TUPLE)
# print('@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@')
# print("ssssssssss", os.path.join(temp_dir_path, '*'))
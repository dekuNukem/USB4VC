import os
import sys
import time
import math
import spidev
import evdev
import threading
import RPi.GPIO as GPIO
import usb4vc_ui
from usb4vc_shared import *
import usb4vc_gamepads
import asyncio

hid_device_info_dict = {}

POLL_INTERVAL_SEC = 1  # how often we rescan for devices

def hid_info_dict_add(dev_path):
    if dev_path in hid_device_info_dict:
        return
    this_device = evdev.InputDevice(dev_path)
    info_dict = {
            'path':this_device.path,
            'name':this_device.name,
            'vendor_id':this_device.info.vendor,
            'product_id':this_device.info.product,
            'axes_info':{},
            'gamepad_type':'Generic',
            'is_kb':False,
            'is_mouse':False,
            'is_gp':False,
        }
    
    this_cap = this_device.capabilities(verbose=True)
    cap_str = str(this_cap)
    if 'BTN_LEFT' in cap_str and "EV_REL" in cap_str:
        info_dict['is_mouse'] = True
    if 'KEY_ENTER' in cap_str and "KEY_Y" in cap_str:
        info_dict['is_kb'] = True

    print(this_cap)

    hid_device_info_dict[dev_path] = info_dict

def hid_info_dict_remove(dev_path):
    hid_device_info_dict.pop(dev_path, None)

async def read_device_events(path: str):
    dev = None
    try:
        dev = evdev.InputDevice(path)
        print(f"[attach] {path} name='{dev.name}'")
        hid_info_dict_add(path)

        # Async loop over input events
        async for event in dev.async_read_loop():
            print(f"!!!!!!!!!!!!!!! {path}: {evdev.categorize(event)}")
            pcard_spi.xfer([0,0,0,0,0,0,0,0])

    except (FileNotFoundError, OSError) as e:
        print(f"[disconnect] {path}: {e}")
        hid_info_dict_remove(path)
    except asyncio.CancelledError:
        raise
    finally:
        if dev is not None:
            try:
                dev.close()
            except Exception:
                pass

ignored_hid_device_name = ['pwr_button', 'hdmi', 'motion']

def is_ignored_device(dev_path):
    dev_name = evdev.InputDevice(dev_path).name.lower()
    for item in ignored_hid_device_name:
        if item.lower() in dev_name:
            return True
    return False

async def watch_all_devices():
    """
    Periodically rescan /dev/input for devices, starting and stopping
    per-device reader tasks as paths appear/disappear.
    """
    tasks: dict[str, asyncio.Task] = {}
    known_paths: set[str] = set()

    while True:
        try:
            current_paths = set([x for x in evdev.list_devices() if is_ignored_device(x) is False])
        except Exception as e:
            print(f"[warn] list_devices failed: {e}")
            current_paths = known_paths

        # New devices -> start a reader task
        for path in current_paths - known_paths:
            tasks[path] = asyncio.create_task(read_device_events(path))

        # Removed devices -> cancel the reader task (if still running)
        for path in known_paths - current_paths:
            print(f"[detach] {path}")
            hid_info_dict_remove(path)
            task = tasks.pop(path, None)
            if task is not None and not task.done():
                task.cancel()

        known_paths = current_paths
        await asyncio.sleep(POLL_INTERVAL_SEC)

async def main():
    # Kick off the watcher; it will create per-device reader tasks
    watcher = asyncio.create_task(watch_all_devices())
    try:
        await watcher
    except asyncio.CancelledError:
        pass

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n[exit] keyboard interrupt")

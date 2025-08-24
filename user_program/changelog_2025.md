# USB4VC 2025 Refactor Notes

### Whats new / Done

* Latest OS version with 64Bit kernel (RPi 3 and up, maybe separate release for RPi 2?)
* Removed joystick curve setting
* Removed mapping profiles from USB (to be replaced with web based setup)
* Switched to asyncIO and evdev instead of reading raw event files from a thread
* Changed usbpoll to 4 to prevent HI-DPI devices from overwhelming the P-Card and retro computer

### Todo:

* Web dashboard
* Go through and rewrite code
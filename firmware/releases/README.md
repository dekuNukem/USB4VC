# USB4VC Firmware Release History

## Table of Contents

[Firmware history for IBM PC Protocol Card](#ibm-pc-protocol-card)

[Firmware history for Apple Desktop Bus (ADB) Protocol Card](#apple-desktop-bus-adb-protocol-card)

## IBM PC Protocol Card

### 0.5.4

* Released 19 May 2022

* Fixed a bug where turning off gameport gamepad will cause a crash.

### 0.5.3

* Released 13 May 2022

* Added mouse event consolidation and adjusted PS/2 timing to better handle high-reporting-rate USB mice.

### 0.5.2

* Released 13 May 2022

* Added improved idle detection to PS/2 mouse too.

### 0.5.1

* Released 12 May 2022

* Improved PS/2 keyboard line idle state detection.

### 0.5.0

* Released 10 May 2022

* Added Mousesystems serial mouse protocol

* Tested working with CTMOUSE driver on a Pentium 1 DOS PC.

### 0.4.0

* Released 6 May 2022

* Added PS/2 keyboard scancode set 3 support (experimental, do try it out!)

* Removed separate PS/2 Mouse KVM compatibility mode

### 0.3.0

* Released 5 May 2022

* Improved PS/2 host inhibit handling, should work more reliably now.

* Added PS/2 Mouse KVM compatibility mode, not sure if I wanna keep this.

### 0.2.0

* Initial Release

## Apple Desktop Bus (ADB) Protocol Card

### 0.2.0

* Released 20 May 2022

* Improved mouse and keyboard event buffering

* Added mouse event consolidation to better support high refresh rate mice

* Enabled watchdog timer

* Pressing `ADB POWER` button now sends a message through ADB too.

### 0.1.0

* Initial Release
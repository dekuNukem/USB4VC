# Getting Started with USB4VC

[Get USB4VC](https://www.tindie.com/) | [Official Discord](https://discord.gg/4sJCBx5) | [Getting Started](getting_started.md) | Table of Contents

------

Congratulations on your new toy! Here is a short guide on how to use your USB4VC.

## SD card Preparation

If your USB4VC already comes with a SD card, feel free to [skip this step](#test-out-raspberry-pi)!

Download image

Burn through ethcher or rufus

## Test out Raspberry Pi

It's good practice to make sure your RPi works before mounting it into USB4VC.

Insert SD card in Raspberry Pi, and hook it up to a monitor. No need for anything else.

Power it on, and it should boot and end up with the program running, showing a bunch of information like this:

[Photo Here]

If so, congrats! Power off and continue.

## Kit Assembly

If your USB4VC has not been assembled yet, [see this guide](/kit_assembly_guide.md) to put it together.

## Cable Connection

**Keep your computer off for now!**

Using the appropriate cable, connect the desired port from Protocol Card to your computer.

Most of the peripherals are **NOT hot pluggable**, so make sure **all cables are connected before using!**

Also plug in any USB Keyboard, Mouse, and Gamepad you're planning to use.

If using Bluetooth, we can pair it later.

## USB4VC Power On

Power up USB4VC using a USB-C cable, you can do it from Protocol Card, Baseboard, or even RPi itself. All will work!

[Top-down photo with arrows pointing to power ports]

After a few seconds, the OLED screen should light up, showing some information:

[photo of OLED showing home screen, with legends and arrows]

Press `+` or `-` to switch pages, and `enter` button to enter the submenu. If you have a Bluetooth device, you can pair it in the menu.

## Protocol Setup

Looking at home screen, if displayed protocol is what you want, you're good to go!

Otherwise, press `Enter` button, and you can switch protocols, adjust mouse sensitivity and gamepad linearity.

I would suggest leaving the mouse sensitivity at 1 and adjust in the system if possible. Same with gamepads.

## Try it out!

With protocol set up, and cable connected, time to power on the computer!

You should be able to use it as normal, now with USB inputs!

## Advanced Features: Mapping gamepad to keyboard/mouse

json file





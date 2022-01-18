# USB4VC: USB Inputs on Retro Computers!

[Get USB4VC](https://www.tindie.com/) | [Official Discord](https://discord.gg/4sJCBx5) | [Getting Started](getting_started.md) | Table of Contents

-----

USB4VC is an **active protocol converter** that let you use **USB keyboard, mouse, and gamepads** on retro computers.

![Alt text](photos/header.jpeg)

With a **modular design**, different platforms are supported by swapping out **Protocol Cards**:

[video clip here]

Two Protocol Cards are available at launch (Feb 2022):

#### IBM PC Compatible

* PS/2 Keyboard
* PS/2 Mouse
* XT Keyboard
* Serial Mouse
* 15-pin Gameport Gamepad
* Mapping USB Gamepad to PC Mouse/Keyboard

#### Apple Desktop Bus

* ADB Keyboard
* ADB Mouse
* Mapping USB Gamepad to ADB Mouse/Keyboard

Of course, more Protocol Cards are planned after launch!

## Launch Timeline / Stay in Touch!

[Clickable small banners here]

**`Interested?`** Please follow the project Discord/Twitter, or join Kickstarter/Tindie waiting list to be notified with latest updates!

**`Questions and Comments?`** Feel free to ask in Discord/Twitter, raise a Github issue, or via `dekunukem` `gmail.com`!

Current launch timeline is:

#### Feb - March 2022

* Public Beta Testing

* Gather Feedback, Fix Bugs, Finishing Touches.

#### March - April 2022

* Kickstarter

* Arrange Production Run

#### April 2022 Onwards

* Public Release

* Work on Improvements, More Protocols and Protocol Cards

## Why?

Before the homogeneity of USB peripherals today, computer input devices were a wild west. Different platforms used their own connectors and protocols for keyboards, mice, and gamepads.

[Photo of collection of my keyboards and mice]

With the popularity of retro computing today, it poses several issues:

* Many proprietary peripherals are simply **hard to find**, commanding a premium on online marketplaces. They are only getting rarer and rarer, and without them, the computer itself is useless.

* After 3 or 4 decades of service, many can be **degraded and unreliable** (e.g. foam & foil keyboards), requiring extensive restoration.

* Let's be honest here, most early peripherals simply **does not feel that great**. Mushy membrane keyboards, sticky ball mice, plasticky gamepads, etc. And of course, high quality ones [gets *very expensive*](https://www.ebay.com/sch/i.html?_nkw=ibm+model+f&_sacat=0&rt=nc&LH_Sold=1&LH_Complete=1).

* On the other hand, high quality USB peripherals can be had for a very reasonable price today with vastly **superior tactility, precision, and ergonomics**.

* So I wanted to enjoy my machines with latest

* I also hope that it can put computers without proprietary peripherals back in action, lower the entry barrier of certain machines, and generally help more people get into this hobby!

wanted enjoy more, bring more people, save computers without keybaords, 

can argue part of the experience, but enjoy more without the drawback

[Gaming DOS PC with RGB keyboard and duckypad here]

## How it works

two part: baseboard and swappable protocol card.

raspberry pi based, drives OLED read buttons, read USB periphrials. sends event to protocol card, who handles keyboard emulation.

by splitting dutis, rpi can do general stuff, while dedicated mcu on protocol card handles timing critical tasks. also modular and swappable.


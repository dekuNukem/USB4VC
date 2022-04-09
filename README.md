# USB4VC: USB Inputs on Retro Computers!

[Get USB4VC!](https://www.tindie.com/products/dekuNukem/usb4vc-usb-inputs-on-retro-computers/) | [Official Discord](https://discord.gg/HAuuh3pAmB) | [Getting Started](getting_started.md) | [Table of Contents](#table-of-contents)

-----

USB4VC is an **active protocol converter** that let you use **USB keyboard, mouse, and gamepads** on a wide range of retro computers.

![Alt text](photos/header.jpeg)

With a **modular design**, different platforms are supported by swapping out **Protocol Cards**:

![Alt text](photos/header.gif)

Two Protocol Cards are available at launch (March 2022):

#### IBM PC Compatible

* PS/2 Keyboard
* PS/2 Mouse
* AT Keyboard
* XT Keyboard
* Serial Mouse
* 15-pin Gameport Gamepad
* Mapping USB Gamepad to PC Mouse/Keyboard

From the very first IBM PC in 1981 until the end of PS/2 ports in early 2000s, this Protocol Card covers it all!

#### Apple Desktop Bus (ADB)

* ADB Keyboard
* ADB Mouse
* Mapping USB Gamepad to ADB Mouse/Keyboard

From Apple IIGS in 1986 to PowerMac G3 in 1999, and everything in-between!

#### More to Come!

Of course, more Protocol Cards are planned, and you can try [make your own](technical_notes.md) too!

## I Want One! / Get in Touch

This project have been successly funded [via Kickstarter!](https://www.kickstarter.com/projects/dekunukem/usb4vc-usb-inputs-on-retro-computers), it is now available [on my Tindie store!](https://www.tindie.com/products/dekuNukem/usb4vc-usb-inputs-on-retro-computers/)

Thank you very much for the support! Estimated shipping **April 2022**.

**Questions or comments?** Ask in [official Discord](https://discord.gg/HAuuh3pAmB), raise a [Github issue](https://github.com/dekuNukem/USB4VC/issues), [Twitter DM](https://twitter.com/dekuNukem_), or email `dekunukem` `gmail.com`!

## Promo Video

Click to see USB4VC in action!

<table>
  <tr>
    <td><a href="https://www.youtube.com/watch?v=54sdPELuu4g" title="YouTube" rel="noopener"><img src="https://i.imgur.com/pFqa7sO.png"></a></td>
  </tr>
</table>

## Highlights

* USB Keyboard/Mouse/Gamepads on Vintage Computers!

* Modular & Swappable Protocol Cards

* Wireless / Bluetooth

* Ultra Low Latency (0.5ms)

* OLED Display

* USB-C Powered

* Fully Open-source

* Make-your-own Protocol Card [Instruction Provided](technical_notes.md)

## Why / Project Goals

Before the homogeneity of USB peripherals today, computer input devices were a wild west of proprietary peripherals, as different platforms used their own connectors and protocols for keyboards, mice, and gamepads.

![Alt text](photos/keyboards.jpeg)

With the popularity of retro computing today, it poses several issues:

* Many of those peripherals are simply **hard to find**, commanding a premium on online marketplaces. They are only getting rarer, and without them, the computer itself is useless.

* After decades of service, many can be **degraded and unreliable** (e.g. foam & foil keyboards), requiring extensive restoration.

* Let's be honest here, most early peripherals simply **does not feel that great**. Mushy membrane keyboards, sticky ball mice, plasticky gamepads, etc. And of course, the good ones gets [*very expensive*](https://www.ebay.com/sch/i.html?_nkw=ibm+model+f&_sacat=0&rt=nc&LH_Sold=1&LH_Complete=1).

* On the other hand, high quality USB peripherals can be had for a very reasonable price today with vastly **superior tactility, precision, and ergonomics**.

* Thus, this project aims to make retro computers simply **more enjoyable to use**.

* I also hope it can **lower the barrier of entry** for certain machines, put more computers **back in action**, help **preserving** existing rare peripherals, and generally **help more people** get into this hobby!

But of course, one can argue that using period-correct peripherals is simply part of the hobby, but just like HDMI upscalers or floppy emus, it's nice to have the option to enhance the experience.

![Alt text](photos/duke3d.jpeg)

## How it Works

USB4VC consists of two halves: **Baseboard** and swappable **Protocol Cards**.

![Alt text](photos/pcards.jpeg)

Baseboard contains user buttons, OLED screen, and a Raspberry Pi. It processes USB input events, which are sent to Protocol Card.

Each Protocol Card has a dedicated microcontroller and the appropriate circuitry and connectors for a specific platform.

By splitting duties, RPi can focus on input parsing, and Protocol Card handles timing critical signal generation, resulting in a flexible architecture and reliable performance.

USB peripherals can be connected via cables, wireless dongles, or Bluetooth.

## Getting Started

[Click me](getting_started.md)

## Technical Details / Make Your Own Protocol Cards

[Click me](technical_notes.md)

## Roadmap / Future Plans

Current focus is having a smooth project launch. So for the first few months I will mostly be working on maintaining the [Kickstarter campaign](https://www.kickstarter.com/projects/dekunukem/usb4vc-usb-inputs-on-retro-computers), shipping orders, and providing updates and bug fixes for existing Protocol Cards.

Of course, more Protocol Cards are planned. Not making any guarantees at this moment, but a shortlist include:

* OG Macintosh/128K/512K/Mac Plus
* Olivetti M24 / AT&T PC 6300
* SUN SPARCstation

Want more? Feel free to [get in touch](#get-one--stay-in-touch) and make suggestions!

## Q&A

### Where can I get one?

This project have been successly funded [via Kickstarter!](https://www.kickstarter.com/projects/dekunukem/usb4vc-usb-inputs-on-retro-computers), now it is available [on my Tindie store!](https://www.tindie.com/products/dekuNukem/usb4vc-usb-inputs-on-retro-computers/)

### Which Raspberry Pi can I use?

Any RPi with a **40-pin header** *should* work, although **Model B/B+** is preferred. See [this table](https://en.wikipedia.org/wiki/Raspberry_Pi#Model_comparison) for details, sort by **GPIO** column.

USB4VC has been officially tested on **Raspberry Pi 2/3/4 Model B**.

Raspberry Pi Zeros should work too, but would need a USB hub.

No need to get the latest and greatest. An older RPi is plenty fast enough!

### What about latency?

0.5ms to 1ms depending on the Raspberry Pi generation.

1ms is *one thousandth* of a second. [More info here](technical_notes.md#latency-information).

### Can you develop a Protocol Card for ___ ?

I'd love to! But I would need a working machine with working peripherals.

So if you're in UK and is happy to lend me one for a while, [let me know!](#i-want-one--get-in-touch)

Also, as I can only work on so many at once, I'll probably focus on the more popular machines at first, but still, do let me know!

### Can I make my own Protocol Cards?

Of course! See [this document](technical_notes.md).

### What does USB4VC stand for?

USB for Vintage Computers. Although come to think of it, it's perfect for vintage consoles too! 🤔

## Questions or Comments?

Feel free to ask in official [Discord Chatroom](https://discord.gg/HAuuh3pAmB), raise a [Github issue](https://github.com/dekuNukem/USB4VC/issues), [DM on Twitter](https://twitter.com/dekuNukem_), or email `dekunukem` `gmail.com`!

## Table of Contents

[Main page](README.md)

[(Youtube Video) USB4VC in Action](https://www.youtube.com/watch?v=54sdPELuu4g)

[Getting Started Guide](getting_started.md)

[Kit Assembly Guide](kit_assembly.md)

[Tinkering Guide / Make Your Own Protocol Card / Technical Notes](technical_notes.md)

[Kickstarter Acknowledgements](kickstarter_info.md)


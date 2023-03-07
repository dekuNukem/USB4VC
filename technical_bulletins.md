# USB4VC Protocol Card Stability Upgrade

[Get USB4VC](https://www.tindie.com/products/dekuNukem/usb4vc-usb-inputs-on-retro-computers/) | [Official Discord](https://discord.gg/HAuuh3pAmB) | [Getting Started](getting_started.md) | [Table of Contents](#table-of-contents)

-----

[UNDER CONSTRUCTION] [UNDER CONSTRUCTION] [UNDER CONSTRUCTION] 

This document covers a potential solution that improves the stability of USB4VC.

## Overview

A `BUSY` signal is added, to prevent Raspberry Pi from sending data when Protocol Card is talking to the retro computer.

This, along with a firmware update, should improve stability and eliminate stuck keys or glitches.

## Instructions

### Solder a New Wire

Solder a wire between `PF1` pin and the pin at **second row right most column**. See photo:

![Alt text](photos/adb_busy.jpeg)

![Alt text](photos/adb_busy_close.jpeg)

Make sure **there are no short circuits**.

### Update RPi Software

[UNDER CONSTRUCTION]

### Update Protocol Card Firmware

[UNDER CONSTRUCTION]

## Questions or Comments?

Feel free to ask in official [Discord Chatroom](https://discord.gg/HAuuh3pAmB), raise a [Github issue](https://github.com/dekuNukem/USB4VC/issues), [DM on Twitter](https://twitter.com/dekuNukem_), or email `dekunukem` `gmail.com`!

## Table of Contents

[Main page](README.md)

[(Youtube Video) USB4VC in Action](https://www.youtube.com/watch?v=54sdPELuu4g)

[Getting Started Guide](getting_started.md)

[Kit Assembly Guide](kit_assembly.md)

[Tinkering Guide / Make Your Own Protocol Card / Technical Notes](technical_notes.md)

[Kickstarter Acknowledgements](kickstarter_info.md)


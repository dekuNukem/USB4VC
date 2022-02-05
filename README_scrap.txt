## Testers / Reviewers Wanted!

Assembled around 7 by hand, can send it out.

want more game controllers
IBM PC AT

**`Interested?`** Please follow the project Discord/Twitter, or join Kickstarter waiting list to be notified with latest updates!

**`Questions and Comments?`** 
want one for review or testing? or more information about launch: [Link to Launch timeline ]

## How it Works

USB4VC consists of two parts: **Baseboard** and swappable **Protocol Cards**.

[Photo of USB4VC parts with 3 RPi generations]

Connect your peripheral via USB/Bluetooth, plug in the cable from P-Card to your computer, and off you go!

Baseboard incorporates a Raspberry Pi Model B. It handles UI and input events, which are processed and sent to Protocol Card. 

Each Protocol Card has a dedicated microcontroller and the appropriate connectors for the target computer.

By splitting duties, RPi can focus on input parsing, and P-Card handles timing critical output generation, resulting in stable performance and a flexible architecture.


------------- scrap below -------------

First of all, we need to get what USB/Bluetooth devices are doing in order to process and send it to Protocol Card.

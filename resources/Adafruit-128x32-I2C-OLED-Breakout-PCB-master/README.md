# PCB for the Adafruit 128x32 I2C OLED breakout board

<a href="http://www.adafruit.com/products/931"><img src="assets/image.jpg?raw=true" width="500px"><br/>Click here to purchase one from the Adafruit shop</a>

__Format is EagleCAD schematic and board layout__

These displays are small, only about 1" diagonal, but very readable due to the high contrast of an OLED display. This display is made of 128x32 individual white OLED pixels, each one is turned on or off by the controller chip. Because the display makes its own light, no backlight is required. This reduces the power required to run the OLED and is why the display has such high contrast; we really like this miniature display for its crispness!


The driver chip SSD1306, __communicates via I2C only.__ 3 pins are required to communicate with the chip in the OLED display, two of which are I2C data/clock pins.

The OLED and driver require a 3.3V power supply and 3.3V logic levels for communication. To make it easier for our customers to use, we've added a 3.3v regulator and level shifter on board! This makes it compatible with any 5V microcontroller, such as the Arduino.

The power requirements depend a little on how much of the display is lit but on average the display uses about 20mA from the 3.3V supply. Built into the OLED driver is a simple switch-cap charge pump that turns 3.3v-5v into a high voltage drive for the OLEDs, making it one of the easiest ways to get an OLED into your project!

Of course, we wouldn't leave you with a datasheet and a "good luck": [We have a detailed tutorial](http://learn.adafruit.com/monochrome-oled-breakouts) and [example code in the form of an Arduino library](https://github.com/adafruit/Adafruit_SSD1306) for text and graphics. You'll need a microcontroller with more than 512 bytes of RAM since the display must be buffered.

You can download our SSD1306 OLED display Arduino library from github which comes with example code. The library can print text, bitmaps, pixels, rectangles, circles and lines. It uses 512 bytes of RAM since it needs to buffer the entire display but its very fast! The code is simple to adapt to any other microcontroller.

## License
Adafruit invests time and resources providing this open source design,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Designed by Limor Fried/Ladyada for Adafruit Industries.
Creative Commons Attribution, Share-Alike license, check license.txt for more information
All text above must be included in any redistribution

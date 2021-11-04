SparkFun Qwiic USB Hub - USB2514B
========================================

[![SparkX USB Hub - USB2514B (Qwiic)](https://cdn.sparkfun.com//assets/parts/1/7/2/2/9/18014-Qwiic_USB_Hub-03.jpg)](https://www.sparkfun.com/products/18014)

[*SparkX USB Hub - USB2514B (Qwiic) (SPX-18014)*](https://www.sparkfun.com/products/18014)

There are times when you have multiple USB devices in your prototype or device and you just want to plug in one cable. You need a built in USB hub! 

The [Qwiic USB Hub](https://www.sparkfun.com/products/18014) is a simple breakout board for the USB2514B USB hub IC. What gives it extra flair is the ability to configure the device over I2C. Out of the box, the board will appear as a standard USB 2.0 Hub; no configuration necessary. Closing the I2C jumper will put the device into configuration mode. We've got an Arduino Library to help you set the various parameters of the hub including VID/PID, downstream max current, even language ID and product names. 

Note: Once the configuration bytes are set and the USB2514B is told to 'attach' it will enumerate as a HUB and the configuration cannot be changed until the IC is reset. 

The USB2514 is a 4-port 2.0 USB hub. Who cares about USB 2.0 these days? The vast majority of projects we work on have a simple USB-to-Serial IC on it that do not require gigabit datarates. The USB251xB is a simple hub IC allowing multiple USB things to live under one USB connection; this is especially useful when prototyping a new product where you want the user to plug in just one cable.

SparkFun labored with love to create this code. Feel like supporting open source hardware? 
Buy a [board](https://www.sparkfun.com/products/18014) from SparkFun!

Repository Contents
-------------------

* **/hardware** - Eagle files
* **/documents** - Datasheets and other source documents

Documentation
--------------

* **[Arduino Library](https://github.com/sparkfun/SparkFun_USB251x_Arduino_Library)** - Main repository (including hardware files)
* **[Installing an Arduino Library Guide](https://learn.sparkfun.com/tutorials/installing-an-arduino-library)** - Basic information on how to install an Arduino library.

Products that use this Repository
--------------

* [SPX-18014](https://www.sparkfun.com/products/18014)

License Information
-------------------

This product is _**open source**_! 

Please review the LICENSE.md file for license information. 

If you have any questions or concerns on licensing, please contact technical support on our [SparkFun forums](https://forum.sparkfun.com/viewforum.php?f=152).

Distributed as-is; no warranty is given.

- Your friends at SparkFun.

_<COLLABORATION CREDIT>_

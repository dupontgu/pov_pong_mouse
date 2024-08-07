## Demo



## How It Works
Most modern computer mice using relative positioning - they report _changes_ in their movement. If you move the mouse slowly to the left, it might spit out a bunch of packets where the x component is just -1 (meaning the mouse moved 1 "pixel" to the left). However! It is possible to implement a USB mouse that uses _absolute_ positioning. It can send an exact position on your monitor (percentage X, percentage Y) that the cursor should move to instantly. This is commonly used for touchscreen drivers - you want the cursor to appear where the finger touches the screen.  This firmware emulates an absolute positioning mouse and quickly moves the mouse cursor between points of interest. It moves fast enough that the cursor (kinda) appears in all positions at once and gives the impression that it is in multiple places at once. I have implemented a simple game of Pong to run in the firmware, and set the cursor's points of interest to be the 2 paddles and the ball while the game is active. So the game runs completely inside the mouse!

## Setup
1. You'll need hardware! Any RP2040 based board wired up with a second USB port will do, but I recommend Adafruit's [Feather RP2040 with USB Host](https://www.adafruit.com/product/5723).
2. Follow [Adafruit's guide](https://learn.adafruit.com/adafruit-feather-rp2040-with-usb-type-a-host/arduino-ide-setup) for getting set up with the Arduino IDE.
3. Install the Adafruit TinyUSB Library and the Pico PIO USB Library (again, [guide](https://learn.adafruit.com/adafruit-feather-rp2040-with-usb-type-a-host/usb-host-device-info)).
4. Ensure that your USB D+/D- pins are [set correctly](./usbh_helper.h).
5. Build and run!
## Setup
1. You'll need hardware! Any RP2040 based board wired up with a second USB port will do, but I recommend Adafruit's [Feather RP2040 with USB Host](https://www.adafruit.com/product/5723).
2. Follow [Adafruit's guide](https://learn.adafruit.com/adafruit-feather-rp2040-with-usb-type-a-host/arduino-ide-setup) for getting set up with the Arduino IDE.
3. Install the Adafruit TinyUSB Library and the Pico PIO USB Library (again, [guide](https://learn.adafruit.com/adafruit-feather-rp2040-with-usb-type-a-host/usb-host-device-info)).
4. Ensure that your USB D+/D- pins are [set correctly](./usbh_helper.h).
5. Build and run!
/*********************************************************************
 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 Copyright (c) 2019 Ha Thach for Adafruit Industries
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

/**
Adapted from Adafruit's TinyUSB sample remapper demo:
https://github.com/adafruit/Adafruit_TinyUSB_Arduino/tree/master/examples/DualRole/HID/hid_remapper

by Guy Dupont, August 2024
**/

// USBHost is defined in usbh_helper.h
#include "usbh_helper.h"
#include "pong.h"

// adapted from https://github.com/jonathanedgecombe/absmouse/blob/master/src/AbsMouse.cpp
uint8_t const abs_mouse_desc[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
  0x09, 0x02,        // Usage (Mouse)
  0xA1, 0x01,        // Collection (Application)
  0x09, 0x01,        //   Usage (Pointer)
  0xA1, 0x00,        //   Collection (Physical)
  0x85, 0x04,        //     Report ID (4)
  0x05, 0x09,        //     Usage Page (Button)
  0x19, 0x01,        //     Usage Minimum (0x01)
  0x29, 0x03,        //     Usage Maximum (0x03)
  0x15, 0x00,        //     Logical Minimum (0)
  0x25, 0x01,        //     Logical Maximum (1)
  0x95, 0x03,        //     Report Count (3)
  0x75, 0x01,        //     Report Size (1)
  0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x95, 0x01,        //     Report Count (1)
  0x75, 0x05,        //     Report Size (5)
  0x81, 0x03,        //     Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
  0x09, 0x30,        //     Usage (X)
  0x09, 0x31,        //     Usage (Y)
  0x16, 0x00, 0x00,  //     Logical Minimum (0)
  0x26, 0xFF, 0x7F,  //     Logical Maximum (32767)
  0x36, 0x00, 0x00,  //     Physical Minimum (0)
  0x46, 0xFF, 0x7F,  //     Physical Maximum (32767)
  0x75, 0x10,        //     Report Size (16)
  0x95, 0x02,        //     Report Count (2)
  0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0xC0,              //   End Collection
  0xC0               // End Collection
};

uint8_t const desc_hid_report[] = {
  TUD_HID_REPORT_DESC_MOUSE()
};

// two HID devices, one for absolute mouse (the game) and one for normal mouse (passthrough)
Adafruit_USBD_HID usb_hid_abs(abs_mouse_desc, sizeof(abs_mouse_desc), HID_ITF_PROTOCOL_NONE, 2, true);
Adafruit_USBD_HID usb_hid(desc_hid_report, sizeof(desc_hid_report), HID_ITF_PROTOCOL_MOUSE, 2, true);
Game game;

long lastFrameTime = 0;

// these numbers are virtual/relative and don't correspond to monitor size
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int PADDLE_WIDTH = 10;
const int PADDLE_HEIGHT = 100;
const int HALF_PADDLE_HEIGHT = PADDLE_HEIGHT / 2;
const int BALL_SIZE = 10;
const float MAX_Y_VEL = 8.0;

int8_t mouseVelocity;
bool gameRunning = false;

void setup() {
  Serial.begin(115200);
  usb_hid_abs.begin();
  usb_hid.begin();
  initGame(&game, 10.0, SCREEN_HEIGHT, SCREEN_WIDTH, BALL_SIZE, PADDLE_HEIGHT, PADDLE_WIDTH);
}

// renders the cursor on screen in a specific spot.
// x and y should be floats between 0.0 and 1.0
// (0, 0) -> top left of screen, (1, 1) -> bottom right
void renderCursor(float x, float y) {
  static uint8_t reportOut[5] = { 0, 0, 0, 0, 0 };
  uint16_t adjX = (uint16_t)((x * 20000) + 6383);  // padding is (32767 - [max]) / 2
  uint16_t adjY = (uint16_t)((y * 26000) + 3383);
  reportOut[1] = adjX & 0xFF;
  reportOut[2] = (adjX >> 8) & 0xFF;
  reportOut[3] = adjY & 0xFF;
  reportOut[4] = (adjY >> 8) & 0xFF;
  if (usb_hid_abs.ready()) {
    // report id 4 was hardcoded in descriptor above
    usb_hid_abs.sendReport(4, &reportOut, 5);
  }
}

void loop() {
  if (gameRunning) {
    long timeNow = millis();
    if ((timeNow - lastFrameTime) >= 16) {
      lastFrameTime = timeNow;
      float leftPaddleVel = paddleAutoPilot(&game, &(game.lPaddle), 5.0, 2);
      float rightPaddleVel = mouseVelocity;
      mouseVelocity = 0;
      tick(&game, leftPaddleVel, rightPaddleVel);
    }

    Frame frame = nextSubframe(&game);
    renderCursor(frame.x, frame.y);
  }

  // ok to delay in this loop, different values produce different POV artifacts
  delay(7);
}

//------------- Core1 -------------//
void setup1() {
  // configure pio-usb: defined in usbh_helper.h
  rp2040_configure_pio_usb();

  // run host stack on controller (rhport) 1
  // Note: For rp2040 pico-pio-usb, calling USBHost.begin() on core1 will have most of the
  // host bit-banging processing works done in core1 to free up core0 for other works
  USBHost.begin(1);
}

void loop1() {
  USBHost.task();
}

//--------------------------------------------------------------------+
// TinyUSB Host callbacks
//--------------------------------------------------------------------+
extern "C" {

  // Invoked when device with hid interface is mounted
  // Report descriptor is also available for use.
  // tuh_hid_parse_report_descriptor() can be used to parse common/simple enough
  // descriptor. Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE,
  // it will be skipped therefore report_desc = NULL, desc_len = 0
  void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len) {
    (void)desc_report;
    (void)desc_len;
    uint16_t vid, pid;
    tuh_vid_pid_get(dev_addr, &vid, &pid);

    Serial.printf("HID device address = %d, instance = %d is mounted\r\n", dev_addr, instance);
    Serial.printf("VID = %04x, PID = %04x\r\n", vid, pid);

    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
    if (itf_protocol == HID_ITF_PROTOCOL_MOUSE) {
      Serial.printf("HID Mouse\r\n");
      if (!tuh_hid_receive_report(dev_addr, instance)) {
        Serial.printf("Error: cannot request to receive report\r\n");
      }
    }
  }

  // Invoked when device with hid interface is un-mounted
  void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    Serial.printf("HID device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
  }

  // Invoked when received report from device via interrupt endpoint
  void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {
    static uint8_t reportOut[5] = { 0, 0, 0, 0, 0 };
    static bool scrollButtonDown;
    // different mice might have different lengths here! My shitty HP mouse only reports [buttons, x, y]
    if (len == 3) {
      // scroll wheel click is 3rd bit, use that to start/stop the game
      if (report[0] & 0b100) {
        if (!scrollButtonDown) {
          scrollButtonDown = true;
          gameRunning = !gameRunning;
        }
      } else {
        scrollButtonDown = false;
      }

      if (!gameRunning) {
        // if game's not running, copy mouse report to passthrough
        reportOut[0] = report[0];
        reportOut[1] = report[1];
        reportOut[2] = report[2];
        usb_hid.sendReport(0, reportOut, 5);
      } else {
        // if game is happening, copy latest Y velocity to be used by game logic
        mouseVelocity = (int8_t)report[2];
      }
    }
    // continue to request to receive report
    if (!tuh_hid_receive_report(dev_addr, instance)) {
      Serial.printf("Error: cannot request to receive report\r\n");
    }
  }
}

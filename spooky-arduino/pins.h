#pragma once

#include <Arduino.h>

// Central pin map for the spooky-arduino firmware. Change pins here only.

constexpr uint8_t kOnboardLedPin = LED_BUILTIN;  // D13
constexpr uint8_t kIrReceiverPin = 2;            // D2
constexpr uint8_t kPirPin = 4;                   // D4
// DMXSerial drives this pin as an output to control data direction. The RS485
// module is auto-direction and ignores it, but it must NOT be D2 (the IR
// receiver) or the library would fight the receiver.
constexpr uint8_t kDmxModePin = 5;               // D5
// DMX output uses the hardware USART: D1 (TX) -> RS485 module TXD. D0 stays
// free for the bootloader; there is no USB serial monitor while DMX runs.

#pragma once

#include <IRremote.hpp>

// IR codes for the Elegoo kit's 21-button remote.
//
// Every button transmits NEC with address 0x00 and a distinct command byte.
// Codes were captured on hardware with IR_CAPTURE_MODE in the sketch (see
// README.md for the capture procedure). Labels match this remote's silkscreen.

constexpr decode_type_t kRemoteProtocol = NEC;
constexpr uint16_t kRemoteAddress = 0x00;

constexpr uint8_t kButtonPower = 0x45;
constexpr uint8_t kButtonVolumeUp = 0x46;
constexpr uint8_t kButtonVolumeDown = 0x15;
constexpr uint8_t kButtonFuncStop = 0x47;
constexpr uint8_t kButtonTrackLeft = 0x44;
constexpr uint8_t kButtonPlayPause = 0x40;
constexpr uint8_t kButtonTrackRight = 0x43;
constexpr uint8_t kButtonDown = 0x07;
constexpr uint8_t kButtonUp = 0x09;
constexpr uint8_t kButtonEq = 0x19;
constexpr uint8_t kButtonStRept = 0x0D;
constexpr uint8_t kButton0 = 0x16;
constexpr uint8_t kButton1 = 0x0C;
constexpr uint8_t kButton2 = 0x18;
constexpr uint8_t kButton3 = 0x5E;
constexpr uint8_t kButton4 = 0x08;
constexpr uint8_t kButton5 = 0x1C;
constexpr uint8_t kButton6 = 0x5A;
constexpr uint8_t kButton7 = 0x42;
constexpr uint8_t kButton8 = 0x52;
constexpr uint8_t kButton9 = 0x4A;

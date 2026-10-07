#pragma once

#include <Arduino.h>

// LED-on timeout settings for spooky-arduino.
//
// The timeout is a single value in whole seconds. It is adjustable only along
// the "ladder" below (so a remote press always moves to a predictable value)
// and is persisted in EEPROM so it survives power cycles.

constexpr uint16_t kDefaultTimeoutSec = 5;
constexpr uint16_t kMinTimeoutSec = 1;
constexpr uint16_t kMaxTimeoutSec = 300;

// Reachable timeout values: 1-second steps up to 10, then 10-second steps to
// 30, a 30-second step to 60, then 60-second steps to 300.
constexpr uint16_t kTimeoutLadderSec[] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 30, 60, 120, 180, 240, 300};
constexpr uint8_t kTimeoutLadderSize =
    sizeof(kTimeoutLadderSec) / sizeof(kTimeoutLadderSec[0]);

// EEPROM layout. `magic` and `version` let the firmware detect a blank or
// outdated store (a fresh chip reads 0xFF) and fall back to defaults.
constexpr uint16_t kSettingsMagic = 0x5A17;
constexpr uint8_t kSettingsVersion = 1;

struct Settings {
  uint16_t magic;
  uint8_t version;
  uint16_t timeoutSec;
};

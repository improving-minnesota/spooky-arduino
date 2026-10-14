#pragma once

#include <Arduino.h>

#include "haze.h"

// Settings for spooky-arduino, persisted in EEPROM so they survive power cycles.
//
// The timeout is a single value in whole seconds, adjustable only along the
// "ladder" below (so a remote press always moves to a predictable value).
//
// The haze machine's two settings sets are stored here too, so a blower speed,
// a bubble toggle, a haze toggle, or a haze rate survives a reboot.

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
// outdated store (a fresh chip reads 0xFF) and fall back to defaults. The
// version was bumped when the haze settings moved in, so an older store is
// discarded rather than misread.
constexpr uint16_t kSettingsMagic = 0x5A17;
constexpr uint8_t kSettingsVersion = 2;

struct Settings {
  uint16_t magic;
  uint8_t version;
  uint16_t timeoutSec;
  HazeState haze;
};

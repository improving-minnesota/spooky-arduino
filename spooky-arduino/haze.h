#pragma once

#include <Arduino.h>

// Haze machine settings. Both sets are persisted in EEPROM alongside the effect
// duration, so a blower speed, a bubble toggle, a haze toggle, or a haze rate
// survives a power cycle.
//
// There are two independent sets: one the machine uses while the device is idle
// and one it uses while an effect runs. The remote edits whichever set matches
// the device's current state, so the machine can be foggy and loud during an
// effect and quiet in between, and each set is changed by making the change
// while the device is in that state.

// Blower bands (dmx-poc/WIRING.md): 0-100 low, 101-200 medium, 201-255 high.
// "Off" is not a documented band, so it is sent as 0 (the bottom of "low").
constexpr uint8_t kBlowerOff = 0;
constexpr uint8_t kBlowerLow = 50;
constexpr uint8_t kBlowerMedium = 150;
constexpr uint8_t kBlowerHigh = 250;

// Six-step haze rate ladder inside the machine's documented continuous-haze
// band (11-126). The exact values are confirmed by the Mac bench test.
constexpr uint8_t kHazeLadder[] = {11, 34, 57, 80, 103, 126};
constexpr uint8_t kHazeLadderSize = sizeof(kHazeLadder) / sizeof(kHazeLadder[0]);
constexpr uint8_t kHazeDefaultStep = 2;  // mid ladder (~57)

// On values for the machine's on/off channels.
constexpr uint8_t kBubblesOn = 255;
constexpr uint8_t kCleaningOn = 255;

struct HazeSettings {
  uint8_t blower;
  bool bubbles;
  bool hazeOn;
  uint8_t rateStep;
};

// Idle defaults to off, so nothing runs between effects and no fluid is spent
// waiting for the next event. An event defaults to haze, bubbles, and a full
// blower, so an effect arrives with the machine already working.
constexpr HazeSettings kHazeIdleDefaults = {kBlowerOff, false, false, kHazeDefaultStep};
constexpr HazeSettings kHazeEventDefaults = {kBlowerHigh, true, true, kHazeDefaultStep};

struct HazeState {
  HazeSettings idle = kHazeIdleDefaults;
  HazeSettings event = kHazeEventDefaults;
};

inline uint8_t hazeRateValue(uint8_t step) {
  return kHazeLadder[step < kHazeLadderSize ? step : kHazeLadderSize - 1];
}

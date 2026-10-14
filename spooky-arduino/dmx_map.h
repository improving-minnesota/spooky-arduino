#pragma once

#include <Arduino.h>
#include <avr/interrupt.h>

#include <DMXSerial.h>

// DMX channel map for the show. One 512-channel universe drives both fixtures:
//   SHEHDS BMH0812D (38-channel mode)  at address 1  (channels 1-38)
//   Chauvet Hurricane Bubble Haze      at address 39 (channels 39-42)
// Change the base addresses here only.

// --- SHEHDS BMH0812D -------------------------------------------------------
constexpr uint16_t kBarBase = 1;
constexpr uint16_t kBarTilt = kBarBase + 0;
constexpr uint16_t kBarTiltSpeed = kBarBase + 1;
constexpr uint16_t kBarEffect = kBarBase + 2;
constexpr uint16_t kBarEffectSpeed = kBarBase + 3;
constexpr uint16_t kBarDimmer = kBarBase + 4;
constexpr uint16_t kBarStrobe = kBarBase + 5;
constexpr uint16_t kBar0 = kBarBase + 6;  // BAR1 red; each bar is R, G, B, W
constexpr uint8_t kBarCount = 8;
constexpr uint8_t kBarChannels = 4;  // R, G, B, W in that order

// --- Chauvet Hurricane Bubble Haze -----------------------------------------
constexpr uint16_t kHazeBase = 39;
constexpr uint16_t kHazeBubbles = kHazeBase + 0;
constexpr uint16_t kHazeVolume = kHazeBase + 1;
constexpr uint16_t kHazeBlower = kHazeBase + 2;
constexpr uint16_t kHazeCleaning = kHazeBase + 3;

// The two fixtures together occupy channels 1-42 of the universe.
constexpr uint16_t kFixtureChannelCount = kHazeCleaning;

// ----- Channel writers -----
//
// DMXSerial transmits the channel buffer from an interrupt, reading one channel
// at a time. A writer that touches several channels therefore hands that
// interrupt a half-updated frame, and the fixture briefly sees the intermediate
// values - which shows up as flicker or a stray strobe. Writers fill a staging
// copy in RAM instead, and commitDmx() publishes only the channels that differ,
// in one atomic step. Never mutate the DMX buffer in place.

inline uint8_t *dmxStaging() {
  static uint8_t staging[kFixtureChannelCount + 1];
  return staging;
}

inline uint8_t *dmxCommitted() {
  static uint8_t committed[kFixtureChannelCount + 1];
  return committed;
}

inline void clearDmxStaging() {
  uint8_t *staging = dmxStaging();
  for (uint16_t channel = 1; channel <= kFixtureChannelCount; channel++) {
    staging[channel] = 0;
  }
}

inline void dmxWrite(uint16_t channel, uint8_t value) {
  if (channel >= 1 && channel <= kFixtureChannelCount) {
    dmxStaging()[channel] = value;
  }
}

inline void commitDmx() {
  uint8_t *staging = dmxStaging();
  uint8_t *committed = dmxCommitted();

  bool changed = false;
  for (uint16_t channel = 1; channel <= kFixtureChannelCount; channel++) {
    if (staging[channel] != committed[channel]) {
      changed = true;
      break;
    }
  }
  if (!changed) {
    return;  // steady state: the bus is already carrying the right frame
  }

  uint8_t savedStatus = SREG;
  cli();
  for (uint16_t channel = 1; channel <= kFixtureChannelCount; channel++) {
    if (staging[channel] != committed[channel]) {
      DMXSerial.write(channel, staging[channel]);
      committed[channel] = staging[channel];
    }
  }
  SREG = savedStatus;
}

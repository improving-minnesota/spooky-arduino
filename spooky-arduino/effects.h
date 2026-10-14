#pragma once

#include <Arduino.h>
#include <DMXSerial.h>

#include "dmx_map.h"

// Effect library for the SHEHDS bar, lifted from dmx-poc/firmware/dmx-fx.
// Each effect writes the bar channels directly; movement is clamped to the safe
// band so the beams never point at the sky. The R, G, B and W emitters do not
// blend, so "white" is the W channel alone.
//
// The show picks one effect at random per activation and runs it for the whole
// timeout, so the table of effects lives in spooky-arduino.ino.

constexpr uint8_t kMinTilt = 0;    // straight down
constexpr uint8_t kMaxTilt = 120;  // ceiling: ~85 degrees up from down

constexpr int16_t kParkMinUnit = 150;  // ~46 degrees up from down
constexpr int16_t kParkMaxUnit = 210;  // ~65 degrees up from down
constexpr int16_t kJumpMinUnit = 0;
constexpr int16_t kJumpMaxUnit = 255;

constexpr float kWanderUnits = 21.0;  // size of the always-on drift

// 255 BREAKS the tilt on this fixture, so 200 is the maximum usable value.
constexpr uint8_t kSpeedDrift = 100;
constexpr uint8_t kSpeedJump = 200;

// Snake shape (see fxSnake).
constexpr uint8_t kSnakeLength = 4;
constexpr unsigned long kSnakeStepMs = 500;

// Defined in spooky-arduino.ino.
extern int16_t gTiltUnit;
extern uint8_t gSnakeColour;  // 0=R 1=G 2=B 3=W, chosen per activation

inline uint8_t tiltInBand(int16_t unit) {
  if (unit < 0) unit = 0;
  if (unit > 255) unit = 255;
  return kMinTilt + static_cast<uint16_t>(unit) * (kMaxTilt - kMinTilt) / 255;
}

inline void setBar(uint8_t i, uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  uint16_t base = kBar0 + i * 4;
  dmxWrite(base + 0, r);
  dmxWrite(base + 1, g);
  dmxWrite(base + 2, b);
  dmxWrite(base + 3, w);
}

inline void bars(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  for (uint8_t i = 0; i < 8; i++) {
    setBar(i, r, g, b, w);
  }
}

// Three incommensurate sines make a smooth, bounded, non-repeating drift.
inline float wobble(unsigned long now) {
  return sin(now / 1700.0) * 0.55 + sin(now / 630.0) * 0.30 +
         sin(now / 290.0) * 0.15;
}

inline int16_t randomJump() { return random(kJumpMinUnit, kJumpMaxUnit + 1); }

// --- beam effects ----------------------------------------------------------

// Chase: hard steps, one bar at a time.
inline void fxChase(unsigned long now) {
  uint8_t step = (now / 180) % 8;
  for (uint8_t i = 0; i < 8; i++) {
    setBar(i, i == step ? 255 : 0, 0, 0, 0);
  }
}

// Wave: smooth, with a phase offset per bar so it rolls.
inline void fxWave(unsigned long now) {
  for (uint8_t i = 0; i < 8; i++) {
    float phase = now / 900.0 - i * 0.6;
    setBar(i, static_cast<uint8_t>(127 + 127 * sin(phase)), 0, 0, 0);
  }
}

// Comet: a bright head with a fading tail.
inline void fxComet(unsigned long now) {
  uint8_t head = (now / 120) % 8;
  for (uint8_t i = 0; i < 8; i++) {
    uint8_t age = (i - head + 8) % 8;
    setBar(i, age < 3 ? (255 >> age) : 0, 0, 0, 0);
  }
}

// Twinkle: random bars sparkle white.
inline void fxTwinkle(unsigned long now) {
  static unsigned long nextAt = 0;
  if (now < nextAt) {
    return;
  }
  nextAt = now + random(40, 180);
  for (uint8_t i = 0; i < 8; i++) {
    setBar(i, 0, 0, 0, random(0, 4) == 0 ? 200 : 0);
  }
}

// Blood pulse: slow, deep red breathing.
inline void fxBloodPulse(unsigned long now) {
  bars(static_cast<uint8_t>(127 + 127 * sin(now / 1400.0)), 0, 0, 0);
}

// Ripple: a wave expanding from the centre outward.
inline void fxRipple(unsigned long now) {
  for (uint8_t i = 0; i < 8; i++) {
    float d = abs(static_cast<int>(i) * 2 - 7) / 7.0;  // 0 centre, 1 ends
    float phase = now / 700.0 - d * 3.0;
    setBar(i, 0, static_cast<uint8_t>(127 + 127 * sin(phase)), 0, 0);
  }
}

// Snake: a body of adjacent bars crawls slowly along and bounces at each end,
// brightest at the head and fading towards the tail, with a travelling sine so
// it undulates. One random colour per activation (gSnakeColour).
inline void fxSnake(unsigned long now) {
  constexpr uint8_t span = 8 - kSnakeLength;         // last valid start bar
  uint8_t step = (now / kSnakeStepMs) % (2 * span);  // 0..2*span-1
  bool rightward = step < span;
  uint8_t start = rightward ? step : (2 * span - step);  // triangle = bounce

  for (uint8_t i = 0; i < 8; i++) {
    if (i < start || i >= start + kSnakeLength) {
      setBar(i, 0, 0, 0, 0);
      continue;
    }
    uint8_t fromHead = rightward ? (start + kSnakeLength - 1 - i) : (i - start);
    float level = (255.0 / (1 << fromHead)) *  // head 255, then 127, 63, 31
                  (0.75 + 0.25 * sin(now / 180.0 - i * 1.2));  // slither
    uint8_t v = static_cast<uint8_t>(level);
    setBar(i, gSnakeColour == 0 ? v : 0, gSnakeColour == 1 ? v : 0,
           gSnakeColour == 2 ? v : 0, gSnakeColour == 3 ? v : 0);
  }
}

// Colour roll: each bar a different single emitter, rotating at a wandering rate.
inline void fxColourRoll(unsigned long now) {
  static float phase = 0;
  static float rate = 3.0;
  static unsigned long lastMs = 0;
  static unsigned long nextRateAt = 0;

  unsigned long dt = now - lastMs;
  lastMs = now;
  if (now >= nextRateAt) {
    nextRateAt = now + random(600, 2200);
    float magnitude = random(15, 70) / 10.0;  // 1.5 .. 7.0 bars/s
    rate = (random(0, 2) == 0) ? magnitude : -magnitude;
  }
  phase += dt * rate / 1000.0;
  while (phase < 0) phase += 8;
  while (phase >= 8) phase -= 8;

  uint8_t off = static_cast<uint8_t>(phase);
  for (uint8_t i = 0; i < 8; i++) {
    uint8_t c = (i + off) % 4;  // 0=R 1=G 2=B 3=W
    setBar(i, c == 0 ? 220 : 0, c == 1 ? 220 : 0, c == 2 ? 220 : 0, c == 3 ? 220 : 0);
  }
}

// Fire: candle-like flicker on red.
inline void fxFire(unsigned long now) {
  static unsigned long nextAt = 0;
  if (now < nextAt) {
    return;
  }
  nextAt = now + random(20, 90);
  for (uint8_t i = 0; i < 8; i++) {
    setBar(i, static_cast<uint8_t>(random(80, 255)), 0, 0, 0);
  }
}

// --- violent effects: they JUMP position on every flash --------------------

// Random strobe: irregular white flashes, snapping to a new spot each time.
inline void fxRandomStrobe(unsigned long now) {
  static bool on = false;
  static unsigned long at = 0;
  if (now < at) {
    return;
  }
  on = !on;
  at = now + (on ? random(20, 70) : random(40, 400));
  if (on) {
    gTiltUnit = randomJump();
  }
  bars(0, 0, 0, on ? 255 : 0);
}

// Blinder: full white, strobe channel hard on, position snapping often.
inline void fxBlinder(unsigned long now) {
  static unsigned long nextAt = 0;
  if (now >= nextAt) {
    nextAt = now + random(80, 300);
    gTiltUnit = randomJump();
  }
  bars(0, 0, 0, 255);
  dmxWrite(kBarStrobe, 255);
}

// How a bolt travels across the sky: it sweeps out and bounces back, jumps at
// random, or zig-zags from flash to flash. One is chosen per bolt, so no two
// bolts trace the same path.
enum class BoltPath : uint8_t { SWEEP_BACK, RANDOM, ZIG_ZAG };

// Lightning: each flash strikes one or two bars picked at random, and lands at a
// different point across the sky, so it reads as scattered bolts rather than a
// strobe (the whole fixture) or a chase (a single bar stepping). A brief, dim red
// afterglow stays on the bars that were struck, then a short dark gap.
inline void fxLightning(unsigned long now) {
  static uint8_t phase = 0;  // 0 dark, 1 strike, 2 afterglow
  static uint8_t flashesLeft = 0;
  static uint8_t litMask = 0;  // which bars the bolt has struck
  static int16_t strikeUnit = 0;
  static int16_t step = 0;  // signed, so the beam can reverse
  static BoltPath path = BoltPath::RANDOM;
  static bool flashOn = false;
  static unsigned long at = 0;
  static unsigned long nextAt = 0;

  if (phase == 0) {
    if (now >= nextAt) {
      phase = 1;
      flashesLeft = random(3, 7);  // 3-6 flashes per bolt
      strikeUnit = random(0, 256);
      path = static_cast<BoltPath>(random(0, 3));
      step = static_cast<int16_t>(random(20, 70)) * (random(0, 2) == 0 ? 1 : -1);
      flashOn = false;
      at = now;
    }
  } else if (phase == 1) {
    if (now < at) {
      return;  // mid-flash or mid-beat: nothing to change yet
    }
    if (flashOn) {
      flashOn = false;
      if (flashesLeft == 0) {
        phase = 2;
        at = now + random(60, 140);
        bars(0, 0, 0, 0);
        for (uint8_t bar = 0; bar < kBarCount; bar++) {
          if (litMask & (1 << bar)) {
            setBar(bar, 40, 0, 0, 0);  // brief, dim afterglow on the struck bars
          }
        }
      } else {
        bars(0, 0, 0, 0);  // dark beat before the next flash
        // A wide spread here, so the flashes come in an uneven rhythm - a quick
        // double-strike, then a longer pause - rather than ticking evenly.
        at = now + random(20, 160);
      }
    } else {
      flashesLeft--;
      // One or two bars at a time, scattered. A bar struck by the previous flash
      // is likelier to be struck again, so a bolt can linger on the same channel
      // rather than always jumping to new ones.
      uint8_t previous = litMask;
      litMask = 0;
      for (uint8_t bar = 0; bar < kBarCount; bar++) {
        uint8_t chance = (previous & (1 << bar)) ? 45 : 12;
        if (random(0, 100) < chance) {
          litMask |= static_cast<uint8_t>(1 << bar);
        }
      }
      if (litMask == 0) {
        litMask = static_cast<uint8_t>(1 << random(0, kBarCount));
      }
      bars(0, 0, 0, 0);
      for (uint8_t bar = 0; bar < kBarCount; bar++) {
        if (litMask & (1 << bar)) {
          setBar(bar, 0, 0, 0, 255);
        }
      }
      // The bolt moves as it flashes, across the sky by whichever path it drew.
      switch (path) {
        case BoltPath::RANDOM:
          strikeUnit = random(0, 256);
          break;
        case BoltPath::ZIG_ZAG:
          step = static_cast<int16_t>(-step);
          strikeUnit = static_cast<int16_t>(strikeUnit + step);
          break;
        case BoltPath::SWEEP_BACK:
          // Turn at an end rather than sitting against it.
          if (strikeUnit + step < 0 || strikeUnit + step > 255) {
            step = static_cast<int16_t>(-step);
          }
          strikeUnit = static_cast<int16_t>(strikeUnit + step);
          break;
      }
      if (strikeUnit < 0) {
        strikeUnit = 0;
      } else if (strikeUnit > 255) {
        strikeUnit = 255;
      }
      gTiltUnit = strikeUnit;
      flashOn = true;
      at = now + random(20, 45);
    }
  } else if (phase == 2 && now >= at) {
    phase = 0;
    // A wide spread, so bolts come in bursts with longer quiet stretches between
    // them rather than at a steady rate.
    nextAt = now + random(200, 2000);
    bars(0, 0, 0, 0);
  }
}

// --- movement effects: they drive the tilt themselves ----------------------

// Shake: the beam jitters hard around the park.
inline void fxShake(unsigned long now) {
  static unsigned long nextAt = 0;
  static int16_t jitter = 0;
  if (now >= nextAt) {
    nextAt = now + random(30, 100);
    jitter = random(-70, 71);  // up to ~50 degrees
  }
  gTiltUnit = kParkMaxUnit / 2 + jitter;
  dmxWrite(kBarTiltSpeed, kSpeedJump);
  bars(255, 0, 0, 0);
}

// Pendulum: the beam swings, easing at the ends.
inline void fxPendulum(unsigned long now) {
  gTiltUnit = static_cast<int16_t>(((sin(now / 900.0) + 1) / 2) * 255);
  dmxWrite(kBarTiltSpeed, kSpeedJump);
  bars(255, 0, 0, 0);
}

// Crawl: the beam creeps across the band, slow and uneasy.
inline void fxCrawl(unsigned long now) {
  gTiltUnit = static_cast<int16_t>((now / 90) % 256);
  dmxWrite(kBarTiltSpeed, 90);
  bars(150, 0, 0, 0);
}

// How each effect moves the beam: DRIFT gets the always-on wander added, JUMP
// uses the snappy tilt speed, OWN drives the tilt itself.
enum Motion : uint8_t { DRIFT, JUMP, OWN };
typedef void (*FxFn)(unsigned long now);
struct Fx {
  FxFn fn;
  Motion motion;
};

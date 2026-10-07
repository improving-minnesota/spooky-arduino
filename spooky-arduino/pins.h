#pragma once

#include <Arduino.h>

// Central pin map for the spooky-arduino firmware. Change pins here only.

constexpr uint8_t kOnboardLedPin = LED_BUILTIN;  // D13
constexpr uint8_t kIrReceiverPin = 2;            // D2
constexpr uint8_t kPirPin = 4;                   // D4

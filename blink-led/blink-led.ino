#include "pins.h"

constexpr unsigned long kBlinkIntervalMs = 1000;

unsigned long lastToggleMs = 0;
bool ledIsOn = false;

void setup() {
  pinMode(kOnboardLedPin, OUTPUT);
}

void loop() {
  if (millis() - lastToggleMs >= kBlinkIntervalMs) {
    lastToggleMs += kBlinkIntervalMs;
    ledIsOn = !ledIsOn;
    digitalWrite(kOnboardLedPin, ledIsOn ? HIGH : LOW);
  }
}

#include <EEPROM.h>
#include <IRremote.hpp>

#include "ir_codes.h"
#include "pins.h"
#include "settings.h"

// Compile-time switches. Set IR_CAPTURE_MODE to 1 to print decoded frames and
// record button codes (see README.md); leave it 0 for normal operation.
#define IR_CAPTURE_MODE 0
#define PIR_DEBUG 0

// The HC-SR501 emits false triggers while it stabilizes after power-on; ignore
// its output for this long so the LED does not turn on spuriously at boot.
constexpr unsigned long kPirWarmupMs = 60000;

// Preview: one visible flash per second, counted out to show the new timeout.
constexpr unsigned long kPreviewPeriodMs = 1000;
constexpr unsigned long kPreviewFlashOnMs = 250;

Settings settings;

// Steady LED state: on until the configured timeout elapses with no motion.
bool ledOn = false;
unsigned long ledLastActivationMs = 0;

// Transient preview overlay. It never changes the steady state; it only drives
// the LED output while active.
bool previewActive = false;
bool previewFlashing = false;
uint16_t previewFlashesLeft = 0;
unsigned long previewTickMs = 0;
unsigned long previewFlashStartMs = 0;

unsigned long bootMs = 0;
bool motionPrev = false;

void refreshLed() {
  bool out = previewActive ? previewFlashing : ledOn;
  digitalWrite(kOnboardLedPin, out ? HIGH : LOW);
}

void activateLed() {
  previewActive = false;
  previewFlashing = false;
  ledOn = true;
  ledLastActivationMs = millis();
}

void activateOff() {
  previewActive = false;
  previewFlashing = false;
  ledOn = false;
}

void beginPreviewFlash(unsigned long now) {
  previewFlashing = true;
  previewFlashStartMs = now;
}

void startPreview() {
  previewFlashesLeft = settings.timeoutSec;
  previewFlashing = false;
  previewTickMs = millis();
  previewActive = previewFlashesLeft > 0;
  if (previewActive) {
    previewFlashesLeft--;
    beginPreviewFlash(previewTickMs);
  }
}

int ladderIndex(uint16_t seconds) {
  for (uint8_t i = 0; i < kTimeoutLadderSize; i++) {
    if (kTimeoutLadderSec[i] == seconds) {
      return i;
    }
  }
  return -1;
}

void saveSettings() {
  EEPROM.put(0, settings);
}

void loadSettings() {
  EEPROM.get(0, settings);
  if (settings.magic != kSettingsMagic || settings.version != kSettingsVersion
      || settings.timeoutSec < kMinTimeoutSec
      || settings.timeoutSec > kMaxTimeoutSec
      || ladderIndex(settings.timeoutSec) < 0) {
    settings.magic = kSettingsMagic;
    settings.version = kSettingsVersion;
    settings.timeoutSec = kDefaultTimeoutSec;
    saveSettings();
  }
}

void stepTimeout(int direction) {
  int index = ladderIndex(settings.timeoutSec);
  if (index < 0) {
    index = ladderIndex(kDefaultTimeoutSec);
  }
  int next = index + direction;
  if (next < 0) {
    next = 0;
  } else if (next >= kTimeoutLadderSize) {
    next = kTimeoutLadderSize - 1;
  }
  uint16_t value = kTimeoutLadderSec[next];
  if (value != settings.timeoutSec) {
    settings.timeoutSec = value;
    saveSettings();
  }
  startPreview();
}

void handleIr() {
  if (!IrReceiver.decode()) {
    return;
  }

  bool isRepeat = IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT;
  if (!isRepeat && IrReceiver.decodedIRData.protocol == kRemoteProtocol
      && IrReceiver.decodedIRData.address == kRemoteAddress) {
    uint8_t command = IrReceiver.decodedIRData.command;
    if (command == kButtonPower) {
      if (ledOn) {
        activateOff();
      } else {
        activateLed();
      }
    } else if (command == kButtonVolumeUp) {
      stepTimeout(+1);
    } else if (command == kButtonVolumeDown) {
      stepTimeout(-1);
    }
  }

#if IR_CAPTURE_MODE
  Serial.print(F("ir t="));
  Serial.print(millis());
  Serial.print(F(" protocol="));
  Serial.print(IrReceiver.getProtocolString());
  Serial.print(F(" protocol_id="));
  Serial.print(static_cast<int>(IrReceiver.decodedIRData.protocol));
  Serial.print(F(" address=0x"));
  Serial.print(IrReceiver.decodedIRData.address, HEX);
  Serial.print(F(" command=0x"));
  Serial.print(IrReceiver.decodedIRData.command, HEX);
  Serial.print(F(" raw=0x"));
  Serial.print(IrReceiver.decodedIRData.decodedRawData, HEX);
  Serial.print(F(" repeat="));
  Serial.println(isRepeat ? 1 : 0);
#endif

  IrReceiver.resume();
}

void updateMotion() {
  unsigned long now = millis();
  bool present = digitalRead(kPirPin) == HIGH;

  if (now - bootMs < kPirWarmupMs) {
    motionPrev = present;
    return;
  }

  if (present && !motionPrev) {
    activateLed();
  } else if (present && ledOn) {
    ledLastActivationMs = now;
  }
  motionPrev = present;

#if PIR_DEBUG
  static bool printed = false;
  if (present != printed) {
    printed = present;
    Serial.print(F("pir t="));
    Serial.print(now);
    Serial.println(present ? F(" present") : F(" clear"));
  }
#endif
}

void updateSteady() {
  if (ledOn && millis() - ledLastActivationMs >= settings.timeoutSec * 1000UL) {
    ledOn = false;
  }
}

void updatePreview() {
  if (!previewActive) {
    return;
  }
  unsigned long now = millis();
  if (previewFlashing) {
    if (now - previewFlashStartMs >= kPreviewFlashOnMs) {
      previewFlashing = false;
      if (previewFlashesLeft == 0) {
        previewActive = false;
      }
    }
  } else if (previewFlashesLeft > 0 && now - previewTickMs >= kPreviewPeriodMs) {
    previewTickMs = now;
    previewFlashesLeft--;
    beginPreviewFlash(now);
  }
}

void setup() {
  pinMode(kOnboardLedPin, OUTPUT);
  pinMode(kPirPin, INPUT);
  IrReceiver.begin(kIrReceiverPin, DISABLE_LED_FEEDBACK);
  pinMode(kIrReceiverPin, INPUT_PULLUP);

  loadSettings();
  bootMs = millis();

#if IR_CAPTURE_MODE || PIR_DEBUG
  Serial.begin(9600);
  Serial.print(F("spooky-arduino ready, timeout="));
  Serial.print(settings.timeoutSec);
  Serial.println(F("s"));
#endif

  refreshLed();
}

void loop() {
  handleIr();
  updateMotion();
  updateSteady();
  updatePreview();
  refreshLed();
}

#include <EEPROM.h>
#include <IRremote.hpp>
#include <DMXSerial.h>

#include "dmx_map.h"
#include "effects.h"
#include "haze.h"
#include "ir_codes.h"
#include "pins.h"
#include "settings.h"

// The board's single USART is owned by DMX output, so there is no USB serial
// monitor: on the Uno R3 the USB-serial converter is wired to the same D0/D1
// pins. tools/dmx_monitor.py, listening on the DMX line, is the window into the
// board instead.

// The HC-SR501 emits false triggers while it stabilizes after power-on; ignore
// its output for this long so the show does not start spuriously at boot.
constexpr unsigned long kPirWarmupMs = 60000;

// Timeout preview: blink the onboard LED quickly, once per step of the new
// timeout, to show the value without the long once-per-second count.
constexpr unsigned long kPreviewPeriodMs = 200;
constexpr unsigned long kPreviewFlashOnMs = 100;

// Cleaning: hold EQ this long to enter; blink the LED this fast while cleaning.
constexpr unsigned long kCleaningHoldMs = 3000;
constexpr unsigned long kCleaningBlinkMs = 100;

// Park position for the idle look (within the safe band). The bar is dark.
constexpr uint8_t kIdleTilt = 56;

Settings settings;

// --- show state ------------------------------------------------------------

// One arbiter for every output. OFF means the device is disabled: everything is
// silent and the PIR is ignored until the power button enables it. IDLE means
// enabled and waiting - the fixture is dark until an event arrives. ACTIVE means
// an effect is running. CLEANING overrides all.
enum class ShowState : uint8_t { OFF, IDLE, ACTIVE, CLEANING };
ShowState showState = ShowState::IDLE;  // enabled at startup, no power press needed

uint8_t currentEffect = 0;
unsigned long showDeadlineMs = 0;
// What the device was doing before cleaning took over, so it can be resumed.
ShowState stateBeforeCleaning = ShowState::OFF;

// Haze machine master enable, toggled by play/pause (on at startup).
bool hazeEnabled = true;

// The set of machine settings the device is using, which is also the set the
// remote edits: the idle set while the device is idle, the event set while an
// effect runs. That is what lets the blower be quiet between effects and full
// during one, each set by changing it while the device is in that state. Both
// sets live in the persisted settings, so they survive a reboot.
HazeSettings &activeHaze() {
  return (showState == ShowState::ACTIVE) ? settings.haze.event : settings.haze.idle;
}

// EQ hold tracking (cleaning is entered from IR repeat frames).
bool eqHeld = false;
unsigned long eqDownMs = 0;

// Transient timeout preview overlay on the LED.
bool previewActive = false;
bool previewFlashing = false;
uint16_t previewFlashesLeft = 0;
unsigned long previewTickMs = 0;
unsigned long previewFlashStartMs = 0;

// Cleaning LED blink.
bool cleaningBlinkOn = false;
unsigned long cleaningBlinkMs = 0;

unsigned long bootMs = 0;
bool motionPrev = false;

// Effect globals (declared extern in effects.h).
int16_t gTiltUnit = kParkMinUnit;
uint8_t gSnakeColour = 0;

// The effect library, one entry per effect with its movement style. The show
// draws one of these at random per activation.
const Fx kEffects[] = {
    {fxChase, DRIFT},      {fxWave, DRIFT},        {fxComet, DRIFT},
    {fxSnake, DRIFT},      {fxColourRoll, DRIFT},  {fxRipple, DRIFT},
    {fxTwinkle, DRIFT},    {fxFire, DRIFT},        {fxBloodPulse, DRIFT},
    {fxRandomStrobe, JUMP}, {fxBlinder, JUMP},     {fxLightning, JUMP},
    {fxShake, OWN},        {fxPendulum, OWN},      {fxCrawl, OWN}};
constexpr uint8_t kFxCount = sizeof(kEffects) / sizeof(kEffects[0]);

// The haze buttons only act when the machine is enabled and the show is not off
// or cleaning.
bool hazeControlsActive() {
  return hazeEnabled && showState != ShowState::OFF
         && showState != ShowState::CLEANING;
}

// --- transitions -----------------------------------------------------------

// Choose the next effect and restart its clock. Shared by every path that moves
// the show on, so the randomisation lives in one place.
void pickNewEffect(unsigned long now) {
  // A new effect starts from a clean slate. The staging buffer deliberately
  // persists between passes, so the previous effect's channels must be cleared
  // here instead: effects that hold a state between updates - lightning's flash,
  // fire and twinkle's sparkle - would otherwise be wiped before their output
  // ever reached the bus.
  clearDmxStaging();
#ifdef LIGHTNING_ONLY
  // Tuning build: always the lightning, so it can be watched and adjusted
  // without waiting for the rotation to reach it.
  currentEffect = 11;  // fxLightning in the kEffects table
#else
  currentEffect = random(0, kFxCount);
#endif
  gSnakeColour = random(0, 4);
  gTiltUnit = random(kParkMinUnit, kParkMaxUnit + 1);
  showDeadlineMs = now + settings.timeoutSec * 1000UL;
}

// An event - enabling the device, a single PIR event, or an EQ press - starts a
// new random effect and restarts the timeout, so pressing eq during an effect
// moves straight on to another one. Nothing happens while the device is disabled
// or cleaning.
void activateShow(unsigned long now) {
  if (showState == ShowState::OFF || showState == ShowState::CLEANING) {
    return;
  }
  pickNewEffect(now);
  showState = ShowState::ACTIVE;
}

void togglePower(unsigned long now) {
  if (showState == ShowState::CLEANING) {
    return;
  }
  if (showState == ShowState::OFF) {
    showState = ShowState::IDLE;  // enable: on but dark, awaiting an event
    activateShow(now);
  } else {
    showState = ShowState::OFF;  // disable, from either IDLE or ACTIVE
  }
}

// play/pause enables or disables the haze machine. Disabled forces the machine
// off and makes the haze buttons inert; both settings sets are left untouched,
// so the machine comes back exactly as it was.
void toggleHazeEnable() {
  if (showState == ShowState::CLEANING) {
    return;
  }
  hazeEnabled = !hazeEnabled;
}

void startCleaning() {
  previewActive = false;
  previewFlashing = false;
  cleaningBlinkOn = false;
  cleaningBlinkMs = millis();
  stateBeforeCleaning = showState;
  showState = ShowState::CLEANING;
}

// Leaving cleaning resumes whatever the device was doing before it.
void stopCleaning() {
  showState = stateBeforeCleaning;
}

void stepHazeRate(int direction) {
  HazeSettings &settings = activeHaze();
  int next = static_cast<int>(settings.rateStep) + direction;
  if (next < 0) {
    next = 0;
  } else if (next >= kHazeLadderSize) {
    next = kHazeLadderSize - 1;
  }
  settings.rateStep = static_cast<uint8_t>(next);
  saveSettings();
}

// --- DMX output ------------------------------------------------------------

// Park the bar: clear the strobe and built-in-effect channels, park it in the
// safe band, and leave it dark.
void parkBar() {
  dmxWrite(kBarStrobe, 0);
  dmxWrite(kBarEffect, 0);
  dmxWrite(kBarEffectSpeed, 0);
  dmxWrite(kBarTiltSpeed, 0);
  dmxWrite(kBarTilt, kIdleTilt);
  bars(0, 0, 0, 0);
}

void renderLights(unsigned long now) {
  // The original set the master dimmer once at boot and left it alone. The
  // staging buffer is cleared every pass, so it has to be restated here; the
  // commit then sees no change and puts nothing extra on the bus.
  dmxWrite(kBarDimmer, 255);

  if (showState == ShowState::ACTIVE) {
    const Fx &fx = kEffects[currentEffect];
    dmxWrite(kBarTiltSpeed, fx.motion == JUMP ? kSpeedJump : kSpeedDrift);
    fx.fn(now);
    int16_t unit = gTiltUnit;
    if (fx.motion == DRIFT) {
      unit += static_cast<int16_t>(wobble(now) * kWanderUnits);
    }
    dmxWrite(kBarTilt, tiltInBand(unit));
  } else {
    parkBar();
  }
}

void renderHaze() {
  uint8_t bubbles = 0, volume = 0, blower = 0, cleaning = 0;
  if (showState == ShowState::CLEANING) {
    cleaning = kCleaningOn;  // bubble pump only; everything else off
  } else if (showState == ShowState::OFF || !hazeEnabled) {
    // machine off
  } else {
    // Enabled: the machine runs from whichever set matches the current state.
    // Each set defaults to something sensible - idle off, an event foggy and
    // loud - and the remote edits the active one.
    const HazeSettings &settings = activeHaze();
    bubbles = settings.bubbles ? kBubblesOn : 0;
    volume = settings.hazeOn ? hazeRateValue(settings.rateStep) : 0;
    blower = settings.blower;
  }
  dmxWrite(kHazeBubbles, bubbles);
  dmxWrite(kHazeVolume, volume);
  dmxWrite(kHazeBlower, blower);
  dmxWrite(kHazeCleaning, cleaning);
}

// --- timeout preview -------------------------------------------------------

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

// --- settings --------------------------------------------------------------

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
      || ladderIndex(settings.timeoutSec) < 0
      || settings.haze.idle.rateStep >= kHazeLadderSize
      || settings.haze.event.rateStep >= kHazeLadderSize) {
    settings.magic = kSettingsMagic;
    settings.version = kSettingsVersion;
    settings.timeoutSec = kDefaultTimeoutSec;
    settings.haze.idle = kHazeIdleDefaults;
    settings.haze.event = kHazeEventDefaults;
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

// --- IR --------------------------------------------------------------------

void onEqPress(unsigned long now) {
  eqHeld = true;
  eqDownMs = now;
  // Fire straight away. Waiting for a gap in the remote's repeat frames made a
  // single press depend on the remote's repeat timing, which is not something
  // the firmware should rely on: press eq and an effect starts, full stop.
  activateShow(now);
}

void onEqRepeat(unsigned long now) {
  if (eqHeld && showState != ShowState::CLEANING && now - eqDownMs >= kCleaningHoldMs) {
    eqHeld = false;
    startCleaning();
  }
}

void handleButton(uint8_t command, unsigned long now) {
  if (showState == ShowState::CLEANING) {
    if (command == kButtonEq) {
      stopCleaning();  // EQ is the only button that acts while cleaning
    }
    return;
  }
  switch (command) {
    case kButtonEq: onEqPress(now); break;
    case kButtonPower: togglePower(now); break;
    case kButtonPlayPause: toggleHazeEnable(); break;
    case kButtonVolumeUp: stepTimeout(+1); break;
    case kButtonVolumeDown: stepTimeout(-1); break;
    case kButton0: if (hazeControlsActive()) { activeHaze().blower = kBlowerOff; saveSettings(); } break;
    case kButton1: if (hazeControlsActive()) { activeHaze().blower = kBlowerLow; saveSettings(); } break;
    case kButton2: if (hazeControlsActive()) { activeHaze().blower = kBlowerMedium; saveSettings(); } break;
    case kButton3: if (hazeControlsActive()) { activeHaze().blower = kBlowerHigh; saveSettings(); } break;
    case kButtonStRept: if (hazeControlsActive()) { activeHaze().bubbles = !activeHaze().bubbles; saveSettings(); } break;
    case kButtonFuncStop: if (hazeControlsActive()) { activeHaze().hazeOn = !activeHaze().hazeOn; saveSettings(); } break;
    case kButtonTrackLeft: if (hazeControlsActive()) stepHazeRate(-1); break;
    case kButtonTrackRight: if (hazeControlsActive()) stepHazeRate(+1); break;
    default: break;
  }
}

void handleIr() {
  if (!IrReceiver.decode()) {
    return;
  }

  bool isRepeat = IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT;
  if (IrReceiver.decodedIRData.protocol == kRemoteProtocol
      && IrReceiver.decodedIRData.address == kRemoteAddress) {
    uint8_t command = IrReceiver.decodedIRData.command;
    if (isRepeat) {
      // Repeats only matter for the EQ hold; holding other buttons is ignored.
      if (command == kButtonEq) {
        onEqRepeat(millis());
      }
    } else {
      handleButton(command, millis());
    }
  }

  IrReceiver.resume();
}

// --- motion and timeouts ---------------------------------------------------

void updateMotion() {
  unsigned long now = millis();
  bool present = digitalRead(kPirPin) == HIGH;

  if (now - bootMs < kPirWarmupMs) {
    motionPrev = present;
    return;
  }

  if (present && !motionPrev) {
    activateShow(now);  // no-op while disabled or cleaning
  } else if (present && showState == ShowState::ACTIVE) {
    showDeadlineMs = now + settings.timeoutSec * 1000UL;
  }
  motionPrev = present;
}

void updateTimeout() {
#ifdef LIGHTNING_ONLY
  // Tuning build: never fall back to idle, so the lightning runs without gaps.
  return;
#endif
  // The effect's time is up: the fixture goes dark and the device waits in idle
  // for the next event.
  if (showState == ShowState::ACTIVE && millis() >= showDeadlineMs) {
    showState = ShowState::IDLE;
  }
}

void updateCleaningBlink() {
  if (showState != ShowState::CLEANING) {
    cleaningBlinkOn = false;
    return;
  }
  unsigned long now = millis();
  if (now - cleaningBlinkMs >= kCleaningBlinkMs) {
    cleaningBlinkMs = now;
    cleaningBlinkOn = !cleaningBlinkOn;
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

// The LED is the device status: on whenever the device is enabled, whether an
// effect is running or it is idle and waiting. The timeout preview and the
// cleaning blink overlay it.
void refreshLed() {
  bool out;
  if (previewActive) {
    out = previewFlashing;
  } else if (showState == ShowState::CLEANING) {
    out = cleaningBlinkOn;
  } else {
    out = showState != ShowState::OFF;
  }
  digitalWrite(kOnboardLedPin, out ? HIGH : LOW);
}

// --- setup / loop ----------------------------------------------------------

void setup() {
  pinMode(kOnboardLedPin, OUTPUT);
  pinMode(kPirPin, INPUT);
  IrReceiver.begin(kIrReceiverPin, DISABLE_LED_FEEDBACK);
  pinMode(kIrReceiverPin, INPUT_PULLUP);

  DMXSerial.init(DMXController, kDmxModePin);
  DMXSerial.maxChannel(512);

  loadSettings();
  randomSeed(analogRead(A0));
  bootMs = millis();

  dmxWrite(kBarDimmer, 255);

#ifdef HAZE_BENCH
  // Bench build: enabled and idle with the haze and bubbles on, no effects, so
  // the only non-zero channels beyond the bar are the machine's. For checking
  // what the fixture does when channels 39 and 40 carry data.
  showState = ShowState::IDLE;
  settings.haze.idle.hazeOn = true;
  settings.haze.idle.bubbles = true;
#endif
  commitDmx();

  // The device comes up enabled and already running, rather than waiting for a
  // power press.
  activateShow(millis());

  refreshLed();
}

void loop() {
  unsigned long now = millis();
  handleIr();
  updateMotion();
  updateTimeout();
  updateCleaningBlink();
  updatePreview();
  renderLights(now);
  renderHaze();
  refreshLed();

  // Publish the frame in one atomic step, so the transmit interrupt can never
  // catch the buffer mid-update.
  commitDmx();
}

## Why

The project has two standalone sketches that each prove one thing (`blink-led`, `ir-led-control`), but the board runs a single program, and the next capabilities — motion-triggered lighting and, later, RS485/DMX output — must share one firmware with one pin, timer, and serial budget. Deferring that merge to a big-bang integration at the end would concentrate the riskiest work (resource conflicts and a shared LED state) at the point where it is hardest to debug. This change makes `spooky-arduino` the single firmware, folds in the existing LED-output and IR-control behavior, retires the throwaway `blink-led` smoke test, and adds the first genuinely new project behavior: motion-triggered lighting with a configurable, persistent timeout.

## What Changes

- Add the consolidated `spooky-arduino` sketch as the project's single firmware, folding in the onboard-LED output and IR-remote control behavior currently split across `blink-led` and `ir-led-control`.
- **BREAKING**: retire the `blink-led` heartbeat sketch and the `led-blink` capability — the LED no longer blinks as a heartbeat; it is now driven by motion and remote input. Retire the standalone `ir-led-control` sketch as its behavior moves into `spooky-arduino`.
- Add HC-SR501 PIR motion detection on `D4`: motion presence turns the onboard LED on, and the LED turns off after the configured timeout once motion is no longer present.
- Add a configurable timeout (default 5 s, range 1 s–5 min) stored in EEPROM so it survives power cycles, adjustable from the IR remote's vol+/vol- buttons along a stepped ladder, with an on-LED flash preview that counts out the new value.
- Change the remote power button semantics: pressing it with the LED off turns the LED on for the configured timeout (not indefinitely); pressing it while the LED is on forces the LED off until the next activation.
- Decode and record every button on the kit's 21-button remote so later features can bind to them.
- No change to the IR decoding approach; the receiver stays on `D2`.

## Capabilities

### New Capabilities
- `motion-triggered-lighting`: motion detected by the HC-SR501 turns the onboard LED on; the LED turns off after the configured timeout once motion is no longer present.
- `configurable-timeout`: the LED-on timeout (default 5 s, 1 s–5 min) is adjustable from the remote's vol+/vol- buttons along a stepped ladder, is previewed on the LED, and persists in EEPROM across power cycles.

### Modified Capabilities
- `ir-led-control`: the power button turns the LED on for the configured timeout (was: indefinitely) and forces it off when already on; the vol+/vol- buttons adjust the configured timeout; all kit-remote buttons are decoded and recorded, with only the power and vol buttons acting on the LED.
- `led-blink`: retired — all requirements removed; the continuous heartbeat blink is superseded by the consolidated firmware's motion- and remote-driven LED behavior.

## Impact

- **Code**: new `spooky-arduino/` sketch folder (`.ino`, `pins.h`, `ir_codes.h`, settings/EEPROM handling); the `blink-led/` and `ir-led-control/` sketch folders are retired.
- **Dependencies**: unchanged — Arduino AVR core plus `IRremote` (already installed); the `EEPROM` library ships with the core.
- **Hardware**: HC-SR501 PIR wired to `D4` (5 V, GND); IR receiver stays on `D2`; onboard LED on `D13`.
- **Resources**: `IRremote` owns Timer2 (so PWM on `D3`/`D11` and `tone()` stay unavailable); the single USART remains reserved for capture/debug and, later, DMX; EEPROM holds a small settings struct.
- **Docs**: `README.md` updated to describe `spooky-arduino` and the retired sketches; the remote button map recorded.
- **Future**: `spooky-arduino` becomes the integration point for the RS485/DMX capability.

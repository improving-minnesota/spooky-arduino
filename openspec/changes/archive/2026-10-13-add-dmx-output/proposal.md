## Why

`spooky-arduino` currently drives only the onboard LED; the project's actual lighting — a SHEHDS 8×12W RGBW beam bar and a Chauvet Hurricane Bubble Haze — is unreachable. Adding DMX512 output turns the firmware into a real controller: the IR remote becomes a lighting desk and the PIR becomes a scare trigger. It also adds the project's first end-to-end test path — a white-chase test pattern that makes a working DMX path obvious on the fixture itself — so wiring and signal integrity can be proven before anything consumes haze or bubble fluid.

## What Changes

- Add DMX512 output on the Uno's hardware USART (TX → `D1`) through a MAX485 transceiver, with the half-duplex direction pin (DE/RE) on `D5`, transmitting a 512-channel universe at 250 kbaud, 8N2, with break/MAB. The USB serial monitor is unavailable while transmitting.
- Add fixture addressing: the beam bar at DMX address 1 in 38-channel mode (channels 1–38) and the haze/bubble machine at address 39 (channels 39–42).
- Add beam-bar control: horizontal position (capped so beams never face the sky, with an idle park), horizontal speed kept inside what the fixture tolerates, master dimming held at full, strobe, and independent per-bar R/G/B/W dimming. Colors are independent and never mixed: at most one emitter per beam is lit at a time, because R, G, B, and W on the same beam read as separate colors rather than blending into magenta or white.
- Add haze/bubble control: haze on/off, haze volume steps, bubble on/off, blower speed off/low/medium/high, and bubble cleaning mode — plus a `play/pause` master enable that silences the machine so the lights can be tested without consuming haze.
- Add a roster of fifteen named spooky effects (chase, wave, comet, snake, colour roll, ripple, twinkle, fire, blood pulse, random strobe, blinder, lightning, shake, pendulum, crawl). Each runs for the configured duration, lights at most one emitter per beam, and is classified by how it moves the beam. The haze machine is independent: it holds separate idle and event settings and uses whichever matches the device's current state.
- **BREAKING**: the remote power button becomes the device's master enable/disable (was: turn the onboard LED on for the configured timeout), and the device comes up enabled and already running rather than waiting for a press. The onboard LED becomes a status light reporting that state, with the duration preview and the cleaning-mode flash overlaying it.
- Change what the remote's vol+/vol- buttons target: they now set the shared effect duration, previewed as fast blinks counting out the new value.
- Change PIR behavior: a single motion event starts an effect while the device is enabled, instead of turning the onboard LED on.
- Add the device remote mappings: play/pause toggles the haze master enable, func/stop toggles haze, trackleft/trackright adjust haze volume, st/rept toggles bubbles, `0`–`3` set blower speed, a single eq press starts an effect (or exits cleaning mode), and holding eq for three seconds stops all effects and enters bubble cleaning mode. `9` is unused.
- Add host-side bench tooling: a monitor that reads the universe off the bus, a script that names the effect from its channel signature, and a build flag that holds the machine's channels active for noise and wiring checks.
- Remove the `IR_CAPTURE_MODE`/`PIR_DEBUG` serial logging switches. They need the same single USART as DMX output, and leaving them in place lets a build silently turn DMX off; `ir_codes.h` already holds the captured button map.

## Capabilities

### New Capabilities
- `dmx-output`: the DMX512 transmit layer — a 512-channel universe, fixture addressing, the 250 kbaud 8N2 frame with break/MAB, half-duplex direction control, non-blocking transmit, atomic frame publication, the dark parked output while the device is disabled, and the contract that a compliant DMX receiver decodes the intended channel values.
- `beam-bar-control`: the SHEHDS 8×12W RGBW beam bar's channel semantics — horizontal position with a sky-safe ceiling and an idle park, horizontal speed inside what the fixture tolerates, master dimming held at full, strobe, and independent per-bar R/G/B/W dimming that never blends.
- `haze-bubble-control`: the Chauvet Hurricane Bubble Haze's channel semantics — haze on/off, haze volume steps, bubble on/off, blower speed steps, and bubble cleaning mode — plus the `play/pause` master enable and the two independent settings sets, idle and event, the machine follows in each state.
- `spooky-effects`: the roster of fifteen named Halloween effects, how one is chosen and run for the configured duration, how each is classified by the way it moves the beam, and how an effect drives the haze and bubble machine while it runs.

### Modified Capabilities
- `ir-led-control`: the power button becomes the device's master enable/disable (was: LED on-for-timeout toggle) and the onboard LED becomes a status light reporting that state, previewing the duration, and flashing in cleaning mode; the remote's other buttons now act on the effects and the haze machine rather than the LED.
- `configurable-timeout`: the stored value becomes the shared effect duration and no longer governs an LED-on timeout; the preview indication counts out the new value in fast blinks.
- `motion-triggered-lighting`: a single PIR motion event triggers an effect (only while the rig is on) instead of turning the onboard LED on.

## Impact

- **Code**: `spooky-arduino/` gains a DMX layer and fixture/effect modules (`.ino`, plus new headers for the universe map, fixture channel maps, effects, and rig state); `pins.h`, `settings.h`, and `ir_codes.h` are extended.
- **Dependencies**: add the `DMXSerial` library (new; owns the hardware USART and a direction pin). `IRremote` and the AVR core are unchanged. Because `DMXSerial` takes the single USART, the `IR_CAPTURE_MODE`/`PIR_DEBUG` serial logging is removed rather than kept as a mutually-exclusive build mode.
- **Hardware**: MAX485 (or equivalent) transceiver — DI → `D1`, DE/RE → `D5`, 5 V/GND, with DMX termination at the controller node; beam bar at address 1 (38-channel mode); Chauvet Hurricane Bubble Haze at address 39. Verification is done with the white-chase test pattern on the fixture: the DSD Tech SH-RS09B is a DMX *controller* cable with no usable receive path, so it cannot monitor the bus.
- **Resources**: `DMXSerial` allocates a fixed 513-byte universe buffer in SRAM, which is significant on the Uno's 2 KB alongside `IRremote`; the design keeps other state small. `D5` becomes the direction pin and `D2` stays the IR receiver — `DMXSerial`'s default direction pin is `D2`, so it must be overridden.
- **Tooling**: a small host-side monitor script (Mac, `pyserial`, 250 kbaud 8N2) reads the universe off the bus when a bidirectional USB-RS485 adapter is available; QLC+ is noted as a reference for fixture channel semantics.
- **Docs**: `README.md` gains the DMX wiring, fixture addresses, channel maps, remote map, and the monitor procedure.

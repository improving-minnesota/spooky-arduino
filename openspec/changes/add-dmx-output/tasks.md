# Tasks

## 1. Toolchain and shared maps

- [x] 1.1 Add `dmx_map.h`: the fixture base addresses (beam bar 1, haze 39), the per-fixture channel offsets, the value bands, and the staging/commit layer. Touches the shared DMX layer. Verify the sketch compiles.
- [x] 1.2 Add `pins.h`: onboard LED `D13`, IR receiver `D2`, PIR `D4`, DMX direction `D5`. Touches the board pinout. Verify the DMX direction pin is `D5` and not `D2`, so the IR receiver is never disturbed.
- [x] 1.3 Write `tools/dmx_monitor.py`: read the DMX path at 250000 8N2, resync on the start code, print channels 1–42. Touches the bench tooling. Verify it frames a live stream and reports the channel values.
- [x] 1.4 Write `tools/dmx_effects.py`: name the running effect from the bus so the roster can be verified without watching for it. Touches the bench tooling. Verify it identifies effects from their channel signature.

## 2. DMX transmit layer

- [x] 2.1 Transmit a 512-channel universe in controller mode with `DMXSerial`, direction pin `D5`. Touches the USART. Verify a monitor reads a continuous stream of frames beginning with the start code.
- [x] 2.2 Drive the beam bar at address 1 and the haze machine at address 39. Touches both fixtures. Verify a monitor shows the two fixtures' blocks carrying distinct, correct values.
- [x] 2.3 Black the fixture out and park the bar while the device is disabled, while still transmitting. Touches both fixtures. Verify the bar is dark and parked and a monitor still reads valid frames.
- [x] 2.4 Remove the `IR_CAPTURE_MODE`/`PIR_DEBUG` serial logging switches. Touches the USART. Verify the sketch compiles with no `Serial` code and `setup()` always initialises DMX.
- [x] 2.5 Stage channel writes and publish the frame atomically, so the transmit interrupt can never sample a half-updated frame. Touches the shared DMX layer. Verify a pattern's frames are consistently whole and the fixture flicker is gone.
- [x] 2.6 Restate the master dimmer each pass, because the staging buffer is cleared every pass and the dimmer is otherwise set only at boot. Touches the beam bar (channel 5). Verify channel 5 reads full in every device state.
- [x] 2.7 Confirm IR input still decodes while transmitting. Touches `D2`. Verify a remote press is acted on while DMX output is active.
- [x] 2.8 Keep the staging buffer persistent between passes and clear it only when a new effect starts. Touches the shared DMX layer. Verify an effect that holds a value between updates, such as lightning's flash or fire's sparkle, keeps that value on the bus, while the blinder's strobe channel still does not leak into the next effect.

## 3. Fixture channel layer

- [x] 3.1 Implement the beam bar's 38 channels: channels 3 and 4 held at 0, the master dimmer at 5, the strobe at 6, and the eight bars' R/G/B/W at 7–38. Touches the beam bar. Verify a static pattern lights the expected bars and colours.
- [x] 3.2 Bound the horizontal position to 0–120 and park it at 56 when no effect drives it. Touches the beam bar (channel 1). Verify the idle value parks at 56 and no effect drives it above 120.
- [x] 3.3 Keep the horizontal speed channel inside what the fixture tolerates, because 255 breaks its tilt. Touches the beam bar (channel 2). Verify no effect sets a value above 200.
- [x] 3.4 Implement the haze machine's four channels at 39–42 within its documented bands. Touches the Chauvet machine. Verify a monitor reads the intended values for each function, or the machine responds as intended.
- [x] 3.5 Enforce the single-emitter rule: at most one of a beam's four emitters above zero at any moment, with white as the W channel alone. Touches the beam bar. Verify no effect lights two emitters on one beam.

## 4. Device state and remote dispatch

- [x] 4.1 Implement the device state machine: `OFF`, `IDLE`, `ACTIVE`, `CLEANING`. Touches the shared show state. Verify each state produces the expected output.
- [x] 4.2 Bind the remote's power button to enable and disable the device. Touches the remote path. Verify a press enables from disabled and disables from enabled or idle.
- [x] 4.3 Start a randomly chosen effect on an event — enabling the device, a PIR motion event, or an EQ press. Touches the effect engine. Verify each of the three starts an effect while the device is enabled.
- [x] 4.4 End the effect after the configured duration and return to idle and dark, so the next effect needs another event. Touches the effect engine and the timeout. Verify the fixture goes dark and stays dark until an event.
- [x] 4.5 Extend a running effect when motion continues, rather than starting another. Touches the PIR path. Verify continued motion keeps the effect running.
- [x] 4.6 Give eq both a single press and a three-second hold: the press fires an event immediately, and repeats for three seconds enter cleaning mode. Touches the remote path. Verify a tap starts a new effect at once and a hold enters cleaning.
- [x] 4.7 Bind `play/pause` to the haze master enable, and leave `9` unused. Touches the remote path. Verify the machine silences and the haze controls go inert, and that the rate is remembered.
- [x] 4.9 Make an event always start a newly chosen effect and restart the duration, so an eq press during an effect moves on to another one rather than extending the current one. Touches the effect engine and the remote path. Verify eq during an effect starts a different effect with a fresh duration.
- [x] 4.10 Use the onboard LED as the device status light: on while enabled, whether an effect is running or the fixture is idle, and off while disabled. Touches `D13`. Verify the LED reports both states and that the preview and cleaning blinks overlay it.

## 5. Effects

- [x] 5.1 Implement the effect engine: a table pairing each effect with a movement class (drifting, jumping, or owning its own movement), with the non-repeating wander added to drifting effects. Touches the effect engine. Verify drifting effects wander and jumping effects snap.
- [x] 5.2 Implement the drifting effects: chase, wave, comet, snake, colour roll, ripple, twinkle, fire, and blood pulse. Touches the beam bar. Verify each produces a distinct look and lights one emitter per beam.
- [x] 5.3 Implement the jumping effects: random strobe, blinder, and lightning. Touches the beam bar. Verify each snaps its position as it flashes, and that lightning sequences its strike and afterglow rather than overlapping them.
- [x] 5.4 Implement the self-moving effects: shake, pendulum, and crawl. Touches the beam bar. Verify each drives the position itself within the safe band.
- [x] 5.5 Give `snake` one random colour per activation and `colour roll` a different single emitter per bar. Touches the beam bar. Verify the colours change between activations and never blend.
- [x] 5.6 Confirm the whole roster is reachable and that every effect runs. Touches the effect engine. Verify a deliberate tour of the roster runs all fifteen.
- [x] 5.7 Keep every effect non-blocking. Touches the effect engine. Verify IR input and motion sensing stay responsive while an effect runs.

## 6. Haze and bubble machine

- [x] 6.1 Hold two independent settings sets, idle and event, and drive the machine from whichever matches the device state, defaulting idle to off and an event to haze, bubbles, and a full blower. Touches the Chauvet machine. Verify each state drives the channels from its own set.
- [x] 6.2 Route the remote's haze controls to the set for the current state, so a control pressed while idle edits the idle set and one pressed during an effect edits the event set without changing either set on its own. Touches the Chauvet machine. Verify a blower change made during an effect applies at once, returns to the idle speed when the effect ends, and is used again by the next effect.
- [x] 6.3 Step the haze rate with `track±`, toggle haze with `func/stop`, toggle bubbles with `st/rept`, and set the blower with `0`–`3`. Touches the Chauvet machine. Verify each control changes its own channel.
- [x] 6.4 Enter cleaning mode on a three-second EQ hold, flashing the LED, and exit on a single press. Touches the Chauvet machine and `D13`. Verify channel 42 goes active, effects stop, and the device resumes its previous state on exit.
- [x] 6.5 Make the haze controls inert while the master enable is off, while the device is disabled, or while cleaning. Touches the Chauvet machine. Verify a stray press changes nothing.
- [x] 6.6 Persist both settings sets in EEPROM with the effect duration, bumping the settings version so an older store falls back to defaults rather than being misread. Touches the EEPROM settings. Verify a change survives a power cycle, and that a blank chip comes up with the defaults.

## 7. Bench verification and documentation

- [x] 7.1 Verify the whole path on the fixture: the roster runs, the colours are correct, and the fixture decodes the intended channel values. Touches both fixtures. Verify by running the show and watching.
- [x] 7.2 Resolve the DMX grounding: the ground loop between the host's USB ground and the haze machine's mains earth, which coupled the machine's motor noise into the bar's last light. Touches the DMX wiring. Verify the interference stops with the transceiver's ground separated by a resistor and the cable common tied to the board's ground.
- [x] 7.3 Fit the 120 Ω bus termination across the pair at the controlling node. Touches the DMX wiring. Verify the bus runs clean with it fitted, which it does once the connection is sound.
- [x] 7.4 Reconcile the specs, design, tasks, and README with the implemented behaviour. Touches the change's artifacts. Verify `openspec validate --strict` passes and no artifact describes superseded behaviour.

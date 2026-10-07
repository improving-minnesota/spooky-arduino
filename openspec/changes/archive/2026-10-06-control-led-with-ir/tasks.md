## 1. Library and wiring setup

- [x] 1.1 Install the IR decoding library (`arduino-cli lib install IRremote`) and record the installed version. Touches no sketch files. Verify `arduino-cli lib list` shows the library with a version, and that `arduino-cli compile --fqbn arduino:avr:uno blink-led` still succeeds so the install did not disturb the existing sketch.
- [x] 1.2 Wire the kit's IR receiver module to the board: data out to `D2`, VCC to `5V`, GND to `GND`, per the pin table in `design.md`. Touches the D2 digital input pin, the 5 V rail, GND, and the receiver module. Verify the module's silkscreen pin labels match the wiring before applying power, and that the board powers up with no overheating or short.

## 2. Sketch skeleton

- [x] 2.1 Create `ir-led-control/ir-led-control.ino` with empty `setup()` and `loop()` and `ir-led-control/pins.h` declaring the onboard LED pin (D13 / `LED_BUILTIN`) and the IR receiver pin (`D2`), and `#include "pins.h"` from the sketch. Touches the D13 and D2 pin constants. Verify `arduino-cli compile --fqbn arduino:avr:uno ir-led-control` succeeds and the pin references resolve from the header rather than literals in the `.ino`.
- [x] 2.2 In `setup()` of `ir-led-control.ino`, configure the D13 onboard LED as an output and drive it off, and start the IR receiver on `D2` using the library's receive API. Touches the D13 output pin and the D2 input pin. Verify the sketch compiles and, once uploaded, the D13 LED is off after power-up with no remote input (spec: LED starts in the off state).

## 3. Capture the remote's power-button code

- [x] 3.1 Add a temporary capture pass to `ir-led-control.ino` that prints each decoded IR frame's protocol and value over the hardware USART at 9600 baud. Touches the D2 input pin and the USART. Verify `arduino-cli compile --fqbn arduino:avr:uno ir-led-control` succeeds and the sketch uploads.
- [x] 3.2 Open the serial monitor and press the remote's power button, recording the reported protocol and value. Touches the D2 input pin, the USART, the remote, and the receiver module. Verify the power button prints a stable, repeatable protocol/value pair across several presses (this is the code the firmware will match).
- [x] 3.3 Record the captured power-button code and protocol as named constants in `ir-led-control/ir_codes.h` and `#include` it from the `.ino`. Touches no pins. Verify the sketch still compiles and the constants are referenced from the header rather than literals in the `.ino`.
- [x] 3.4 Remove the capture print, or gate it behind a compile-time switch, so the normal build does not spam serial. Touches the USART. Verify the default build compiles and prints nothing on serial while still decoding frames.

## 4. Implement IR → LED control

- [x] 4.1 In `loop()` of `ir-led-control.ino`, poll the receiver for a decoded frame without blocking. Touches the D2 input pin and the main loop. Verify the sketch compiles and that no `delay()` call exists anywhere under `ir-led-control/` — the project constraint forbids blocking delays.
- [x] 4.2 Ignore repeat frames so a held button toggles once, and toggle the D13 LED only when the decoded code and protocol match the recorded power-button constants from `ir_codes.h`. Touches the D13 output pin and the D2 input pin. Verify the sketch compiles and the comparison references the header constants rather than inline literals.
- [x] 4.3 Upload to the connected board (`arduino-cli upload --fqbn arduino:avr:uno -p <port> ir-led-control`). Touches the Uno R3 over USB, the D13 LED, and the D2 input. Verify the upload completes without error and the remote's power button toggles the D13 LED — this is the primary acceptance check for the change.

## 5. Verify on hardware

- [x] 5.1 Verify toggling both ways: with the LED off, one power-button press turns it on; with the LED on, one press turns it off.
- [x] 5.2 Verify the startup state: reset the board and confirm the D13 LED is off before any press, then confirm the first press turns it on.
- [x] 5.3 Verify other buttons are ignored: press several non-power buttons on the remote and confirm the D13 LED state does not change.
- [x] 5.4 Verify repeat handling: press and hold the power button and confirm the LED toggles once rather than flickering (spec: repeat frames do not double-toggle).
- [x] 5.5 Verify state persistence: leave the LED on for at least a minute with no further input and confirm it stays on with no timeout.
- [x] 5.6 Verify responsiveness: while the loop is running, confirm presses are decoded promptly and the LED responds without a visible stall.
- [x] 5.7 Verify range and aim: from a few meters away, aim the remote at the receiver and confirm the LED still responds.

## 6. Wrap-up and baseline

- [x] 6.1 Confirm the sketch needs no libraries beyond the standard AVR core and the IR library — `arduino-cli compile --fqbn arduino:avr:uno ir-led-control` succeeds with no missing-library errors. Touches no hardware.
- [x] 6.2 Record in `README.md`: the new sketch, the IR receiver wiring (D2 / 5 V / GND), the pinned library version, the captured power-button code and protocol, and the compiler-reported flash and SRAM usage. Touches the D2 and D13 pin documentation. Verify the recorded numbers match the most recent compiler output.
- [x] 6.3 Confirm `blink-led` is unchanged and still builds independently — `arduino-cli compile --fqbn arduino:avr:uno blink-led` succeeds and the `led-blink` capability is untouched.

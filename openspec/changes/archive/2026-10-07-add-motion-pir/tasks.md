## 1. Consolidate into `spooky-arduino`

- [x] 1.1 Create `spooky-arduino/spooky-arduino.ino` with empty `setup()`/`loop()` and `spooky-arduino/pins.h` declaring the onboard LED (`D13` / `LED_BUILTIN`), the IR receiver (`D2`), and the PIR (`D4`). Touches the D13, D2, and D4 pin constants. Verify `arduino-cli compile --fqbn arduino:avr:uno spooky-arduino` succeeds and pin references resolve from the header.
- [x] 1.2 Port the IR power-button behavior from `ir-led-control` into `spooky-arduino`, reusing the captured power-button code in `spooky-arduino/ir_codes.h`. Touches the D2 input, the D13 output, and the IR receiver module. Verify the sketch compiles, uploads, and the power button reproduces the existing on/off toggle on the board before any new behavior is added.
- [x] 1.3 Confirm `spooky-arduino/` contains no blocking `delay()` call. Touches no hardware. Verify by searching the sketch folder for `delay(` and finding none — the project constraint forbids blocking delays.

## 2. Capture the full remote button map

- [x] 2.1 Add a compile-time capture mode to `spooky-arduino.ino` that prints each decoded frame's protocol, address, and command over the USART at 9600 baud. Touches the D2 input, the USART, and the IR receiver module. Verify the sketch compiles, uploads, and prints a frame when a button is pressed.
- [x] 2.2 Press all 21 buttons on the kit remote and record each button's protocol, address, and command. Touches the D2 input, the USART, the remote, and the receiver module. Verify every button reports a stable code that is repeatable across presses and distinct from the others.
- [x] 2.3 Commit the captured map as named constants in `spooky-arduino/ir_codes.h`, including the volume-up, volume-down, and power buttons, and `#include` it from the `.ino`. Touches no pins. Verify the sketch compiles and the codes are referenced from the header rather than literals.

## 3. PIR motion detection

- [x] 3.1 Wire the HC-SR501 to the board: data out to `D4`, VCC to `5V`, GND to `GND`; set the time-delay trimpot to minimum and select repeatable trigger mode. Touches the D4 input, the 5 V rail, GND, and the PIR module. Verify the wiring matches the module's silkscreen before powering, the board powers up with no overheating, and the retrigger mode is confirmed empirically (the H/L labels are inconsistent across sources).
- [x] 3.2 Add non-blocking PIR polling to `spooky-arduino.ino` that reads `D4` as a digital input. Touches the D4 input and the main loop. Verify the sketch compiles and, once uploaded, motion presence is reported (for example via a temporary serial print) without any `delay()`.
- [x] 3.3 Add a warm-up mask so the PIR output is ignored for a warm-up period after power-on or reset. Touches the D4 input. Verify that on power-up the onboard LED does not turn on from the sensor until the warm-up period completes (spec: no spurious activation during warm-up).

## 4. Persistent timeout in EEPROM

- [x] 4.1 Add a settings header for `spooky-arduino` declaring the stored struct (magic, version, timeout seconds) and the 5-second default. Touches no pins. Verify the sketch compiles and the default is a single named constant.
- [x] 4.2 Load settings from EEPROM in `setup()` and store them on change using `EEPROM.put()`, treating a blank or version-mismatched store as "use defaults and write them". Touches the EEPROM. Verify that a freshly flashed board reports the 5-second default, and that a changed value survives a power cycle.

## 5. Timeout ladder, volume buttons, and preview

- [x] 5.1 Add the timeout ladder (1–10, 20, 30, 60, 120, 180, 240, 300 seconds) and handle the volume-up/volume-down buttons to move one step per press, clamped to 1 s and 300 s. Touches the D2 input and the IR receiver. Verify the sketch compiles and the steps match the spec: 4→5 s, 10→20 s, 30→60 s, 60→120 s, and no change beyond 1 s or 300 s.
- [x] 5.2 Persist the timeout whenever it changes. Touches the EEPROM. Verify a value set with the volume buttons survives a power cycle.
- [x] 5.3 Implement the preview: after a change, flash the onboard LED once per second for the new timeout in seconds, then turn it off. Touches the D13 output. Verify that setting 5 seconds produces 5 flashes once per second and then off, with no `delay()`.

## 6. Unified LED state machine

- [x] 6.1 Implement the steady-state model (`OFF` / `ON` with a deadline): motion and a power press while off activate the LED with the configured timeout, continued motion retriggers the deadline, the deadline turns the LED off, and a power press while on forces it off until the next activation. Touches the D13 output, the D4 input, and the D2 input. Verify the sketch compiles.
- [x] 6.2 Verify on the board that motion turns the LED on and it turns off after the configured timeout, that continued motion keeps it on, that the power button turns it on for the timeout when off, and that a power press while on turns it off immediately. Touches the D13 output, the D2 input, and the D4 input. Verify no `delay()` exists and remote input stays responsive while the LED is on.

## 7. Hardware verification and cleanup

- [x] 7.1 Run the end-to-end acceptance pass on the board: motion activation, retrigger, timeout expiry, power on-with-timeout, power force-off, volume adjustment with preview, persistence across a power cycle, other buttons ignored, and stray/unrecognized IR ignored. Touches the D13 output, the D2 input, the D4 input, the remote, the receiver, and the PIR. Verify each scenario matches its spec.
- [x] 7.2 Retire the `blink-led/` and `ir-led-control/` sketch folders. Touches no hardware. Verify `arduino-cli compile --fqbn arduino:avr:uno spooky-arduino` still succeeds and no remaining file references the retired sketches.
- [x] 7.3 Update `README.md` to describe `spooky-arduino` (pins D13/D2/D4, wiring, library version, flash/SRAM usage) and remove the retired sketch sections. Touches the pin documentation. Verify the recorded numbers match the most recent compiler output and the documented commands run verbatim.

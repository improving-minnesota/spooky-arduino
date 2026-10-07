## 1. Toolchain and board setup

- [x] 1.1 Confirm the standard Arduino AVR core is installed — `arduino-cli core list` shows `arduino:avr`, or the Arduino IDE has the Uno board package. Touches no sketch files. Verify by compiling the bundled `Blink` example for `arduino:avr:uno` with no errors.
- [x] 1.2 Connect the Uno R3 board over USB and identify its serial port (`arduino-cli board list`, or Tools > Port in the IDE). Touches the board and USB cable only. Verify the port appears and is identified as an Uno R3 — every later upload task depends on this.
- [x] 1.3 Record the board FQBN (`arduino:avr:uno`) and the discovered port in `README.md` so the build-and-upload path is reproducible. Touches no sketch files. Verify by running the documented command verbatim and getting a clean compile.

## 2. Sketch skeleton

- [x] 2.1 Create `blink-led/blink-led.ino` with empty `setup()` and `loop()` — the folder name must match the sketch name for the Arduino IDE and `arduino-cli`. Touches no pins yet. Verify `arduino-cli compile --fqbn arduino:avr:uno blink-led` succeeds.
- [x] 2.2 Add `blink-led/pins.h` declaring the onboard LED pin (D13 / `LED_BUILTIN`) and `#include` it from `blink-led.ino`. Touches the D13 onboard LED pin constant. Verify the sketch still compiles and the pin reference resolves from the header rather than a literal `13` in the `.ino`.

## 3. Blink implementation

- [x] 3.1 In `blink-led/blink-led.ino`, configure the D13 onboard LED pin as an output in `setup()` and define the cadence constant `kBlinkIntervalMs = 1000`. Touches the D13 onboard LED. Verify the sketch compiles cleanly with `arduino-cli compile --fqbn arduino:avr:uno blink-led`.
- [x] 3.2 Implement the non-blocking toggle in `loop()` using `millis()` with an accumulated next-toggle target, driving the D13 onboard LED. Touches the D13 onboard LED and the main loop (no interrupts or timers). Verify the sketch compiles and that no `delay()` call exists anywhere under `blink-led/` — the project constraint forbids blocking delays.
- [x] 3.3 Upload to the connected board (`arduino-cli upload --fqbn arduino:avr:uno -p <port> blink-led`). Touches the Uno R3 board over USB. Verify the upload completes without error and the D13 LED starts blinking 1 second on / 1 second off — this is the primary acceptance check for the change.
- [x] 3.4 Verify cadence stability: watch the D13 onboard LED for 60 seconds and count approximately 30 on/off cycles with no visible drift or stutter (spec: cadence does not drift over time).
- [x] 3.5 Verify the no-external-components scenario: power the board from the barrel jack or a USB power supply with the computer disconnected, and confirm the D13 LED still blinks after power-up.
- [x] 3.6 Verify automatic startup: press the board's reset button, or remove and re-apply power, and confirm the blink restarts on its own with no host interaction, button press, or serial connection.

## 4. Wrap-up and baseline

- [x] 4.1 Confirm the sketch needs no third-party libraries — with only the `arduino:avr` core installed, `arduino-cli compile --fqbn arduino:avr:uno blink-led` succeeds and reports no missing-library errors. Touches no hardware.
- [x] 4.2 Record the compiler-reported flash and SRAM usage, plus the D13 pin assignment, in `README.md` as a baseline for later capabilities. Touches the D13 onboard LED pin documentation. Verify the recorded numbers match the most recent compiler output.

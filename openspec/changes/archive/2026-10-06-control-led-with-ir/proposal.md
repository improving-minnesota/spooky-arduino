## Why

The board can blink on its own, but nothing can control it yet — every capability so far is output-only. The IR receiver is the project's first input device, so adding it proves we can decode a remote button press on the Uno R3 and react to it in firmware. Driving the onboard LED from the remote's power button is the smallest end-to-end IR loop: one extra component, a result that is visible immediately, and a reusable decode path that motion sensing and remote-controlled lighting will build on.

## What Changes

- Add a new `ir-led-control` sketch that reads the IR receiver module and toggles the onboard LED (D13) on and off each time the remote's power button is pressed.
- Wire the Elegoo kit's IR receiver module to a digital input pin (D2) and decode its output in firmware.
- Add the project's first third-party dependency: the `IRremote` library (already anticipated in the project context).
- Capture the remote's power-button IR code and record it as a named constant, since the code is specific to the remote in the kit and cannot be assumed.
- Leave the existing `blink-led` sketch and the `led-blink` capability unchanged.
- No **BREAKING** changes.

## Capabilities

### New Capabilities
- `ir-led-control`: The board reads IR remote button presses and uses the remote's power button to turn the onboard LED on and off, keeping the visible LED state in sync with the last command received.

### Modified Capabilities
<!-- None: led-blink is unchanged; IR control is a separate capability in a separate sketch. -->

## Impact

- **New code**: an `ir-led-control/` sketch folder containing the `.ino`, a `pins.h` following the `blink-led` pattern, and the captured IR code constants.
- **Dependencies**: first third-party library — `IRremote`. The stock Arduino AVR core alone is no longer sufficient, so the library must be installed and its version pinned.
- **Hardware**: the Elegoo kit's IR receiver module wired to a digital pin, plus the kit's IR remote. The onboard D13 LED is reused, so no external LED or current-limiting resistor is needed.
- **Toolchain**: unchanged (`arduino:avr:uno`). Serial output may be used briefly to discover the remote's button codes, then removed or kept behind a compile-time switch.
- **Future**: establishes the IR decode path that remote-controlled lighting, configurable timing, and DMX/haze control from the remote will reuse.

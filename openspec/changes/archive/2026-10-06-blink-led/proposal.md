## Why

The project has no firmware yet. Before building motion sensing, IR control, or DMX output, we need the smallest sketch that proves the whole physical loop works: toolchain installed, correct board and port selected, code compiled, and binary actually running on the Uno R3. A blinking onboard LED is the classic smoke test for that — it needs no external wiring, yet the blink is only visible if real firmware is executing on the real board.

It also gives us our first end-to-end pass through the AI-assisted workflow (propose → spec → design → tasks → apply → verify on hardware) on something small enough to debug quickly.

## What Changes

- Add the project's first Arduino sketch: it blinks the Uno R3's onboard LED (`LED_BUILTIN`, D13) at a fixed **1 second on / 1 second off** cadence, repeating indefinitely while the board is powered.
- Centralize pin assignments in one place so the PIR, IR, and DMX work can follow the same pattern instead of scattering magic numbers across sketches.
- Document the build-and-upload path (board selection, port, upload command) so the loop is reproducible and not just tribal knowledge.
- No changes to existing behavior — this is the first firmware in the repo.

## Capabilities

### New Capabilities
- `led-blink`: The board's onboard LED blinks continuously at a fixed 1 s on / 1 s off cadence whenever the board is powered, serving as a visible "firmware is running" indicator.

### Modified Capabilities
<!-- None: no existing specs yet. -->

## Impact

- **New code**: a `blink-led` sketch (`.ino`) plus a shared pin-assignment header it includes.
- **Dependencies**: none beyond the Arduino core (`arduino:avr`). No third-party libraries.
- **Hardware**: Arduino Uno R3 (Elegoo) or compatible, connected over USB. No external components or wiring.
- **Toolchain**: Arduino IDE or `arduino-cli` with the AVR core installed; USB serial port known and selectable.
- **Future**: the sketch layout and pin-constant pattern established here become the base the motion-triggered lighting and IR-remote capabilities build on.

## Context

See `proposal.md` — Why for motivation, and `specs/` for the behavior this must satisfy. Current state: the repo holds two standalone sketches, `blink-led` (heartbeat only, no inputs) and `ir-led-control` (IR power button toggles the LED indefinitely). This change replaces both with a single `spooky-arduino` firmware and adds PIR motion sensing plus a configurable, persistent timeout.

Constraints that shape the approach:

- **Board**: Arduino Uno R3 (Elegoo) — ATmega328P at 16 MHz, ~32 KB flash with ~0.5 KB used by the bootloader, 2 KB SRAM, 1 KB EEPROM, three timers, one hardware USART, two external-interrupt pins (D2/D3).
- **Project constraint**: prefer non-blocking code so motion sensing and IR input stay responsive.
- **Project constraint**: keep pin assignments centralized so they can change in one place.
- **Existing dependency**: `IRremote` (already installed) owns Timer2 for receive sampling on AVR.
- **New hardware**: an HC-SR501 PIR module, which has its own adjustable delay and trigger-mode jumper (see Decision 3).
- **Unknown until measured**: the kit remote's per-button IR codes are specific to the remote and must be captured on hardware.

Pin assignments this change relies on:

| Function | Pin | Notes |
| --- | --- | --- |
| Onboard LED | `D13` / `LED_BUILTIN` | Digital output. Also SPI SCK — keep it for the LED only. |
| IR receiver data out | `D2` | Digital input. Unchanged from `ir-led-control`. |
| PIR data out | `D4` | Digital input. HC-SR501 3.3 V TTL output. |
| Serial (USART) | `D0`/`D1` | Reserved: USB programming, capture/debug, and later DMX. |
| RS485 direction (DE/RE) | *unassigned* | Reserved for the future RS485/DMX change. |

Power requirements: none beyond the board's own supply. The IR receiver and the HC-SR501 both run from the board's 5 V rail and GND; the PIR module draws ~65 mA, so it is powered from the rail and never from a GPIO. All devices are 5 V logic; the PIR's 3.3 V TTL output is comfortably above the ATmega328P's 5 V logic-high threshold.

Uno R3 resources the design relies on:

- **Timer2**: owned by the `IRremote` receive path. As a result PWM on `D3`/`D11` and `tone()` are unavailable. No application ISR is written; the timer belongs to the library.
- **EEPROM (1 KB)**: holds one small settings struct (a few bytes). Writes happen only when a setting changes, so the 100,000-cycle endurance is not a practical limit.
- **USART**: used only for button capture and debug, then left free for the future DMX work.
- **Flash/RAM**: the firmware adds PIR polling, a timeout state machine, and EEPROM access; the exact figures are recorded during implementation and must stay well inside 32 KB / 2 KB.

Where behavior runs: **all application logic is in `loop()`** — poll the IR receiver, poll the PIR, advance the timeout and preview state machines. `setup()` configures pins, loads settings from EEPROM, and starts the receiver. The only interrupt in play is the `IRremote` sampling timer; the design adds no ISRs. EEPROM writes are synchronous (~3.3 ms each) but occur only on a setting change.

## Goals / Non-Goals

**Goals:**

- Make `spooky-arduino` the single firmware, reproducing the existing IR + LED behavior before any new behavior is layered on.
- Add HC-SR501 motion detection that turns the LED on, with the off-timeout owned by the firmware (not the sensor's trimpot).
- Add a configurable timeout (default 5 s, 1 s–5 min) that is adjustable from the remote's volume buttons, previewed on the LED, and persisted in EEPROM.
- Give the power button timeout-based semantics: on-with-timeout when off, force-off when on.
- Decode and record every button on the kit remote so later features can bind to them.
- Establish one pin map and one resource budget that the future RS485/DMX work slots into.

**Non-Goals:**

- No RS485/DMX output or the DMX library — that is a later change.
- No external LED, PWM dimming, brightness, or color — the target is the onboard LED only.
- No IR transmission (receive only).
- No multiple sensors, zones, or fixtures.
- No configuration surface beyond the volume buttons (no menus, displays, or serial-config UI).
- No re-capture of the receiver wiring — the IR receiver stays on `D2`.

## Decisions

**1. One consolidated `spooky-arduino` sketch, and retire the per-feature sketches.**

- *Why*: the board runs one program, and the next capability (RS485/DMX) must share the pin/timer/USART budget with IR and PIR. Integrating incrementally spreads the risk that a big-bang merge would concentrate at the end. A single `pins.h` becomes genuinely shared instead of duplicated per sketch.
- *Alternatives considered*: keep one sketch per feature and combine later (defers the riskiest integration and the resource conflicts to the hardest feature); keep `blink-led` as a standalone diagnostic (the archived change and README already preserve the toolchain/board context, so nothing is lost by retiring it).

**2. Fold existing behavior in and verify it before adding PIR.**

- *Why*: the consolidation is the part most likely to regress (Timer2 interaction, LED ownership). Reproducing the current IR power-button behavior in `spooky-arduino` and confirming it on the board before adding motion gives a clean bisect point.
- *Alternatives considered*: add PIR and the merge in one step (smaller task list, but a failure no longer tells you whether the merge or the new feature broke).

**3. Use the HC-SR501 as a motion *detector*; own the timeout in firmware.**

- *Why*: the module's own delay is a physical trimpot (roughly 0.3 s–5 min), not software- or remote-configurable — which contradicts the project's "configurable timing" goal. Driving the off-timeout from `millis()` keeps timing configurable, testable, and composable with the power-button and preview logic, and avoids the sensor's ~2–3 s post-delay lock-out. The sensor's HIGH is still a useful "motion present" signal for retriggering.
- *Alternatives considered*: use the sensor's built-in delay and mirror the output to the LED (simplest firmware, but the timeout is coarse, imprecise, and cannot be set from the remote); a hybrid (sensor delay for coarse timing, firmware for fine) — needless complexity for one output.
- *Setup*: set the sensor's time-delay trimpot to its minimum and select repeatable trigger mode. The H/L silkscreen labeling is inconsistent across datasheets and board revisions, so the mode is confirmed empirically during implementation.

**4. A single LED model: steady state plus a transient preview mode.**

- *Why*: motion, the power button, and the volume buttons all act on one LED, so they need one arbiter. A tiny state machine keeps the rules in one place and makes each source's interaction explicit.

  - **Steady state**: `OFF` or `ON` with a deadline (`now + timeout`).
  - **Activation** (motion detected, or power pressed while `OFF`): set `ON`, deadline `= now + timeout`.
  - **Retrigger**: while `ON` and the PIR still reports motion, deadline `= now + timeout`.
  - **Timeout**: when `ON` and `now >= deadline`, set `OFF`.
  - **Power while `ON`**: set `OFF` immediately and cancel the deadline; stays off until the next activation.
  - **Preview mode** (volume press): a transient output mode that flashes the LED once per second for `timeout` seconds, then returns to `OFF`. A new activation (motion or power) cancels the preview and takes over.

- *Alternatives considered*: separate flags per source (more state, easy to contradict); a full FSM library (overkill for two states).

**5. Timeout is a discrete ladder, moved one step per volume press.**

- *Why*: the requested growing increments map cleanly onto a fixed set of reachable values — 1–10 s in 1-second steps, then 20, 30 (10-second steps), 60 (30-second step), then 120, 180, 240, 300 (60-second steps). A lookup table makes the "how much does a press change it" rule trivial, clampable, and easy to test, and avoids arithmetic that depends on the current value's range.
- *Alternatives considered*: compute the step from the current value each time (same result, more code and more edge cases at the range boundaries); a linear 1-second step throughout (300 presses to reach the maximum — impractical from a remote).

**6. Persist settings in EEPROM behind a magic + version guard.**

- *Why*: the timeout must survive power cycles, reset, and re-upload (EEPROM is untouched by the bootloader). A small struct (`magic`, `version`, `timeoutSec`) lets the firmware detect a blank or outdated store — a fresh chip reads `0xFF` — and fall back to defaults instead of trusting garbage, and lets the layout evolve as fields are added.
- *Alternatives considered*: no version guard (breaks silently the first time a field is added); `EEPROM.write()` on every change (needless wear — `EEPROM.put()`/`update()` writes only changed bytes); external I2C EEPROM or SD (unnecessary at this size).

**7. Volume press previews the new timeout by flashing once per second.**

- *Why*: the volume buttons are not on/off controls, so they need a distinct, visible confirmation that also conveys the new value. Flashing once per second and letting the user count the flashes is the requested behavior and reads as "N seconds".
- *Alternatives considered* (proposed because the literal behavior is long at the top of the range — a 300 s timeout means five minutes of flashing):
  - **Cap the preview** at a fixed number of flashes (e.g., 10) and signal "long" differently.
  - **Grouped flashes** — flash in bursts of ten (like a count), so 120 s reads as twelve quick bursts.
  - **Encode tens and ones** — a long flash per ten seconds, a short flash per second.
  - **Blink rate proportional** to the timeout instead of counting.
  - **Single confirmation flash** only, relying on the persisted value rather than counting.
  The spec currently records the literal once-per-second count; if one of these is preferred, it is a spec change and should be raised before or during apply.

**8. Constants live in headers, following the existing pattern.**

- *Why*: `blink-led`/`ir-led-control` already centralize pins in `pins.h`. `spooky-arduino` mirrors that with `pins.h` (LED, IR, PIR), `ir_codes.h` (captured button codes), and a settings header for the EEPROM struct and defaults, so each kind of constant changes in one place.
- *Alternatives considered*: everything inline in the `.ino` (scatters values); a single combined header (mixes pins, codes, and settings).

**9. Capture the full button map on hardware.**

- *Why*: the kit remote's codes are specific to the remote and cannot be assumed. A compile-time capture mode prints each decoded frame's protocol, address, and command over the USART at 9600 baud; every button is pressed and recorded, then the map is committed to `ir_codes.h`. The existing `IR_CAPTURE_MODE` switch carries over.
- *Alternatives considered*: hard-coding a "typical" Elegoo/NEC set (guesswork that fails on a differing remote); capturing only the buttons used now (defeats the goal of mapping all buttons for later features).

**10. Keep timing non-blocking with `millis()`.**

- *Why*: the project forbids long blocking delays so IR and motion stay responsive. The LED deadline, preview flashing, and warm-up masking are all `millis()` comparisons with no `delay()` in the loop.

## Risks / Trade-offs

- **`IRremote` owns Timer2** → PWM on `D3`/`D11` and `tone()` are unavailable. Mitigation: reserve non-Timer2 pins for any future PWM output and record this constraint when dimming is planned.
- **Single USART shared between capture/debug and future DMX** → you cannot run serial debug and hardware-serial DMX at once. Mitigation: keep the USART free of runtime use outside capture, and resolve the DMX serial plan in the RS485 change.
- **The 300 s preview is five minutes of flashing** → tedious at the top of the range. Mitigation: alternatives listed in Decision 7; the value persists regardless, so the preview is confirmation, not the source of truth.
- **HC-SR501 warm-up and lock-out** → ~30–60 s of false triggers at power-on and a ~2–3 s blind window after each delay. Mitigation: mask PIR output for a warm-up period after boot (spec requirement) and treat motion as a level ("present") rather than a single edge.
- **H/L jumper labeling is inconsistent across sources** → the wrong mode silently changes retrigger behavior. Mitigation: confirm empirically during implementation and record the observed mode.
- **EEPROM settings persist across uploads** → stale values can survive a rebuild and confuse testing. Mitigation: the magic/version guard and an explicit "restore defaults" path (e.g., version bump or a documented erase).
- **Consolidation increases blast radius** → every later change touches the whole firmware. Mitigation: the bring-up-verify step in Decision 2, plus keeping throwaway hardware-test sketches out of the deliverable.
- **Stray IR or sunlight could trigger a mapped button** → mitigated by matching exact protocol/address/command, so unrelated signals are ignored.
- **`D13` is also SPI SCK** → fine for the onboard LED, poor for a future external peripheral; unchanged from the earlier note.
- **IR is line-of-sight** → a missed press is usually aim/range, not a code bug; verification aims the remote at the receiver.

## Migration Plan

1. Create `spooky-arduino/` and reproduce the current IR power-button behavior; verify on the board before adding PIR (bisect point).
2. Add PIR, the timeout state machine, EEPROM settings, the volume-button ladder and preview.
3. Retire the `blink-led/` and `ir-led-control/` sketch folders and update `README.md` to describe `spooky-arduino`.
4. Rollback: re-upload either retired sketch from git history (the archived `blink-led` and `control-led-with-ir` changes preserve the original code and the board/toolchain context), or unplug the board. The firmware holds no state that is not reconstructible; the EEPROM initializes to defaults on first run.

## Open Questions

- Should `spooky-arduino` keep a diagnostic heartbeat blink (for example, a brief boot flash or a compile-time-switchable blink) now that `blink-led` is retired? Deferrable — it does not change the specs, the approach, or the task breakdown.
- Which free pin will carry the RS485 direction (DE/RE) line? Deferrable to the RS485/DMX change; no pins are consumed by this change beyond `D2` and `D4`.

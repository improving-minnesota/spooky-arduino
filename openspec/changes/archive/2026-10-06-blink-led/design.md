## Context

See `proposal.md` — Why for motivation. Current state: the repo has no firmware at all; this is the first sketch, so there is no existing code to fit into and no compatibility surface.

Constraints that shape the approach:

- **Board**: Arduino Uno R3 (Elegoo) — ATmega328P at 16 MHz, ~32 KB flash with ~0.5 KB used by the bootloader, 2 KB SRAM, single hardware USART.
- **Project constraint**: prefer non-blocking code so motion sensing and IR input stay responsive later.
- **Project constraint**: keep pin assignments centralized so they can change in one place.
- **Available now**: one board, one USB cable, no breadboard wiring. Nothing else in the hardware list (PIR, IR receiver, DMX adapter, haze machine) is involved in this change.

Pin assignments this change relies on:

| Function | Pin | Notes |
| --- | --- | --- |
| Onboard LED | `D13` / `LED_BUILTIN` | Digital output. The LED and its series resistor are on the board, so no external components or current-limiting resistor are needed. |

Power requirements: none beyond the board's own supply — USB 5 V or a 7–12 V barrel jack. The onboard LED draws a few mA. No external supply, level shifting, or driver circuitry is involved.

Uno R3 resources the design relies on: essentially none. No timers, no interrupts, no ADC, and the hardware serial port is left untouched and free for later use. Flash/RAM cost is a few hundred bytes, well inside the board's limits.

Where behavior runs: **everything is in `loop()`**. No ISRs, no timer configuration, no `setup()`-time work beyond configuring the LED pin as an output. This keeps the loop free to host PIR polling and IR decoding later.

## Goals / Non-Goals

**Goals:**

- Prove the full physical loop end to end: toolchain installed, board and port selected, sketch compiles, binary uploads, and code visibly runs on real hardware.
- Establish the sketch layout and the centralized pin-constant pattern that the motion-triggered lighting, IR-remote, and DMX capabilities will reuse.
- Keep the main loop non-blocking from day one, so adding PIR and IR handling later is an addition rather than a restructure.

**Non-Goals:**

- No external LEDs, no brightness or PWM control, no color.
- No serial logging, no configurable timing, no button or remote input — the cadence is fixed at 1 s on / 1 s off.
- No status-indicator abstraction or state machine. The LED is a heartbeat, not yet a general-purpose signal.
- No shared multi-sketch pin header yet; that is premature with a single sketch.

## Decisions

**1. Blink the onboard LED on D13 rather than an external LED.**

- *Why*: zero wiring means zero electrical failure modes. If the blink does not work, the problem is the toolchain, the board, or the port — not a backwards LED or a missing resistor. That is exactly what a first smoke test should isolate.
- *Alternatives considered*: an external LED on a breadboard (more representative of the real project, but adds wiring as a competing explanation for failure); both at once (more code and more things to check, no benefit at this stage).

**2. Reference the pin as `LED_BUILTIN`, not the literal `13`.**

- *Why*: self-documenting, and it keeps the physical pin mapping in one named constant instead of a magic number sprinkled through the sketch. Matches the project's "centralize pin assignments" guidance.
- *Alternatives considered*: hard-coded `13` (works, hides intent, and would need touching in several places once more pins exist).

**3. Use non-blocking `millis()` timing instead of `delay(1000)`.**

- *Why*: the project explicitly prefers non-blocking code so motion sensing and IR input stay responsive. The `loop()` skeleton written here is the same one PIR polling and IR decoding will slot into, so choosing it now avoids a rewrite when the next capability lands. `delay()` would work fine today and be thrown away tomorrow.
- *Alternatives considered*: `delay(1000)` — the idiomatic Blink, simplest possible, but it blocks the loop and would have to be replaced; a hardware Timer1 interrupt toggle — accurate and fully asynchronous, but consumes one of the board's scarce timers and adds ISR complexity for no observable benefit at a 1 s cadence.

**4. Advance the next toggle by accumulating the interval, not by resetting it to `millis()`.**

- *Why*: `nextToggle += kBlinkIntervalMs` keeps the cadence stable over long runs, satisfying the spec's "no cumulative timing drift" scenario. Resetting `nextToggle = millis()` after each toggle silently adds the loop's own execution time to every cycle.
- *Alternatives considered*: resetting to `millis()` (simpler, drifts by the loop overhead each cycle — small, but free to avoid); comparing `millis() % 1000` (elegant-looking, breaks on wraparound and couples phase to boot time).

**5. Layout: a `blink-led/` sketch folder containing `blink-led.ino` plus a `pins.h` header.**

- *Why*: the Arduino IDE and `arduino-cli` require the `.ino` filename to match its containing folder, so the folder name is fixed. Putting pin constants in a header inside the sketch folder gives one obvious place to declare pins while keeping the sketch self-contained and directly openable in the IDE.
- *Alternatives considered*: everything in one `.ino` with `const` declarations at the top (acceptable for one sketch, but pins then get duplicated as more sketches appear); a repo-root shared `spooky_pins.h` (the right end state once a second sketch actually needs it, premature now).

**6. Cadence is a named constant, `kBlinkIntervalMs = 1000`.**

- *Why*: one source of truth for the 1 s on / 1 s off requirement, and it makes the eventual "configurable timing" capability a matter of changing where this value comes from rather than hunting literals.
- *Alternatives considered*: inline `1000` literals (works, but scatters the requirement).

## Risks / Trade-offs

- **Blocking code creeps in later and makes the LED stutter** → the loop is non-blocking now, and the accumulated-target approach means a slow loop shifts the phase rather than corrupting the cadence; a visible stutter would itself be a useful warning sign.
- **`millis()` wraps after roughly 49.7 days** → the unsigned `millis() - lastToggle >= interval` idiom handles wraparound correctly by construction. No extra handling needed as long as the subtraction form is used rather than comparing raw timestamps.
- **D13 is also SPI SCK**, used by ISP/programming and any future SPI device → harmless for the onboard LED, but it means D13 is a poor choice for a future external peripheral; worth remembering when the DMX or other wiring gets planned.
- **The most likely failure is configuration, not code** → board model and serial port selection are the usual culprits. Tasks include explicit port discovery and board selection, and the blink itself is the verification.
- **Timing accuracy is approximate** → `millis()` is driven by a ceramic resonator, not a crystal, so the cadence is only as accurate as the board's clock. The spec asks for "approximately" one second, so this is acceptable; if precision ever matters, a timer interrupt is the upgrade path.
- **Uploads reset the board** → each upload restarts the blink, so "it was blinking before I uploaded" is never valid evidence; verification must follow a fresh upload.

## Migration Plan

Not applicable — this is the repo's first firmware, so there is no existing behavior to migrate or preserve. Rollback is trivial: the sketch holds no persistent state, so re-uploading a previous sketch or unplugging the board fully reverts it.

## Open Questions

- Should the heartbeat blink survive into the final firmware once motion-triggered lighting and IR control exist, or become a separate diagnostic-only sketch? Deferrable — it does not change this change's specs, approach, or task breakdown.

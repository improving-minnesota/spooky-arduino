## Context

See `proposal.md` — Why for motivation, and `specs/ir-led-control/spec.md` for the behavior this must satisfy. Current state: the repo has one firmware sketch, `blink-led`, which blinks the onboard LED and reads no inputs. This change adds the first input path.

Constraints that shape the approach:

- **Board**: Arduino Uno R3 (Elegoo) — ATmega328P at 16 MHz, ~32 KB flash, 2 KB SRAM, three timers, one hardware USART, two external-interrupt pins (D2/D3).
- **Project constraint**: prefer non-blocking code so motion sensing and IR input stay responsive.
- **Project constraint**: keep pin assignments centralized so they can change in one place.
- **New dependency**: the IR receiver needs a decoding library. This is the project's first third-party library, so how it is installed and pinned matters.
- **Unknown until measured**: the remote's power-button IR code is specific to the remote in the kit and cannot be assumed — it must be captured on hardware.

Pin assignments this change relies on:

| Function | Pin | Notes |
| --- | --- | --- |
| IR receiver data out | `D2` (INT0) | Digital input. 38 kHz demodulated output from the kit's receiver module. Chosen as the conventional interrupt-capable pin; the library's receive path does not actually require an external interrupt, so this is a free choice. |
| IR receiver VCC | `5V` | Powered from the board's 5 V rail. Module draws a few mA. |
| IR receiver GND | `GND` | Common ground with the board. |
| Onboard LED | `D13` / `LED_BUILTIN` | Digital output. Reused from `blink-led` — no external LED or series resistor needed. |

Power requirements: none beyond the board's own supply. The receiver module runs from the board's 5 V rail and draws a few mA, so no external supply, level shifting, or driver circuitry is involved (the module and the Uno are both 5 V logic).

Uno R3 resources the design relies on:

- **Timer2**: the IRremote library's AVR receive path installs its own timer interrupt for sampling. This is the one real resource cost — it means Timer2 is no longer free for PWM on `D3`/`D11` or `tone()`. No application-level ISR is written; the timer belongs to the library.
- **Flash/RAM**: the library adds a few KB of flash and a few hundred bytes of RAM. Both stay comfortably inside the board's limits; the exact figures are recorded during implementation.
- **USART**: used only temporarily, to capture and print the remote's button codes. Otherwise left free.

Where behavior runs: **application logic is entirely in `loop()`** — poll for a decoded IR frame, compare it to the known power-button code, and toggle the LED. `setup()` configures the pins, brings the LED up off, and starts the receiver. The only interrupt in play is the one the library installs internally for sampling.

## Goals / Non-Goals

**Goals:**

- Establish a working, non-blocking IR receive path on the Uno R3 that later remote-controlled features can reuse.
- Make the remote's power button reliably toggle the onboard LED, with the LED state matching the last decoded press.
- Keep the receiver's wiring, pin, and the remote's code centralized and documented so they can be changed in one place.
- Leave the existing `blink-led` sketch and `led-blink` capability untouched.

**Non-Goals:**

- No control of an external LED, no PWM/brightness, no color — the target is the onboard LED only.
- No remote-controlled timing or configuration; the only remote input handled is the power button.
- No coexistence with the heartbeat blink — this sketch does not blink; the LED is steady on or off.
- No abstraction layer or command dispatch yet; a single code comparison is the right size for one button.
- No infrared transmission (sending) — receive only.

## Decisions

**1. A separate `ir-led-control/` sketch rather than extending `blink-led`.**

- *Why*: keeps each capability a standalone sketch that can be compiled, uploaded, and reasoned about on its own; `blink-led` stays a clean, dependency-free heartbeat. This matches the repo's existing one-folder-per-sketch layout.
- *Alternatives considered*: adding IR handling into `blink-led` (fewer folders, but mixes a no-dependency smoke test with a library-dependent feature and forces a choice about whether the heartbeat keeps blinking).

**2. IR receiver data output on `D2`.**

- *Why*: the conventional choice for IR receivers on the Uno, it is interrupt-capable (INT0) if a future decoder needs it, and it is not shared with the onboard LED. Any digital pin works for the library's polling decode, so the cost of this choice is essentially zero.
- *Alternatives considered*: `D3` (equally valid, also interrupt-capable); an analog pin (works, but wastes an ADC channel and gains nothing).

**3. Use the `IRremote` library, version pinned, rather than a hand-rolled decoder.**

- *Why*: the project context already names `IRremote` as the intended library, and it handles the timing-sensitive demodulation, protocol decoding, and repeat-frame detection that are genuinely fiddly to write correctly. Reusing it is the point of a learning project that is about the AI-assisted workflow, not about re-implementing IR timing.
- *Alternatives considered*: writing a raw timer/ISR decoder (instructive, but a large amount of timing-sensitive code for no behavioral gain); `IRLremote` (lighter and interrupt-based, but less common and less documented than `IRremote`).
- *Version note*: `IRremote`'s receive API changed between 3.x and 4.x (`IRrecv`/`enableIRIn()`/`decode()` vs `IrReceiver.begin()`/`decode()`). Target the current 4.x API and pin whatever version the toolchain installs; if the toolchain resolves 3.x, adapt those three calls. Either way the version is recorded so builds are reproducible.

**4. Poll the decoder in `loop()`; never block waiting for a frame.**

- *Why*: the project forbids long blocking delays so inputs stay responsive. The library exposes a non-blocking "has a frame arrived?" poll, which slots directly into the existing `loop()` skeleton and keeps the door open for PIR polling alongside IR later.
- *Alternatives considered*: blocking until a frame arrives (simple, but stalls everything else); decoding in an ISR (the library already owns a timer interrupt for sampling — adding application ISRs here would fight it for no benefit).

**5. Ignore repeat frames so a held button toggles once.**

- *Why*: holding a remote button sends a continuous stream of repeat frames. If those toggled the LED, a single press-and-hold would strobe the LED. Toggling only on a fresh (non-repeat) frame makes "each press toggles once" true and matches the spec's repeated-press scenario.
- *Alternatives considered*: toggling on every frame (fast flicker while held); debouncing by time window (extra state and a magic interval for a problem the repeat flag already solves).

**6. Capture the power-button code on hardware and record it as a named constant.**

- *Why*: the IR code is specific to the remote in the kit, so it cannot be written from knowledge — it has to be observed. The plan is to build a temporary capture pass that prints decoded values and protocol over serial, press the power button, and record the value as a named constant (with the protocol) in a header.
- *Alternatives considered*: hard-coding a "typical" Elegoo/NEC code (guesswork that fails on any remote whose code differs); matching any decoded frame (would make every button toggle the LED, violating the spec).

**7. Constants live in headers, following the `blink-led` pattern.**

- *Why*: `blink-led` already centralizes its pin in `pins.h`. This sketch mirrors that with a `pins.h` for the LED and receiver pins, plus a separate `ir_codes.h` for the captured code and protocol, so pins and codes each change in one obvious place.
- *Alternatives considered*: everything inline in the `.ino` (scatters the values); one combined header (works, but mixes two different kinds of constant).

**8. LED comes up off in `setup()`.**

- *Why*: gives the spec's "defined state at startup" a trivial, deterministic implementation and guarantees the first press produces a visible change.
- *Alternatives considered*: restoring a previous state (no persistence exists, and it would make startup non-deterministic).

## Risks / Trade-offs

- **The library takes Timer2 for receive sampling** → PWM on `D3`/`D11` and `tone()` are unavailable while receiving. Acceptable now, but it constrains the future PWM LED-dimming work; mitigation is to reserve non-Timer2 pins for PWM outputs and record this constraint when that capability is planned.
- **The recorded remote code is specific to one remote** → a different remote (or a swapped-out kit) would stop working. Mitigation: document the code and protocol in `ir_codes.h`, and keep the capture procedure in the tasks/README so it can be repeated.
- **Repeat frames could double-toggle** → mitigated by ignoring repeat frames (Decision 5) and verifying with a press-and-hold test on hardware.
- **Stray IR from other remotes or sunlight could toggle the LED** → mitigated by matching the exact code *and* protocol, so unrelated signals are ignored; the spec's "unrecognized IR signals are ignored" scenario covers this.
- **Library size could push flash/RAM** → `IRremote` adds a few KB, still well within the Uno's 32 KB flash and 2 KB SRAM; the compiler-reported figures are recorded as a check, and `blink-led` remains a zero-dependency fallback.
- **Line-of-sight and range** → IR needs a clear path and roughly 5–10 m; verification should aim the remote at the receiver rather than treating a missed press as a code bug.
- **The receiver's pins are easy to mis-wire (VCC/GND/OUT)** → verify the module's silkscreen against the pin table before powering, and confirm the LED toggles before assuming a firmware fault.
- **`D13` is also SPI SCK** → harmless for the onboard LED, but it means `D13` remains a poor choice for a future external peripheral; unchanged from the `blink-led` note.

## Migration Plan

Not applicable in the deployment sense — this adds a new sketch and changes no existing behavior. `blink-led` still compiles and runs independently, so rollback is simply re-uploading `blink-led` or unplugging the board; the IR sketch holds no persistent state.

## Open Questions

- Should the temporary serial capture mode stay in the sketch behind a compile-time switch (handy when a different remote is used), or be removed once the power-button code is recorded? Deferrable — it does not change the specs, the approach, or the task breakdown.

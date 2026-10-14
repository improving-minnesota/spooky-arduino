> **Archive note (2026-10-14).** This change is a historical record, kept as it
> was written. Two of its bench findings were later overturned; the README holds
> the current wiring.
>
> - Decision 10 says the DSD Tech SH-RS09B is output-only and cannot monitor the
>   bus. It is not output-only: its FT232R is configured for FTDI auto-direction
>   (`CBUS2 = TXDEN`), and with it listening it reads the full stream off the bus
>   (about 22.6 kB/s) and decodes the intended channel values. The zero-byte
>   result recorded below was a loose pin in the adapter's cable, now resoldered.
>   The same note applies to the matching claim in `proposal.md`.
> - Decision 11's series ground resistor has been removed. The interference on
>   the bar's last light that it was meant to break came from that faulty
>   connection rather than the ground loop, and the transceiver's ground is now
>   direct.

## Context

See `proposal.md` — Why for motivation, and `specs/` for the behavior this must satisfy.

Current state: `spooky-arduino` is a single non-blocking sketch — `IRremote` on `D2`, an HC-SR501 PIR on `D4`, the onboard LED on `D13`, a small EEPROM settings struct holding the timeout, and `Serial` at 9600 used only behind the `IR_CAPTURE_MODE`/`PIR_DEBUG` switches.

Constraints that shape the approach:

- **Board**: Arduino Uno R3 (Elegoo) — ATmega328P at 16 MHz, ~32 KB flash (≈0.5 KB used by the bootloader), 2 KB SRAM, 1 KB EEPROM, three timers, one hardware USART (`D0`/`D1`), and two external-interrupt pins (`D2`/`D3`).
- **Project constraint**: prefer non-blocking code so motion sensing and IR input stay responsive.
- **Project constraint**: keep pin assignments centralized so they can change in one place.
- **Existing dependency**: `IRremote` owns Timer2 for receive sampling on AVR.
- **Single USART**: `D0`/`D1` are the only hardware serial pins, and on the Uno R3 the USB-serial converter (ATmega16U2) is wired to those same pins, so a sketch that owns the USART for DMX cannot use the USB serial monitor.
- **DMX512 protocol**: 250 kbaud, 8N2, a break of ≥88 µs and a mark-after-break of ≥8 µs before the start code, then up to 512 channel bytes; the frame repeats continuously.
- **New hardware**: a MAX485 (or equivalent) transceiver, the SHEHDS 8×12W RGBW beam bar, the Chauvet Hurricane Bubble Haze, and a DSD Tech SH-RS09B USB-to-DMX adapter — which testing showed to be output-only, so it cannot serve as the bus monitor (see Decision 10). *(Superseded 2026-10-14: the adapter does read the bus; see the archive note at the top of this file.)*

## Goals / Non-Goals

**Goals:**

- Transmit a valid DMX512 universe from the Uno through a MAX485, verifiable from the bus by a passive monitor before any fixture is trusted.
- Drive the beam bar (38 channels at address 1) and the Chauvet machine (4 channels at address 39) with the documented channel semantics.
- Provide the roster of fifteen named spooky effects, each lighting at most one emitter per beam and moving the beam inside its safe band, gated by the device's enabled state.
- Bind the specified remote buttons to rig actions while keeping the main loop non-blocking.
- Keep pin assignments centralized so wiring changes stay in one place.

**Non-Goals:**

- DMX input, RDM, fixture discovery, or personality switching — this is transmit-only.
- Networked lighting (Art-Net/sACN) or a QLC+ universe — output is a physical DMX512 line.
- Persisting the haze-volume step, blower setting, or the haze/bubble toggles — these start at defined defaults on every boot, and only the effect duration is stored.
- Mixing colors on the beam bar: the four emitters are driven independently and never blended in software.

## Decisions

### 1. Transmit DMX with the `DMXSerial` library in controller mode, direction pin on `D5`

`DMXSerial` drives the ATmega328P's hardware USART at 250 kbaud 8N2, generates the break/mark-after-break in its USART interrupts, pumps the frame in the background, and supports a dedicated direction pin (`DMXMODEPIN`) for a half-duplex transceiver — exactly the wiring this change uses (`DI → D1`, `DE/RE → D5`).

The direction pin **must** be passed explicitly as `D5` (`DMXSerial.init(DMXController, kDmxDirPin)`); the library defaults it to `D2`, which is the IR receiver here and would be driven out from under it.

Alternatives considered:

- **Conceptinetics** — also USART-based, but older, RDM-oriented, and heavier for a transmit-only need. Rejected.
- **Hand-rolled USART sender** — owning the break/MAB timing and the `UDRE`/`TXC` ISRs is precisely what the library already does well, and it would spend the project's effort on protocol plumbing rather than behavior. Rejected as the default, but it is the documented fallback if SRAM proves tight (see Risks): it would replace the library's fixed 513-byte universe buffer with a ~48-byte one.

The existing `IR_CAPTURE_MODE`/`PIR_DEBUG` serial logging is removed rather than kept as a mutually-exclusive build mode. Leaving it in would let a build compile and run normally while silently emitting no DMX — a trap on a firmware whose whole job is DMX — and its capture purpose is already served by the committed map in `ir_codes.h`.

### 2. Centralize the universe in one header

A `dmx_map.h` holds the fixture base addresses (beam bar `1`, haze `39`), the per-fixture channel offsets, and the fixture value bands. All writes go through small helpers (`setBar(bar, r, g, b, w)` and `bars(r, g, b, w)`) so an address or band change touches one file. This mirrors the existing `pins.h`/`settings.h`/`ir_codes.h` convention.

### 3. Beam bar: one emitter per beam, a bounded band, and a fixed dimmer

The fixture's R, G, B, and W emitters do not blend, so every effect drives **at most one emitter per beam at a time** and "white" is the W channel alone. Channels 3 (built-in effects) and 4 (their speed) are held at 0 so the fixture's own effects never engage. The horizontal position (channel 1) is bounded to 0–120 so the beams never face the sky, and parks at 56 whenever no effect drives it. The master dimmer (channel 5) is held at **full** for as long as the board is powered, and brightness is expressed through the per-bar colour values instead — which is what makes the single-emitter rule work, since a colour channel doubling as a brightness level needs no master fade.

Channel 2 (horizontal speed) must stay inside what the fixture tolerates: **255 breaks its tilt**, so the usable maximum is 200, with 100 used for drifting effects.

### 4. The device state machine

One arbiter governs every output: `OFF`, `IDLE`, `ACTIVE`, or `CLEANING`. The remote's power button toggles between **disabled** (`OFF`) and **enabled**; the LED reports that state, which is what makes `IDLE` — enabled but dark, waiting for an event — visibly different from disabled.

The device comes up **enabled and already running**, so the show starts on its own without a power press; the power button still stops it.

An **event** — enabling the device, a PIR motion event, or an EQ press — always starts a **newly chosen** effect and restarts its clock, so pressing eq during an effect moves straight on to another one rather than extending the current one. Motion *continuing* while an effect runs only extends that effect's deadline. `CLEANING` overrides everything and returns the device to the state it came from.

### 5. Effect engine: a table of effects classified by how they move

The engine holds a table pairing each effect with a movement classification:

- **DRIFT** — the effect lights the bars, and the engine adds a slow drift with a non-repeating three-sine wander on top.
- **JUMP** — the effect lights the bars and snaps the position itself; the engine sets the fast tilt speed.
- **OWN** — the effect drives the position as part of its own look.

Every effect is written against `now`, using sines, random walks, and phase accumulators rather than `delay()`, so the loop stays non-blocking. Effects write through the `dmx_map.h` helpers into the staging buffer, never the DMX buffer directly.

Alternatives considered: the per-effect, hand-rolled horizontal motion of the earlier implementation. It worked, but every effect re-invented movement, and they ended up reading alike; classifying movement once is what makes 15 effects look genuinely different.

### 6. Effect roster

| Effect | Kind | Look |
| --- | --- | --- |
| chase | drift | hard steps, one bar at a time, red |
| wave | drift | smooth rolling sine, red |
| comet | drift | a bright head with a fading tail, red |
| snake | drift | a body of four bars crawls and bounces, undulating; one random colour per activation |
| colour roll | drift | each bar a different single emitter, rotating at a wandering rate |
| ripple | drift | a wave expanding from the centre outward, green |
| twinkle | drift | random bars sparkle white |
| fire | drift | candle-like flicker on red |
| blood pulse | drift | slow, deep red breathing |
| random strobe | jump | irregular white flashes, snapping to a new spot each time |
| blinder | jump | full white with the strobe channel hard on, position snapping |
| lightning | jump | each flash strikes one or two bars, scattered, at a different point across the sky on a path drawn per bolt — sweeping out and back, jumping at random, or zig-zagging — on an uneven rhythm, with a bar struck by the previous flash likelier to be struck again so a bolt can linger; then a faint brief afterglow and a gap between bolts of up to two seconds |
| shake | own | the beam jitters hard around the park, red |
| pendulum | own | the beam swings, easing at the ends, red |
| crawl | own | the beam creeps across the band, slow and uneasy, red |

Every effect lights one emitter per beam, and lightning sequences its two colours rather than overlapping them.

### 7. Remote dispatch

Button handling stays in the loop's IR path. `power` toggles the device, `vol±` steps the duration, `play/pause` toggles the haze master enable, `func/stop` toggles haze, `st/rept` toggles bubbles, `0`–`3` set the blower, and `track±` steps the haze rate. `9` is unused.

`eq` needs both a single press and a three-second hold. A press fires an **event immediately** and arms the hold; if repeats keep arriving for three seconds, cleaning mode takes over. An earlier version waited for a gap in the repeats to detect the release, which made a single press depend on the remote's repeat timing — not something the firmware should rely on. A press while cleaning exits it.

### 8. Haze and bubble machine

Channels 39 (bubbles), 40 (haze volume), 41 (blower), and 42 (cleaning) are driven within the machine's documented bands: the blower is 0 for off, up to 100 low, 101–200 medium, and 201–255 high, and the haze rate steps through a short ladder inside its continuous-haze band.

A **master enable**, toggled by `play/pause` and on at power-on, silences the machine and makes the haze controls inert, leaving both settings sets untouched.

Otherwise the machine holds **two independent sets** of settings — idle and event — and uses whichever matches the device's current state. The remote's haze controls edit the set in use, so a setting is changed by making the change while the device is in the state it is meant for: press `0`–`3` while idle to set the idle blower, press it during an effect to set the event blower. A change made during an effect takes effect at once for that effect, and the next effect uses it too.

Idle defaults to off, so no fluid is spent between events, and an event defaults to haze on, bubbles on, and a full blower, so an effect arrives with the machine already working. An effect never changes either set; only *which* set is in use depends on it.

Both sets live in the EEPROM settings struct with the effect duration, so a blower speed, a toggle, or a rate survives a reboot. That moved the settings struct to a second version, so an older store is discarded rather than misread. Writes happen only when a haze control changes something, which keeps EEPROM wear far below the cell rating.

Both earlier attempts collapsed this into a single set, and each failed for a different reason. Forcing the machine on during an effect meant a press during an effect appeared to do nothing. Making the one set purely manual then lost the distinction between how the machine should behave in a gap and how it should behave during an effect — the gap would run as loudly and foggily as the effect.

### 9. Onboard LED as the device status light

The LED reports whether the device is **enabled**, staying on for both `ACTIVE` and `IDLE` and going off when disabled. The duration preview and the cleaning-mode flash overlay it. No indication changes the device state.

### 10. Verification: the fixture is the test, the monitor is optional

The primary check is the fixture itself: run the show and watch it. A `HAZE_BENCH` build flag holds the device enabled and idle with the haze and bubbles forced on, which is what localised the ground-loop noise that showed up on the bar's last light.

`tools/dmx_monitor.py` (pyserial, 250000 8N2, resync on the start code `0x00`) reads the universe off the bus and prints channels 1–42. It resyncs on the start code rather than relying on the adapter reporting the DMX break.

**The SH-RS09B cannot be used for this, and must not be left on the bus.** It is a DMX *controller* cable — FTDI FT232RL plus a MAX485, the same architecture ENTTEC describes as "output only" — and it delivered no bytes at all in testing: 250000, 115200, 9600, and 4800 baud, every RTS/DTR combination, its own transmitter looped back, and even a slow 9600-baud text pattern driven through the same MAX485. Its receiver path simply is not available. Worse, its driver is permanently enabled, so with it connected the beam bar showed random colours and horizontal movement; the correct colours appeared only once the adapter was removed from the pair. It contends with the Arduino's driver and garbles the stream. Channel-level readback therefore needs a genuinely bidirectional USB-RS485 adapter — a CP2102/CH340 dongle with an auto-direction transceiver such as the MAX13487 — and the script is written for that.

> **Superseded 2026-10-14:** the adapter does read the bus; see the archive note at the top of this file.

QLC+ was considered because fixtures may exist for these devices, but its DMX-USB input modes target specific DMX-USB protocols; QLC+ remains a useful reference for the fixtures' channel semantics.

### 11. Pin assignments and power

| Function | Pin | Notes |
| --- | --- | --- |
| Onboard LED | `D13` / `LED_BUILTIN` | Digital output; device status light. |
| IR receiver data out | `D2` | Digital input; unchanged. Must not be the DMX direction pin. |
| PIR data out | `D4` | Digital input; unchanged. |
| DMX direction (DE/RE) | `D5` | Digital output; `DMXSerial` direction pin. |
| DMX data out (DI) | `D1` | USART TX; drives the MAX485 driver input. |
| USART RX | `D0` | Unused (transmit-only); leave the MAX485 receiver output unconnected. |
| Power | `5V` / `GND` | MAX485 VCC/GND from the board's 5 V rail. |

- The MAX485 runs from the board's 5 V rail; DMX carries no power over the cable. Both fixtures are mains-powered independently (AC 100–240 V) and are never powered from the board.
- This board is the DMX **controlling node**, so the bus termination belongs here: a **120 Ω across the pair (XLR pins 2 and 3)**, plus the bias resistors of the reference DMX shield. An earlier attempt appeared to break the signal, but that turned out to be a connection rather than the resistor — with the termination properly fitted the bus runs clean, which is what a healthy MAX485 driving 120 Ω should do.
- The ground arrangement that does work: a **resistor in series between the Arduino's ground and the MAX485's ground**, with the **DMX common (pin 1) tied directly to the Arduino's ground**. That separates the transceiver's reference from the ground loop that runs host USB ground → board → cable common → haze machine mains earth, which is the path the machine's pump and heater noise was using. The cost is a small offset between the Arduino's logic reference and the transceiver's, so keep the resistor small (about 100 Ω) and check the drop across it stays well under 0.2 V. The durable fixes remain an isolated transceiver (ADM2483/ADM2587) or a USB isolator, either of which allows a direct ground.
- Haze and bubble fluids are consumables; the `play/pause` master enable silences the machine so the lighting can be tested without spending them.

### 12. Main loop versus interrupts and timers

- **Main loop**: IR decode and `resume()`, the eq press-versus-hold check, PIR polling, the device state machine, the effect update, the haze render, and the LED indication.
- **Interrupts / timers**: `DMXSerial` owns the USART `UDRE`/`TXC` interrupts that emit the break/MAB and pump the frame; `IRremote` owns Timer2 and the receiver's pin interrupt. Timer0 (`millis()`) and Timer1 are untouched, and this change adds no new timer.

### 13. Uno R3 resource limits the design relies on

- **SRAM**: `DMXSerial` allocates a fixed 513-byte universe buffer, and the staging/commit layer adds two 43-byte arrays. The build measures **1128 bytes (55%)**, leaving 920 bytes for locals. Effect state stays in scalars (phase, counters) — no arrays of floats.
- **Flash**: the build measures **16442 bytes (50%)** of the 32256 available.
- **Timers**: no new timer is claimed (see Decision 12).
- **EEPROM**: the settings struct holds the effect duration and both haze settings sets. The version was bumped when the haze sets moved in, so an older store falls back to defaults instead of being misread.
- **USART**: owned exclusively by DMX output; the firmware contains no `Serial` code at all.

### 14. Stage the frame, then publish it atomically

`DMXSerial` transmits the channel buffer **from an interrupt**, reading one channel at a time. A writer that touches several channels therefore hands that interrupt a half-updated frame, and the fixture briefly sees the intermediate values. That is not theoretical: clearing all 42 channels before re-setting them — which the test patterns did — leaves a window in which the buffer genuinely holds all zeros, so the fixture received intermittent black frames and stray strobe **on wiring that was otherwise fine**.

Writers therefore fill a **staging copy** in RAM, and `commitDmx()` publishes only the channels that differ, inside one brief critical section (saving and restoring `SREG`). A steady frame then costs nothing on the bus, and a change lands atomically. This is the single most important implementation constraint in this change, and it is worth keeping in mind for any future writer: never mutate the DMX buffer in place.

One subtlety cost a bug on the bench: the staging copy must **persist between passes**, and be cleared only when a *new effect* starts. Clearing it every pass looks harmless, but several effects hold a value between updates — lightning's flash, fire and twinkle's sparkle, the random strobe's lit frame — and clearing each pass wiped them microseconds after they were written, so their output never reached the bus at all. Clearing on the effect change keeps their held state while still stopping one effect's leftovers, such as the blinder's strobe channel, leaking into the next.

## Risks / Trade-offs

- **SRAM pressure from the fixed 513-byte buffer** → the build sits at 54%, so there is room, but keep new state scalar and small; if it ever tightens, fall back to the hand-rolled sender in Decision 1, which needs only a ~48-byte buffer for the 42 used channels.
- **USB serial monitor unavailable while DMX transmits** → the monitor role moves to the Mac on the DMX bus; the serial logging switches are removed so no build can silently disable DMX, and re-capturing a remote's codes would mean temporarily freeing the USART from DMX.
- **`DMXSerial`'s default direction pin is `D2`** → always pass `D5`; a wrong default would drive the IR receiver and break remote input.
- **`D0`/`D1` are also the USB-programming lines** → uploads still work because DMX only transmits after boot, but a fixture connected during an upload may briefly see malformed data; power the fixtures after upload or accept the glitch.
- **Generic USB-RS485 adapter may not expose the DMX break** → the monitor resyncs on the start code `0x00` and is documented as a passive read, not a conformance tester.
- **Long-press detection in a non-blocking loop** → a slow press could be misread; use a clear three-second threshold and treat any earlier release as a single press.
- **Random effects can look chaotic** → constrain every effect's motion to the idle-to-ceiling band, avoid immediate repeats, and keep intensity/speed tied to the settings so the operator can tame them.
- **A USB-to-DMX *controller* cable cannot monitor the bus** → the SH-RS09B is output-only; verification relies on the white-chase test pattern on the fixture, and channel-level readback needs a bidirectional USB-RS485 adapter. *(Superseded 2026-10-14: the SH-RS09B does monitor the bus; see the archive note at the top of this file.)*

- **Animated effects still land mid-frame** → the commit is atomic, so a frame is never torn *within* a write, but an effect that animates on every loop pass can still have its channels published between two of the transmit interrupt's reads. A frame can therefore blend two adjacent animation steps. The values are close, so it is not visible; publishing exactly once per frame would need a frame-boundary hook that `DMXSerial` does not expose.

- **Ground loop between the host's USB ground and the haze machine's mains earth** → the DMX common ties them together through the cable, and the machine's motors couple noise into the bar's last light whenever the machine is powered and running. Confirmed on the bench: a series resistor between the board's ground and the transceiver's, with the cable common tied directly to the board's ground, clears it. An isolated transceiver or a USB isolator is the durable fix.

## Migration Plan

- **Additive and in place**: the DMX, fixture, and effect code lands in `spooky-arduino/` alongside the existing behavior; the remote and LED semantics change where they already live. The EEPROM settings struct is reused unchanged, so there is no data migration.
- **Rollback**: revert the sketch; the fixtures return to standalone operation. No fixture settings are written by the firmware.
- **Test order** (matches the "lights first" intent): prove the DMX path on the beam bar first, then bring in the haze/bubble machine last, using the `play/pause` master enable while tuning the lights.

## Open Questions

- The safe band's ceiling (120) and the idle park (56) may need tuning once the bar is mounted; both are single constants.
- The haze rate ladder's six step values may need tuning; the step table is one place.
- The default blower setting at boot (design assumes low, so haze works once the machine is enabled) can be revisited without changing the specs.
- Whether to add a bidirectional USB-RS485 adapter for channel-level readback, or rely on the fixture, is a hardware choice outside this change.

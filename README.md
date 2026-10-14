# spooky-arduino

Spooky Arduino — a hands-on learning project exploring AI-assisted development for physical devices.

## Toolchain

- Board: Arduino Uno R3 (Elegoo) or compatible
- FQBN: `arduino:avr:uno`
- Core: `arduino:avr` (Arduino AVR Boards) — install with `arduino-cli core install arduino:avr`
- Libraries:
  - `IRremote` 4.7.1 — install with `arduino-cli lib install IRremote`
  - `DMXSerial` 1.5.3 — install with `arduino-cli lib install DMXSerial`
- Serial port (as currently detected): `/dev/cu.usbmodem11301`

Check the board and port with `arduino-cli board list`. The port changes if you
use a different USB port, a different cable, or a different board.

## Build and upload

Compile the sketch:

```
arduino-cli compile --fqbn arduino:avr:uno spooky-arduino
```

Upload it to the board:

```
arduino-cli upload --fqbn arduino:avr:uno -p /dev/cu.usbmodem11301 spooky-arduino
```

## `spooky-arduino`

The project's single firmware. It drives DMX512 lighting from the IR remote and
the PIR sensor:

- A **device master enable/disable** (the remote's power button) gates everything.
  Disabled is silent and the PIR is ignored. Enabled means the device is on: an
  effect starts, and when it finishes the fixture goes dark and waits for the
  next event. The onboard LED reports the enabled state. **The device comes up
  enabled and already running**, so the show starts without a power press.
- An **event** — enabling the device, a single PIR motion event, or an `eq` press —
  starts a randomly chosen effect.
- The **haze/bubble machine** holds two independent sets of settings — one for
  **idle** time and one **during an effect** — and uses whichever matches the
  device's current state. Pressing a haze control edits the set for the state you
  are in, so each can be tuned independently.
- A **master enable** (the remote's `play/pause`) silences the machine entirely, so
  the lights can be tested without consuming fluid.

| Item | Value |
| --- | --- |
| Onboard LED pin | `D13` / `LED_BUILTIN` (`pins.h`) |
| IR receiver signal pin | `D2` (`pins.h`) |
| PIR signal pin | `D4` (`pins.h`) |
| DMX direction pin (DE/RE) | `D5` (`pins.h`) |
| DMX data out (DI) | `D1` (USART TX) |
| Libraries | `IRremote` 4.7.1, `DMXSerial` 1.5.3 |
| Flash used | 13110 bytes (40% of 32256 bytes) |
| SRAM used | 967 bytes (47% of 2048 bytes) |
| Default duration | 5 seconds (`kDefaultTimeoutSec` in `settings.h`) |
| Duration range | 1 s – 300 s, stored in EEPROM |
| Beam bar | SHEHDS 8x12W RGBW, address 1, 38-channel mode |
| Haze/bubble machine | Chauvet Hurricane Bubble Haze, address 39 |

### Wiring

- **IR receiver module** — pins labelled `G` (ground), `R` (voltage), `Y` (signal):
  `G` → `GND`, `R` → `5V`, `Y` → `D2`. Wire by the silkscreen labels, not by position.
- **HC-SR501 PIR** — `VCC` → `5V`, `GND` → `GND`, `OUT` → `D4`. Turn the
  time-delay trimpot fully counter-clockwise (the firmware owns the timing) and
  set the trigger jumper to repeatable. The sensitivity trimpot turned clockwise
  increases the detection range (about 3 m to 7 m).
- **MAX485 (or equivalent) RS-485 transceiver** — `VCC` → `5V`, `GND` → `GND`
  directly, `DI` (driver input) → `D1`, `DE` and `RE` tied together → `D5`.
  Leave `RO` (receiver output) unconnected; the board only transmits. This board
  is the DMX **controlling node**. The module in use is auto-direction and
  ignores the direction pin, but the pin is still assigned so the library never
  falls back to its default of `D2`.
- **Ground.** The transceiver's ground is tied **directly** to the board's
  ground, and the DMX cable's common (pin 1) to the same point. An earlier build
  put a series resistor (about 100 Ω) between the board's ground and the
  transceiver's ground, to break the loop that the host's USB ground, the board,
  the cable common, and the haze machine's mains earth form. That resistor has
  been **removed**: the interference on the bar's last light that prompted it was
  the faulty cable connection below, not the loop, and with the connection sound
  the direct ground runs clean. An isolated transceiver (ADM2483/ADM2587) or a
  USB isolator remains the durable fix should the loop ever couple noise in.
- **A 120 Ω terminator is fitted across the pair (XLR pins 2 and 3)**, as the DMX
  controlling node should have. An earlier attempt appeared to break the signal,
  but that was a connection rather than the resistor.
- **Check the cable before blaming anything else.** A single loose pin in a DMX
  connector reads as total silence — the fixture does nothing and a monitor sees
  zero bytes — and is easy to blame on the adapter or the ground. One pin in this
  build's adapter cable came loose and was resoldered; with that, the terminator,
  and the direct ground, the bus runs clean end to end.
- Both fixtures are mains-powered independently and are never powered from the
  board. Haze and bubble fluids are consumables — use the `play/pause` master
  enable to silence the machine while testing the lights.

The DMX direction pin is `D5` deliberately: `DMXSerial` defaults its direction
pin to `D2`, which is the IR receiver here, so the pin is always passed
explicitly.

### DMX channel maps

The beam bar is addressed 1 in 38-channel mode; the haze machine is addressed
39. Together they occupy channels 1–42 of a full 512-channel universe.

**Beam bar (channels 1–38)**

| Channel | Function |
| --- | --- |
| 1 | Horizontal position — capped at 120; parks at 56 when no effect drives it |
| 2 | Horizontal speed |
| 3–4 | Built-in effects and effect speed — held at 0, never used |
| 5 | Total dimming |
| 6 | Strobe |
| 7–10 | BAR1 R, G, B, W |
| 11–14 | BAR2 R, G, B, W |
| … | … |
| 35–38 | BAR8 R, G, B, W |

The four emitters per bar are independent and never mixed in software: with R,
G, B, and W all on for one beam, the beam shows four separate colors rather than
magenta or white.

**Haze/bubble machine (channels 39–42)**

| Channel | Function | Values |
| --- | --- | --- |
| 39 | Bubble output | 0–10 off, 11–255 on |
| 40 | Haze volume | 0–10 no function, then a six-step ladder inside the continuous-haze band |
| 41 | Blower speed | 0–10 off, 11–100 low, 101–200 medium, 201–255 high |
| 42 | Bubble cleaning mode | 0–10 off, 11–255 on |

Channel assignments live in `spooky-arduino/dmx_map.h`.

### Effects

Fifteen named effects make up the roster. One runs for the configured duration,
then the fixture goes dark and waits for the next event — enabling the device, a
PIR motion event, or an `eq` press — which picks another at random.

Every effect lights **at most one emitter per beam** and moves the beam inside
its safe band. Each is one of three kinds: **drifting** (a slow drift plus a
non-repeating wander), **jumping** (it snaps its position as it flashes), or
**own** (it drives the position itself).

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

The haze machine is separate: it uses its **event** settings while an effect runs
and its **idle** settings in between, and an effect changes neither.

### Controls

| Button | Action |
| --- | --- |
| power | Enable / disable the device. The onboard LED reports it |
| vol+ / vol- | Step the effect duration along the ladder in `settings.h` (1–10 s in 1 s steps, then 20, 30, 60, 120, 180, 240, 300 s), previewed on the LED |
| eq (press) | An event: start a new random effect and restart the duration, or leave bubble cleaning mode |
| eq (hold 3 s) | Enter bubble cleaning mode and stop all effects |
| play/pause | Master enable for the haze machine |
| func/stop | Toggle haze |
| st/rept | Toggle bubbles |
| 0 / 1 / 2 / 3 | Blower off / low / medium / high |
| trackleft / trackright | Haze volume down / up |

**What the device does.** Power toggles between disabled and enabled. Disabled
means everything is silent and the PIR is ignored. Enabled means the device is
on: the LED is lit, an effect starts, and when that effect finishes the fixture
goes dark and waits for the next event — so the LED lit over a dark fixture means
"enabled and waiting", not "off".

**The LED** is the device status light: on whenever the device is enabled,
whether an effect is running or the fixture is idle, and off while disabled. The
duration preview and the cleaning-mode flash overlay it.

**The duration preview** blinks the LED quickly, once per second of the new value
(5 s is five quick flashes), then returns the LED to reporting the status.

**The haze machine** has two independent sets of settings: **idle**, used between
effects and off by default so no fluid is spent waiting, and **event**, used
during an effect and defaulting to haze on, bubbles on, and a full blower. Each
control edits the set for the state you are in — press `0`–`3` while idle to set
the idle blower, press it during an effect to set the event blower. A change made
during an effect applies at once, and the next effect uses it too.

**Both sets are saved**, so a blower speed, a toggle, or a rate survives a reboot.
The `play/pause` master enable is not saved and starts on every boot.

**The eq button** fires an event on the press itself. Holding it for three seconds
enters cleaning mode instead; a press while cleaning exits it.

### Remote button map

All buttons are NEC, address `0x00` (`ir_codes.h`):

| Button | Code | Button | Code | Button | Code |
| --- | --- | --- | --- | --- | --- |
| power | `0x45` | vol+ | `0x46` | vol- | `0x15` |
| func/stop | `0x47` | trackleft | `0x44` | play/pause | `0x40` |
| trackright | `0x43` | down | `0x07` | up | `0x09` |
| eq | `0x19` | st/rept | `0x0D` | 0 | `0x16` |
| 1 | `0x0C` | 2 | `0x18` | 3 | `0x5E` |
| 4 | `0x08` | 5 | `0x1C` | 6 | `0x5A` |
| 7 | `0x42` | 8 | `0x52` | 9 | `0x4A` |

play/pause is the haze machine's master enable. down, up, and 4–8 are decoded
but unused. Distant or off-axis remote presses can be seen by the receiver but
not decode into a valid command; those frames are reported as `UNKNOWN` and
correctly ignored.

There is no serial capture mode: DMX output owns the board's single USART, so
re-capturing a remote's codes would mean temporarily freeing the USART from DMX.

## Verifying the DMX output

The fixture is the test. Flash a test pattern (below) and watch the beam bar: a
white chase stepping from bar 1 to bar 8 proves the MAX485 and the DMX wiring
end to end, with no extra hardware.

The USB serial monitor is unavailable — `DMXSerial` owns the board's single
hardware USART, which is the same port the Uno's USB-serial converter is wired
to — so board-side logging cannot be used while DMX runs.

`tools/dmx_monitor.py` reads the universe off the bus instead, and the DSD Tech
SH-RS09B works for it. The adapter is FT232R + MAX485, and its FT232R is
configured for FTDI auto-direction (`CBUS2 = TXDEN`), so its driver is not
permanently enabled and its receiver is active whenever it is not transmitting.
Confirmed on the bench: it reads the full stream off the bus — about 22.6 kB/s,
the 250 kbaud limit — and decodes the frames to the same channel values the
fixture is being sent.

```
python3 -m pip install pyserial
python3 tools/dmx_monitor.py --list
python3 tools/dmx_monitor.py /dev/cu.usbserial-B0037A1S
python3 tools/dmx_monitor.py /dev/cu.usbserial-B0037A1S --once --all
python3 tools/dmx_monitor.py /dev/cu.usbserial-B0037A1S --raw --seconds 3
python3 tools/dmx_monitor.py /dev/cu.usbserial-B0037A1S --watch
```

`--raw` reports the raw byte count with no framing, and `--watch` prints the
byte rate once a second; both are for working on the wiring. A fault there reads
as **no bytes at all**, not as garbage, so check the count first — a single loose
pin in a connector looks exactly like a dead adapter, and cost a wrong diagnosis
once already (see the wiring notes above).

The monitor is a passive read, not a conformance tester. It resynchronises on
the long run of zero channels at the end of each frame rather than relying on
the adapter reporting the DMX break.

## Bench build

Before trusting a fixture, build with the bench flag. It holds the device enabled
and idle with the haze and bubbles forced on and no effect running, so the
machine's channels carry data without any button press. That is how the
ground-loop interference on the bar's last light was localised. The remote does
nothing in this build; flash the normal build afterwards.

```
# bench build: idle, haze and bubbles forced on
arduino-cli compile --upload -p /dev/cu.usbmodem11401 --fqbn arduino:avr:uno \
  --build-property "compiler.cpp.extra_flags=-DHAZE_BENCH" spooky-arduino

# lightning only, running without gaps, for tuning it by eye
arduino-cli compile --upload -p /dev/cu.usbmodem11401 --fqbn arduino:avr:uno \
  --build-property "compiler.cpp.extra_flags=-DLIGHTNING_ONLY" spooky-arduino

# back to normal operation
arduino-cli compile --upload -p /dev/cu.usbmodem11401 --fqbn arduino:avr:uno spooky-arduino
```

## Where things live

Pin assignments live in `spooky-arduino/pins.h`, the DMX universe and fixture
channel maps in `spooky-arduino/dmx_map.h`, the duration settings in
`spooky-arduino/settings.h`, the effects in `spooky-arduino/effects.h`, and the
captured remote codes in `spooky-arduino/ir_codes.h`, so each can change in one
place.

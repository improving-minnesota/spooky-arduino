# spooky-arduino

Spooky Arduino — a hands-on learning project exploring AI-assisted development for physical devices.

## Toolchain

- Board: Arduino Uno R3 (Elegoo) or compatible
- FQBN: `arduino:avr:uno`
- Core: `arduino:avr` (Arduino AVR Boards) — install with `arduino-cli core install arduino:avr`
- Library: `IRremote` 4.7.1 — install with `arduino-cli lib install IRremote`
- Serial port (as currently detected): `/dev/cu.usbmodem11401`

Check the board and port with `arduino-cli board list`. The port changes if you
use a different USB port, a different cable, or a different board.

## Build and upload

Compile the sketch:

```
arduino-cli compile --fqbn arduino:avr:uno spooky-arduino
```

Upload it to the board:

```
arduino-cli upload --fqbn arduino:avr:uno -p /dev/cu.usbmodem11401 spooky-arduino
```

## `spooky-arduino`

The project's single firmware. It turns the onboard LED on when the PIR sensor
detects motion, and turns it off after a configurable timeout once motion stops.
The IR remote's power button also turns the LED on (for the same timeout) or
forces it off, and the volume buttons adjust the timeout. The timeout is stored
in EEPROM, so it survives power cycles.

| Item | Value |
| --- | --- |
| Onboard LED pin | `D13` / `LED_BUILTIN` (`pins.h`) |
| IR receiver signal pin | `D2` (`pins.h`) |
| PIR signal pin | `D4` (`pins.h`) |
| Library | `IRremote` 4.7.1 |
| Flash used | 8424 bytes (26% of 32256 bytes) |
| SRAM used | 373 bytes (18% of 2048 bytes) |
| Default timeout | 5 seconds (`kDefaultTimeoutSec` in `settings.h`) |
| Timeout range | 1 s – 300 s, stored in EEPROM |

### Wiring

- **IR receiver module** — pins labelled `G` (ground), `R` (voltage), `Y` (signal):
  `G` → `GND`, `R` → `5V`, `Y` → `D2`. Wire by the silkscreen labels, not by position.
- **HC-SR501 PIR** — `VCC` → `5V`, `GND` → `GND`, `OUT` → `D4`. Turn the
  time-delay trimpot fully counter-clockwise (the firmware owns the timeout) and
  set the trigger jumper to repeatable. The sensitivity trimpot turned clockwise
  increases the detection range (about 3 m to 7 m).

### Controls

- **Power** — turns the LED on for the configured timeout, or forces it off if it
  is already on.
- **Volume up / down** — move the timeout one step along the ladder in
  `settings.h`: 1–10 s in 1 s steps, then 20, 30, 60, 120, 180, 240, 300 s. The
  new value is previewed by flashing the LED once per second and is saved to EEPROM.
- All other buttons are decoded and recorded in `ir_codes.h` for later features.

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

The codes are specific to the remote supplied with the kit. To re-capture them
(for example, with a different remote), set `IR_CAPTURE_MODE` to `1` in
`spooky-arduino.ino`, upload, press each button, and read the decoded protocol,
address, and command over the serial monitor at 9600 baud.

### Debug logging

Set `IR_CAPTURE_MODE` to `1` to log every decoded IR frame, and `PIR_DEBUG` to
`1` to log PIR motion transitions, both over the serial monitor at 9600 baud.
Leave both at `0` for normal operation. Note that distant or off-axis remote
presses can be seen by the receiver but not decode into a valid command; those
frames are reported as `UNKNOWN` and correctly ignored.

Pin assignments live in `spooky-arduino/pins.h`, the timeout settings in
`spooky-arduino/settings.h`, and the captured remote codes in
`spooky-arduino/ir_codes.h`, so each can change in one place.

# spooky-arduino

Spooky Arduino — a hands-on learning project exploring AI-assisted development for physical devices.

## Toolchain

- Board: Arduino Uno R3 (Elegoo) or compatible
- FQBN: `arduino:avr:uno`
- Core: `arduino:avr` (Arduino AVR Boards) — install with `arduino-cli core install arduino:avr`
- Serial port (as currently detected): `/dev/cu.usbmodem11301`

Check the board and port with `arduino-cli board list`. The port changes if you
use a different USB port, a different cable, or a different board.

## Build and upload

Compile a sketch:

```
arduino-cli compile --fqbn arduino:avr:uno blink-led
```

Upload it to the board:

```
arduino-cli upload --fqbn arduino:avr:uno -p /dev/cu.usbmodem11301 blink-led
```

## Sketches

### `blink-led`

Blinks the onboard LED 1 second on / 1 second off for as long as the board is
powered. Needs no external components and no libraries beyond the Arduino AVR
core.

| Item | Value |
| --- | --- |
| Onboard LED pin | `D13` / `LED_BUILTIN`, declared in `blink-led/pins.h` |
| Blink interval | 1000 ms on / 1000 ms off (`kBlinkIntervalMs` in `blink-led/blink-led.ino`) |
| Flash used | 834 bytes (2% of 32256 bytes) |
| SRAM used | 14 bytes (0% of 2048 bytes) |
| Libraries | none — Arduino AVR core only |

Pin assignments live in `blink-led/pins.h` so they can be changed in one place.

### `ir-led-control`

Toggles the onboard LED on and off each time the IR remote's power button is
pressed. The LED is steady on or off (it does not blink), and the state persists
until the next press. Needs the kit's IR receiver module wired to the board.

| Item | Value |
| --- | --- |
| IR receiver signal pin | `Y` → `D2` (input with internal pull-up), declared in `ir-led-control/pins.h` |
| IR receiver power pins | `R` → `5V`, `G` → `GND` |
| Onboard LED pin | `D13` / `LED_BUILTIN`, declared in `ir-led-control/pins.h` |
| Remote power button | NEC, address `0x00`, command `0x45` (`ir-led-control/ir_codes.h`) |
| Library | `IRremote` 4.7.1 — install with `arduino-cli lib install IRremote` |
| Flash used | 7316 bytes (22% of 32256 bytes) |
| SRAM used | 313 bytes (15% of 2048 bytes) |

The receiver module's pins are labelled `G` (ground), `R` (voltage) and `Y`
(signal): `G` → `GND`, `R` → `5V`, `Y` → `D2`. Wire by the silkscreen labels,
not by position.

The power-button code is specific to the remote supplied with the kit. To
re-capture it (for example, with a different remote), set `IR_CAPTURE_MODE` to
`1` in `ir-led-control.ino`, upload, and read the decoded protocol and value
over the serial monitor at 9600 baud.

Pin assignments live in `ir-led-control/pins.h` and the captured remote codes in
`ir-led-control/ir_codes.h` so they can be changed in one place.

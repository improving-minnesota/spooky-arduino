# dmx-output Specification

## Purpose

Owns the DMX512 transmit layer: the channel universe, fixture addressing, the
250 kbaud 8N2 frame with break, the direction pin, non-blocking transmission,
atomic frame publication, the dark parked output while the device is disabled,
and the guarantee that a receiver decodes the intended channel values.

## Requirements

### Requirement: DMX512 universe is transmitted continuously

The system SHALL transmit a DMX512 universe of 512 channels at 250 kbaud, 8 data
bits, no parity, and 2 stop bits, preceded by a break and a mark-after-break, and
SHALL repeat the frame continuously while the board is powered. Each frame SHALL
begin with the DMX start code (0x00) followed by the 512 channel values.

#### Scenario: A monitor sees a repeating frame

- **WHEN** a DMX monitor is attached to the DMX path and the board is powered
- **THEN** it reads a continuous stream of frames, each starting with the start code 0x00 followed by 512 channel values

#### Scenario: Frames are separated by a break

- **WHEN** a monitor examines the stream between frames
- **THEN** it observes a break and a mark-after-break before the next start code

### Requirement: Fixture addressing matches the configured addresses

The system SHALL address the beam bar at DMX address 1 in its 38-channel mode
(channels 1–38) and the haze/bubble machine at DMX address 39 (channels 39–42),
so the two fixtures occupy channels 1–42 and the transmitted addresses match the
addresses configured on the fixtures.

#### Scenario: Beam bar is driven at address 1

- **WHEN** the firmware drives the beam bar
- **THEN** it writes the bar's values to channels 1–38, matching the bar's configured start address of 1

#### Scenario: Haze machine is driven at address 39

- **WHEN** the firmware drives the haze/bubble machine
- **THEN** it writes the machine's values to channels 39–42, matching the machine's configured start address of 39

### Requirement: Half-duplex direction uses the assigned pin

The system SHALL assign the RS-485 direction pin (DE/RE) to the pin in the
project's pin map (D5) rather than the DMX library's default, so the IR receiver
on D2 is never disturbed. The transceiver module in use is auto-direction, so it
may ignore the pin.

#### Scenario: Direction pin is D5

- **WHEN** the board initialises DMX output
- **THEN** the direction pin is D5, and D2 is left for the IR receiver

#### Scenario: IR receiver is unaffected

- **WHEN** DMX output is active
- **THEN** remote button presses are still decoded, because the direction pin does not share the IR receiver's pin

### Requirement: A frame is never transmitted partially updated

The system SHALL publish channel changes so that a transmitted frame is always
internally consistent — never a blend of the previous frame and the next one.
Writers SHALL stage their values, and the staged frame SHALL be committed in a
single step that the transmit path cannot interrupt. Transient states a writer
passes through, such as clearing channels before setting them, SHALL NOT reach
the bus.

#### Scenario: The fixture never sees a half-updated frame

- **WHEN** an effect or test pattern changes several channels at once
- **THEN** every transmitted frame carries either the previous values or the new ones, never a mixture

#### Scenario: Transient states never reach the bus

- **WHEN** a writer clears channels before setting them
- **THEN** the cleared values are never transmitted, so the fixture does not flash dark or strobe

#### Scenario: An effect that holds a state is not wiped between passes

- **WHEN** an effect writes a channel and then leaves it unchanged for several passes, as lightning's flash or fire's sparkle does
- **THEN** the channel keeps that value on the bus until the effect changes it, rather than being cleared by the next pass

#### Scenario: A steady frame costs nothing on the bus

- **WHEN** nothing has changed since the last frame
- **THEN** no channel is rewritten

### Requirement: Transmission does not block the main loop

The system SHALL transmit the universe without suspending the main loop, so IR
input, PIR sensing, and effect updates remain responsive while DMX output is
active.

#### Scenario: IR input stays responsive while transmitting

- **WHEN** the board is transmitting DMX and a remote button is pressed
- **THEN** the press is decoded and acted on promptly, with no long blocking delay

#### Scenario: Effects keep updating while transmitting

- **WHEN** an effect is running and DMX output is active
- **THEN** the effect's channel values continue to advance over time

### Requirement: Output is dark and parked while the device is disabled

While the device is disabled, the system SHALL leave the fixture dark — every
colour channel at 0 — with the bar parked at its idle position and the master
dimmer at full, and SHALL continue transmitting valid frames so the fixtures do
not hold stale values or lose signal.

#### Scenario: Disabling the device darkens the fixture

- **WHEN** the device is disabled
- **THEN** every bar colour channel reads 0, the horizontal position holds the idle value, and the dimmer reads full

#### Scenario: Transmission continues while disabled

- **WHEN** the device is disabled
- **THEN** a monitor still reads a continuous, valid DMX stream rather than a loss of signal

#### Scenario: Enabling hands the channels to the effects

- **WHEN** the device is enabled and an effect runs
- **THEN** the effects drive the fixture channels and a monitor reads their current values

### Requirement: The transmitted universe is decodable by a compliant receiver

The system SHALL produce a valid DMX512 signal that a compliant DMX receiver
decodes into the channel values the firmware intends, so the path can be verified
with a fixture or a monitor. A documented verification procedure SHALL exist for
wiring and signal checking.

#### Scenario: A receiver decodes the intended values

- **WHEN** the firmware sets a known pattern and a compliant receiver decodes the universe
- **THEN** the values it reports match the intended channel values

#### Scenario: A wiring fault is visible

- **WHEN** the DMX path is wired incorrectly
- **THEN** the fixture shows no valid output, exposing the fault before any fixture is trusted

### Requirement: The single UART is owned by DMX output

The system SHALL use the board's single hardware USART exclusively for DMX
output, and SHALL NOT use the USB serial monitor.

#### Scenario: No USB serial monitor

- **WHEN** the firmware is built for normal operation
- **THEN** it compiles with no serial-monitor code and does not depend on the USB serial monitor

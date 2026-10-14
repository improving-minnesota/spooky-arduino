# ir-led-control Specification

## Purpose

Lets the IR remote supplied with the kit control the board's onboard LED: each press of the remote's power button turns the LED on or off. It gives the project its first working input path — a remote command that produces an immediately visible response — and establishes the IR decoding behavior later remote-controlled features will reuse.

## Requirements

### Requirement: Remote power button enables and disables the device

The system SHALL toggle the device between enabled and disabled each time the
remote's power button is pressed. While the device is disabled everything is
silent and the PIR is ignored; while it is enabled the device is on and awaiting
or running an effect. The onboard LED SHALL report that state. No external LED or
additional wiring beyond the IR receiver SHALL be required.

#### Scenario: The device is enabled from disabled

- **WHEN** the device is disabled and the remote's power button is pressed once
- **THEN** the device is enabled, the onboard LED comes on, and an effect starts

#### Scenario: The device is disabled from enabled

- **WHEN** the device is enabled and the remote's power button is pressed once
- **THEN** the device is disabled, everything goes silent, and the onboard LED goes off

#### Scenario: Enabling works from either enabled state

- **WHEN** the device is enabled and idle, and the power button is pressed
- **THEN** the device is disabled, rather than starting another effect

#### Scenario: No external components required for the LED

- **WHEN** the board is powered with only the IR receiver attached and no external LED or resistor wired up
- **THEN** the onboard LED still reports the device's state in response to the remote's power button

### Requirement: Defined state at startup

The system SHALL bring the device up in a known, predictable state after
power-on or reset, without requiring any remote input or host interaction, so the
show begins on its own. At startup the device SHALL be **enabled**, the onboard
LED SHALL be on, and an effect SHALL already be running.

#### Scenario: The device starts enabled and running

- **WHEN** the board is powered on or reset
- **THEN** the device is enabled, the onboard LED is on, and an effect is running before any button is pressed

#### Scenario: The power button still stops it

- **WHEN** the board has just started and the remote's power button is pressed
- **THEN** the device is disabled and the onboard LED goes off

### Requirement: Only the power button changes the device's enabled state

The system SHALL change the device's enabled state only in response to the
remote's power button. The remote's other buttons act on the effects and the haze
machine and SHALL NOT change the enabled state. IR noise from unrelated sources
SHALL NOT change it.

#### Scenario: Other buttons do not change the enabled state

- **WHEN** any button other than power is pressed while the device is enabled or disabled
- **THEN** the device's enabled state does not change, though the button's own function may take effect

#### Scenario: Volume buttons do not change the enabled state

- **WHEN** a volume button is pressed
- **THEN** the enabled state is unchanged and only the effect duration is adjusted and previewed

#### Scenario: Unrecognized IR signals are ignored

- **WHEN** the receiver picks up an IR signal that does not match a known remote button
- **THEN** the enabled state does not change and the board keeps running normally

### Requirement: Onboard LED indicates the device status

The system SHALL use the onboard LED as the device's status light. It SHALL be on
whenever the device is enabled — whether an effect is running or the fixture is
idle and dark — and off while the device is disabled. The duration preview and
bubble cleaning mode SHALL overlay that indication, and no indication SHALL
change the enabled state.

#### Scenario: The LED reports the enabled state

- **WHEN** the device is enabled
- **THEN** the onboard LED is on, whether an effect is running or the fixture is idle

#### Scenario: The LED reports the disabled state

- **WHEN** the device is disabled
- **THEN** the onboard LED is off

#### Scenario: A preview overlays the indication

- **WHEN** the effect duration is changed
- **THEN** the LED shows the preview and then returns to reporting the enabled state, without changing that state

#### Scenario: Cleaning mode flashes the LED

- **WHEN** bubble cleaning mode is active
- **THEN** the LED flashes to show the mode

#### Scenario: Indications never change the enabled state

- **WHEN** any status indication is shown
- **THEN** the device's enabled state is unchanged

### Requirement: IR input is decoded without blocking the main loop

The system SHALL keep decoding IR remote signals while continuing to run its main loop, so remote input stays responsive and does not stall the board.

#### Scenario: Presses are registered while the loop keeps running

- **WHEN** the board is running normally and the power button is pressed
- **THEN** the press is decoded promptly and the LED responds, with the main loop never suspended for a long blocking delay

#### Scenario: Repeated presses each register

- **WHEN** the power button is pressed several times in quick succession
- **THEN** each decoded press toggles the LED, so the final state reflects the number of presses

### Requirement: Firmware builds and runs on the Uno R3 with the IR and DMX libraries

The system SHALL build and run on the Arduino Uno R3 using the standard Arduino
core plus the IR-decoding library and the DMX library the project adopts, and
SHALL fit within the board's limited flash and RAM. Because DMX output owns the
board's single hardware USART, the USB serial monitor SHALL NOT be required for
normal operation.

#### Scenario: Build with the required library installed

- **WHEN** the sketch is compiled for the Uno R3 with the standard core, the IR-decoding library, and the DMX library installed
- **THEN** compilation succeeds with no missing-library errors and the reported flash and RAM usage stay inside the board's limits

#### Scenario: Upload to a connected board succeeds

- **WHEN** a board is connected over USB and the compiled sketch is uploaded with the board and port selected
- **THEN** the upload completes without error and the remote controls the device immediately afterward

#### Scenario: No USB serial monitor is needed

- **WHEN** the firmware runs in normal operation
- **THEN** it does not depend on the USB serial monitor, which is unavailable while DMX output transmits

### Requirement: Every kit-remote button is decoded and recorded

The system SHALL decode the kit's IR remote and recognize every button on it, recording each button's protocol, address, and command so later features can bind to them. Each button SHALL be distinguishable from the others.

#### Scenario: Each button is distinguished

- **WHEN** each button on the kit remote is pressed in turn
- **THEN** the firmware reports a stable, distinct identifier for every button

#### Scenario: The button map is recorded

- **WHEN** the capture is complete
- **THEN** every button's protocol, address, and command is recorded for later use

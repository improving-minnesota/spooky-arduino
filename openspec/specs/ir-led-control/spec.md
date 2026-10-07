# ir-led-control Specification

## Purpose

Lets the IR remote supplied with the kit control the board's onboard LED: each press of the remote's power button turns the LED on or off. It gives the project its first working input path — a remote command that produces an immediately visible response — and establishes the IR decoding behavior later remote-controlled features will reuse.

## Requirements

### Requirement: Remote power button toggles the onboard LED

The system SHALL toggle the board's onboard LED between on and off each time the remote's power button is pressed. Each press SHALL change the LED to the opposite state, and the new state SHALL persist until the next press. No external LED or additional wiring beyond the IR receiver SHALL be required.

#### Scenario: LED turns on when it is off

- **WHEN** the LED is off and the remote's power button is pressed once
- **THEN** the onboard LED turns on and stays on

#### Scenario: LED turns off when it is on

- **WHEN** the LED is on and the remote's power button is pressed once
- **THEN** the onboard LED turns off and stays off

#### Scenario: State persists between presses

- **WHEN** the LED has been left on (or off) and no further button press occurs
- **THEN** the LED keeps that state indefinitely, with no timeout and no need to repeat the command

#### Scenario: No external components required for the LED

- **WHEN** the board is powered with only the IR receiver attached and no external LED or resistor wired up
- **THEN** the onboard LED still responds to the remote's power button

### Requirement: Defined LED state at startup

The system SHALL bring the LED up in a known, predictable state after power-on or reset, without requiring any remote input or host interaction, so the first button press always produces an observable change.

#### Scenario: LED starts in the off state

- **WHEN** the board is powered on or reset
- **THEN** the onboard LED is off before any button is pressed

#### Scenario: First press after startup turns the LED on

- **WHEN** the board has just started and the remote's power button is pressed
- **THEN** the LED turns on, confirming the remote is being decoded

### Requirement: Only the power button changes the LED state

The system SHALL change the LED state only in response to the remote's power button. Presses of other buttons on the remote, and IR noise from unrelated sources, SHALL NOT change the LED state.

#### Scenario: Other buttons are ignored

- **WHEN** any button other than the power button is pressed while the LED is on or off
- **THEN** the LED state does not change

#### Scenario: Unrecognized IR signals are ignored

- **WHEN** the receiver picks up an IR signal that does not match a known remote button
- **THEN** the LED state does not change and the board keeps running normally

### Requirement: IR input is decoded without blocking the main loop

The system SHALL keep decoding IR remote signals while continuing to run its main loop, so remote input stays responsive and does not stall the board.

#### Scenario: Presses are registered while the loop keeps running

- **WHEN** the board is running normally and the power button is pressed
- **THEN** the press is decoded promptly and the LED responds, with the main loop never suspended for a long blocking delay

#### Scenario: Repeated presses each register

- **WHEN** the power button is pressed several times in quick succession
- **THEN** each decoded press toggles the LED, so the final state reflects the number of presses

### Requirement: Firmware builds and runs on the Uno R3 with the IR library

The system SHALL build and run on the Arduino Uno R3 using the standard Arduino core plus the single IR-decoding library the project adopts, so it works on a machine with no other third-party libraries installed and fits within the board's limited flash and RAM.

#### Scenario: Build with the required library installed

- **WHEN** the sketch is compiled for the Uno R3 with the standard core and the IR-decoding library installed
- **THEN** compilation succeeds with no missing-library errors and the reported flash and RAM usage stay well inside the board's limits

#### Scenario: Upload to a connected board succeeds

- **WHEN** a board is connected over USB and the compiled sketch is uploaded with the board and port selected
- **THEN** the upload completes without error and the remote controls the LED immediately afterward

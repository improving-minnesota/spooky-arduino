## MODIFIED Requirements

### Requirement: Remote power button toggles the onboard LED

The system SHALL toggle the board's onboard LED between on and off each time the remote's power button is pressed. When the LED is off, a press SHALL turn it on for the configured timeout, after which it SHALL turn off automatically unless another activation occurs. When the LED is on, a press SHALL turn it off immediately and keep it off until the next activation (motion or another power press). No external LED or additional wiring beyond the IR receiver SHALL be required.

#### Scenario: LED turns on when it is off

- **WHEN** the LED is off and the remote's power button is pressed once
- **THEN** the onboard LED turns on and stays on for the configured timeout

#### Scenario: LED turns off when it is on

- **WHEN** the LED is on and the remote's power button is pressed once
- **THEN** the onboard LED turns off immediately and stays off until the next activation

#### Scenario: Power-on activation times out

- **WHEN** the power button turns the LED on and no further activation occurs
- **THEN** the LED turns off after the configured timeout, rather than staying on indefinitely

#### Scenario: State persists between presses

- **WHEN** the LED has been turned on by the power button and no further activation occurs
- **THEN** the LED stays on until the configured timeout elapses and then turns off, and that off state persists until the next activation

#### Scenario: No external components required for the LED

- **WHEN** the board is powered with only the IR receiver attached and no external LED or resistor wired up
- **THEN** the onboard LED still responds to the remote's power button

### Requirement: Only the power button changes the LED state

The system SHALL change the LED's steady on/off state only in response to the remote's power button. The remote's volume buttons SHALL NOT change the steady state; they adjust the configured timeout and preview it, as defined by the configurable-timeout capability. Presses of any other button, and IR noise from unrelated sources, SHALL NOT change the LED state.

#### Scenario: Other buttons are ignored

- **WHEN** any button other than power or volume is pressed while the LED is on or off
- **THEN** the LED state does not change

#### Scenario: Volume buttons do not change the steady state

- **WHEN** a volume button is pressed
- **THEN** the steady on/off state is unchanged and only the timeout is adjusted and previewed

#### Scenario: Unrecognized IR signals are ignored

- **WHEN** the receiver picks up an IR signal that does not match a known remote button
- **THEN** the LED state does not change and the board keeps running normally

## ADDED Requirements

### Requirement: Every kit-remote button is decoded and recorded

The system SHALL decode the kit's IR remote and recognize every button on it, recording each button's protocol, address, and command so later features can bind to them. Each button SHALL be distinguishable from the others.

#### Scenario: Each button is distinguished

- **WHEN** each button on the kit remote is pressed in turn
- **THEN** the firmware reports a stable, distinct identifier for every button

#### Scenario: The button map is recorded

- **WHEN** the capture is complete
- **THEN** every button's protocol, address, and command is recorded for later use

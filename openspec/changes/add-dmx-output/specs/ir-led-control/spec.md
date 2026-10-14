## RENAMED Requirements

- FROM: `### Requirement: Firmware builds and runs on the Uno R3 with the IR library`
- TO: `### Requirement: Firmware builds and runs on the Uno R3 with the IR and DMX libraries`

## MODIFIED Requirements

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

## REMOVED Requirements

### Requirement: Defined LED state at startup

**Reason**: The device no longer starts disabled and dark. It comes up enabled and running, so the show begins without a power press.

**Migration**: Use the added "Defined state at startup" requirement.

### Requirement: Remote power button toggles the onboard LED

**Reason**: The power button no longer toggles the LED on a timeout. It enables and disables the device, and the LED reports that enabled state.

**Migration**: Use the added "Remote power button enables and disables the device" requirement for the press behaviour and "Onboard LED indicates the device status" for how the LED reports it.

### Requirement: Only the power button changes the LED state

**Reason**: The LED's steady state now follows the device's enabled state, and the remote's other buttons act on the effects and the haze machine rather than on the LED.

**Migration**: Use the added "Only the power button changes the device's enabled state" requirement.

## ADDED Requirements

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

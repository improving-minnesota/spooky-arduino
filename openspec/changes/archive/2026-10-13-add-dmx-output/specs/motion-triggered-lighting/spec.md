## RENAMED Requirements

- FROM: `### Requirement: Sensor warm-up does not trigger the LED`
- TO: `### Requirement: Sensor warm-up does not trigger an effect`

## MODIFIED Requirements

### Requirement: Motion sensing does not block the main loop

The system SHALL detect motion without suspending the main loop, so remote input
and effect updates stay responsive.

#### Scenario: Remote input stays responsive during motion

- **WHEN** motion is detected
- **THEN** remote button presses are still decoded and acted on promptly

### Requirement: Sensor warm-up does not trigger an effect

The system SHALL ignore the PIR sensor's output for a warm-up period after
power-on or reset, so sensor initialization does not spuriously start an effect
or light the LED.

#### Scenario: No spurious activation during warm-up

- **WHEN** the board is powered on or reset
- **THEN** the sensor starts no effect until the warm-up period completes

## REMOVED Requirements

### Requirement: Motion turns the LED on

**Reason**: Motion no longer turns the onboard LED on. It is an event that starts an effect while the device is enabled, and the LED reports the device's enabled state instead.

**Migration**: Use the added "Motion starts an effect" requirement for the sensor behaviour and the ir-led-control capability for the LED.

### Requirement: LED turns off after the configured timeout with no motion

**Reason**: The onboard LED no longer times out; it reports the device's enabled state. The configured timeout now bounds how long an effect runs.

**Migration**: Use the spooky-effects capability for how long an effect runs, and the ir-led-control capability for the LED's steady indication.

## ADDED Requirements

### Requirement: Motion starts an effect

The system SHALL treat a PIR motion event as an event that starts a randomly
chosen effect while the device is enabled. Motion SHALL NOT turn the onboard LED
on, and SHALL do nothing while the device is disabled. Motion continuing while an
effect is running SHALL keep that effect running and extend its deadline, rather
than starting another.

#### Scenario: Motion starts an effect while the device is enabled

- **WHEN** the device is enabled and a motion event occurs
- **THEN** a randomly chosen effect starts

#### Scenario: Motion does not turn the LED on

- **WHEN** motion occurs
- **THEN** the onboard LED does not change, because it reports the device's enabled state

#### Scenario: Continued motion extends the running effect

- **WHEN** motion continues while an effect is running
- **THEN** that effect keeps running and its deadline is extended

#### Scenario: Motion while the device is disabled does nothing

- **WHEN** the device is disabled and motion occurs
- **THEN** no effect starts and nothing changes

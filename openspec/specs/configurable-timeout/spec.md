# configurable-timeout Specification

## Purpose

Owns the LED-on timeout: its default value, the range and step ladder it can be set to, how the remote's volume buttons change it, the on-LED preview that shows the new value, and its persistence across power cycles.

## Requirements

### Requirement: Timeout value, default, and bounds

The system SHALL hold a single LED-on timeout expressed in whole seconds, bounded to a minimum of 1 second and a maximum of 300 seconds (5 minutes). On first use, with no stored value, the timeout SHALL default to 5 seconds.

#### Scenario: Default timeout on first use

- **WHEN** the firmware starts and no timeout has been stored
- **THEN** the timeout is 5 seconds

#### Scenario: Minimum bound is enforced

- **WHEN** the timeout is 1 second and a decrease is requested
- **THEN** the timeout remains 1 second

#### Scenario: Maximum bound is enforced

- **WHEN** the timeout is 300 seconds and an increase is requested
- **THEN** the timeout remains 300 seconds

### Requirement: Timeout adjusts along a growing step ladder

The system SHALL change the timeout in steps that grow with the current value: 1-second steps between 1 and 10 seconds, 10-second steps between 10 and 30 seconds, 30-second steps between 30 and 60 seconds, and 60-second steps between 60 and 300 seconds. The reachable values SHALL be 1–10, 20, 30, 60, 120, 180, 240, and 300 seconds.

#### Scenario: One-second step below 10 seconds

- **WHEN** the timeout is 4 seconds and an increase is requested
- **THEN** the timeout becomes 5 seconds

#### Scenario: Ten-second step between 10 and 30 seconds

- **WHEN** the timeout is 10 seconds and an increase is requested
- **THEN** the timeout becomes 20 seconds

#### Scenario: Thirty-second step between 30 and 60 seconds

- **WHEN** the timeout is 30 seconds and an increase is requested
- **THEN** the timeout becomes 60 seconds

#### Scenario: Sixty-second step above 60 seconds

- **WHEN** the timeout is 60 seconds and an increase is requested
- **THEN** the timeout becomes 120 seconds

#### Scenario: Decrease follows the same ladder in reverse

- **WHEN** the timeout is 20 seconds and a decrease is requested
- **THEN** the timeout becomes 10 seconds

### Requirement: Volume buttons change the timeout

The system SHALL increase the timeout by one ladder step when the remote's volume-up button is pressed, and decrease it by one ladder step when the volume-down button is pressed. Each accepted change SHALL take effect immediately.

#### Scenario: Volume-up increases the timeout

- **WHEN** the timeout is 5 seconds and volume-up is pressed once
- **THEN** the timeout becomes 6 seconds

#### Scenario: Volume-down decreases the timeout

- **WHEN** the timeout is 5 seconds and volume-down is pressed once
- **THEN** the timeout becomes 4 seconds

### Requirement: Timeout persists across power cycles

The system SHALL store the timeout in non-volatile memory so the value survives power-off, reset, and re-upload, and SHALL load the stored value on startup.

#### Scenario: Setting survives a power cycle

- **WHEN** the user sets the timeout to 30 seconds, then the board is powered off and on again
- **THEN** the timeout is 30 seconds after startup

#### Scenario: Uninitialized storage falls back to default

- **WHEN** the board starts with no valid stored setting
- **THEN** the timeout is 5 seconds and that value is stored for next time

### Requirement: New timeout is previewed on the LED

After the timeout is changed, the system SHALL preview the new value on the onboard LED by flashing it once per second; the number of flashes SHALL equal the new timeout in seconds, and the LED SHALL then turn off.

#### Scenario: Preview counts out the new value

- **WHEN** the timeout is set to 5 seconds
- **THEN** the LED flashes 5 times, once per second, and then turns off

#### Scenario: Preview follows the direction of change

- **WHEN** the timeout is increased from 5 seconds to 6 seconds
- **THEN** the LED flashes 6 times and then turns off

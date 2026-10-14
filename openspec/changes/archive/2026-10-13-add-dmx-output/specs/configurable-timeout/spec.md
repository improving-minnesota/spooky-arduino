## MODIFIED Requirements

### Requirement: Timeout value, default, and bounds

The system SHALL hold a single rig timeout expressed in whole seconds, bounded to
a minimum of 1 second and a maximum of 300 seconds (5 minutes). On first use,
with no stored value, the timeout SHALL default to 5 seconds. The timeout SHALL
be how long each effect runs, and SHALL no longer turn the onboard LED off after
a delay.

#### Scenario: Default timeout on first use

- **WHEN** the firmware starts and no timeout has been stored
- **THEN** the timeout is 5 seconds

#### Scenario: Minimum bound is enforced

- **WHEN** the timeout is 1 second and a decrease is requested
- **THEN** the timeout remains 1 second

#### Scenario: Maximum bound is enforced

- **WHEN** the timeout is 300 seconds and an increase is requested
- **THEN** the timeout remains 300 seconds

#### Scenario: The timeout bounds how long an effect runs

- **WHEN** an effect starts and the timeout elapses
- **THEN** the effect has run for the timeout's duration and the fixture goes dark

### Requirement: Volume buttons change the timeout

The system SHALL increase the timeout by one ladder step when the remote's
volume-up button is pressed, and decrease it by one ladder step when the
volume-down button is pressed. A change SHALL apply to the next effect that runs,
rather than shortening one already running.

#### Scenario: Volume-up increases the timeout

- **WHEN** the timeout is 5 seconds and volume-up is pressed once
- **THEN** the timeout becomes 6 seconds

#### Scenario: Volume-down decreases the timeout

- **WHEN** the timeout is 5 seconds and volume-down is pressed once
- **THEN** the timeout becomes 4 seconds

#### Scenario: A change applies to the next effect

- **WHEN** the timeout is changed while an effect is running
- **THEN** the running effect keeps its original deadline and the new value applies to the next effect

### Requirement: New timeout is previewed on the LED

After the timeout is changed, the system SHALL preview the new value on the
onboard LED by blinking it quickly, once per second of the new value, and SHALL
then return the LED to reporting the device's status.

#### Scenario: Preview counts out the new value

- **WHEN** the timeout is set to 5 seconds
- **THEN** the LED blinks five times quickly and then returns to reporting the device's status

#### Scenario: Preview follows the direction of change

- **WHEN** the timeout is increased from 5 seconds to 6 seconds
- **THEN** the LED blinks six times quickly

#### Scenario: A long value takes correspondingly longer

- **WHEN** the timeout is set to 30 seconds
- **THEN** the LED blinks thirty times quickly, at the same fast cadence

## Purpose

Turns the onboard LED on when the HC-SR501 PIR sensor detects motion and turns it off after the configured timeout once motion is no longer present, giving the project its motion-triggered lighting behavior.

## ADDED Requirements

### Requirement: Motion turns the LED on

The system SHALL turn the onboard LED on when the PIR sensor reports motion.

#### Scenario: LED turns on when motion is detected

- **WHEN** the LED is off and motion is detected
- **THEN** the onboard LED turns on

#### Scenario: Motion while already on keeps the LED on

- **WHEN** the LED is on and further motion is detected
- **THEN** the LED stays on and its timeout restarts

### Requirement: LED turns off after the configured timeout with no motion

The system SHALL turn the LED off once the configured timeout has elapsed with no motion. The timeout SHALL be the value held by the configurable-timeout capability.

#### Scenario: LED turns off after motion stops

- **WHEN** motion stops and the configured timeout elapses
- **THEN** the onboard LED turns off

#### Scenario: Continued motion extends the timeout

- **WHEN** motion is detected again before the timeout elapses
- **THEN** the timeout restarts and the LED stays on

### Requirement: Motion sensing does not block the main loop

The system SHALL detect motion and run the timeout without suspending the main loop, so remote input stays responsive.

#### Scenario: Remote input stays responsive during motion

- **WHEN** the LED is on from motion
- **THEN** remote button presses are still decoded and acted on promptly

### Requirement: Sensor warm-up does not trigger the LED

The system SHALL ignore the PIR sensor's output for a warm-up period after power-on or reset, so sensor initialization does not spuriously turn the LED on.

#### Scenario: No spurious activation during warm-up

- **WHEN** the board is powered on or reset
- **THEN** the LED does not turn on from the sensor until the warm-up period completes

# spooky-effects Specification

## Purpose

Owns the roster of spooky lighting effects, how one is chosen and run for the
configured duration, how each moves the beam within its safe band, and how an
effect drives the haze and bubble machine while it runs.

## Requirements

### Requirement: A roster of named effects exists

The system SHALL provide a roster of named effects — chase, wave, comet, snake,
colour roll, ripple, twinkle, fire, blood pulse, random strobe, blinder,
lightning, shake, pendulum, and crawl — each producing a distinct, repeatable
look.

#### Scenario: Each named effect is distinct

- **WHEN** the roster is run in turn
- **THEN** each effect produces a visibly different look from the others

#### Scenario: Each effect is repeatable

- **WHEN** the same effect runs twice
- **THEN** it produces the same kind of look both times

#### Scenario: The roster is drawn from at random

- **WHEN** an effect is chosen
- **THEN** it is drawn from the whole roster, so every effect is reachable

### Requirement: Effects light one emitter per beam at a time

Each bar carries independent R, G, B, and W emitters that do not blend, so an
effect SHALL drive at most one emitter per beam at any moment. White SHALL be the
bar's white emitter alone, never R, G, B, and W together.

#### Scenario: A beam shows a single colour

- **WHEN** any effect lights a beam
- **THEN** at most one of that beam's four emitters is above zero

#### Scenario: White is the white emitter

- **WHEN** an effect shows white
- **THEN** it uses the bar's W channel alone

#### Scenario: A sequential effect switches rather than overlaps

- **WHEN** an effect shows two colours in sequence, such as a strike followed by an afterglow
- **THEN** it changes from one emitter to the other rather than lighting both at once

### Requirement: Effects move the beam within its safe band

The system SHALL move the beam's horizontal position within the band that never
points at the sky. Each effect SHALL be one of three kinds: **drifting**, which
drifts slowly with a non-repeating wander added; **jumping**, which repositions
sharply; or **own**, which drives the position itself. The tilt speed channel
SHALL be set to match the kind.

#### Scenario: The beam stays inside the band

- **WHEN** any effect runs
- **THEN** the horizontal position stays within the safe band

#### Scenario: Drifting effects wander

- **WHEN** a drifting effect runs
- **THEN** its position drifts slowly with a non-repeating wander added

#### Scenario: Jumping effects reposition sharply

- **WHEN** a jumping effect runs
- **THEN** its position snaps to new values as it flashes

#### Scenario: The tilt speed stays within what the fixture tolerates

- **WHEN** any effect sets the tilt speed
- **THEN** the value stays at or below the maximum the fixture tolerates, because 255 breaks its tilt

### Requirement: An effect runs for the configured duration, then the fixture goes dark

The system SHALL run the chosen effect for the configured duration and then leave
the fixture dark, waiting for the next event. Each event SHALL choose a new
effect at random.

#### Scenario: The effect ends dark

- **WHEN** the configured duration elapses
- **THEN** the fixture goes dark and waits, rather than choosing the next effect on its own

#### Scenario: The next event picks a new effect

- **WHEN** an event arrives after an effect has ended
- **THEN** a randomly chosen effect starts

#### Scenario: An event starts a new effect and restarts the clock

- **WHEN** an event arrives, whether or not an effect is already running
- **THEN** a newly chosen effect starts and the configured duration begins again

#### Scenario: Continued motion extends the running effect

- **WHEN** motion continues while an effect is running, without a fresh event
- **THEN** that effect keeps running and its deadline is extended

### Requirement: Effects leave the haze machine alone

An effect SHALL NOT change either of the machine's settings sets. Which set the
machine uses SHALL depend only on whether an effect is running, so an effect
brings the event settings into use and its ending returns the machine to the idle
settings, without either set being modified.

#### Scenario: An effect uses the event settings

- **WHEN** an effect runs
- **THEN** the machine is driven from the event settings

#### Scenario: An effect does not change either set

- **WHEN** effects start and end
- **THEN** both the idle and the event settings are exactly as they were

#### Scenario: The machine returns to the idle settings

- **WHEN** an effect ends
- **THEN** the machine goes back to the idle settings

### Requirement: Effects do not block the main loop

The system SHALL update effects without suspending the main loop, so IR input and
PIR sensing stay responsive while an effect runs.

#### Scenario: IR input stays responsive during an effect

- **WHEN** an effect is running and a remote button is pressed
- **THEN** the press is decoded and acted on promptly, with no long blocking delay

#### Scenario: Motion is detected during an effect

- **WHEN** an effect is running and motion occurs
- **THEN** the motion event is registered

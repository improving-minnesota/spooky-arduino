## Purpose

Owns the SHEHDS beam bar's DMX semantics: its 38-channel map, the safe band its
horizontal position moves within, the master dimmer's fixed value, and the rule
that its four emitters per beam never blend.

## ADDED Requirements

### Requirement: Beam bar occupies its 38-channel block

The system SHALL drive the beam bar's 38 channels (channels 1–38) in the
fixture's 38-channel mode, and SHALL hold channels 3 (built-in effects) and 4
(built-in effect speed) at 0 so the fixture's own effects stay disabled.

#### Scenario: The bar responds in 38-channel mode

- **WHEN** the firmware drives channels 1–38 with the fixture set to 38-channel mode
- **THEN** each channel controls its documented function on the bar

#### Scenario: Built-in effects stay disabled

- **WHEN** any effect runs
- **THEN** channels 3 and 4 remain 0, so the fixture's internal effects never engage

### Requirement: The master dimmer is held at full

The system SHALL hold the bar's master dimmer (channel 5) at full while the board
is powered, and SHALL vary brightness through the per-bar colour channels instead.
The dimmer SHALL NOT be left to a writer's transient state.

#### Scenario: The dimmer stays at full

- **WHEN** the board is powered, whatever the device or effect state
- **THEN** channel 5 reads full

#### Scenario: Brightness comes from the colour channels

- **WHEN** an effect fades or flickers
- **THEN** it does so through the per-bar colour values, not the master dimmer

### Requirement: Colors are independent and never blend

Each bar's R, G, B, and W emitters SHALL be treated as four separate colours that
do not combine, because the fixture cannot mix them. The system SHALL NOT rely on
two emitters together producing a blended colour.

#### Scenario: Two emitters are two colours

- **WHEN** two of a bar's emitters are above zero at once
- **THEN** the fixture shows two separate colours rather than a blend

#### Scenario: White is its own emitter

- **WHEN** the system wants white
- **THEN** it uses the bar's white emitter, because red, green, and blue together would not produce white

### Requirement: Horizontal position is bounded and has an idle park

The system SHALL set the bar's horizontal position (channel 1) inside a band
bounded by a ceiling (initially 120) so the beams never face the sky, and SHALL
park it at the idle position (initially 56) whenever the device is disabled or no
effect is driving it.

#### Scenario: Position never exceeds the ceiling

- **WHEN** any effect drives the horizontal position
- **THEN** the channel value stays at or below the ceiling

#### Scenario: The bar parks when idle or disabled

- **WHEN** no effect is driving the position
- **THEN** channel 1 holds the idle position

### Requirement: The tilt speed channel has a usable maximum

The system SHALL drive the bar's horizontal speed (channel 2) only with values the
fixture tolerates, because its maximum value breaks the tilt on this fixture. A
slower value SHALL be used for drifting effects and a faster one for jumping
effects, both inside the usable range.

#### Scenario: The speed stays inside the usable range

- **WHEN** any effect sets the horizontal speed
- **THEN** the value stays at or below the maximum the fixture tolerates

#### Scenario: Drifting and jumping effects differ in speed

- **WHEN** a drifting effect and a jumping effect each run
- **THEN** the jumping effect uses the faster speed of the two

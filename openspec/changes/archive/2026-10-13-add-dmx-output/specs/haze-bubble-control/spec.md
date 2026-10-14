## Purpose

Owns the Chauvet Hurricane Bubble Haze's DMX semantics — haze output, haze
volume, bubble output, blower speed, and bubble cleaning mode — together with the
master enable and the two independent settings sets, idle and event, that decide
what the machine does in each state.

## ADDED Requirements

### Requirement: Haze machine occupies its 4-channel block

The system SHALL drive the haze/bubble machine's four channels at address 39:
channel 39 bubble output, channel 40 haze volume, channel 41 blower speed, and
channel 42 bubble cleaning mode. It SHALL use the machine's documented bands.

#### Scenario: Each channel drives its documented function

- **WHEN** the firmware writes channels 39–42
- **THEN** bubble output, haze volume, blower speed, and cleaning mode respond respectively

#### Scenario: Blower bands match the machine

- **WHEN** a blower speed is selected
- **THEN** channel 41 carries a value in the machine's band for that speed: low up to 100, medium 101–200, high 201–255, and off as 0

### Requirement: A master enable turns the machine on and off

The system SHALL provide a master enable for the machine, toggled by the remote's
play/pause button, and ON at power-on. While it is off, the machine SHALL be
silent and the haze controls SHALL have no effect. The selected haze rate SHALL
be remembered while it is off.

#### Scenario: play/pause silences the machine

- **WHEN** play/pause is pressed while the master enable is on
- **THEN** haze, bubbles, and the blower all stop and the haze controls stop responding

#### Scenario: play/pause re-enables it

- **WHEN** play/pause is pressed again
- **THEN** the machine responds to the controls again, at the rate it had before

#### Scenario: The master enable is on at startup

- **WHEN** the board powers on
- **THEN** the master enable is on

### Requirement: The machine has independent idle and event settings

The system SHALL hold two independent sets of machine settings — one used while
the device is idle and one used while an effect runs. The set in use SHALL be the
one matching the device's current state. The remote's haze controls SHALL edit
whichever set is in use, so a setting is changed by making the change while the
device is in the state it is meant for. An effect starting or ending SHALL NOT
change either set.

Idle SHALL default to everything off, so no fluid is spent between events, and an
event SHALL default to haze on, bubbles on, and a full blower, so an effect
arrives with the machine already working. While the device is disabled, the
machine SHALL be silent.

#### Scenario: An effect uses the event settings

- **WHEN** an effect is running
- **THEN** the machine is driven from the event settings

#### Scenario: Idle uses the idle settings

- **WHEN** the device is enabled and no effect is running
- **THEN** the machine is driven from the idle settings

#### Scenario: A control changes the set for the current state

- **WHEN** a haze control is pressed while the device is idle
- **THEN** the idle setting changes and the event setting is untouched

#### Scenario: A control during an effect changes the event set

- **WHEN** the blower is set to high while an effect runs
- **THEN** the blower speeds up during that effect, returns to the idle speed when the effect ends, and runs at the new speed during the next effect

#### Scenario: The settings survive effects

- **WHEN** effects start and end
- **THEN** neither settings set is changed by them

#### Scenario: The defaults differ between idle and an event

- **WHEN** the board powers on and the device is enabled
- **THEN** the idle set is off, and starting an effect brings haze, bubbles, and a full blower

#### Scenario: Disabling the device silences the machine

- **WHEN** the device is disabled
- **THEN** every haze channel is at its off value

### Requirement: The settings survive a reboot

The system SHALL persist both settings sets in EEPROM, so a blower speed, a
bubble toggle, a haze toggle, and a haze rate all survive a power cycle. Only
these and the effect duration SHALL be stored; the master enable SHALL start on
at every boot.

#### Scenario: A setting survives a reboot

- **WHEN** a setting is changed and the board is power-cycled
- **THEN** that setting is unchanged after the reboot

#### Scenario: A stale store is discarded

- **WHEN** the board starts with a blank or older stored settings layout
- **THEN** the defaults are used and stored afresh, rather than the older layout being misread

#### Scenario: The master enable is not persisted

- **WHEN** the board is power-cycled while the master enable is off
- **THEN** the master enable is on again after the reboot

### Requirement: Haze volume steps through a ladder

The system SHALL decrease haze volume with trackleft and increase it with
trackright, along a fixed ladder of steps inside the machine's continuous-haze
band, and SHALL remember the chosen step.

#### Scenario: trackright raises the volume one step

- **WHEN** the volume is below the top of the ladder and trackright is pressed
- **THEN** the volume moves up one step

#### Scenario: trackleft lowers the volume one step

- **WHEN** the volume is above the bottom of the ladder and trackleft is pressed
- **THEN** the volume moves down one step

#### Scenario: The ladder is bounded

- **WHEN** the volume is at either end of the ladder and the corresponding button is pressed
- **THEN** the volume stays at that step

### Requirement: Haze output is toggled from the remote

The system SHALL turn haze on and off with the remote's func/stop button, and
SHALL remember the setting.

#### Scenario: func/stop turns haze on

- **WHEN** haze is off and func/stop is pressed
- **THEN** haze is on

#### Scenario: func/stop turns haze off

- **WHEN** haze is on and func/stop is pressed
- **THEN** haze is off

### Requirement: Bubble output is toggled from the remote

The system SHALL turn bubble output on and off with the remote's st/rept button,
and SHALL remember the setting.

#### Scenario: st/rept turns bubbles on

- **WHEN** bubbles are off and st/rept is pressed
- **THEN** bubbles are on

#### Scenario: st/rept turns bubbles off

- **WHEN** bubbles are on and st/rept is pressed
- **THEN** bubbles are off

### Requirement: Blower speed is set from the remote

The system SHALL set blower speed with the remote's number buttons — 0 for off,
1 for low, 2 for medium, and 3 for high — and SHALL remember the setting.

#### Scenario: Each number button selects its speed

- **WHEN** the remote's 0, 1, 2, or 3 is pressed
- **THEN** the blower is set to off, low, medium, or high respectively

### Requirement: Bubble cleaning mode is entered and exited from the remote

The system SHALL enter bubble cleaning mode when the remote's eq button is held
for three seconds, driving channel 42 above its off band and stopping all
effects, and SHALL flash the onboard LED while cleaning mode is active. A single
eq press while cleaning mode is active SHALL exit it and resume the device's
previous state.

#### Scenario: Holding eq enters cleaning mode

- **WHEN** eq is held for three seconds
- **THEN** channel 42 goes above its off band, cleaning mode becomes active, and effects stop

#### Scenario: The LED flashes during cleaning

- **WHEN** cleaning mode is active
- **THEN** the onboard LED flashes

#### Scenario: A single eq press exits cleaning mode

- **WHEN** cleaning mode is active and eq is pressed once
- **THEN** cleaning ends, channel 42 returns to its off value, and the device resumes its previous state

#### Scenario: Only eq acts while cleaning

- **WHEN** any other button is pressed while cleaning mode is active
- **THEN** nothing changes

### Requirement: The haze controls are inert while the machine is off

The system SHALL ignore the haze controls when the master enable is off, when the
device is disabled, or while cleaning mode is active, so a stray press cannot
change the machine's state.

#### Scenario: Controls do nothing while the master enable is off

- **WHEN** the master enable is off and a haze control is pressed
- **THEN** the machine's settings are unchanged

#### Scenario: Controls do nothing while the device is disabled

- **WHEN** the device is disabled and a haze control is pressed
- **THEN** the machine's settings are unchanged

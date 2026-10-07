# led-blink Specification

## Purpose

Gives the board a visible heartbeat: the onboard LED blinks continuously whenever the board is powered, so anyone can confirm at a glance that firmware is running and that an upload actually took effect.

## Requirements

### Requirement: Onboard LED blinks while the board is powered

The system SHALL blink the board's onboard LED continuously for as long as the board has power. Blinking SHALL begin automatically after power-on or reset, with no button press, serial connection, or other external input required.

#### Scenario: Blinking starts on power-up

- **WHEN** the board is powered on or reset
- **THEN** the onboard LED starts blinking automatically, without any host computer interaction or user input

#### Scenario: Blinking continues indefinitely

- **WHEN** the board stays powered
- **THEN** the LED keeps blinking for the entire time the board is powered, with no timeout, stop condition, or need to re-trigger it

#### Scenario: No external components required

- **WHEN** the board is powered with nothing attached except the power or USB cable
- **THEN** the onboard LED still blinks

### Requirement: Blink cadence is a fixed 1 second on / 1 second off

The system SHALL alternate the onboard LED between on and off, holding each state for approximately one second, for a roughly 50% duty cycle.

#### Scenario: On and off intervals are each about one second

- **WHEN** the board is blinking in steady state
- **THEN** each on interval and each off interval lasts approximately one second

#### Scenario: Cadence does not drift over time

- **WHEN** the board has been powered and blinking for 60 seconds
- **THEN** the LED has completed approximately 30 on/off cycles, with no cumulative timing drift beyond a few seconds

### Requirement: Firmware builds and runs on the stock Arduino toolchain

The system SHALL build and run using only the standard Arduino core for the Uno R3, so the blink works on a freshly set up machine with no third-party libraries installed and fits comfortably in the board's limited flash and RAM.

#### Scenario: Build with a stock toolchain

- **WHEN** the sketch is compiled with the standard Arduino AVR core and no additional libraries
- **THEN** compilation succeeds with no missing-library errors and the resulting binary produces the blink once uploaded

#### Scenario: Upload to a connected board succeeds

- **WHEN** a board is connected over USB and the compiled sketch is uploaded with the board and port selected
- **THEN** the upload completes without error and the new firmware runs immediately after reset

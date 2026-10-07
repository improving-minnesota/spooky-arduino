## REMOVED Requirements

### Requirement: Onboard LED blinks while the board is powered
**Reason**: The continuous heartbeat blink was a toolchain smoke test and is no longer part of the firmware; the LED is now driven by motion and remote input.
**Migration**: Use the motion-triggered-lighting and ir-led-control capabilities. No replacement heartbeat is provided.

### Requirement: Blink cadence is a fixed 1 second on / 1 second off
**Reason**: No continuous blink remains in the firmware.
**Migration**: None; the LED's on duration is now governed by the configurable timeout.

### Requirement: Firmware builds and runs on the stock Arduino toolchain
**Reason**: The consolidated firmware also depends on the IRremote library, so the "no third-party libraries" contract no longer holds.
**Migration**: The build contract is now covered by the ir-led-control capability's build requirement.

# CraneControl Implementation Notes

## Overview
This project implements a compact Arduino-based crane winch controller for an ATmega328P Pro Mini running under PlatformIO. The controller is structured around a single `CraneController` class that owns the crane state machine, servo output, I2C command/status handling, and light behavior.

The implementation follows the specification in `docs/Specification` and keeps the hardware pin map configurable through a `GpioConfig` structure so the final wiring can be adjusted later without rewriting the state logic.

## Main Structure

### `src/main.cpp`
Responsibilities:
- Initializes the serial port
- Configures the I2C slave interface
- Creates the `CraneController` instance with the default GPIO mapping
- Registers the I2C receive/request handlers
- Calls `begin()` once on startup and `update()` continuously in the main loop

The main loop is intentionally simple because all crane logic is centralized in the controller object.

### `include/CraneController.h`
Responsibilities:
- Declares the command and status enums used by the I2C interface
- Defines the GPIO mapping structure used by the controller
- Declares the public API for start-up, update, and I2C handling
- Contains the internal state variables for the winch, status, pulse tracking, and light patterns

Important types:
- `CraneCommand`: `DisableAllMovement`, `MoveToPosition1`, `MoveToPosition2`, `MoveToPositions3`, `Home`
- `CraneStatus`: `DisableAllMovement`, `Position1`, `Position2`, `Home`, `NotInitialised`, `ErrorCondition`
- `GpioConfig`: configurable board pin mapping for:
  - upper limit switch
  - keyphaser sensor
  - servo output
  - lights
  - servo power control
  - I2C address

### `src/CraneController.cpp`
Responsibilities:
- Initializes hardware pins
- Performs startup power-up sequence and homing to upper limit
- Executes the control loop for lower/raise movement
- Tracks winch position-related pulse counts
- Updates status and lights according to the specification
- Enforces error handling when required conditions are exceeded

## Control Flow

### Startup / Power-up
On `begin()` the controller configures the input and output pins and resets internal state. The startup sequence then:
1. sets servo power to enabled
2. holds the servo in the stop state
3. turns the lights off
4. raises the payload until the upper limit switch is reached
5. clears the pulse count
6. marks the winch as initialised and at upper limit

This matches the specification's requirement to reach the upper limit before normal operation begins.

## Runtime Operation
The main loop calls `update()`, which checks the current state in order:

1. If not initialised, run the startup procedure
2. If an error condition is active, stop the winch and flash in error pattern
3. If the disable command is active, stop movement and report disabled status
4. If the upper limit switch indicates the payload is raised, reset pulse count and status to upper limit
5. Otherwise, move toward the active target demand

### Movement Rules
- `Home`: servo drives upward until upper limit is reached, this establishes the reference position for the winch.
- `MoveToPosition1`: servo drives until pulse count reaches position 1 threshold
- `MoveToPosition2`: servo drives until pulse count reaches position 2 threshold
- `MoveToPosition3`: servo drives until pulse count reaches position 3 threshold
- `DisableAllMovement`: stops the servo immediately

## I2C Interface
The controller exposes a simple read/write interface:
- `handleI2CReceive()` accepts a command byte from the master device
- `handleI2CRequest()` returns the current crane status byte

The command values and status values follow the specification directly, with default values matching the power-up and error semantics described in the design.

## Lighting Behaviour
The lights are controlled in a very lightweight pattern:
- Pre-initialisation: lights off
- Initialised normal operation: low-frequency PWM-like blink pattern with a bright window
- Error condition: alternating error flash pattern

The `setLights()` and `updateLights()` logic keeps the code straightforward while staying aligned with the specification's “pattern 0 / pattern 1 / error pattern” approach.

## Hardware Pin Mapping
The current mapping is intentionally abstracted and can be changed later in one place:

- upper limit switch: pin 2
- keyphasor input: pin 3
- winch servo: pin 9
- lights: pin 10
- servo power: pin 11
- I2C address: 0x40

This is defined in the `GpioConfig` initializer used by the controller in `src/main.cpp`.

## Notes for Future Work
- Replace the placeholder GPIO mapping with the final harness/wiring assignment when the crane hardware is confirmed.
- Add real KeyPhasor pulse counting and validation once the sensor is connected to hardware.
- Improve the exact motion calibration when the servo and winch travel characteristics are measured on the actual crane.
- Consider splitting the logic into separate modules if the project grows beyond this single controller class.

## Immediate Next Steps
1. Confirm the real pin assignments for the upper-limit switch, KeyPhasor, lights, servo power, and servo output.
2. Validate the active-low / active-high logic against the actual sensor and load device wiring.
3. Tune the servo pulse widths to match the real winch travel and stop characteristics.
4. Add a hardware test harness or bench setup to verify the homing sequence and each I2C command.
5. Define the final I2C address and master-side command/response protocol once the external bus design is confirmed.

## Known Gaps
- The current implementation uses placeholder I/O mapping and a simplified movement model.
- The KeyPhasor pulse counter is not yet tied to a full hardware interrupt-driven measurement path.
- The final stop positions and pulse thresholds should be refined with measured crane travel data.
- The exact error-1 detection path should be validated against the real sensor behaviour during commissioning.

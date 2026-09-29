# CraneControl
This project is for automated control of a crane winch and navigation lights on a scale model building site diorama.
The small tower crane can rotate and raise/lower a payload using the winch.
The overall control (including rotation) is handled by an instance of the ESP32 Edge Node project (node28)
which is stored in the EdgeNode repository.

Initial plan was for the ESP32 to perform winch control, but this was not optimum because the
slip rings which handle crane rotation only pass 4 wires, including power, so while it was possible
to control the winch servo and lights, the limit switches had to be stationary at the predetermined payload 'landing' points.

By introducing a simple dedicated controller for the winch and lights, the control is improved significantly.
This controller "Home's" the winch against a Hall effect sensor and tracks rotations of the winch servo to determine
the current position of the payload (similar to CNC practice). 

A simple I2C communication link to the EdgeNode provides a command/status interface, with the Winch Controller acting as
a slave.

This is a PlatformIO Arduino project for an Arduino Pro Mini (5V, 16MHz, ATmega328P).

## Build

```
pio run
```

## Upload

```
pio run -t upload
```

## Notes

Arduino Pro Mini boards have no onboard USB; use an FTDI/USB-serial adapter
connected to the DTR pin for upload and reset.

Specification to be added later.

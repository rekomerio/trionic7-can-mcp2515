# Trionic 7 MCP2515 Communication

## So what does this do?

When connected to a CAN bus of car with Trionic 7 engine management system, this sketch can be used to read and write data
in the I-BUS or P-BUS.

### Required parts

- Arduino Nano or similar microcontoller capable of SPI communication
- MCP2515 CAN shield
- Bluetooth receiver of your choice
- Voltage stepdown converter
- 12 pixel WS2812 LED ring (LED is not necessary for the project if you dont want one. Just delete LED related code)

To use the telephone mode, it is required to make custom AUX connection to your radio.

[Link to DIY AUX tutorial](http://saabworld.net/showthread.php?t=28000)

### Currently implemented:

- Read button presses from SID and steering wheel
- Write custom messages to SID
- Adjustment of LED brightness by the car light level sensor
- Turn on bluetooth from steering wheel SRC button
- Change tracks from steering wheel seek buttons
- Text "NEXT TRACK" or "PREV TRACK" is shown on SID when changing the track
- Scale LED color by engine RPM
- LED animations

## Notes:

- It seems that light level sensor is not the same in all SID's so you might need to change DIMMER_MAX and DIMMER_MIN.
  Minimum value for dimmer can be found by logging the dimmer value while holding finger over the sensor and maximum by shining flaslight to it.

CAN messages and protocols for T7 I-BUS can be found [here.](http://pikkupossu.1g.fi/tomi/projects/i-bus/i-bus.html)

[![Saab interior](http://img.youtube.com/vi/v_cQQGTZ-Sc/0.jpg)](http://www.youtube.com/watch?v=v_cQQGTZ-Sc "Saab bluetooth")

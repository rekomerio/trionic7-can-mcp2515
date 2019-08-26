# Trionic 7 MCP2515 Communication

## So what does this do?

When connected to a CAN bus of car with Trionic 7 engine management system, this sketch can be used to read and write data
in to the I-BUS or P-BUS.

### Required parts
 - Arduino Nano or similar microcontoller capable of SPI communication
 - MCP2515 CAN shield
 - Bluetooth receiver of your choice
 - Voltage stepdown converter
 - 12 pixel WS2812 LED ring

To use the telephone mode, it is required to make custom AUX connection to your radio.  

[Link to DIY AUX tutorial](http://saabworld.net/showthread.php?t=28000)  

### Currently implemented:
 - Read button presses from SID and steering wheel
 - Write custom messages to SID
 - Adjustment of LED brightness by the car light level sensor
 - Turn on bluetooth from steering wheel SRC button
 - Change tracks from steering wheel seek buttons
 - Text "BLUETOOTH" is shown on SID when Bluetooth is on
 - Text "NEXT TRACK" or "PREV TRACK" is shown on SID when changing the track 
 - LED animations

CAN messages and protocols for T7 I-BUS can be found [here.](pikkupossu.1g.fi/tomi/projects/i-bus/i-bus.html)

[![Saab interior](http://img.youtube.com/vi/0LvEN18u2Zs/0.jpg)](http://www.youtube.com/watch?v=0LvEN18u2Zs "Saab bluetooth")

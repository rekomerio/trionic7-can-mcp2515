#include <Arduino.h>

/*** ENABLE FUNCTIONALITIES ***/
#define LED             1
#define CANBUS          1
#define DEBUG           0

/*** DATA PINS ***/
#define BUTTON_PIN      2
#define LED_RING_PIN    3
#define BT_PREVIOUS     4
#define BT_NEXT         5
#define LED_STRIP_PIN   6
#define TRANSISTOR_PIN  7 // Turns on radio telephone channel
#define BLUETOOTH_PIN0  8 // Turns on bluetooth module
#define BLUETOOTH_PIN1  9 // Bluetooth shares power from 2 pins
#define CAN_CS_PIN      10

/*** CAN addresses ***/
// #define IBUS_BUTTONS    0x290
// #define RADIO_MSG       0x328
// #define RADIO_PRIORITY  0x348
// #define O_SID_MSG       0x33F
// #define O_SID_PRIORITY  0x358
// #define TEXT_PRIORITY   0x368
// #define LIGHTING        0x410
// #define SPEED_RPM       0x460

/*** ID's for devices talking to SID ***/
#define SPA             0x12
#define RADIO           0x19
#define TRIONIC         0x21
#define ACC             0x23
#define TWICE           0x2D
#define OPEN_SID        0x32

/*** CAN bytes ***/
#define AUDIO           2
#define SID             3

/*** CAN bits ***/

/*   AUDIO      */
// #define NXT             2
// #define SEEK_DOWN       3
// #define SEEK_UP         4
// #define SRC             5
// #define VOL_UP          6
// #define VOL_DOWN        7

/*    SID       */
// #define NPANEL          3
// #define UP              4
// #define DOWN            5
// #define SET             6
// #define CLR             7

/*    LIGHTING  */
#define DIMM1           1
#define DIMM0           2
#define LIGHT1          3
#define LIGHT0          4

/*    SPEED AND RPM    */
#define RPM1            1
#define RPM0            2
#define SPD1            3
#define SPD0            4

/*** CAN speeds for Trionic 7 ***/
#define I_BUS CAN_47KBPS
#define P_BUS CAN_500KBPS

/*
   Range for light level sensor depends on SID version.
   Dimmer values might also vary.
*/
#define LIGHT_MAX   0xC7FB
#define LIGHT_MIN   0x2308
#define DIMMER_MAX  0xFE9D
#define DIMMER_MIN  0x423F

/*** SID message ***/
#define MESSAGE_LENGTH 12
#define LETTERS_IN_MSG 5

/*** LED ***/
#define NUM_LEDS_RING    12
#define NUM_LEDS_STRIP   9
#define STRIP_BRIGHTNESS 180

#if DEBUG
#define DEBUG_MESSAGE(msg) Serial.println(msg);
#else
#define DEBUG_MESSAGE(msg) 
#endif
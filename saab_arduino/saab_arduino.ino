
/*
  By Reko Meriö
  26.07.2019
*/

#include <mcp_can.h>
#include <FastLED.h>

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define DEBUG

/*** DATA PINS ***/
#define BLUETOOTH_PIN0 9
#define BLUETOOTH_PIN1 10
#define TRANSISTOR_PIN 2 //TRANSISTOR PIN turns on radio telephone channel
#define BUTTON_PIN     7
#define LED_PIN        3
#define CAN_CS_PIN     1

/*** CAN addresses ***/
#define CBUS_BUTTONS   0x290

/*** CAN bytes ***/
#define AUDIO          2
#define SID            3

/*** CAN bits ***/

/*   AUDIO      */
#define NXT            2
#define SEEK_DOWN      3
#define SEEK_UP        4
#define SRC            5
#define VOL_UP         6
#define VOL_DOWN       7

/*    SID       */
#define NPANEL         3
#define UP             4
#define DOWN           5
#define SET            6
#define CLR            7

#define NUM_LEDS       12

MCP_CAN CAN(CAN_CS_PIN);
CRGB leds[NUM_LEDS];

void setup() {
#ifdef DEBUG
  Serial.begin(9600);
#endif
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(BLUETOOTH_PIN0, OUTPUT);
  pinMode(BLUETOOTH_PIN1, OUTPUT);
  pinMode(TRANSISTOR_PIN, OUTPUT);

  while (CAN.begin(CAN_50KBPS, MCP_8MHz) != CAN_OK) {
    delay(100);
  }
  startingEffect(10); // Spinning animation at startup
}

uint8_t mode = 0;

void loop() {
  if (buttonPressed()) {
    mode++;
    mode %= 2;
  }

  switch (mode) {
    case 0:
      spinner(100, 50);
      bluetooth(false);
      break;

    case 1:
      spinner(220, 60);
      bluetooth(true);
      break;

    default:
      FastLED.clear();
      FastLED.show();
      delay(500);
      break;
  }
  readCANBus();
}
/*
    @return - is the button being pressed
*/
bool buttonPressed() {
  if (digitalRead(BUTTON_PIN)) {
    return true;
  }
  return false;
}
/*
   Turn bluetooth on or off
   @param on - is bluetooth turned on (true) or off (false)
*/
void bluetooth(bool on) {
  digitalWrite(BLUETOOTH_PIN0, on);
  digitalWrite(BLUETOOTH_PIN1, on);
  digitalWrite(TRANSISTOR_PIN, on);
}
/*
   Spinning LED animation with trailing tail
   @param hue        - CHSV color
   @param brightness - brightness of LED's
*/
void spinner(uint8_t hue, uint8_t brightness) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(hue, 255, brightness);
    FastLED.show();
    fadeToBlackBy(leds, NUM_LEDS, brightness / (NUM_LEDS / 4));
    delay(85);
  }
}
/*
   Two spinning LED particles, one half a round further than the other
   @param rounds - how many complete rounds to do
*/
void startingEffect(uint8_t rounds) {
  for (uint16_t i = 0; i < NUM_LEDS * rounds; i++) {
    uint16_t j = i + (NUM_LEDS / 2);
    leds[i % NUM_LEDS] = CHSV(220, 255, 255);
    leds[j % NUM_LEDS] = CHSV(180, 255, 255);
    FastLED.show();
    FastLED.clear();
    delay(50);
  }
}

typedef void (*Callback) (uint8_t action);

void runAction(uint8_t action, Callback cb);
/*
  Runs given function, if given action is less than 8.
  One byte has 8 bits, so only actions between 0-7 are accepted.
*/
void runAction(uint8_t action, Callback cb) {
  if (action < 8) {
    cb(action);
  }
}

/*
  Reads incoming data from CAN bus, if there is any and runs desired action.
*/
void readCANBus() {
  uint8_t len = 0;
  uint8_t data[8];

  if (CAN.checkReceive() == CAN_MSGAVAIL) {
    CAN.readMsgBuf(&len, data);

    uint16_t id = CAN.getCanId();
    uint8_t action;

    switch (id) {
      case CBUS_BUTTONS:
        action = getHighBit(data[AUDIO]);
        runAction(action, audioActions);

        action = getHighBit(data[SID]);
        runAction(action, sidActions);
        break;
    }
  }
}
/*
  Checks every bit of the given value until finds the first high bit and then
  returns the position of that; or if none are high, returns 0xFF (255).
  @param value
  @return - first high bit of the given value or 0xFF
*/
uint8_t getHighBit(uint8_t value) {
  if (!value) return 0xFF;

  for (uint8_t i = 0; i < 8; i++) {
    if (value >> i & 0x01) {
      return i;
    }
  }
}

void audioActions(uint8_t action) {
  switch (action) {
    case NXT:
      //TODO
#ifdef DEBUG
      Serial.println("NEXT");
#endif
      break;
    case SEEK_DOWN:
      //TODO change the track down
#ifdef DEBUG
      Serial.println("SEEK DOWN");
#endif
      break;
    case SEEK_UP:
      //TODO change the track up
#ifdef DEBUG
      Serial.println("SEEK UP");
#endif
      break;
    case SRC:
      //TODO
#ifdef DEBUG
      Serial.println("SRC");
#endif
      break;
    case VOL_UP:
      //TODO
#ifdef DEBUG
      Serial.println("VOL+");
#endif
      break;
    case VOL_DOWN:
      //TODO
#ifdef DEBUG
      Serial.println("VOL-");
#endif
      break;
  }
}

void sidActions(uint8_t action) {
  switch (action) {
    case NPANEL:
      //TODO
#ifdef DEBUG
      Serial.println("NIGHT PANEL");
#endif
      break;
    case UP:
      //TODO
#ifdef DEBUG
      Serial.println("UP");
#endif
      break;
    case DOWN:
      //TODO
#ifdef DEBUG
      Serial.println("DOWN");
#endif
      break;
    case SET:
      //TODO
#ifdef DEBUG
      Serial.println("SET");
#endif
      break;
    case CLR:
      //TODO
#ifdef DEBUG
      Serial.println("CLEAR");
#endif
      break;
  }
}


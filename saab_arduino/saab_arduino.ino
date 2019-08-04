/*
  By Reko Meriö
  Last update: 27.07.2019
*/

#include <mcp_can.h>
#include <FastLED.h>

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

/*** ENABLE FUNCTIONALITIES ***/
#define LED            1
#define CANBUS         0
#define DEBUG          0

/*** DATA PINS ***/
#define BLUETOOTH_PIN0 9  // Turns on bluetooth module
#define BLUETOOTH_PIN1 8
#define TRANSISTOR_PIN 7  // Turns on radio telephone channel
#define BUTTON_PIN     2
#define LED_PIN        3
#define CAN_CS_PIN     10

/*** CAN addresses ***/
#define IBUS_BUTTONS   0x290

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
#if DEBUG
  Serial.begin(115200);
#endif
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(BLUETOOTH_PIN0, OUTPUT);
  pinMode(BLUETOOTH_PIN1, OUTPUT);
  pinMode(TRANSISTOR_PIN, OUTPUT);
#if CANBUS
  while (CAN.begin(MCP_ANY, CAN_50KBPS, MCP_8MHZ) != CAN_OK) {
    delay(100);
  }
#endif
#if LED
  startingEffect(5); // Spinning animation at startup
#endif
}

uint8_t mode = 0;

void loop() {
  if (buttonPressed()) {
    ++mode %= 2;
  }
#if LED
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
#endif
#if CANBUS
  readCanBus();
#endif
}
/*
    Saves the previous state of button in bool lastState.
    Returns true only once when button is being pressed. Returns true again
    after button has been released and then pressed again.
    Interrupt could be used here, but the button I have sometimes reads
    false clicks.
*/
bool buttonPressed() {
  static bool lastState = false;
  bool buttonState = digitalRead(BUTTON_PIN);
  if (buttonState && lastState != buttonState) {
    lastState = buttonState;
    return true;
  }
  lastState = buttonState;
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
   i is static variable, so it is initialized only once and remembers its position after function exits
*/
void spinner(uint8_t hue, uint8_t brightness) {
  static uint8_t i = 0;
  leds[i++] = CHSV(hue, 255, brightness);
  i %= NUM_LEDS;
  FastLED.show();
  fadeToBlackBy(leds, NUM_LEDS, brightness / (NUM_LEDS / 4));
  delay(85);
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

typedef void (*Callback) (uint8_t);

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
void readCanBus() {
  uint8_t action;
  uint8_t len = 0;
  uint8_t data[8];
  uint32_t id;

  if (CAN.checkReceive() == CAN_MSGAVAIL) {
    CAN.readMsgBuf(&id, &len, data);

    Serial.println(id, HEX);

    switch (id) {
      case IBUS_BUTTONS:
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
      Serial.println("NEXT");
      break;
    case SEEK_DOWN:
      //TODO change the track down
      Serial.println("SEEK DOWN");
      break;
    case SEEK_UP:
      //TODO change the track up
      Serial.println("SEEK UP");
      break;
    case SRC:
      //TODO
      Serial.println("SRC");
      break;
    case VOL_UP:
      //TODO
      Serial.println("VOL+");
      break;
    case VOL_DOWN:
      //TODO
      Serial.println("VOL-");
      break;
  }
}

void sidActions(uint8_t action) {
  switch (action) {
    case NPANEL:
      //TODO
      Serial.println("NIGHT PANEL");
      break;
    case UP:
      //TODO
      Serial.println("UP");
      break;
    case DOWN:
      //TODO
      Serial.println("DOWN");
      break;
    case SET:
      //TODO
      Serial.println("SET");
      break;
    case CLR:
      //TODO
      Serial.println("CLEAR");
      break;
  }
}

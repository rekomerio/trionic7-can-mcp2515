/*
  By Reko Meriö
*/

#include <mcp_can.h>
#include <FastLED.h>

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

/*** ENABLE FUNCTIONALITIES ***/
#define LED            1
#define CANBUS         1
#define DEBUG          1

/*** DATA PINS ***/
#define BUTTON_PIN     2
#define LED_PIN        3
#define BT_PREVIOUS    4
#define BT_NEXT        5
#define TRANSISTOR_PIN 7  // Turns on radio telephone channel
#define BLUETOOTH_PIN0 8  // Turns on bluetooth module
#define BLUETOOTH_PIN1 9
#define CAN_CS_PIN     10

/*** CAN addresses ***/
#define IBUS_BUTTONS   0x290
#define ENGINE         0x460

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

/*    ENGINE    */
#define RPM1           1
#define RPM0           2
#define SPD1           3
#define SPD0           4

/*** CAN speeds for Trionic 7 ***/
#define I_BUS          CAN_47KBPS
#define P_BUS          CAN_500KBPS

#define MAX_RPM        6000
#define MAX_SPD        240
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
  while (CAN.begin(MCP_ANY, I_BUS, MCP_8MHZ) != CAN_OK) {
    delay(100);
  }
  CAN.setMode(MCP_NORMAL);
#endif
#if LED
  ledSweep(); // Sweep all leds on and then off
#endif
}

uint8_t hue = 100;   // Green
uint8_t brightness = 50;
uint8_t ledMode = 0;

void loop() {
  if (buttonPressed()) {
    ++ledMode %= 3;
  }

#if LED
  if (!ledMode) {
    spinner();
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

void nextTrack() {
  pinMode(BT_NEXT, OUTPUT);
  digitalWrite(BT_NEXT, LOW);
  delay(50);
  pinMode(BT_NEXT, INPUT);
}

void previousTrack() {
  pinMode(BT_PREVIOUS, OUTPUT);
  digitalWrite(BT_PREVIOUS, LOW);
  delay(50);
  pinMode(BT_PREVIOUS, INPUT);
}
/*
   Spinning LED animation with trailing tail
   i is static variable, so it is initialized only once and remembers its position after function exits
*/
void spinner() {
  static uint8_t i = 0;
  EVERY_N_MILLISECONDS(85) {
    leds[i++] = CHSV(hue, 255, brightness);
    i %= NUM_LEDS;
    FastLED.show();
    fadeToBlackBy(leds, NUM_LEDS, brightness / (NUM_LEDS / 4));
  }
}

/* Visualizes given value with leds.
  Color range is from green to red.
  Green : 100
  Red   : 0
*/

void gauge(uint16_t val, uint16_t maximum) {
  val = map(val, 0, maximum, 0, NUM_LEDS);
  FastLED.clear();
  for (uint8_t i = 0; i < val; i++) {
    leds[i] = CHSV(100 - i * (100 / NUM_LEDS), 255, brightness);
  }
  FastLED.show();
}

/*
  Sweep through the LED ring, first by coloring leds one by one
  and then clearing them again one by one.
*/

void ledSweep() {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(hue, 255, 255);
    FastLED.show();
    delay(40);
  }
  for (uint8_t i = NUM_LEDS - 1; i > 0; i--) {
    leds[i] = CHSV(0, 0, 0);
    FastLED.show();
    delay(40);
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
  long unsigned id;

  if (CAN.checkReceive() == CAN_MSGAVAIL) {
    CAN.readMsgBuf(&id, &len, data);
    switch (id) {
      case IBUS_BUTTONS:
        action = getHighBit(data[AUDIO]);
        runAction(action, audioActions);

        action = getHighBit(data[SID]);
        runAction(action, sidActions);
        break;
      case ENGINE:
        engineActions(data);
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
uint8_t getHighBit(const uint8_t value) {
  if (!value) return 0xFF;

  for (uint8_t i = 0; i < 8; i++) {
    if (value >> i & 1) {
      return i;
    }
  }
}

void audioActions(const uint8_t action) {
  switch (action) {
    case NXT:
      //TODO
      Serial.println("NEXT");
      break;
    case SEEK_DOWN:
      previousTrack();
      Serial.println("SEEK DOWN");
      break;
    case SEEK_UP:
      nextTrack();
      Serial.println("SEEK UP");
      break;
    case SRC:
      bluetooth(!digitalRead(BLUETOOTH_PIN0));
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

void sidActions(const uint8_t action) {
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
      brightness = constrain(brightness + 10, 10, 250);
      Serial.println("SET");
      break;
    case CLR:
      brightness = constrain(brightness - 10, 10, 250);
      Serial.println("CLEAR");
      break;
  }
}

void engineActions(const uint8_t data[]) {
  uint16_t rpm = combineBytes(data[RPM1], data[RPM0]);
  uint16_t spd = combineBytes(data[SPD1], data[SPD0]) / 10;
#if LED
  switch (ledMode) {
    case 1:
      gauge(rpm, MAX_RPM);
      Serial.println(rpm);
      break;
    case 2:
      gauge(spd, MAX_SPD);
      Serial.println(spd);
      break;
  }
#endif
}

uint16_t combineBytes(uint8_t _byte1, uint8_t _byte2) {
  return (_byte1 << 8 | _byte2);
}

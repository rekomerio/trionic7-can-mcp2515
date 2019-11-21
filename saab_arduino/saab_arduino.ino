/*
  By Reko Meriö
  github.com/rekomerio
*/

#include <mcp_can.h>
#include <FastLED.h>

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#MAX "Requires FastLED 3.1 or later; check github for latest code."
#endif

/*** ENABLE FUNCTIONALITIES ***/
#define LED            1
#define CANBUS         1
#define DEBUG          0

/*** DATA PINS ***/
#define BUTTON_PIN     2
#define LED_PIN        3
#define BT_PREVIOUS    4
#define BT_NEXT        5
#define TRANSISTOR_PIN 7  // Turns on radio telephone channel
#define BLUETOOTH_PIN0 8  // Turns on bluetooth module
#define BLUETOOTH_PIN1 9  // Bluetooth shares power from 2 pins
#define CAN_CS_PIN     10

/*** CAN addresses ***/
#define IBUS_BUTTONS   0x290
#define RADIO_MSG      0x328
#define RADIO_PRIORITY 0x348
#define O_SID_MSG      0x33F
#define O_SID_PRIORITY 0x358
#define TEXT_PRIORITY  0x368
#define LIGHTING       0x410
#define ENGINE         0x460

/*** ID's for devices talking to SID ***/
#define SPA            0x12
#define RADIO          0x19
#define TRIONIC        0x21
#define ACC            0x23
#define TWICE          0x2D
#define OPEN_SID       0x32

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

/*    LIGHTING  */
#define DIMM1          1
#define DIMM0          2
#define LIGHT1         3
#define LIGHT0         4

/*    ENGINE    */
#define RPM1           1
#define RPM0           2
#define SPD1           3
#define SPD0           4

/*** CAN speeds for Trionic 7 ***/
#define I_BUS          CAN_47KBPS
#define P_BUS          CAN_500KBPS

/*
   Range for light level sensor depends on SID version.
   Dimmer values might also vary.
*/
#define LIGHT_MAX      0xC7FB
#define LIGHT_MIN      0x2308
#define DIMMER_MAX     0xFE9D
#define DIMMER_MIN     0x423F

/*** SID message ***/
#define MESSAGE_LENGTH   12
#define LETTERS_IN_MSG   5

/*** LED ***/
#define HUE_GREEN        100
#define HUE_RED          0

#define NUM_LEDS         12

MCP_CAN CAN(CAN_CS_PIN);
CRGB leds[NUM_LEDS];

uint8_t priorities[3];
uint8_t hue = HUE_GREEN;
uint8_t brightness = 50;

bool bluetooth = false;

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
  startingEffect(2);
#endif
}

void loop() {
#if LED
  spinner();
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
*/
void switchBluetooth() {
  bluetooth = !bluetooth;
  digitalWrite(BLUETOOTH_PIN0, bluetooth);
  digitalWrite(BLUETOOTH_PIN1, bluetooth);
  digitalWrite(TRANSISTOR_PIN, bluetooth);
}

void nextTrack() {
  sendSidMessage("NEXT TRACK  ");
  pinMode(BT_NEXT, OUTPUT);
  digitalWrite(BT_NEXT, LOW);
  delay(70);
  pinMode(BT_NEXT, INPUT);
}

void previousTrack() {
  sendSidMessage("PREV TRACK  ");
  pinMode(BT_PREVIOUS, OUTPUT);
  digitalWrite(BT_PREVIOUS, LOW);
  delay(70);
  pinMode(BT_PREVIOUS, INPUT);
}
/*
   Spinning LED animation with trailing tail
   i is static variable, so it is initialized only once and remembers its position after function exits
*/
void spinner() {
  static uint8_t i = 0;
  EVERY_N_MILLISECONDS(85) {
    leds[i++] = CHSV(hue, 255, 255);
    i %= NUM_LEDS;
    FastLED.show();
    fadeToBlackBy(leds, NUM_LEDS, brightness / (NUM_LEDS / 4));
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
/*
  Scale brightness coming from sensor for LED ring to use.
*/
uint8_t scaleBrightness(uint16_t val, uint16_t minimum, uint16_t maximum) {
  return map(val, minimum, maximum, 40, 255);
}
/*
  Scale hue from green to red for LED ring.
*/
uint8_t scaleHue(uint16_t val, uint16_t minimum, uint16_t maximum) {
  return map(val, minimum, maximum, HUE_GREEN, HUE_RED);
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
      case TEXT_PRIORITY:
        setPriority(data[0], data[1]);
        break;
      case LIGHTING:
        lightActions(data);
        break;
      case ENGINE:
        engineActions(data);
        break;
    }
  }
}

void audioActions(const uint8_t action) {
  switch (action) {
    case NXT:
      Serial.println("NEXT");
      break;
    case SEEK_DOWN:
      if (bluetooth) {
        previousTrack();
      }
      Serial.println("SEEK DOWN");
      break;
    case SEEK_UP:
      if (bluetooth) {
        nextTrack();
      }
      Serial.println("SEEK UP");
      break;
    case SRC:
      switchBluetooth();
      Serial.println("SRC");
      break;
    case VOL_UP:
      Serial.println("VOL+");
      break;
    case VOL_DOWN:
      Serial.println("VOL-");
      break;
  }
}

void sidActions(const uint8_t action) {
  switch (action) {
    case NPANEL:
      Serial.println("NIGHT PANEL");
      break;
    case UP:
      Serial.println("UP");
      break;
    case DOWN:
      Serial.println("DOWN");
      break;
    case SET:
      Serial.println("SET");
      break;
    case CLR:
      Serial.println("CLEAR");
      break;
  }
}
/*
  Read value of manual dimmer and light level sensor in SID.
*/
void lightActions(const uint8_t data[]) {
  uint16_t dimmer = combineBytes(data[DIMM1], data[DIMM0]);
  uint16_t lightLevel = combineBytes(data[LIGHT1], data[LIGHT0]);

  brightness = scaleBrightness(lightLevel, LIGHT_MIN, LIGHT_MAX);
  FastLED.setBrightness(brightness);
}
/*
  Read rpm and vehicle speed (km/h).
*/
void engineActions(const uint8_t data[]) {
  uint16_t rpm = combineBytes(data[RPM1], data[RPM0]);
  uint16_t spd = combineBytes(data[SPD1], data[SPD0]) / 10;
  // If rpm is less than 3000, color is green, otherwise scale it from green to red as the rpm gets higher.
  hue = rpm < 3000 ? HUE_GREEN : scaleHue(rpm, 3000, 6000);
}
/*
  Set current priority for all SID rows.
  Priority informs which device is using the row.

  Priority 0: Are both rows being used.
  Priority 1: Is row one being used.
  Priority 2: Is row two being used.

  If priority is equal to 0xFF, row is not being used.
*/
void setPriority(uint8_t row, uint8_t priority) {
  switch (row) {
    case 0:
      priorities[0] = priority;
      break;
    case 1:
      priorities[1] = priority;
      break;
    case 2:
      priorities[2] = priority;
      break;
  }
}
/*
  Check priority for SID rows to see if it's wise to overwrite them.
  If both rows are used, write is not allowed.
  If priority for requested row is equal to your device id, write is allowed.
*/
bool allowedToWrite(uint8_t row, uint8_t writeAs) {
  if (priorities[0] != 0xFF)      return false;
  if (priorities[row] == writeAs) return true;
}
/*
  Request SID to not display message from radio on row 2.
*/
void doNotDisplay() {
  uint8_t data[] = {0x11, 0x02, 0xFF, RADIO, 0, 0, 0, 0};
  CAN.sendMsgBuf(RADIO_PRIORITY, 0, 8, data);
}
/*
  Request write to SID row 2 as OPEN SID
*/
void requestWrite() {
  uint8_t data[] = {0x21, 0x02, 0x03, OPEN_SID, 0, 0, 0, 0};
  CAN.sendMsgBuf(O_SID_PRIORITY, 0, 8, data);
}
/*
    Message is sent to address that radio uses for writing to SID.
    Message length needs to be 12 characters, so if your message is not that long, simply add spaces to fill it up,
    or otherwise there will be some trash written on the SID.
*/
void sendSidMessage(const char letters[MESSAGE_LENGTH]) {
  if (allowedToWrite(2, RADIO)) {      // Check if it's ok to write as RADIO to row 2.
    uint8_t message[8];
    enum BYTE {ORDER, IDK, ROW, LETTER0, LETTER1, LETTER2, LETTER3, LETTER4};
    message[ORDER] = 0x42;             // Must be 0x42 on the first msg, when message length is equal to 12
    message[IDK]   = 0x96;             // Unknown, potentially SID id?
    message[ROW]   = 0x02;             // Row 2
    for (uint8_t i = 0; i < 3; i++) {  // Group of 3 messages need to be sent
      if (i > 0) {
        message[ORDER] = 2 - i;        // 2nd message: order is 1, 3rd message: order is 0
      }
      for (uint8_t j = 0; j < LETTERS_IN_MSG; j++) {
        uint8_t letter = i * LETTERS_IN_MSG + j;
        if (letters[letter] && letter < MESSAGE_LENGTH) {
          message[LETTER0 + j] = letters[letter];
        } else {
          message[LETTER0 + j] = 0x00;
        }
      }
      CAN.sendMsgBuf(RADIO_MSG , 0, 8, message);
      delay(10);
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

uint16_t combineBytes(uint8_t _byte1, uint8_t _byte2) {
  return (_byte1 << 8 | _byte2);
}

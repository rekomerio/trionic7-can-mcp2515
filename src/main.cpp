/*
  By Reko Meriö
  github.com/rekomerio
*/

#include <Arduino.h>
#include <FastLED.h>
#include "mcp_can.h"
#include "defines.h"
#include "communication.h"

void controlLeds();
void readCanBus();
void spinner();
void ledInit();
uint8_t scaleBrightness(uint16_t val, uint16_t minimum, uint16_t maximum);
void readCanBus();
void steeringWheelActions(const uint8_t action);
void sidActions(const uint8_t action);
void lightActions(uint8_t *data);
void vehicleActions(uint8_t *data);
void setPriority(uint8_t row, uint8_t priority);
bool allowedToWrite(uint8_t row, uint8_t writeAs);
void sendSidMessage(const char *letters);
uint8_t getHighBit(const uint8_t value);
uint16_t combineBytes(uint8_t byte1, uint8_t byte2);
uint32_t elapsed(uint32_t time);

MCP_CAN CAN(CAN_CS_PIN);

CRGB ledsOfRing[NUM_LEDS_RING];
CRGB ledsOfStrip[NUM_LEDS_STRIP];

uint8_t hue;
uint8_t priorities[3];

bool isBluetoothEnabled;
bool isNightPanelEnabled;
bool isLightLevelSet;
bool isLedInit;
bool areLedStripsEnabled;

uint32_t clearLastPressedAt = 0;
uint32_t setLastPressedAt = 0;

void setup()
{
    hue = HUE_GREEN;
    isBluetoothEnabled = false;
    isNightPanelEnabled = false;
    isLightLevelSet = false;
    isLedInit = false;
    areLedStripsEnabled = true;
#if DEBUG
    Serial.begin(115200);
#endif
    FastLED.addLeds<NEOPIXEL, LED_RING_PIN>(ledsOfRing, NUM_LEDS_RING);
    FastLED.addLeds<NEOPIXEL, LED_STRIP_PIN>(ledsOfStrip, NUM_LEDS_STRIP);
    pinMode(BUTTON_PIN, INPUT);
    pinMode(BLUETOOTH_PIN0, OUTPUT);
    pinMode(BLUETOOTH_PIN1, OUTPUT);
    pinMode(TRANSISTOR_PIN, OUTPUT);
#if CANBUS
    while (CAN.begin(MCP_ANY, I_BUS, MCP_8MHZ) != CAN_OK)
    {
        delay(100);
    }
    CAN.setMode(MCP_NORMAL);
#endif
}

void loop()
{
#if LED
    controlLeds();
#endif
#if CANBUS
    readCanBus();
#endif
}

void controlLeds()
{
    if (isLightLevelSet)
    {
        if (isLedInit)
        {
            EVERY_N_MILLISECONDS(85)
            {
                fill_solid(ledsOfStrip, NUM_LEDS_STRIP, CHSV(hue, 255, areLedStripsEnabled ? STRIP_BRIGHTNESS : 0));
                spinner();
                FastLED.show();
            }
        }
        else
        {
            EVERY_N_MILLISECONDS(50)
            {
                ledInit();
            }
        }
    }
}

/*
   Turn bluetooth on or off
*/
void toggleBluetooth()
{
    isBluetoothEnabled = !isBluetoothEnabled;
    digitalWrite(BLUETOOTH_PIN0, isBluetoothEnabled);
    digitalWrite(BLUETOOTH_PIN1, isBluetoothEnabled);
    digitalWrite(TRANSISTOR_PIN, isBluetoothEnabled);
}

const char *nextTrackStr = "NEXT TRACK  ";

void nextTrack()
{
    sendSidMessage(nextTrackStr);
    pinMode(BT_NEXT, OUTPUT);
    digitalWrite(BT_NEXT, LOW);
    delay(70);
    pinMode(BT_NEXT, INPUT);
}

const char *prevTrackStr = "PREV TRACK  ";

void previousTrack()
{
    sendSidMessage(prevTrackStr);
    pinMode(BT_PREVIOUS, OUTPUT);
    digitalWrite(BT_PREVIOUS, LOW);
    delay(70);
    pinMode(BT_PREVIOUS, INPUT);
}
/*
	Animate LED's on startup
*/
void ledInit()
{
    static uint8_t i = 0;
    static uint8_t brightness = 0;
    constexpr uint8_t bIncrement = STRIP_BRIGHTNESS / NUM_LEDS_RING;
    uint8_t j = i + (NUM_LEDS_RING / 2);

    brightness += bIncrement;
    ledsOfRing[i % NUM_LEDS_RING] = CHSV(220, 255, 255);
    ledsOfRing[j % NUM_LEDS_RING] = CHSV(180, 255, 255);
    fill_solid(ledsOfStrip, NUM_LEDS_STRIP, CHSV(hue, 255, brightness));
    FastLED.show();
    ledsOfRing[i % NUM_LEDS_RING] = CRGB::Black;
    ledsOfRing[j % NUM_LEDS_RING] = CRGB::Black;

    if (++i >= NUM_LEDS_RING)
    {
        isLedInit = true;
    }
}
/*
   Spinning LED animation with trailing tail
*/
void spinner()
{
    static uint8_t i = 0;

    fadeToBlackBy(ledsOfRing, NUM_LEDS_RING, 85);
    ledsOfRing[i++] = CHSV(hue, 255, 255);
    i %= NUM_LEDS_RING;
}
/*
  Scale brightness coming from sensor for LED ring to use.
*/
uint8_t scaleBrightness(uint16_t val, uint16_t minimum, uint16_t maximum)
{
    return map(val, minimum, maximum, 20, 255);
}
/*
  Reads incoming data from CAN bus, if there is any and runs desired action.
*/
void readCanBus()
{
    uint8_t len;
    uint8_t data[8];
    long unsigned id;

    if (CAN.checkReceive() == CAN_MSGAVAIL)
    {
        CAN.readMsgBuf(&id, &len, data);
        switch (static_cast<CAN_ID>(id))
        {
        case CAN_ID::IBUS_BUTTONS:
        {
            uint8_t action = getHighBit(data[AUDIO]);
            steeringWheelActions(action);

            action = getHighBit(data[SID]);
            sidActions(action);
            break;
        }
        case CAN_ID::TEXT_PRIORITY:
            setPriority(data[0], data[1]);
            break;
        case CAN_ID::LIGHTING:
            lightActions(data);
            break;
        case CAN_ID::SPEED_RPM:
            vehicleActions(data);
            break;
        }
    }
}

void steeringWheelActions(const uint8_t action)
{
    switch (static_cast<STEERING_WHEEL>(action))
    {
    case STEERING_WHEEL::NXT:
        DEBUG_MESSAGE("NEXT");
        break;
    case STEERING_WHEEL::SEEK_DOWN:
        if (isBluetoothEnabled)
        {
            previousTrack();
        }
        DEBUG_MESSAGE("SEEK DOWN");
        break;
    case STEERING_WHEEL::SEEK_UP:
        if (isBluetoothEnabled)
        {
            nextTrack();
        }
        DEBUG_MESSAGE("SEEK UP");
        break;
    case STEERING_WHEEL::SRC:
        toggleBluetooth();
        DEBUG_MESSAGE("SRC");
        break;
    case STEERING_WHEEL::VOL_UP:
        DEBUG_MESSAGE("VOL+");
        break;
    case STEERING_WHEEL::VOL_DOWN:
        DEBUG_MESSAGE("VOL-");
        break;
    }
}

void sidActions(const uint8_t action)
{
    switch (static_cast<SID_BUTTON>(action))
    {
    case SID_BUTTON::NPANEL:
        isNightPanelEnabled = !isNightPanelEnabled;
        DEBUG_MESSAGE("NIGHT PANEL");
        break;
    case SID_BUTTON::UP:
        DEBUG_MESSAGE("UP");
        break;
    case SID_BUTTON::DOWN:
        DEBUG_MESSAGE("DOWN");
        break;
    case SID_BUTTON::SET:
        if (elapsed(setLastPressedAt) < 500)
        {
            areLedStripsEnabled = true;
            DEBUG_MESSAGE("SET DOUBLETAP");
        }
        setLastPressedAt = millis();
        DEBUG_MESSAGE("SET");
        break;
    case SID_BUTTON::CLR:
        if (elapsed(clearLastPressedAt) < 500)
        {
            areLedStripsEnabled = false;
            DEBUG_MESSAGE("CLEAR DOUBLETAP");
        }
        clearLastPressedAt = millis();
        DEBUG_MESSAGE("CLEAR");
        break;
    }
}
/*
  Read value of manual dimmer and light level sensor in SID.
*/
void lightActions(uint8_t *data)
{
    // uint16_t dimmer = combineBytes(data[DIMM1], data[DIMM0]);
    uint16_t lightLevel = combineBytes(data[LIGHT1], data[LIGHT0]);

    uint8_t brightness = scaleBrightness(lightLevel, LIGHT_MIN, LIGHT_MAX);
    FastLED.setBrightness(isNightPanelEnabled ? 0 : brightness);
    isLightLevelSet = true;
}
/*
  Read rpm and vehicle speed (km/h).
*/
void vehicleActions(uint8_t *data)
{
    // uint16_t rpm = combineBytes(data[RPM1], data[RPM0]);
    // uint16_t spd = combineBytes(data[SPD1], data[SPD0]) / 10;
}
/*
  Set current priority for all SID rows.
  Priority informs which device is using the row.

  Priority 0: Are both rows being used by one device.
  Priority 1: Is row one being used.
  Priority 2: Is row two being used.

  If priority is equal to 0xFF, row is not being used.
*/
void setPriority(uint8_t row, uint8_t priority)
{
    priorities[row] = priority;
}
/*
  Check priority for SID rows to see if it's wise to overwrite them.
  If both rows are used, write is not allowed.
  If priority for requested row is equal to your device id, write is allowed.
*/
bool allowedToWrite(uint8_t row, uint8_t writeAs)
{
    if (priorities[0] != 0xFF)
        return false;
    if (priorities[row] == writeAs)
        return true;
    return false;
}
/*
    Message is sent to address that radio uses for writing to SID.
    Message length needs to be 12 characters, so if your message is not that long, simply add spaces to fill it up,
    or otherwise there will be some trash written on the SID.
*/
void sendSidMessage(const char *letters)
{
    if (allowedToWrite(2, RADIO))
    { // Check if it's ok to write as RADIO to row 2.
        uint8_t message[8];
        message[SID_MESSAGE::ORDER] = 0x42; // Must be 0x42 on the first msg, when message length is equal to 12
        message[SID_MESSAGE::IDK] = 0x96;   // Unknown, potentially SID id?
        message[SID_MESSAGE::ROW] = 0x02;   // Row 2
        for (uint8_t i = 0; i < 3; i++)
        { // Group of 3 messages need to be sent
            if (i > 0)
            {
                message[SID_MESSAGE::ORDER] = 2 - i; // 2nd message: order is 1, 3rd message: order is 0
            }
            for (uint8_t j = 0; j < LETTERS_IN_MSG; j++)
            {
                uint8_t letter = i * LETTERS_IN_MSG + j;
                if (letters[letter] && letter < MESSAGE_LENGTH)
                {
                    message[SID_MESSAGE::LETTER0 + j] = letters[letter];
                }
                else
                {
                    message[SID_MESSAGE::LETTER0 + j] = 0x00;
                }
            }
            CAN.sendMsgBuf(static_cast<unsigned long>(CAN_ID::RADIO_MSG), 0, 8, message);
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
uint8_t getHighBit(const uint8_t value)
{
    if (!value)
        return 0xFF;

    for (uint8_t i = 0; i < 8; i++)
    {
        if (value >> i & 1)
        {
            return i;
        }
    }
    return 0xFF;
}

uint16_t combineBytes(uint8_t byte1, uint8_t byte2)
{
    return (byte1 << 8 | byte2);
}

uint32_t elapsed(uint32_t time)
{
    return millis() - time;
}
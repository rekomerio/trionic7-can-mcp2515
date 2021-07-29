/*
  By Reko Meriö
  github.com/rekomerio
*/

#include <Arduino.h>
#include "mcp_can.h"
#include "defines.h"
#include "communication.h"
#include "headers.h"
#include "LEDController.h"
#include "SidMessageHandler/SidMessageHandler.h"

MCP_CAN CAN(CAN_CS_PIN);
LEDController ledController;
SidMessageHandler sidMessageHandler(&CAN);

bool isBluetoothEnabled;
bool isNightPanelEnabled;

struct 
{
    uint32_t clearLastPressedAt = 0;
    uint32_t setLastPressedAt = 0;
} sidButtons;

void setup()
{
    isBluetoothEnabled = false;
    isNightPanelEnabled = false;
#if DEBUG
    Serial.begin(115200);
#endif
    ledController.init();
    pinMode(BUTTON_PIN, INPUT);
    pinMode(BLUETOOTH_PIN0, OUTPUT);
    pinMode(BLUETOOTH_PIN1, OUTPUT);
    pinMode(TRANSISTOR_PIN, OUTPUT);
    while (CAN.begin(MCP_ANY, I_BUS, MCP_8MHZ) != CAN_OK)
    {
        delay(100);
    }
    CAN.setMode(MCP_NORMAL);
}

void loop()
{
    readCanBus();
    ledController.update();
    sidMessageHandler.update();
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

void nextTrack()
{
    const static char *nextTrackStr = "NEXT TRACK";
    sidMessageHandler.sendMessage(nextTrackStr, 800);
    pinMode(BT_NEXT, OUTPUT);
    digitalWrite(BT_NEXT, LOW);
    delay(70);
    pinMode(BT_NEXT, INPUT);
}

void previousTrack()
{
    const static char *prevTrackStr = "PREV TRACK";
    sidMessageHandler.sendMessage(prevTrackStr, 800);
    pinMode(BT_PREVIOUS, OUTPUT);
    digitalWrite(BT_PREVIOUS, LOW);
    delay(70);
    pinMode(BT_PREVIOUS, INPUT);
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
            steeringWheelActions(static_cast<STEERING_WHEEL>(action));

            action = getHighBit(data[SID]);
            sidActions(static_cast<SID_BUTTON>(action));
            break;
        }
        case CAN_ID::LIGHTING:
            lightActions(data);
            break;
        case CAN_ID::SPEED_RPM:
            vehicleActions(data);
            break;
        case CAN_ID::TEXT_PRIORITY:
            sidMessageHandler.setPriority(data[0], data[1]);
            break;
        case CAN_ID::RADIO_MSG:
            sidMessageHandler.onReceive(id, data);
            break;
        }
    }
}

void steeringWheelActions(STEERING_WHEEL action)
{
    switch (action)
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

void sidActions(SID_BUTTON action)
{
    switch (action)
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
        if (elapsed(sidButtons.setLastPressedAt) < 500)
        {
            ledController.config.areLedStripsEnabled = true;
            DEBUG_MESSAGE("SET DOUBLETAP");
        }
        sidButtons.setLastPressedAt = millis();
        DEBUG_MESSAGE("SET");
        break;
    case SID_BUTTON::CLR:
        if (elapsed(sidButtons.clearLastPressedAt) < 500)
        {
            ledController.config.areLedStripsEnabled = false;
            DEBUG_MESSAGE("CLEAR DOUBLETAP");
        }
        sidButtons.clearLastPressedAt = millis();
        DEBUG_MESSAGE("CLEAR");
        break;
    }
}
/*
  Read value of manual dimmer and light level sensor in SID.
*/
void lightActions(const uint8_t *data)
{
    // uint16_t dimmer = combineBytes(data[DIMM1], data[DIMM0]);
    uint16_t lightLevel = combineBytes(data[LIGHT1], data[LIGHT0]);

    uint8_t brightness = scaleBrightness(lightLevel, LIGHT_MIN, LIGHT_MAX);
    ledController.setBrightness(isNightPanelEnabled ? 0 : brightness);
}
/*
  Read rpm and vehicle speed (km/h).
*/
void vehicleActions(const uint8_t *data)
{
    // uint16_t rpm = combineBytes(data[RPM1], data[RPM0]);
    // uint16_t spd = combineBytes(data[SPD1], data[SPD0]) / 10;
}
/*
  Checks every bit of the given value until finds the first high bit and then
  returns the position of that; or if none are high, returns 0xFF (255).
  @param value
  @return - first high bit of the given value or 0xFF
*/
uint8_t getHighBit(uint8_t value)
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
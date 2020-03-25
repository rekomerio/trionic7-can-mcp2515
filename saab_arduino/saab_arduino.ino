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
#define LED    1
#define CANBUS 1
#define DEBUG  0

/*** DATA PINS ***/
#define BUTTON_PIN 		2
#define LED_RING_PIN 	3
#define BT_PREVIOUS 	4
#define BT_NEXT 		5
#define LED_STRIP_PIN	6
#define TRANSISTOR_PIN 	7 // Turns on radio telephone channel
#define BLUETOOTH_PIN0 	8 // Turns on bluetoothState module
#define BLUETOOTH_PIN1 	9 // Bluetooth shares power from 2 pins
#define CAN_CS_PIN 		10

/*** CAN addresses ***/
#define IBUS_BUTTONS  	0x290
#define RADIO_MSG 		0x328
#define RADIO_PRIORITY 	0x348
#define O_SID_MSG 		0x33F
#define O_SID_PRIORITY 	0x358
#define TEXT_PRIORITY 	0x368
#define LIGHTING 		0x410
#define ENGINE 			0x460

/*** ID's for devices talking to SID ***/
#define SPA 			0x12
#define RADIO 			0x19
#define TRIONIC 		0x21
#define ACC 			0x23
#define TWICE 			0x2D
#define OPEN_SID 		0x32

/*** CAN bytes ***/
#define AUDIO 		2
#define SID 		3

/*** CAN bits ***/

/*   AUDIO      */
#define NXT 		2
#define SEEK_DOWN 	3
#define SEEK_UP 	4
#define SRC 		5
#define VOL_UP 		6
#define VOL_DOWN 	7

/*    SID       */
#define NPANEL 		3
#define UP 			4
#define DOWN 		5
#define SET 		6
#define CLR 		7

/*    LIGHTING  */
#define DIMM1 		1
#define DIMM0 		2
#define LIGHT1 		3
#define LIGHT0 		4

/*    ENGINE    */
#define RPM1 		1
#define RPM0 		2
#define SPD1 		3
#define SPD0 		4

/*** CAN speeds for Trionic 7 ***/
#define I_BUS CAN_47KBPS
#define P_BUS CAN_500KBPS

/*
   Range for light level sensor depends on SID version.
   Dimmer values might also vary.
*/
#define LIGHT_MAX 	0xC7FB
#define LIGHT_MIN 	0x2308
#define DIMMER_MAX	0xFE9D
#define DIMMER_MIN 	0x423F

/*** SID message ***/
#define MESSAGE_LENGTH 12
#define LETTERS_IN_MSG 5

/*** LED ***/
#define HUE_GREEN 100

#define NUM_LEDS_RING 12
#define NUM_LEDS_STRIP 9

MCP_CAN CAN(CAN_CS_PIN);
CRGB ledsOfRing[NUM_LEDS_RING];
CRGB ledsOfStrip[NUM_LEDS_STRIP];

uint8_t priorities[3];
uint8_t activeMode;
uint8_t hue;

bool isBluetoothEnabled;
bool isNightPanelEnabled;

void setup()
{
	hue = HUE_GREEN;
	activeMode = 0;
	isBluetoothEnabled = false;
	isNightPanelEnabled = false;

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
#if LED
	fill_solid(ledsOfStrip, NUM_LEDS_STRIP, CHSV(hue, 255, 255));
	startingEffect(1);
#endif
}

void loop()
{
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
bool isButtonPressed()
{
	static uint8_t lastState = 0;
	uint8_t buttonState = digitalRead(BUTTON_PIN);

	if (buttonState && lastState != buttonState)
	{
		lastState = buttonState;
		return true;
	}
	lastState = buttonState;
	return false;
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
	sendSidMessage("NEXT TRACK  ");
	pinMode(BT_NEXT, OUTPUT);
	digitalWrite(BT_NEXT, LOW);
	delay(70);
	pinMode(BT_NEXT, INPUT);
}

void previousTrack()
{
	sendSidMessage("PREV TRACK  ");
	pinMode(BT_PREVIOUS, OUTPUT);
	digitalWrite(BT_PREVIOUS, LOW);
	delay(70);
	pinMode(BT_PREVIOUS, INPUT);
}
/*
   Spinning LED animation with trailing tail
*/
void spinner()
{
	static uint8_t i = 0;
	EVERY_N_MILLISECONDS(85)
	{
		ledsOfRing[i++] = CHSV(hue, 255, 255);
		i %= NUM_LEDS_RING;
		FastLED.show();
		fadeToBlackBy(ledsOfRing, NUM_LEDS_RING, 85);
	}
}
/*
   Two spinning LED particles, one half a round further than the other
   @param rounds - how many complete rounds to do
*/
void startingEffect(uint8_t rounds)
{
	for (uint16_t i = 0; i < NUM_LEDS_RING * rounds; i++)
	{
		uint16_t j = i + (NUM_LEDS_RING / 2);
		ledsOfRing[i % NUM_LEDS_RING] = CHSV(220, 255, 255);
		ledsOfRing[j % NUM_LEDS_RING] = CHSV(180, 255, 255);
		FastLED.show();
		ledsOfRing[i % NUM_LEDS_RING] = CRGB::Black;
		ledsOfRing[j % NUM_LEDS_RING] = CRGB::Black;
		delay(50);
	}
}
/*
  Scale brightness coming from sensor for LED ring to use.
*/
uint8_t scaleBrightness(uint16_t val, uint16_t minimum, uint16_t maximum)
{
	return map(val, minimum, maximum, 20, 255);
}
/*
  Scale hue from green to red for LED ring.
*/
uint8_t scaleHue(uint16_t val, uint16_t minimum, uint16_t maximum)
{
	return map(val, minimum, maximum, HUE_GREEN, HUE_RED);
}
/*
  Reads incoming data from CAN bus, if there is any and runs desired action.
*/
void readCanBus()
{
	uint8_t action;
	uint8_t len;
	uint8_t data[8];
	long unsigned id;

	if (CAN.checkReceive() == CAN_MSGAVAIL)
	{
		CAN.readMsgBuf(&id, &len, data);
		switch (id)
		{
		case IBUS_BUTTONS:
			action = getHighBit(data[AUDIO]);
			audioActions(action);

			action = getHighBit(data[SID]);
			sidActions(action);
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

void audioActions(const uint8_t action)
{
	switch (action)
	{
	case NXT:
		Serial.println("NEXT");
		break;
	case SEEK_DOWN:
		if (isBluetoothEnabled)
		{
			previousTrack();
		}
		Serial.println("SEEK DOWN");
		break;
	case SEEK_UP:
		if (isBluetoothEnabled)
		{
			nextTrack();
		}
		Serial.println("SEEK UP");
		break;
	case SRC:
		toggleBluetooth();
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

void sidActions(const uint8_t action)
{
	switch (action)
	{
	case NPANEL:
		isNightPanelEnabled = !isNightPanelEnabled;
		Serial.println("NIGHT PANEL");
		break;
	case UP:
		if (activeMode)
		{
			hue += 20;
			fill_solid(ledsOfStrip, NUM_LEDS_STRIP, CHSV(hue, 255, 255));
		}
		Serial.println("UP");
		break;
	case DOWN:
		if (activeMode)
		{
			hue -= 20;
			fill_solid(ledsOfStrip, NUM_LEDS_STRIP, CHSV(hue, 255, 255));
		}
		Serial.println("DOWN");
		break;
	case SET:
		activeMode++;
		Serial.println("SET");
		break;
	case CLR:
		activeMode = 0;
		Serial.println("CLEAR");
		break;
	}
}
/*
  Read value of manual dimmer and light level sensor in SID.
*/
void lightActions(const uint8_t data[])
{
	uint16_t dimmer = combineBytes(data[DIMM1], data[DIMM0]);
	uint16_t lightLevel = combineBytes(data[LIGHT1], data[LIGHT0]);

	uint8_t brightness = scaleBrightness(lightLevel, LIGHT_MIN, LIGHT_MAX);
	FastLED.setBrightness(isNightPanelEnabled ? 0 : brightness);
}
/*
  Read rpm and vehicle speed (km/h).
*/
void engineActions(const uint8_t data[])
{
	uint16_t rpm = combineBytes(data[RPM1], data[RPM0]);
	uint16_t spd = combineBytes(data[SPD1], data[SPD0]) / 10;
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
  Request SID to not display message from radio on row 2.
*/
void doNotDisplay()
{
	uint8_t data[] = {0x11, 0x02, 0xFF, RADIO, 0, 0, 0, 0};
	CAN.sendMsgBuf(RADIO_PRIORITY, 0, 8, data);
}
/*
  Request write to SID row 2 as OPEN SID
*/
void requestWrite()
{
	uint8_t data[] = {0x21, 0x02, 0x03, OPEN_SID, 0, 0, 0, 0};
	CAN.sendMsgBuf(O_SID_PRIORITY, 0, 8, data);
}
/*
    Message is sent to address that radio uses for writing to SID.
    Message length needs to be 12 characters, so if your message is not that long, simply add spaces to fill it up,
    or otherwise there will be some trash written on the SID.
*/
void sendSidMessage(const char letters[MESSAGE_LENGTH])
{
	if (allowedToWrite(2, RADIO))
	{ // Check if it's ok to write as RADIO to row 2.
		uint8_t message[8];
		enum BYTE
		{
			ORDER,
			IDK,
			ROW,
			LETTER0,
			LETTER1,
			LETTER2,
			LETTER3,
			LETTER4
		};
		message[ORDER] = 0x42; // Must be 0x42 on the first msg, when message length is equal to 12
		message[IDK] = 0x96;   // Unknown, potentially SID id?
		message[ROW] = 0x02;   // Row 2
		for (uint8_t i = 0; i < 3; i++)
		{ // Group of 3 messages need to be sent
			if (i > 0)
			{
				message[ORDER] = 2 - i; // 2nd message: order is 1, 3rd message: order is 0
			}
			for (uint8_t j = 0; j < LETTERS_IN_MSG; j++)
			{
				uint8_t letter = i * LETTERS_IN_MSG + j;
				if (letters[letter] && letter < MESSAGE_LENGTH)
				{
					message[LETTER0 + j] = letters[letter];
				}
				else
				{
					message[LETTER0 + j] = 0x00;
				}
			}
			CAN.sendMsgBuf(RADIO_MSG, 0, 8, message);
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
}

uint16_t combineBytes(uint8_t _byte1, uint8_t _byte2)
{
	return (_byte1 << 8 | _byte2);
}

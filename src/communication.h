enum SID_MESSAGE
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

enum class CAN_ID : unsigned long
{
    IBUS_BUTTONS = 0x290,
    RADIO_MSG = 0x328,
    RADIO_PRIORITY = 0x348,
    O_SID_MSG = 0x33F,
    O_SID_PRIORITY = 0x358,
    TEXT_PRIORITY = 0x368,
    LIGHTING = 0x410,
    SPEED_RPM = 0x460
};

enum class STEERING_WHEEL : unsigned char
{
    NXT = 2,
    SEEK_DOWN = 3,
    SEEK_UP = 4,
    SRC = 5,
    VOL_UP = 6,
    VOL_DOWN = 7
};

enum class SID_BUTTON : unsigned char
{
    NPANEL = 3,
    UP = 4,
    DOWN = 5,
    SET = 6,
    CLR = 7
};
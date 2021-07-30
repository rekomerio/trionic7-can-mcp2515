#include <Arduino.h>
#include "mcp_can.h"
#include "../../include/defines.h"
#include "../../include/communication.h"
#include "../util/util.h"

class SidMessageHandler
{
    enum class DisplayedMessage : uint8_t
    {
        User,
        Trionic
    };

public:
    SidMessageHandler(MCP_CAN *CAN);
    void onReceive(unsigned long id, uint8_t *data);
    bool sendMessage(const char *buffer, uint16_t displayTime);
    void setPriority(uint8_t row, uint8_t priority);
    bool isAllowedToWrite(uint8_t row, uint8_t writeAs);
    void update();
    void cancelMessage();

private:
    bool sendMessage(uint8_t *buffer, DisplayedMessage displayedMessage);
    void constructMessage(const char *message, uint8_t *buffer, uint8_t length);

    struct {
        // Original string is stored here in case we need to roll it
        char messageString[MESSAGE_MAX_LENGTH];
        uint8_t messageLength;
        // SID message is stored here
        uint8_t messageBuffer[24];
        uint32_t messageSentAt;
        uint16_t messageDisplayTime;
    } _user;

    struct {
        uint8_t rollingIndex;
        uint16_t rollingDelay;
        uint32_t lastRolledAt;
    } _messageRolling;

    bool _isReceivedMessageComplete;
    uint8_t _receivedMessageBuffer[24];
    uint8_t _priorities[3];
    DisplayedMessage _displayedMessage;
    MCP_CAN *CAN;
};
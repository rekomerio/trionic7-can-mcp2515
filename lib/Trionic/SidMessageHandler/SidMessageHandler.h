#include <Arduino.h>
#include "mcp_can.h"
#include "../../include/defines.h"
#include "../../include/communication.h"

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
    void constructMessage(const char *message, uint8_t *buffer);

    bool _isReceivedMessageComplete;
    uint8_t _receivedMessageBuffer[24];
    uint8_t _userMessageBuffer[24];
    uint8_t _priorities[3];
    uint32_t _userMessageSentAt;
    uint16_t _userMessageDisplayTime;
    DisplayedMessage _displayedMessage;
    MCP_CAN *CAN;
};
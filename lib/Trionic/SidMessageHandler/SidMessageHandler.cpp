#include "SidMessageHandler.h"

SidMessageHandler::SidMessageHandler(MCP_CAN *CAN)
{
    this->CAN = CAN;
    _isReceivedMessageComplete = false;
    _userMessageDisplayTime = 0;
    _userMessageSentAt = 0;
    _displayedMessage = DisplayedMessage::Trionic;
}

/**
 * Forward received messages to here
 */
void SidMessageHandler::onReceive(unsigned long id, uint8_t *data)
{
    // Only handle messages coming from radio
    if (id != static_cast<unsigned long>(CAN_ID::RADIO_MSG)) return;

    // This is how the messages are ordered. First we will receive 42, then 1 and then 0.
    if (data[0] == 42)
    {
        memcpy(_receivedMessageBuffer, data, 8);
        _isReceivedMessageComplete = false;
        return;
    }

    if (data[0] == 1)
    {
        memcpy(_receivedMessageBuffer + 8, data, 8);
        return;
    }

    if (data[0] == 0)
    {
        memcpy(_receivedMessageBuffer + 16, data, 8);
        _displayedMessage = DisplayedMessage::Trionic;
        _isReceivedMessageComplete = true;

        // When receiving last message, check has user message been displayed for enough time and if not, resend
        if (_userMessageSentAt + _userMessageDisplayTime > millis())
        {
            sendMessage(_userMessageBuffer, DisplayedMessage::User);
        }

        return;
    }
}

/**
 * Call this method to ensure that sent messages are not displayed for too long
 */
void SidMessageHandler::update()
{
    // Check if user message has been displayed for too long
    if (_isReceivedMessageComplete && _displayedMessage == DisplayedMessage::User && _userMessageSentAt + _userMessageDisplayTime < millis())
    {
        sendMessage(_receivedMessageBuffer, DisplayedMessage::Trionic);
    }
}

// In the future queueing the message could be better
bool SidMessageHandler::sendMessage(uint8_t *buffer, DisplayedMessage displayedMessage)
{
    if (!isAllowedToWrite(2, RADIO)) return false;    

    for (uint8_t i = 0; i < 3; i++)
    {
        uint8_t *addr = buffer + i * 8;
        CAN->sendMsgBuf(static_cast<unsigned long>(CAN_ID::RADIO_MSG), 0, 8, addr);
        delay(10);
    }

    _displayedMessage = displayedMessage;
    return true;
}

bool SidMessageHandler::sendMessage(const char *buffer, uint16_t displayTime)
{
    // Store the message in _userMessageBuffer
    constructMessage(buffer, _userMessageBuffer);
    if (!sendMessage(_userMessageBuffer, DisplayedMessage::User))
    {
        return false;
    }
    _userMessageSentAt = millis();
    _userMessageDisplayTime = displayTime;
    return true;
}

void SidMessageHandler::cancelMessage()
{
    sendMessage(_receivedMessageBuffer, DisplayedMessage::Trionic);
    _userMessageDisplayTime = 0;
    _userMessageSentAt = 0;
}

void SidMessageHandler::constructMessage(const char *message, uint8_t *buffer)
{
    // This is to make sure we always have a message that is long enough
    static uint8_t temp[15];
    // Fill with zeros
    memset(temp, 0, 15);
    // Copy message to temp
    memcpy(temp, message, strlen(message));

    uint8_t i = 0;
    uint8_t msgIndex = 0;

    // Could be done in a loop but I like this =>
    buffer[i++] = 0x42; // Message order
    buffer[i++] = 0x96; // Unknown, potentially SID id?
    buffer[i++] = 0x02; // Row 2
    buffer[i++] = temp[msgIndex++];
    buffer[i++] = temp[msgIndex++];
    buffer[i++] = temp[msgIndex++];
    buffer[i++] = temp[msgIndex++];
    buffer[i++] = temp[msgIndex++];
    buffer[i++] = 0x01;
    buffer[i++] = 0x96; 
    buffer[i++] = 0x02;
    buffer[i++] = temp[msgIndex++];
    buffer[i++] = temp[msgIndex++];
    buffer[i++] = temp[msgIndex++];
    buffer[i++] = temp[msgIndex++];
    buffer[i++] = temp[msgIndex++];
    buffer[i++] = 0x00;
    buffer[i++] = 0x96; 
    buffer[i++] = 0x02;
    buffer[i++] = temp[msgIndex++];
    buffer[i++] = temp[msgIndex++];
    buffer[i++] = temp[msgIndex++];
    buffer[i++] = temp[msgIndex++];
    buffer[i++] = temp[msgIndex++];
}
/*
  Set current priority for all SID rows.
  Priority informs which device is using the row.

  Priority 0: Are both rows being used by one device.
  Priority 1: Is row one being used.
  Priority 2: Is row two being used.

  If priority is equal to 0xFF, row is not being used.
*/
void SidMessageHandler::setPriority(uint8_t row, uint8_t priority)
{
    _priorities[row] = priority;
}
/*
  Check priority for SID rows to see if it's wise to overwrite them.
  If both rows are used, write is not allowed.
  If priority for requested row is equal to your device id, write is allowed.
*/
bool SidMessageHandler::isAllowedToWrite(uint8_t row, uint8_t writeAs)
{
    if (_priorities[0] != 0xFF)
        return false;
    if (_priorities[row] == writeAs)
        return true;
    return false;
}
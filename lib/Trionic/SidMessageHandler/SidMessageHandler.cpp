#include "SidMessageHandler.h"

SidMessageHandler::SidMessageHandler(MCP_CAN *CAN)
{
    this->CAN = CAN;
    _isReceivedMessageComplete = false;
    _user.messageDisplayTime = 0;
    _user.messageSentAt = 0;
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
        if (_user.messageSentAt + _user.messageDisplayTime > millis())
        {
            sendMessage(_user.messageBuffer, DisplayedMessage::User);
        }

        return;
    }
}

/**
 * Call this method to ensure that sent messages are not displayed for too long and rolling messages are shown correctly
 */
void SidMessageHandler::update()
{
    uint32_t now = millis();
    // Check if user message has been displayed for too long
    if (_isReceivedMessageComplete && _displayedMessage == DisplayedMessage::User && _user.messageSentAt + _user.messageDisplayTime < now)
    {
        sendMessage(_receivedMessageBuffer, DisplayedMessage::Trionic);
        return;
    }

    if (_displayedMessage != DisplayedMessage::User)
    {
        return;
    }

    // Check if we need to roll the message in case it is too long
    uint8_t msgLength = _user.messageLength - _messageRolling.rollingIndex;
    if (msgLength > SID_MAX_CHAR && now - _messageRolling.lastRolledAt > _messageRolling.rollingDelay) 
    {
        msgLength--;
        _messageRolling.rollingIndex++;

        char *addr = _user.messageString + _messageRolling.rollingIndex;
        constructMessage(addr, _user.messageBuffer, msgLength);
        sendMessage(_user.messageBuffer, DisplayedMessage::User);
        _messageRolling.lastRolledAt = now;
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
        // No need to delay last iteration
        if (i != 2) {
            delay(10);
        }
    }

    _displayedMessage = displayedMessage;
    return true;
}
/**
 * Maximum length for the message is MESSAGE_MAX_LENGTH, overlapping chararctes will not be displayed.
 * Strings longer than 12 characters will be rolled visible on the SID
 */
bool SidMessageHandler::sendMessage(const char *buffer, uint16_t displayTime)
{
    _messageRolling.rollingIndex = 0;
    _user.messageLength = util::minVal<uint8_t>(strlen(buffer), MESSAGE_MAX_LENGTH);
    if (_user.messageLength > SID_MAX_CHAR)
    {
        // For example:
        // displayTime = 1000
        // Message length = 13
        // Overlap = 13 - 12 = 1
        // RollingDelay = 1000 / (1 + 1) = 500
        _messageRolling.rollingDelay = displayTime / ((_user.messageLength - SID_MAX_CHAR) + 1);

        // Store the original string for rolling the text
        memcpy(_user.messageString, buffer, _user.messageLength);
    }
    // Store the constructed message in buffer
    constructMessage(buffer, _user.messageBuffer, _user.messageLength);
    if (!sendMessage(_user.messageBuffer, DisplayedMessage::User))
    {
        return false;
    }

    _user.messageSentAt = millis();
    _messageRolling.lastRolledAt = _user.messageSentAt;
    _user.messageDisplayTime = displayTime;
    return true;
}
/**
 * Cancels the last message user has sent if it is displayed and sends the original by Trionic
 */
void SidMessageHandler::cancelMessage()
{
    if (_displayedMessage == DisplayedMessage::Trionic)
    {
        return;
    }

    sendMessage(_receivedMessageBuffer, DisplayedMessage::Trionic);
    _user.messageDisplayTime = 0;
    _user.messageSentAt = 0;
}

void SidMessageHandler::constructMessage(const char *message, uint8_t *buffer, uint8_t length)
{
    // This is to make sure we always have a message that is long enough
    static uint8_t temp[SID_MAX_CHAR];
    // Fill with zeros
    memset(temp, 0, SID_MAX_CHAR);
    // Copy message to temp and make sure we dont go over if too long message is given
    memcpy(temp, message, util::minVal<uint8_t>(length, SID_MAX_CHAR));

    uint8_t i = 0;
    uint8_t msgIndex = 0;

    // Could be done in a loop but I like this =>
    buffer[i++] = 0x42; // Message order, 7th bit is set to indicate new message
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
    buffer[i++] = 0;
    buffer[i++] = 0;
    buffer[i++] = 0;
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
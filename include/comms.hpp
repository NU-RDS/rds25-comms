#ifndef __COMMS_H__
#define __COMMS_H__

#include "impl/can_comms_driver.hpp"
#include "impl/command.hpp"
#include "impl/comms_driver.hpp"
#include "impl/debug.hpp"
#include "impl/id.hpp"
#include "impl/option.hpp"

namespace comms {

class CommsController {
   public:
    CommsController(CommsDriver &driver, MCUID id) : _driver(driver), _me(id) {}

    void initialize() {
        _driver.install();
    }

    void sendCommand(CommandMessagePayload payload) {
        RawCommsMessage raw;
        raw.payload = payload.raw;

        // get the id for the message
        Option<uint32_t> idOpt = MessageInfo::getMessageID(_me, MessageContentType::MT_COMMAND);

        if (idOpt.isNone()) {
            COMMS_DEBUG_PRINT_ERRORLN("Unable to send a command! No ID found for command messages for me\n");
            return;
        }

        raw.id = idOpt.value();

        // Serial.printf("Sending message %u!\n", payload.raw);
        _driver.sendMessage(raw);
    }

    void tick() {
        RawCommsMessage message;
        if (!_driver.receiveMessage(&message)) return;
        // figure out message type it is
        Option<MessageInfo> senderInfoOpt = MessageInfo::getInfo(message.id);

        if (senderInfoOpt.isNone()) {
            COMMS_DEBUG_PRINT_ERROR("Recieved an unregistered ID! 0x%04x\n", message.id);
            return;
        }

        MessageInfo info = senderInfoOpt.value();

        if (info.sender == _me) {
            COMMS_DEBUG_PRINT_ERRORLN("Recieved a message from self!!!");
            return;
        }

        if (!info.shouldListen(_me)) {
            return;  // no need to listen
        }

        switch (info.type) {
            case MessageContentType::MT_COMMAND:
                handleCommand(message);
                break;
            case MessageContentType::MT_HEARTBEAT:
                handleHeartbeat(message);
                break;
            case MessageContentType::MT_ERROR:
                handleError(message);
                break;
            default:
                break;
        }
    }

    MCUID me() const {
        return _me;
    }

   private:
    void handleCommand(RawCommsMessage message) {
        Result<CommandMessagePayload> cmdRes = CommandMessagePayload::fromRaw(message);

        if (cmdRes.isError()) {
            COMMS_DEBUG_PRINT_ERROR("Unable to handle command: %s\,", cmdRes.error);
            return;
        }

        CommandMessagePayload cmd = cmdRes.value();

        COMMS_DEBUG_PRINT("Recieved a command! From %d, command type %d, command id %d\n ", senderInfo.mcu, cmd.type, cmd.commandID);

        switch (cmd.type) {
            case CMD_BEGIN:
                _cmdBuf.startExecution();
                break;
            case CMD_STOP:
                // stop
                break;
            case CMD_MOTOR_CONTROL:
                _cmdBuf.addCommand(cmd);
            case CMD_SENSOR_TOGGLE:
                // control sensor stream
                break;
            default:
                COMMS_DEBUG_PRINT_ERRORLN("Invalid command recieved!");
                break;
        }
    }

    void handleHeartbeat(RawCommsMessage message) {
        COMMS_DEBUG_PRINT_ERROR("Unimplemented!!!");
    }

    void handleError(RawCommsMessage message) {
        COMMS_DEBUG_PRINT_ERROR("Received errror! %d", message.payload);
    }

    CommsDriver &_driver;
    CommandBuffer _cmdBuf;
    MCUID _me;
};

}  // namespace comms

#endif  // __COMMS_H__
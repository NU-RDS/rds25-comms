#ifndef __COMMS_H__
#define __COMMS_H__

#include "impl/can_comms_driver.hpp"
#include "impl/command.hpp"
#include "impl/comms_driver.hpp"
#include "impl/debug.hpp"
#include "impl/id.hpp"

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
        _driver.sendMessage(raw);
    }

    void tick() {
        RawCommsMessage raw;
        if (!_driver.receiveMessage(&raw)) return;
        // figure out message type it is
        SenderInformation info = SenderInformation::getInfo(raw.id);
        // sanity check, but if this is our message, ignore it
        if (info.mcu == _me) return;

        switch (info.type) {
            case MessageContentType::MT_COMMAND:
                handleCommand(raw);
                break;
            case MessageContentType::MT_HEARTBEAT:
                handleHeartbeat(raw);
                break;
            case MessageContentType::MT_ERROR:
                handleError(raw);
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

        SenderInformation senderInfo = SenderInformation::getInfo(message.id);

        CommandMessagePayload cmd = cmdRes.value();

        COMMS_DEBUG_PRINT("Recieved a command! From %d, command type %d, command id %d\n ", senderInfo.mcu, cmd.type, cmd.commandID);

        _cmdBuf.addCommand(cmd);
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
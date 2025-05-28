#include "comms.hpp"

namespace comms {

CommsController::CommsController(CommsDriver& driver, MCUID id) : _driver(driver), _me(id) {}

void CommsController::initialize() {
    _driver.install();
}

void CommsController::sendCommand(CommandMessagePayload payload) {
    RawCommsMessage raw;
    raw.payload = payload.raw;

    Option<uint32_t> idOpt = MessageInfo::getMessageID(_me, MessageContentType::MT_COMMAND);
    if (idOpt.isNone()) {
        COMMS_DEBUG_PRINT_ERRORLN(
            "Unable to send a command! No ID found for command messages for me\n");
        return;
    }
    raw.id = idOpt.value();

    _driver.sendMessage(raw);
}

Option<float> CommsController::getSensorValue(MCUID sender, uint8_t sensorID) {
    // simple linear search
    bool found = false;
    SensorStatus status;

    for (SensorStatus s : _sensorStatuses) {
        if (s.sender == sender && s.sensorID == sensorID) {
            found = true;
            status = s;
        }
    }

    if (found) return Option<float>::some(status.value);

    return Option<float>::none();
};

void CommsController::addSensor(uint32_t updateRateMs, uint8_t id, std::shared_ptr<Sensor> sensor) {
    SensorDatastream stream(&_driver, me(), updateRateMs, id, sensor);
    _sensorDatastreams[id] = stream;
};

Option<CommsTickResult> CommsController::tick() {
    RawCommsMessage message;
    if (!_driver.receiveMessage(&message)) return Option<CommsTickResult>::none();

    Option<MessageInfo> senderInfoOpt = MessageInfo::getInfo(message.id);
    if (senderInfoOpt.isNone()) {
        COMMS_DEBUG_PRINT_ERROR("Recieved an unregistered ID! 0x%04x\n", message.id);
        return Option<CommsTickResult>::none();
    }
    MessageInfo info = senderInfoOpt.value();

    if (info.sender == _me) {
        COMMS_DEBUG_PRINT_ERRORLN("Recieved a message from self!!!");
        return Option<CommsTickResult>::none();
    }

    if (!info.shouldListen(_me)) {
        return Option<CommsTickResult>::none();
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

    CommsTickResult res{message, info};
    return Option<CommsTickResult>::some(res);
}

MCUID CommsController::me() const {
    return _me;
}

void CommsController::handleCommand(RawCommsMessage message) {
    Result<CommandMessagePayload> cmdRes = CommandMessagePayload::fromRaw(message);
    if (cmdRes.isError()) {
        COMMS_DEBUG_PRINT_ERROR("Unable to handle command: %s,", cmdRes.error);
        return;
    }

    CommandMessagePayload cmd = cmdRes.value();
    COMMS_DEBUG_PRINT("Recieved a command! From %d, command type %d, command id %d\n ",
                      senderInfo.mcu, cmd.type, cmd.commandID);

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

void CommsController::handleHeartbeat(RawCommsMessage message) {
    COMMS_DEBUG_PRINT_ERROR("Unimplemented!!!");
}

void CommsController::handleError(RawCommsMessage message) {
    COMMS_DEBUG_PRINT_ERROR("Received errror! %d", message.payload);
}

}  // namespace comms

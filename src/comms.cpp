#include "comms.hpp"

namespace comms {

CommsController::CommsController(CommsDriver& driver, MCUID id)
    : _driver(driver), _me(id), _heartbeatManager(&driver, id) {}

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
    // update all of our sensor datastreams
    for (auto s : _sensorDatastreams) {
        s.second.tick();
    }

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
            handleCommand(info, message);
            break;
        case MessageContentType::MT_HEARTBEAT:
            handleHeartbeat(info, message);
            break;
        case MessageContentType::MT_ERROR:
            handleError(info, message);
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

void CommsController::handleCommand(MessageInfo info, RawCommsMessage message) {
    Result<CommandMessagePayload> cmdRes = CommandMessagePayload::fromRaw(message);
    if (cmdRes.isError()) {
        COMMS_DEBUG_PRINT_ERROR("Unable to handle command: %s,", cmdRes.error());
        return;
    }

    CommandMessagePayload cmd = cmdRes.value();

    switch (cmd.type) {
        case CMD_BEGIN:
            handleCommandBegin(info, cmd);
            break;
        case CMD_STOP:
            handleCommandStop(info, cmd);
            break;
        case CMD_MOTOR_CONTROL:
            handleCommandMotorControl(info, cmd);
            break;
        case CMD_SENSOR_TOGGLE:
            handleCommandSensorToggle(info, cmd);
            break;
        default:
            COMMS_DEBUG_PRINT_ERRORLN("Invalid command recieved!");
            break;
    }
}

void CommsController::handleHeartbeat(MessageInfo info, RawCommsMessage message) {
    _heartbeatManager.sendHeartbeatResponse();
}

void CommsController::handleError(MessageInfo info, RawCommsMessage message) {
    COMMS_DEBUG_PRINT_ERROR("Received errror! %d", message.payload);
}

void CommsController::handleCommandBegin(MessageInfo info, CommandMessagePayload payload) {
    _cmdBuf.startExecution();
}

void CommsController::handleCommandStop(MessageInfo info, CommandMessagePayload payload) {
    COMMS_DEBUG_PRINT_ERROR("Command stop unimplemented!!!");
}

void CommsController::handleCommandMotorControl(MessageInfo info, CommandMessagePayload payload) {
    _cmdBuf.addCommand(payload);
}

void CommsController::handleCommandSensorToggle(MessageInfo info, CommandMessagePayload payload) {
    SensorToggleCommandOpt toggleOpt;
    toggleOpt.payload = payload.payload;

    if (_sensorDatastreams.find(toggleOpt.sensorID) == _sensorDatastreams.end()) {
        // unable to find
        COMMS_DEBUG_PRINT_ERRORLN(
            "Unable to enable/disable sensor with id %d -- don't have one registered!",
            toggleOpt.sensorID);
    } else {
        _sensorDatastreams[toggleOpt.sensorID].setStatus(toggleOpt.enable);
    }
}

}  // namespace comms

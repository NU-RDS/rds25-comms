#include "comms.hpp"

namespace comms {

CommsController::CommsController(CommsDriver& driver, MCUID id)
    : _driver(driver), _me(id), _heartbeatManager(&driver, id), _errorManager(&driver, id) {}

void CommsController::initialize() {
    _driver.install();
    _errorManager.initialize(500);
}

void CommsController::sendCommand(CommandMessagePayload payload) {
    if (_me != MCUID::MCU_HIGH_LEVEL) {
        // we should not be able to send the command
        COMMS_DEBUG_PRINT_ERRORLN("Unable to send a command! We are not high level!");
        return;
    }

    RawCommsMessage raw;
    raw.payload = payload.raw;

    Option<uint32_t> idOpt = MessageInfo::getMessageID(_me, MessageContentType::MT_COMMAND);
    if (idOpt.isNone()) {
        COMMS_DEBUG_PRINT_ERRORLN(
            "Unable to send a command! No ID found for command messages for me\n");
        return;
    }
    raw.id = idOpt.value();

    if (payload.type == CommandType::CMD_BEGIN) {
        // enqueue the command
        _startCommandEnqueued = true;
        _startCommandMessage = raw;
        COMMS_DEBUG_PRINTLN("Enqueuing start command!");
        return;
    }

    _driver.sendMessage(raw);

    // add this to the list of unacknowledged commands
    CommandAcknowledgementInfo ackInfo;
    ackInfo.lastSent = millis();
    ackInfo.numRetries = 0;
    ackInfo.message = raw;

    _unackedCommands[payload.commandID] = ackInfo;
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
}

void CommsController::enableHeartbeatRequestDispatching(uint32_t intvervalMs,
                                                        const std::vector<MCUID> toMonitor) {
    _heartbeatManager.initialize(intvervalMs, toMonitor);
}

void CommsController::reportError(ErrorCode error, ErrorSeverity severity, ErrorBehavior behavior) {
    _errorManager.reportError(error, severity, behavior);
}

void CommsController::clearError(ErrorCode error) {
    _errorManager.clearError(error);
}

void CommsController::addSensor(uint32_t updateRateMs, uint8_t id, std::shared_ptr<Sensor> sensor) {
    SensorDatastream stream(&_driver, me(), updateRateMs, id, sensor);
    _sensorDatastreams[id] = stream;
}

Option<CommsTickResult> CommsController::tick() {
    updateDatastreams();
    updateHeartbeats();
    updateCommandAcknowledgements();

    if (_startCommandEnqueued && _unackedCommands.size() == 0) {
        // we are good to go!
        _driver.sendMessage(_startCommandMessage);
        _startCommandEnqueued = false;
    }

    RawCommsMessage message;
    if (!_driver.receiveMessage(&message)) return Option<CommsTickResult>::none();

    Option<MessageInfo> senderInfoOpt = MessageInfo::getInfo(message.id);
    if (senderInfoOpt.isNone()) {
        if (_unregisteredMessageHandler != nullptr) {
            COMMS_DEBUG_PRINTLN("Unregistered message, but handling it gracefully!");
            _unregisteredMessageHandler(message);
        } else {
            COMMS_DEBUG_PRINT_ERROR("Recieved an unregistered ID! 0x%04x\n", message.id);
        }
        return Option<CommsTickResult>::none();
    }
    MessageInfo info = senderInfoOpt.value();

    if (info.sender == _me) {
        if (_unregisteredMessageHandler != nullptr) {
            COMMS_DEBUG_PRINTLN("Unregistered message, but handling it gracefully!");
            _unregisteredMessageHandler(message);
        } else {
            COMMS_DEBUG_PRINT_ERRORLN("Recieved a message from self!!!");
        }
        return Option<CommsTickResult>::none();
    }

    if (!info.shouldListen(_me)) {
        if (_unregisteredMessageHandler != nullptr) {
            COMMS_DEBUG_PRINTLN("Unregistered message, but handling it gracefully!");
            _unregisteredMessageHandler(message);
        }
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

void CommsController::setUnregisteredMessageHandler(std::function<void(RawCommsMessage)> handler) {
    _unregisteredMessageHandler = handler;
}

void CommsController::updateDatastreams() {
    // update all of our sensor datastreams
    for (auto s : _sensorDatastreams) {
        s.second.tick();
    }
}

void CommsController::updateHeartbeats() {
    // update our heartbeat manager
    bool good = _heartbeatManager.tick() || _me != MCUID::MCU_HIGH_LEVEL;
    if (!good) {
        COMMS_DEBUG_PRINT_ERRORLN("Heartbeat failure!");
    }
}

void CommsController::updateCommandAcknowledgements() {
    // figure out if we need to retransmit
    uint32_t now = millis();
    for (auto& pair : _unackedCommands) {
        if (now - pair.second.lastSent > 1000) {
            // retransmit
            if (pair.second.numRetries <= 3) {
                _driver.sendMessage(pair.second.message);
                pair.second.numRetries++;
                COMMS_DEBUG_PRINT("Retransmitting command...");
            } else {
                // mark for removal
                CommandMessagePayload payload =
                    CommandMessagePayload::fromRaw(pair.second.message).value();
                _toRemoveUnackedCommands.push_back(payload.commandID);
            }
        }
    }

    // remove the commands
    for (uint16_t commandID : _toRemoveUnackedCommands) {
        _unackedCommands.erase(commandID);
    }
    _toRemoveUnackedCommands.clear();
}

void CommsController::handleCommand(MessageInfo info, RawCommsMessage message) {
    Result<CommandMessagePayload> cmdRes = CommandMessagePayload::fromRaw(message);
    if (cmdRes.isError()) {
        COMMS_DEBUG_PRINT_ERROR("Unable to handle command: %s,", cmdRes.error());
        return;
    }

    CommandMessagePayload cmd = cmdRes.value();
    if (_me != MCUID::MCU_HIGH_LEVEL) {
        // acknoweldge the command by copying the payload
        RawCommsMessage ack;
        ack.payload = cmd.raw;
        _driver.sendMessage(ack);

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
    } else {
        // we are recieving an acknowledgement
        // check if it's true
        if (_unackedCommands.find(cmd.commandID) == _unackedCommands.end()) {
            // we recieved an ack for a command that we never sent
            COMMS_DEBUG_PRINT_ERRORLN("Received acknowledgement for command %d but don't need one!",
                                      cmd.commandID);
            return;
        }

        // erase it from the unacked commdns
        _unackedCommands.erase(cmd.commandID);
    }
}

void CommsController::handleHeartbeat(MessageInfo info, RawCommsMessage message) {
    switch (info.sender) {
        case MCUID::MCU_HIGH_LEVEL:
            _heartbeatManager.sendHeartbeatResponse();
            break;
        default:
            if (_me == MCUID::MCU_HIGH_LEVEL)
                _heartbeatManager.updateHeartbeatStatus(info.sender);
            else
                COMMS_DEBUG_PRINT_ERRORLN("Cannot handle a heartbeat response! Not the high level");
            break;
    }
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

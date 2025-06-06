#include "comms.hpp"

namespace comms {

CommsController::CommsController(CommsDriver& driver, MCUID id)
    : _driver(driver),
      _me(id),
      _heartbeatManager(&driver, id),
      _errorManager(&driver, id),
      _commandManager(&driver, id) {}

void CommsController::initialize() {
    _driver.install();
    _errorManager.initialize(500);
}

void CommsController::sendCommand(CommandMessagePayload payload) {
    _commandManager.sendCommand(payload);
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
    _commandManager.tick();
    _errorManager.tick();

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
            _commandManager.handleCommandMessage(info, message);
            break;
        case MessageContentType::MT_HEARTBEAT:
            // this is a response
            if (_me != MCUID::MCU_HIGH_LEVEL) {
                _heartbeatManager.updateHeartbeatStatus(info.sender);
            }
            break;
        case MessageContentType::MT_ERROR:
            _errorManager.handleErrorRecieve(info, message);
            break;
        case MessageContentType::MT_SENSOR_DATA:
            {
                SensorMessagePayload sensorPayload;
                sensorPayload.raw = message.payload;
                // find a sensor status and update it
                bool found = false;
                for (SensorStatus &status : _sensorStatuses) {
                    if (status.sender == info.sender && status.value == sensorPayload.sensorID) {
                        // update this guy
                        found = true;
                        status.value = sensorPayload.value;
                    }
                }

                if (!found) {
                    // add a new sensor status
                    COMMS_DEBUG_PRINTLN("Recieved sensor message for the first time!");
                    SensorStatus status;
                    status.sender = info.sender;
                    status.sensorID = sensorPayload.sensorID;
                    status.value = sensorPayload.value;
                    _sensorStatuses.push_back(status);
                }
            }
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

}  // namespace comms

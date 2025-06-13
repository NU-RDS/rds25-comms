#include "impl/sensor.hpp"

#include <Arduino.h>

#include "impl/debug.hpp"

namespace comms {

SensorDatastream::SensorDatastream()
    : _driver(nullptr),
      _sender(MCUID::MCU_ANY),
      _sensorPtr(),
      _enabled(false),
      _updateRateMs(0),
      _id(0),
      _lastSendTime(0) {}

SensorDatastream::SensorDatastream(CommsDriver* driver, MCUID sender, uint32_t updateRateMs,
                                   uint8_t id, std::shared_ptr<Sensor> sensor)
    : _driver(driver),
      _sender(sender),
      _sensorPtr(std::move(sensor)),
      _enabled(true),
      _updateRateMs(updateRateMs),
      _id(id),
      _lastSendTime(0) {}

void SensorDatastream::initialize() {
    // initialize hardware sensor
    _sensorPtr->initialize();
    // reset timer
    // random offset!
    _lastSendTime = millis();
}

void SensorDatastream::tick() {
    if (!_enabled) return;

    uint32_t now = millis();
    if (now - _lastSendTime < _updateRateMs) return;

    // read sensor and package payload
    float val = _sensorPtr->read();
    SensorMessagePayload payload{};
    payload.value = val;
    payload.sensorID = _id;

    RawCommsMessage msg{};
    Option<uint32_t> midOpt =
        MessageInfo::getMessageID(_sender, MessageContentType::MT_SENSOR_DATA);

    if (midOpt.isNone()) {
        COMMS_DEBUG_PRINT_ERRORLN(
            "Unable to send sensor data! Message ID has no mapping for %d (MCUID)", _sender);
    }

    msg.id = midOpt.value();
    msg.length = sizeof(payload);
    msg.payload = payload.raw;

    _lastSendTime = now;

    if (_driver == nullptr) {
        COMMS_DEBUG_PRINT_ERRORLN("Unable to send sensor data! Driver is null!");
        return;
    }
    _driver->sendMessage(msg);
}

void SensorDatastream::setStatus(bool enabled) {
    _enabled = enabled;
}

}  // namespace comms

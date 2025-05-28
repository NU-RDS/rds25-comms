#include "impl/sensor.hpp"

#include <Arduino.h>

namespace comms {

SensorDatastream::SensorDatastream(CommsDriver& driver, float updateRateMs, uint8_t id,
                                   std::shared_ptr<Sensor> sensor)
    : _driver(driver),
      _sensorPtr(std::move(sensor)),
      _enabled(true),
      _updateRateMs(updateRateMs),
      _id(id),
      _lastSendTime(0) {}

void SensorDatastream::initialize() {
    // initialize hardware sensor
    _sensorPtr->initialize();
    // reset timer
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
    msg.id = static_cast<uint32_t>(_id);
    msg.length = sizeof(payload);
    msg.payload = payload.raw;

    _driver.sendMessage(msg);
    _lastSendTime = now;
}

void SensorDatastream::setStatus(bool enabled) {
    _enabled = enabled;
}

}  // namespace comms

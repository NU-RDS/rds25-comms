#ifndef __SENSOR_H__
#define __SENSOR_H__

#include <functional>

#include "comms_driver.hpp"
#include "id.hpp"
#include "option.hpp"
#include "result.hpp"

namespace comms {

/// @brief A payload for a message representing some sensor information
/// This is sent from the MCU that manages the sensor, to the bus
struct SensorMessagePayload {
    union {
        uint64_t raw;
        struct {
            float value;       // 4 bytes
            uint8_t sensorID;  // 1 byte
        };
    };
};

/// @brief An abstract class for handling sensors
class Sensor {
   public:
    virtual bool initialize() = 0;
    virtual float read() = 0;
    virtual void cleanup() = 0;
};

/// @brief An implementation of Sensor that uses lambda functions
class LambdaSensor : public Sensor {
   public:
    LambdaSensor(std::function<bool()> initializeFn, std::function<float()> readFn, std::function<void()> cleanupFn) : _initialize(initializeFn), _read(readFn), _cleanup(cleanupFn) {}

    bool initialize() { return _initialize(); }
    float read() { return _read(); }
    void cleanup() { return _cleanup(); }

   private:
    std::function<bool()> _initialize;
    std::function<float()> _read;
    std::function<void()> _cleanup;
};

/// @brief A management structure for the sender of sensor information
/// This is used to keep track of 
class SensorDatastream {
   public:
    SensorDatastream(float updateRateMs, uint8_t id, std::shared_ptr<Sensor> sensor) : _updateRateMs(updateRateMs), _id(id), _sensorPtr(sensor) {}

    bool initialize() { return _sensorPtr->initialize(); }
    float read() { return _sensorPtr->read(); }
    void cleanup() { _sensorPtr->cleanup(); }

   private:
    float _updateRateMs;
    uint8_t _id;
    bool _enabled = true;
    std::shared_ptr<Sensor> _sensorPtr;
};

struct SensorStatus {
    MCUID sender;
    uint8_t sensorID;
    float value;
};

}  // namespace comms

#endif  // __SENSOR_H__
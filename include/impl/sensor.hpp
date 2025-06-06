// sensor.hpp
#ifndef __SENSOR_H__
#define __SENSOR_H__

#include <cstdint>
#include <functional>
#include <memory>

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
    virtual ~Sensor() = default;
};

/// @brief An implementation of Sensor that uses lambda functions
class LambdaSensor : public Sensor {
   public:
    LambdaSensor(std::function<bool()> initializeFn, std::function<float()> readFn,
                 std::function<void()> cleanupFn)
        : _initialize(std::move(initializeFn)),
          _read(std::move(readFn)),
          _cleanup(std::move(cleanupFn)) {}

    bool initialize() override { return _initialize(); }
    float read() override { return _read(); }
    void cleanup() override { _cleanup(); }

   private:
    std::function<bool()> _initialize;
    std::function<float()> _read;
    std::function<void()> _cleanup;
};

/// @brief Sends sensor readings periodically over the comms bus
class SensorDatastream {
   public:
    SensorDatastream();
    SensorDatastream(CommsDriver* driver, MCUID sender, uint32_t updateRateMs, uint8_t id,
                     std::shared_ptr<Sensor> sensor);

    /// Initialize sensor and timestamp
    void initialize();

    /// Called frequently; sends when the interval has elapsed
    void tick();

    void setStatus(bool enabled);

   private:
    CommsDriver* _driver;
    MCUID _sender;
    std::shared_ptr<Sensor> _sensorPtr;
    bool _enabled;
    uint32_t _updateRateMs;
    uint8_t _id;
    uint32_t _lastSendTime;
};

/// @brief Status decoded from a sensor message
struct SensorStatus {
    MCUID sender;
    uint8_t sensorID;
    float value;
};

}  // namespace comms

#endif  // __SENSOR_H__

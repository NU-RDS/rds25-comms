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
        /// @brief The raw representation of the payload
        /// This is a 64-bit value that contains the sensor value and its ID
        uint64_t raw;
        struct {
            /// @brief The sensor value and ID
            float value;       // 4 bytes
            uint8_t sensorID;  // 1 byte
        };
    };
};

/// @brief An abstract class for handling sensors
class Sensor {
   public:

    /// @brief Initializes the sensor
    /// @return True if initialization was successful, false otherwise
    virtual bool initialize() = 0;
    
    /// @brief Reads the sensor value
    /// @return The sensor value as a float
    virtual float read() = 0;

    /// @brief Cleans up the sensor
    /// @note This is called when the sensor is no longer needed, to free resources
    virtual void cleanup() = 0;

    virtual ~Sensor() = default;
};

/// @brief An implementation of Sensor that uses lambda functions
class LambdaSensor : public Sensor {
   public:

    /// @brief Constructs a LambdaSensor with the given lambda functions
    /// @param initializeFn The lambda function to call for initialization
    /// @param readFn The lambda function to call for reading the sensor value
    /// @param cleanupFn The lambda function to call for cleanup
    LambdaSensor(std::function<bool()> initializeFn, std::function<float()> readFn,
                 std::function<void()> cleanupFn)
        : _initialize(std::move(initializeFn)),
          _read(std::move(readFn)),
          _cleanup(std::move(cleanupFn)) {}


    /// @brief Initializes the sensor
    /// @note This will call the provided initialization function
    /// @return True if initialization was successful, false otherwise
    bool initialize() override { return _initialize(); }

    /// @brief Reads the sensor value
    /// @note This will call the provided read function
    float read() override { return _read(); }

    /// @brief Cleans up the sensor
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

    /// @brief Constructs a SensorDatastream with the given parameters
    /// @param driver The communication driver to use for sending messages
    /// @param sender The ID of the MCU sending the sensor data
    /// @param updateRateMs The rate at which to send sensor updates
    /// @param id The ID of the sensor
    /// @param sensor The sensor object to read data from
    SensorDatastream(CommsDriver* driver, MCUID sender, uint32_t updateRateMs, uint8_t id,
                     std::shared_ptr<Sensor> sensor);

    /// @brief Initializes the sensor datastream
    void initialize();

    /// @brief Ticks the sensor datastream, sending data if it's time to do so
    /// @note This will read the sensor value and send it over the communication bus
    void tick();

    /// @brief Sets the status of the sensor datastream
    /// @param enabled True to enable the datastream, false to disable it
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

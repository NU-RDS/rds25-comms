// comms.hpp
#ifndef __COMMS_H__
#define __COMMS_H__

#include "impl/can_comms_driver.hpp"
#include "impl/command.hpp"
#include "impl/comms_driver.hpp"
#include "impl/debug.hpp"
#include "impl/id.hpp"
#include "impl/option.hpp"
#include "impl/sensor.hpp"
#include "impl/heartbeat.hpp"
#include "impl/error.hpp"

namespace comms {

/// @brief A result of a tick operation, containing the raw message and its info
struct CommsTickResult {
    RawCommsMessage rawMessage;
    MessageInfo info;
};


/// @brief The central controller for managing communication between MCUs
class CommsController {
   public:
    /// @brief Constructs a CommsController with the given driver and ID
    /// @param driver The communication driver to use for sending and receiving messages
    /// @param id The ID of this MCU
    /// @note The ID should be unique across all MCUs in the system
    CommsController(CommsDriver& driver, MCUID id);

    /// @brief Initializes the communication controller
    /// @note This should be called once before using the controller
    /// @note It sets up the heartbeat manager, error manager, and command manager
    /// @note It also initializes all sensor datastreams
    void initialize();

    // high-level controls

    /// @brief Sends a command message with the given payload
    /// @param payload The command message payload to send
    /// @note This will send the command to the appropriate MCU based on the payload's mcuID
    void sendCommand(CommandMessagePayload payload);

    /// @brief Gets the value of a sensor from a specific sender
    /// @param sender The ID of the MCU that sent the sensor data
    /// @param sensorID The ID of the sensor to get the value for
    /// @return An Option containing the sensor value if available, or none if not
    /// @note This will return the most recent value received from the specified sensor
    Option<float> getSensorValue(MCUID sender, uint8_t sensorID);

    /// @brief Enables heartbeat request dispatching
    /// @param intervalMs The interval in milliseconds to send heartbeat requests
    /// @param toMonitor A vector of MCUIDs to monitor for heartbeats
    /// @note This will periodically send heartbeat requests to the specified MCUs
    void enableHeartbeatRequestDispatching(uint32_t intervalMs, const std::vector<MCUID> toMonitor);

    // low-level controls

    /// @brief Adds a sensor datastream to the controller
    /// @param updateRateMs The rate in milliseconds at which to update the sensor
    /// @param sensorID The ID of the sensor to add
    /// @param sensor The sensor object to add
    /// @note This will create a new SensorDatastream and start sending sensor data
    /// @note The sensorID should be unique for each sensor added on this MCU -- does not need to be unique across all MCUs
    void addSensor(uint32_t updateRateMs, uint8_t sensorID, std::shared_ptr<Sensor> sensor);

    // general controls

    /// @brief Reports an error with the given code, severity, and behavior
    /// @param error The error code to report
    /// @param severity The severity level of the error
    /// @param behavior The behavior to take in response to the error
    void reportError(ErrorCode error, ErrorSeverity severity, ErrorBehavior behavior);

    /// @brief Clears an error with the given code
    /// @param error The error code to clear
    /// @note This will remove the error from the error manager and notify any handlers
    /// @note If the error was latched, it will be unlatched
    /// @note If the error was non-latching, it will simply be cleared
    /// @note If the error was critical, it will not be cleared until the system is reset
    void clearError(ErrorCode error);
    

    /// @brief Ticks the communication controller, processing any incoming messages and updating state
    /// @return An Option containing the result of the tick operation, or none if no messages were processed
    Option<CommsTickResult> tick();

    /// @brief Returns the ID of this MCU
    /// @return The ID of this MCU
    MCUID me() const;

    /// @brief Sets a handler for unregistered messages
    /// @param handler A function that takes a RawCommsMessage and handles it
    /// @note This handler will be called for any messages that do not match a registered sensor or command
    void setUnregisteredMessageHandler(std::function<void(RawCommsMessage)> handler);

   private:
   
    /// @brief Updates the sensor datastreams, sending any new data
    void updateDatastreams();

    /// @brief Updates the heartbeat manager, sending heartbeats and checking for timeouts
    void updateHeartbeats();

    /// @brief The HAL driver used for sending and receiving messages
    CommsDriver& _driver;

    /// @brief A handler for unregistered messages
    /// @note This will be called for any messages that do not match a registered sensor or command
    std::function<void(RawCommsMessage)> _unregisteredMessageHandler;

    /// @brief Maps sensor IDs to their datastreams
    /// @note This allows for quick access to sensor data by ID
    std::unordered_map<uint8_t, SensorDatastream> _sensorDatastreams;

    /// @brief A vector of sensor statuses, containing the most recent values from each sensor
    /// @note This is used to provide quick access to the latest sensor values
    std::vector<SensorStatus> _sensorStatuses;

    /// @brief The heartbeat manager for handling heartbeats
    HeartbeatManager _heartbeatManager;

    /// @brief The error manager for handling errors
    ErrorManager _errorManager;

    /// @brief The command manager for handling commands
    CommandManager _commandManager;

    /// @brief The ID of this MCU
    MCUID _me;
};

}  // namespace comms

#endif  // __COMMS_H__

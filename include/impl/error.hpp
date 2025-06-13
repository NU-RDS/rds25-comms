#ifndef __ERROR_H__
#define __ERROR_H__

#include "comms_driver.hpp"
#include "id.hpp"
#include "option.hpp"
#include "result.hpp"

namespace comms {

/// @brief Descrives how severe the error is, for handling them differently
enum ErrorSeverity : uint8_t {
    ES_LOW,
    ES_MED,
    ES_CRIT,  // shutdown whole system
    ES_COUNT
};

/// @brief Describes if the error should be latching or not
enum ErrorBehavior : uint8_t {
    EB_NON_LATCHING,
    EB_LATCH,
    EB_COUNT,
};

/// @brief The code for the error
enum ErrorCode : uint8_t {
    EC_HEARTBEAT_ERR,
    EC_ODRIVE_COMM_ERR,
    EC_ENCODER_FAIL,
    EC_COMMAND_FAIL,
    EC_COUNT
};

/// @brief Represents an error in the system, including the severity, behavior and the code
class Error {
   public:
    ErrorSeverity severity;
    ErrorBehavior behavior;
    ErrorCode error;
};

/// @brief An 8 byte representation of the eror message payload
/// Used for communication with the comms driver
struct ErrorMessagePayload {
    union {
        uint64_t raw;
        struct {
            uint32_t errorNumber;
            Error error;
        };
    };
};

/// @brief A structure representing the status of an error managed by the ErrorManager
struct ManagedErrorStatus {
    Error error;
    uint32_t lastTransmissionTime;
};

/// @brief Handles errors within the system, responsible for sending errors, retransmission
/// and calling handlers
class ErrorManager {
   public:

    /// @brief Constructs an ErrorManager with the given driver and ID
    /// @param driver The communication driver to use for sending messages
    /// @param me The ID of this MCU
    /// @note The ErrorManager will use this driver to send error messages
    /// @note The ID should be unique across all MCUs in the system
    ErrorManager(CommsDriver* driver, MCUID me);

    /// @brief Initializes the error manager
    /// @param errorRetransitionTimeMs The time in milliseconds to wait before retransmitting an error
    void initialize(uint32_t errorRetransitionTimeMs = 100);

    /// @brief Ticks the error manager, checking for errors to send and retransmit
    /// @note This should be called periodically to ensure errors are sent and retransmitted
    void tick();

    /// @brief Adds an error handler for a specific severity level
    /// @param severity The severity level of the error to handle
    /// @param error A function that takes an Error and handles it
    /// @note This function will be called whenever an error of the specified severity is reported
    void addErrorHandler(ErrorSeverity severity, std::function<void(Error)> error);

    /// @brief Handles an error message received from the communication driver
    /// @param sender The sender of the error message
    /// @param payload The payload of the error message
    /// @note This will parse the error message and call the appropriate error handler
    void handleErrorRecieve(MessageInfo sender, RawCommsMessage payload);

    /// @brief Reports an error with the given code, severity, and behavior
    /// @param error The error code to report
    /// @param severity The severity level of the error
    /// @param behavior The behavior to take in response to the error
    /// @note This will send the error message over the communication bus
    void reportError(ErrorCode error, ErrorSeverity severity, ErrorBehavior behavior);

    /// @brief Clears an error with the given code
    /// @param error The error code to clear
    /// @note This will remove the error from the error manager and notify any handlers
    /// @note If the error was latched, it will be unlatched
    void clearError(ErrorCode error);

   private:
    /// @brief Counts the number of errors sent
    static uint32_t _errorCounter;

    /// @brief maps from error severity to a function for callback
    std::array<std::function<void(Error)>, ErrorSeverity::ES_COUNT> _errorHandlers;

    /// @brief The
    std::unordered_map<uint32_t, ManagedErrorStatus> _errorStatus;

    CommsDriver* _driver;
    MCUID _me;
    uint32_t _errorRetransmissionTimeMs;

};

}  // namespace comms

#endif  // __ERROR_H__
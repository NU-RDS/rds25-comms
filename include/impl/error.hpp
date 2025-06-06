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

struct ManagedErrorStatus {
    Error error;
    uint32_t lastTransmissionTime;
};

/// @brief Handles errors within the system, responsible for sending errors, retransmission
/// and calling handlers
class ErrorManager {
   public:
    ErrorManager(CommsDriver* driver, MCUID me);

    void initialize(uint32_t errorRetransitionTimeMs = 100);
    void tick();
    void addErrorHandler(ErrorSeverity severity, std::function<void(Error)> error);
    void handleErrorRecieve(MessageInfo sender, RawCommsMessage payload);

    void reportError(ErrorCode error, ErrorSeverity severity, ErrorBehavior behavior);
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
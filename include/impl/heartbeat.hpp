#ifndef __HEARTBEAT_H__
#define __HEARTBEAT_H__

#include "comms_driver.hpp"
#include "id.hpp"
#include "option.hpp"
#include "result.hpp"

namespace comms {

/// @brief A payload for a message representing heartbeat information.
/// @note This is sent from the MCU that manages the heartbeat, to the bus. A response is expected
struct HearbeatMessageRequestPayload {
    union {
        uint64_t raw;
        struct {
            MCUID id;
        };
    };
};

/// @brief A payload for a message representing heartbeat information
/// @note This is sent from the MCU that receives the heartbeat request, to the bus
/// @note The heartbeat value is a 64-bit unsigned integer that represents the heartbeat count
struct HeartbeatMessageResponsePayload {
    union {
        uint64_t raw;
        struct {
            uint64_t heartbeatValue;
        };
    };
};

/// @brief A structure representing the status of a heartbeat request
/// @note This contains the expected and actual heartbeat counts, as well as the last request and response times
struct HeartbeatRequestStatus {
    uint64_t expectedHeartbeatCount;
    uint64_t actualHeartbeatCount;
    uint32_t lastRequest;
    uint32_t lastResponse;
    MCUID id;
};

/// @brief A structure representing the status of a heartbeat response
/// @note This contains the heartbeat count for the MCU that sent the response
struct HeartbeatResponseStatus {
    uint64_t heartbeatCount;
};

/// @brief A structure representing the payload of a heartbeat request message
class HeartbeatManager {
   public:

    /// @brief Constructs a HeartbeatManager with the given driver and ID
    /// @param driver The communication driver to use for sending messages
    /// @param me The ID of this MCU
    HeartbeatManager(CommsDriver* driver, MCUID me);

    /// @brief Initializes the heartbeat manager
    /// @param intervalTimeMs The interval time in milliseconds for sending heartbeat messages
    /// @param nodesToCheck The list of nodes to check for heartbeat responses
    void initialize(uint32_t intervalTimeMs, const std::vector<MCUID> nodesToCheck);

    /// @brief Ticks the heartbeat manager, updating the status of heartbeat requests
    /// @return True if a heartbeat request was sent, false otherwise
    bool tick();

    /// @brief Updates the heartbeat status for a given MCU ID
    /// @param id The ID of the MCU to update
    void updateHeartbeatStatus(MCUID id);

    /// @brief Sends a heartbeat request to the specified destination
    /// @note This will send a heartbeat request message to the destination MCU
    /// @param destination The ID of the destination MCU
    void sendHeartbeatRequest(MCUID destination);

    /// @brief Sends a heartbeat response to the requesting MCU
    /// @note This will send a heartbeat response message to the requesting MCU
    void sendHeartbeatResponse();

   private:
    std::unordered_map<MCUID, HeartbeatRequestStatus> _requestStatuses;
    HeartbeatResponseStatus _myStatus;
    CommsDriver* _driver;
    MCUID _me;

    uint32_t _intervalTimeMs;
    uint32_t _lastDispatch;

    std::vector<MCUID> _nodesToCheck;
    std::vector<MCUID> _badNodes;
};

}  // namespace comms

#endif  // __HEARTBEAT_H__
#ifndef __HEARTBEAT_H__
#define __HEARTBEAT_H__

#include "comms_driver.hpp"
#include "id.hpp"
#include "option.hpp"
#include "result.hpp"

namespace comms {

struct HearbeatMessageRequestPayload {
    union {
        uint64_t raw;
        struct {
            MCUID id;
        };
    };
};

/// @brief A payload for a message representing heartbeat information
struct HeartbeatMessageResponsePayload {
    union {
        uint64_t raw;
        struct {
            uint64_t heartbeatValue;
        };
    };
};

struct HeartbeatRequestStatus {
    uint64_t expectedHeartbeatCount;
    uint64_t actualHeartbeatCount;
    uint32_t lastRequest;
    uint32_t lastResponse;
    MCUID id;
};

struct HeartbeatResponseStatus {
    uint64_t heartbeatCount;
};

class HeartbeatManager {
   public:
    HeartbeatManager(CommsDriver* driver, MCUID me);

    void initialize(uint32_t intervalTimeMs, const std::vector<MCUID> nodesToCheck);

    bool tick();
    
    void updateHeartbeatStatus(MCUID id);
    void sendHeartbeatRequest(MCUID destination);
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
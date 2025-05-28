#ifndef __HEARTBEAT_H__
#define __HEARTBEAT_H__

#include "comms_driver.hpp"
#include "id.hpp"
#include "option.hpp"
#include "result.hpp"

namespace comms {

/// @brief A payload for a message representing heartbeat information
struct HeartbeatMessagePayload {
    union {
        uint64_t raw;
        struct {
            uint64_t heartbeatValue;
        };
    };
};

class HeartbeatManager {};

}  // namespace comms

#endif  // __HEARTBEAT_H__
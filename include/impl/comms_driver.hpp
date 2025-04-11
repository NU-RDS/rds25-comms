#ifndef __COMMS_DRIVER_H__
#define __COMMS_DRIVER_H__

#include "stdint.h"
#include <functional>

struct RawCommsMessage {
    uint32_t id;
    uint8_t length;
    union {
        uint64_t payload;
        uint8_t payloadBytes[8]
    };
};

class CommsDriver {
   public:

    virtual void install() { /* no-op */ }
    virtual void uninstall() { /* no-op */ }
    virtual void sendMessage(RawCommsMessage &message) { /* no-op */ }

    // message handling for interrupt-based drivers
    virtual void attachCallback(uint32_t id, std::function<void(const RawCommsMessage&)> callback) {
        /* no-op */
    }

    bool receiveMessage(RawCommsMessage *res) { return false; }
};

#endif  // __COMMS_DRIVER_H__
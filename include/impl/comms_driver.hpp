#ifndef __COMMS_DRIVER_H__
#define __COMMS_DRIVER_H__

#include <functional>

#include "stdint.h"

namespace comms {

/// @brief The structure of a raw communication message, mostly with CAN in mind
struct RawCommsMessage {
    uint32_t id;
    uint8_t length;
    union {
        uint64_t payload;
        uint8_t payloadBytes[8];
    };
};

/// @brief HAL for Sending/Recieving these Comms messages
class CommsDriver {
   public:
    virtual void install() = 0;
    virtual void uninstall() = 0;
    virtual void sendMessage(const RawCommsMessage& message) = 0;

    // message handling for interrupt-based drivers
    void attachRXCallback(uint32_t id, std::function<void(const RawCommsMessage&)> callback) {
        if (_callbackTable.find(id) == _callbackTable.end()) {
            // a vector doesn't exist yet
            _callbackTable[id] = std::vector<std::function<void(const RawCommsMessage&)>>();
        }
        _callbackTable[id].push_back(callback);
    }

    bool receiveMessage(RawCommsMessage* res) { return false; }

   private:
    std::unordered_map<uint32_t, std::vector<std::function<void(const RawCommsMessage&)>>>
        _callbackTable;
};

}  // namespace comms

#endif  // __COMMS_DRIVER_H__
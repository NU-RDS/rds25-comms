#ifndef __COMMS_DRIVER_H__
#define __COMMS_DRIVER_H__

#include <functional>

#include "stdint.h"

struct RawCommsMessage {
    uint32_t id;
    uint8_t length;
    union {
        uint64_t payload;
        uint8_t payloadBytes[8];
    };
};

class CommsDriver {
   public:
    virtual void install() { /* no-op */ }
    virtual void uninstall() { /* no-op */ }
    virtual void sendMessage(RawCommsMessage &message) { /* no-op */ }

    // message handling for interrupt-based drivers
    void attachRXCallback(uint32_t id, std::function<void(const RawCommsMessage &)> callback) {
        if (_callbackTable.find(id) == _callbackTable.end()) {
            // a vector doesn't exist yet
            _callbackTable[id] = std::vector<std::function<void(const RawCommsMessage &)>>();
        }
        _callbackTable[id].push_back(callback);
    }

    bool receiveMessage(RawCommsMessage *res) { return false; }

   private:
    std::unordered_map<uint32_t, std::vector<std::function<void(const RawCommsMessage &)>>> _callbackTable;
};

#endif  // __COMMS_DRIVER_H__
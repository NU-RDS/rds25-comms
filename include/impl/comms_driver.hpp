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
    /// @brief Initializes the communication driver
    /// @note This should be called once before using the driver
    virtual void install() = 0;

    /// @brief Uninstalls the communication driver
    /// @note This should be called when the driver is no longer needed
    virtual void uninstall() = 0;

    /// @brief Sends a raw communication message
    /// @param message The raw communication message to send
    /// @note This will send the message over the communication bus
    virtual void sendMessage(const RawCommsMessage& message) = 0;

    /// @brief Receives a raw communication message
    /// @param res The received raw communication message
    /// @return True if a message was received, false if no message was available
    virtual bool receiveMessage(RawCommsMessage* res) = 0;

    /// @brief Attaches a callback for receiving messages with a specific ID
    /// @param id The ID of the messages to listen for
    void attachRXCallback(uint32_t id, std::function<void(const RawCommsMessage&)> callback) {
        if (_callbackTable.find(id) == _callbackTable.end()) {
            // a vector doesn't exist yet
            _callbackTable[id] = std::vector<std::function<void(const RawCommsMessage&)>>();
        }
        _callbackTable[id].push_back(callback);
    }

   private:
    std::unordered_map<uint32_t, std::vector<std::function<void(const RawCommsMessage&)>>>
        _callbackTable;
};

}  // namespace comms

#endif  // __COMMS_DRIVER_H__
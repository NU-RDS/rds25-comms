#include "impl/heartbeat.hpp"

#include <Arduino.h>

#include "impl/debug.hpp"

namespace comms {

HeartbeatManager::HeartbeatManager(CommsDriver* driver, MCUID me) : _driver(driver), _me(me) {}

bool HeartbeatManager::tick() {
    uint32_t currentTime = millis();
    _badNodes.clear();

    for (auto statusPair : _requestStatuses) {
        HeartbeatRequestStatus status = statusPair.second;

        if (currentTime - status.lastRequest > 5000) {
            // too long of a time has passed
            COMMS_DEBUG_PRINT_ERRORLN(
                "Too much time has elapsed between heartbeat request and last response for node %d",
                status.id);
            COMMS_DEBUG_PRINTLN("Resending request...");

            Option<uint32_t> idOpt =
                MessageInfo::getMessageID(_me, MessageContentType::MT_HEARTBEAT);
            if (idOpt.isNone()) {
                COMMS_DEBUG_PRINT_ERRORLN("Cannot send a heartbeat response! No ID available!");
                continue;
            }

            // send the message
            HearbeatMessageRequestPayload payload;
            payload.id = status.id;

            RawCommsMessage message;
            message.id = idOpt.value();
            message.payload = payload.raw;

            _driver->sendMessage(message);

            _badNodes.push_back(status.id);
            continue;
        }

        // the expected should always be equal to, or one greater than the actual heartbeat count
        if (status.expectedHeartbeatCount == status.actualHeartbeatCount ||
            status.expectedHeartbeatCount == status.actualHeartbeatCount + 1)
            continue;

        COMMS_DEBUG_PRINT_ERRORLN("Hearbeat mismatch on node %d. Expected %d, got %d", status.id,
                                  status.expectedHeartbeatCount, status.actualHeartbeatCount);
    }
}

void HeartbeatManager::sendHeartbeatRequest(MCUID destination) {
    if (_me != MCUID::MCU_HIGH_LEVEL) {
        COMMS_DEBUG_PRINT_ERRORLN("Cannot send a heartbeat request! Not the high level teensy!");
        return;
    }

    Option<uint32_t> idOpt = MessageInfo::getMessageID(_me, MessageContentType::MT_HEARTBEAT);
    if (idOpt.isNone()) {
        COMMS_DEBUG_PRINT_ERRORLN("Cannot send a heartbeat response! No ID available!");
        return;
    }

    // send the message
    HearbeatMessageRequestPayload payload;
    payload.id = destination;

    RawCommsMessage message;
    message.id = idOpt.value();
    message.payload = payload.raw;

    _driver->sendMessage(message);

    // update the hearbeat requests statues
    HeartbeatRequestStatus status = {0};
    if (_requestStatuses.find(destination) == _requestStatuses.end()) {
        status = _requestStatuses[destination];
    }

    status.id = destination;
    status.expectedHeartbeatCount++;
    status.lastRequest = millis();

    _requestStatuses[destination] = status;
}

void HeartbeatManager::sendHeartbeatResponse() {
    if (_me == MCUID::MCU_HIGH_LEVEL) {
        COMMS_DEBUG_PRINT_ERRORLN("Cannot send a heartbeat response! Am the high level teensy!");
        return;
    }

    _myStatus.heartbeatCount++;
    Option<uint32_t> idOpt = MessageInfo::getMessageID(_me, MessageContentType::MT_HEARTBEAT);
    if (idOpt.isNone()) {
        COMMS_DEBUG_PRINT_ERRORLN("Cannot send a heartbeat request! No ID available!");
        return;
    }

    HeartbeatMessageResponsePayload payload;
    payload.heartbeatValue = _myStatus.heartbeatCount;

    RawCommsMessage message;
    message.id = idOpt.value();
    message.payload = payload.raw;

    _driver->sendMessage(message);
};

}  // namespace comms
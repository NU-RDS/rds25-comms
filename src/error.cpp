#include "impl/error.hpp"
#include "impl/id.hpp"

#include <Arduino.h>  // for millis()

#include <array>
#include <cstring>
#include <functional>
#include <unordered_map>

namespace comms {

uint32_t ErrorManager::_errorCounter = 0;

ErrorManager::ErrorManager(CommsDriver* driver, MCUID me)
    : _driver(driver), _me(me), _errorRetransmissionTimeMs(0) {
}

void ErrorManager::initialize(uint32_t errorRetransmissionTimeMs) {
    _errorRetransmissionTimeMs = errorRetransmissionTimeMs;
}

void ErrorManager::tick() {
    uint32_t now = millis();

    // Iterate over every outstanding error that we have in the map.
    for (auto& kv : _errorStatus) {
        uint32_t errorNumber = kv.first;
        ManagedErrorStatus& status = kv.second;

        // If enough time has passed, retransmit
        if (now - status.lastTransmissionTime >= _errorRetransmissionTimeMs) {
            // Re‐build the raw CAN message with the same payload
            ErrorMessagePayload wrapper;
            wrapper.error = status.error;       // same severity/behavior/code
            wrapper.errorNumber = errorNumber;  // same unique number

            // Look up our own MT_ERROR ID to send on
            Option<uint32_t> idOpt = MessageInfo::getMessageID(_me, MessageContentType::MT_ERROR);
            if (idOpt.isSome()) {
                RawCommsMessage raw;
                raw.id = idOpt.value();
                raw.payload = wrapper.raw;
                _driver->sendMessage(raw);

                // Stamp the time so we don’t retransmit too often
                status.lastTransmissionTime = now;
            }
        }
    }
}

void ErrorManager::addErrorHandler(ErrorSeverity severity, std::function<void(Error)> handler) {
    if (severity < ES_COUNT) {
        _errorHandlers[static_cast<size_t>(severity)] = handler;
    }
}

void ErrorManager::handleErrorRecieve(MessageInfo sender, ErrorMessagePayload payload) {
    // If a handler was registered for this severity, call it.
    ErrorSeverity sev = payload.error.severity;
    if (sev < ES_COUNT) {
        auto& maybeHandler = _errorHandlers[static_cast<size_t>(sev)];
        if (maybeHandler) {
            maybeHandler(payload.error);
        }
    }

    if (payload.error.behavior == EB_LATCH) {
        ManagedErrorStatus& status = _errorStatus[payload.errorNumber];
        status.error = payload.error;
        status.lastTransmissionTime = millis();  // store the time we first saw it
    }
}

void ErrorManager::reportError(ErrorCode code, ErrorSeverity severity, ErrorBehavior behavior) {
    // Create a fresh errorNumber
    uint32_t newNumber = _errorCounter++;

    // Populate the struct
    ErrorMessagePayload wrapper;
    wrapper.errorNumber = newNumber;
    wrapper.error.severity = severity;
    wrapper.error.behavior = behavior;
    wrapper.error.error = code;

    // Store it so tick() will retransmit later
    ManagedErrorStatus status;
    status.error = wrapper.error;
    status.lastTransmissionTime = millis();
    _errorStatus[newNumber] = status;

    // Immediately send out the first copy
    Option<uint32_t> idOpt = MessageInfo::getMessageID(_me, MessageContentType::MT_ERROR);
    if (idOpt.isSome()) {
        RawCommsMessage raw;
        raw.id = idOpt.value();
        raw.payload = wrapper.raw;
        _driver->sendMessage(raw);
    }
}

void ErrorManager::clearError(ErrorCode code) {
    // Collect all errorNumbers that match code, then erase them
    std::vector<uint32_t> toErase;
    for (auto& kv : _errorStatus) {
        uint32_t errNum = kv.first;
        ManagedErrorStatus& s = kv.second;
        if (s.error.error == code) {
            toErase.push_back(errNum);
        }
    }

    for (uint32_t num : toErase) {
        _errorStatus.erase(num);
    }
}

}  // namespace comms

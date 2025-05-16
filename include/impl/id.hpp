#ifndef __ID_H__
#define __ID_H__

#include <stdint.h>

#include <map>

#include "option.hpp"

namespace comms {

enum MCUID : uint8_t {
    MCU_HIGH_LEVEL,
    MCU_LOW_LEVEL_0,
    MCU_LOW_LEVEL_1,
    MCU_LOW_LEVEL_2,
    MCU_LOW_LEVEL_3,
    MCU_SENSOR,
    MCU_INVALID,
    MCU_LOW_LEVEL_ANY,
    MCU_ANY,
};

enum MessageIDs : uint32_t {
    MID_ERROR_GLOBAL = 0x000,
    MID_ERROR_LL0 = 0x010,
    MID_ERROR_LL1 = 0x020,
    MID_ERROR_LL2 = 0x030,
    MID_HEARTBEAT_REQ = 0x10A,
    MID_HEARTBEAT_RESP0 = 0x110,
    MID_HEARTBEAT_RESP1 = 0x120,
    MID_HEARTBEAT_RESP2 = 0x130,
    MID_COMMAND_HL = 0x200,
    MID_COMMAND_RESP0 = 0x310,
    MID_COMMAND_RESP1 = 0x320,
    MID_COMMAND_RESP2 = 0x330,
};

enum MessageContentType : uint8_t {
    MT_ERROR,
    MT_HEARTBEAT,
    MT_COMMAND
};

struct MessageInfo {
    MCUID sender;
    MCUID target;
    MessageContentType type;

    static const Option<MessageInfo> getInfo(uint32_t id);
    static const Option<uint32_t> getMessageID(MCUID sender, MessageContentType type);

    bool shouldListen(MCUID me);
};

inline const std::map<uint32_t, MessageInfo> __infoLUT{
    {MID_ERROR_GLOBAL, {MCU_HIGH_LEVEL, MCU_ANY, MT_ERROR}},
    {MID_ERROR_LL0, {MCU_LOW_LEVEL_0, MCU_ANY, MT_ERROR}},
    {MID_ERROR_LL1, {MCU_LOW_LEVEL_1, MCU_ANY, MT_ERROR}},
    {MID_ERROR_LL2, {MCU_LOW_LEVEL_2, MCU_ANY, MT_ERROR}},
    {MID_HEARTBEAT_REQ, {MCU_HIGH_LEVEL, MCU_LOW_LEVEL_ANY, MT_HEARTBEAT}},
    {MID_HEARTBEAT_RESP0, {MCU_LOW_LEVEL_0, MCU_HIGH_LEVEL, MT_HEARTBEAT}},
    {MID_HEARTBEAT_RESP1, {MCU_LOW_LEVEL_1, MCU_HIGH_LEVEL, MT_HEARTBEAT}},
    {MID_HEARTBEAT_RESP2, {MCU_LOW_LEVEL_2, MCU_HIGH_LEVEL, MT_HEARTBEAT}},
    {MID_COMMAND_HL, {MCU_HIGH_LEVEL, MCU_LOW_LEVEL_ANY, MT_COMMAND}},
    {MID_COMMAND_RESP0, {MCU_LOW_LEVEL_0, MCU_HIGH_LEVEL, MT_COMMAND}},
    {MID_COMMAND_RESP1, {MCU_LOW_LEVEL_1, MCU_HIGH_LEVEL, MT_COMMAND}},
    {MID_COMMAND_RESP2, {MCU_LOW_LEVEL_2, MCU_HIGH_LEVEL, MT_COMMAND}},
};

inline const Option<MessageInfo> MessageInfo::getInfo(uint32_t id) {
    if (__infoLUT.find(id) == __infoLUT.end()) {
        // no id
        return Option<MessageInfo>::none();
    }

    return Option<MessageInfo>::some(__infoLUT.at(id));
}

inline const Option<uint32_t> MessageInfo::getMessageID(MCUID sender, MessageContentType type) {
    for (auto const& kv : __infoLUT) {
        if (kv.second.sender == sender && kv.second.type == type) {
            return Option<uint32_t>::some(kv.first);
        }
    }

    return Option<uint32_t>::none();
}

bool MessageInfo::shouldListen(MCUID me) {
    if (target == MCU_ANY) return true;
    if (target == me) return true;

    switch (me) {
        case MCU_LOW_LEVEL_0:
        case MCU_LOW_LEVEL_1:
        case MCU_LOW_LEVEL_2:
        case MCU_LOW_LEVEL_3:
            return target == MCU_LOW_LEVEL_ANY;
        default:
            return false;
    }
}

}  // namespace comms

#endif  // __ID_H__
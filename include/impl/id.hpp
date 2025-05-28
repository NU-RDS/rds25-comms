// id.hpp
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
    MCU_PALM,
    MCU_LOW_LEVEL_ANY,
    MCU_ANY,
};

enum MessageIDs : uint32_t {
    MID_ERROR_GLOBAL = 0x000,
    MID_ERROR_LL0 = 0x010,
    MID_ERROR_LL1 = 0x020,
    MID_ERROR_LL2 = 0x030,
    MID_ERROR_LL3 = 0x030,
    MID_ERROR_PALM = 0x040,
    MID_HEARTBEAT_REQ = 0x10A,
    MID_HEARTBEAT_RESP_LL0 = 0x100,
    MID_HEARTBEAT_RESP_LL1 = 0x110,
    MID_HEARTBEAT_RESP_LL2 = 0x120,
    MID_HEARTBEAT_RESP_LL3 = 0x130,
    MID_COMMAND_HL = 0x200,
    MID_COMMAND_RESP_LL0 = 0x300,
    MID_COMMAND_RESP_LL1 = 0x310,
    MID_COMMAND_RESP_LL2 = 0x320,
    MID_COMMAND_RESP_LL3 = 0x330,
    MID_COMMAND_RESP_PALM = 0x340,
    MID_SENSOR_DATA_LL0 = 0x400,
    MID_SENSOR_DATA_LL1 = 0x410,
    MID_SENSOR_DATA_LL2 = 0x420,
    MID_SENSOR_DATA_LL3 = 0x430,
    MID_SENSOR_DATA_PALM = 0x440,
};

enum MessageContentType : uint8_t { MT_ERROR, MT_HEARTBEAT, MT_COMMAND, MT_SENSOR_DATA };

struct MessageInfo {
    MCUID sender;
    MCUID target;
    MessageContentType type;

    static const Option<MessageInfo> getInfo(uint32_t id);
    static const Option<uint32_t> getMessageID(MCUID sender, MessageContentType type);

    bool shouldListen(MCUID me) const {
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
};

inline const std::map<uint32_t, MessageInfo> __infoLUT = {
    // Errors — any target
    {MID_ERROR_GLOBAL, {MCU_HIGH_LEVEL, MCU_ANY, MT_ERROR}},
    {MID_ERROR_LL0, {MCU_LOW_LEVEL_0, MCU_ANY, MT_ERROR}},
    {MID_ERROR_LL1, {MCU_LOW_LEVEL_1, MCU_ANY, MT_ERROR}},
    {MID_ERROR_LL2, {MCU_LOW_LEVEL_2, MCU_ANY, MT_ERROR}},
    {MID_ERROR_LL3, {MCU_LOW_LEVEL_3, MCU_ANY, MT_ERROR}},
    {MID_ERROR_PALM, {MCU_PALM, MCU_ANY, MT_ERROR}},

    // Heartbeats
    {MID_HEARTBEAT_REQ, {MCU_HIGH_LEVEL, MCU_LOW_LEVEL_ANY, MT_HEARTBEAT}},
    {MID_HEARTBEAT_RESP_LL0, {MCU_LOW_LEVEL_0, MCU_HIGH_LEVEL, MT_HEARTBEAT}},
    {MID_HEARTBEAT_RESP_LL1, {MCU_LOW_LEVEL_1, MCU_HIGH_LEVEL, MT_HEARTBEAT}},
    {MID_HEARTBEAT_RESP_LL2, {MCU_LOW_LEVEL_2, MCU_HIGH_LEVEL, MT_HEARTBEAT}},
    {MID_HEARTBEAT_RESP_LL3, {MCU_LOW_LEVEL_3, MCU_HIGH_LEVEL, MT_HEARTBEAT}},

    // Commands
    {MID_COMMAND_HL, {MCU_HIGH_LEVEL, MCU_LOW_LEVEL_ANY, MT_COMMAND}},
    {MID_COMMAND_RESP_LL0, {MCU_LOW_LEVEL_0, MCU_HIGH_LEVEL, MT_COMMAND}},
    {MID_COMMAND_RESP_LL1, {MCU_LOW_LEVEL_1, MCU_HIGH_LEVEL, MT_COMMAND}},
    {MID_COMMAND_RESP_LL2, {MCU_LOW_LEVEL_2, MCU_HIGH_LEVEL, MT_COMMAND}},
    {MID_COMMAND_RESP_LL3, {MCU_LOW_LEVEL_3, MCU_HIGH_LEVEL, MT_COMMAND}},
    {MID_COMMAND_RESP_PALM, {MCU_PALM, MCU_HIGH_LEVEL, MT_COMMAND}},

    // Sensor data
    {MID_SENSOR_DATA_LL0, {MCU_LOW_LEVEL_0, MCU_HIGH_LEVEL, MT_SENSOR_DATA}},
    {MID_SENSOR_DATA_LL1, {MCU_LOW_LEVEL_1, MCU_HIGH_LEVEL, MT_SENSOR_DATA}},
    {MID_SENSOR_DATA_LL2, {MCU_LOW_LEVEL_2, MCU_HIGH_LEVEL, MT_SENSOR_DATA}},
    {MID_SENSOR_DATA_LL3, {MCU_LOW_LEVEL_3, MCU_HIGH_LEVEL, MT_SENSOR_DATA}},
    {MID_SENSOR_DATA_PALM, {MCU_PALM, MCU_HIGH_LEVEL, MT_SENSOR_DATA}},
};

inline const Option<MessageInfo> MessageInfo::getInfo(uint32_t id) {
    auto it = __infoLUT.find(id);
    if (it == __infoLUT.end()) return Option<MessageInfo>::none();
    return Option<MessageInfo>::some(it->second);
}

inline const Option<uint32_t> MessageInfo::getMessageID(MCUID sender, MessageContentType type) {
    for (auto const& kv : __infoLUT) {
        if (kv.second.sender == sender && kv.second.type == type) {
            return Option<uint32_t>::some(kv.first);
        }
    }
    return Option<uint32_t>::none();
}

}  // namespace comms

#endif  // __ID_H__

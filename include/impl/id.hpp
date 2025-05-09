#ifndef __ID_H__
#define __ID_H__

#include <stdint.h>

#include <map>

namespace comms {

enum MCUID : uint8_t {
    MCU_HIGH_LEVEL,
    MCU_LOW_LEVEL_0,
    MCU_LOW_LEVEL_1,
    MCU_LOW_LEVEL_2,
    MCU_LOW_LEVEL_3,
    MCU_SENSOR,
    MCU_INVALID,
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

struct SenderInformation {
    MCUID mcu;
    MessageContentType type;

    static const SenderInformation getInfo(uint32_t id);
};

inline const std::map<uint32_t, SenderInformation> senderLUT{
    {MID_ERROR_GLOBAL, {MCU_HIGH_LEVEL, MT_ERROR}},
    {MID_ERROR_LL0, {MCU_LOW_LEVEL_0, MT_ERROR}},
    {MID_ERROR_LL1, {MCU_LOW_LEVEL_1, MT_ERROR}},
    {MID_ERROR_LL2, {MCU_LOW_LEVEL_2, MT_ERROR}},
    {MID_HEARTBEAT_REQ, {MCU_HIGH_LEVEL, MT_HEARTBEAT}},
    {MID_HEARTBEAT_RESP0, {MCU_LOW_LEVEL_0, MT_HEARTBEAT}},
    {MID_HEARTBEAT_RESP1, {MCU_LOW_LEVEL_1, MT_HEARTBEAT}},
    {MID_HEARTBEAT_RESP2, {MCU_LOW_LEVEL_2, MT_HEARTBEAT}},
    {MID_COMMAND_HL, {MCU_HIGH_LEVEL, MT_COMMAND}},
    {MID_COMMAND_RESP0, {MCU_LOW_LEVEL_0, MT_COMMAND}},
    {MID_COMMAND_RESP1, {MCU_LOW_LEVEL_1, MT_COMMAND}},
    {MID_COMMAND_RESP2, {MCU_LOW_LEVEL_2, MT_COMMAND}},
};

inline const SenderInformation SenderInformation::getInfo(uint32_t id) {
    return senderLUT.at(id);  
}

}  // namespace comms

#endif  // __ID_H__
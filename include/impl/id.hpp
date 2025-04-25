#ifndef __ID_H__
#define __ID_H__

#include <stdint.h>

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

}  // namespace comms

#endif  // __ID_H__
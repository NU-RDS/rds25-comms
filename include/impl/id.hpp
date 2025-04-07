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
    MCU_COUNT
};

}

#endif  // __ID_H__
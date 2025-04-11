#ifndef __COMMAND_H__
#define __COMMAND_H__

/**========================================================================
 *                             command.hpp
 *
 *  (Reference the electrical architecture for clarification)
 *  Commands are issued from the High Level MCU to the Lower Level/Sensor MCUs
 *
 *
 *
 *========================================================================**/

#include <stdint.h>

#include "id.hpp"
#include "command.hpp"

namespace comms {

enum CommandType : uint8_t {
    // General Commands, for any MCU
    CMD_BEGIN,  // begin the operation of the device
    CMD_STOP,   // end the operation of the device

    // Motor-Driver Specific Commands
    CMD_MOTOR_CONTROL,

    // Sensor-Board Specific Commands
    CMD_SENSOR_TOGGLE,
};

struct RawCommand {
    union {
        uint64_t raw;
        struct inner {
            // first two bytes
            CommandType type;
            MCUID mcuID;
            uint16_t commandId;
            // last 4 bytes
            uint8_t payload[4];
        }
    }
};



#endif  // __COMMAND_H__
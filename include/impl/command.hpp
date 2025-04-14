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

#include "command.hpp"
#include "id.hpp"

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

struct CommandMessagePayload {
    union {
        uint64_t raw;
        struct {
            // meta information (4 bytes)
            CommandType type;
            MCUID mcuID;
            uint16_t commandId;
            // last 4 bytes for command-specific things
            uint32_t payload;
        };
    };

    CommandMessagePayload(CommandType cType, MCUID mid, uint16_t cid, uint32_t data) : type(cType), mcuID(mid), commandId(cid), payload(data) {}
};

enum MotorControlCommandType : uint8_t {
    MC_CMD_POS,
    MC_CMD_VEL
};

struct MotorControlCommand {
    MCUID id;
    MotorControlCommandType type;
    uint32_t payload;

    static MotorControlCommand position(MCUID id, float position) {
        uint32_t payload = 0;
        memcpy(&payload, &position, sizeof(float));
        return MotorControlCommand(id, MC_CMD_POS, payload);
    }

    static MotorControlCommand velocity(MCUID id, float speed) {
        uint32_t payload = 0;
        memcpy(&payload, &speed, sizeof(float));
        return MotorControlCommand(id, MC_CMD_POS, payload);
    }

    RawCommsMessage toRaw() const {
        CommandMessagePayload payloadCmd(CMD_MOTOR_CONTROL, id, payload);
        return (RawCommsMessage) {
            .id = MessageIDs::MID_COMMAND_HL,
            .length = 8,
            .payload = payloadCmd.raw
        };
    }

   private:
    MotorControlCommand(MCUID id, MotorControlCommandType type, uint32_t payload) : id(id), type(type), payload(payload) {}
};

}  // namespace comms

#endif  // __COMMAND_H__
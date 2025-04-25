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

#include <cstring>
#include <functional>
#include <memory>

#include "comms_driver.hpp"
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
    CMD_INVALID,
    CMD_COUNT
};

struct CommandMessagePayload {
    union {
        uint64_t raw;
        struct {
            // meta information (4 bytes)
            CommandType type;
            MCUID mcuID;
            uint16_t commandID;
            // last 4 bytes for command-specific things
            uint32_t payload;
        };
    };

    CommandMessagePayload() : type(CommandType::CMD_INVALID), mcuID(MCUID::MCU_INVALID), commandID(0), payload(0) {}
    CommandMessagePayload(CommandType cType, MCUID mid, uint16_t cid, uint32_t data) : type(cType), mcuID(mid), commandID(cid), payload(data) {}
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

    // static MotorControlCommand fromRaw(const RawCommsMessage &raw) {
    //     CommandMessagePayload payload;
    //     memcpy(&payload.raw, &raw.payload, sizeof(RawCommsMessage));
    //     return MotorControlCommand(payload.mcuID, payload., payload.payload);
    // }

    // RawCommsMessage toRaw() const {
    //     CommandMessagePayload payloadCmd(id, CMD_MOTOR_CONTROL, payload);
    //     return (RawCommsMessage){
    //         .id = MessageIDs::MID_COMMAND_HL,
    //         .length = 8,
    //         .payload = payloadCmd.raw};
    // }

   private:
    MotorControlCommand(MCUID id, MotorControlCommandType type, uint32_t payload) : id(id), type(type), payload(payload) {}
};

class CommandHandler {
   public:
    virtual void start(const CommandMessagePayload &payload) {}
    virtual void update(const CommandMessagePayload &payload) {}
    virtual void end(const CommandMessagePayload &payload) {}
    virtual bool isParallelizable(const std::vector<CommandMessagePayload> slice);
};

/// @brief A buffer that manages user commands.
/// This class collects user commands and executes them in slices.
class CommandBuffer {
   public:
    struct ExecutionStats {
        uint32_t time;     ///< Execution time.
        uint8_t executed;  ///< Number of executed commands.
        bool success;      ///< Whether the execution
    };

    /// @brief Constructor.
    CommandBuffer();

    /// @brief Adds a command to the buffer.
    /// @param command Shared pointer to a UserCommand.
    void addCommand(CommandMessagePayload command);

    /// @brief Clears the command buffer.
    void clear();

    /// @brief Resets all commands in the buffer.
    void reset();

    /// @brief Continues executing the current slice of commands.
    void tick();

    /// @brief Executes the current slice of commands.
    void startExecution();

    /// @brief Registers a callback to be called when execution is complete.
    /// @param callback The callback to register.
    void onExecutionComplete(std::function<void(ExecutionStats)> callback);

    void setHandler(CommandType type, std::shared_ptr<CommandHandler> handler);

   private:
    /// @brief Nested class representing a slice of commands.
    class CommandSlice {
       public:
        /// @brief Constructs a command slice.
        /// @param start Starting index.
        /// @param end Ending index.
        CommandSlice(std::size_t start, std::size_t end);

        /// @brief Gets the starting index.
        /// @return The start index.
        std::size_t start() const;

        /// @brief Gets the ending index.
        /// @return The end index.
        std::size_t end() const;

        /// @brief Gets the number of commands in the slice.
        /// @return The slice size.
        std::size_t size() const;

        /// @brief Returns an empty command slice.
        /// @return An empty CommandSlice.
        static CommandSlice empty();

        /// @brief Checks if the given slice is empty.
        /// @param slice The CommandSlice to check.
        /// @return true if the slice is empty.
        static bool isEmpty(const CommandSlice &slice);

       private:
        std::size_t _start;  ///< Starting index.
        std::size_t _end;    ///< Ending index.
    };

    std::vector<CommandMessagePayload> _commands;  ///< All commands.
    CommandSlice _currentSlice;                    ///< The current command slice.
    size_t _numCompletedCommands;                  ///< Number of completed commands.
    bool _isExecuting;                             ///< Whether the buffer is executing commands.
    uint32_t _startTime;                           ///< Time when execution
    bool _isCalibrating;                           ///< Whether the buffer is calibrating.

    std::vector<std::function<void(ExecutionStats)>> _onExecutionCompleteCallbacks;  ///< Callbacks to call when execution is complete.
    std::array<std::shared_ptr<CommandHandler>, CommandType::CMD_COUNT> _handlers;

    /// @brief Finds the next slice of commands to execute.
    /// @param currentSlice The current command slice.
    /// @return The next CommandSlice.
    CommandSlice findNextSlice(const CommandSlice &currentSlice);
};

}  // namespace comms

#endif  // __COMMAND_H__
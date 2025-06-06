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

#include "command.hpp"
#include "comms_driver.hpp"
#include "id.hpp"
#include "result.hpp"

namespace comms {

enum CommandType : uint8_t {
    // General Commands, for any MCU
    CMD_BEGIN,  // begin the operation of the device
    CMD_STOP,   // end the operation of the device
    CMD_MOTOR_CONTROL,     // Motor-Driver Specific Commands
    CMD_INVALID,
    CMD_COUNT
};

/// @brief Exact same memory footprint as the RawCommsMessage Payload (uint64_t)
/// Used to get a bit of type safety and nice accessors, rather than deal with a bunch of
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

    CommandMessagePayload()
        : type(CommandType::CMD_INVALID), mcuID(MCUID::MCU_PALM), commandID(0), payload(0) {}
    CommandMessagePayload(CommandType cType, MCUID mid, uint16_t cid, uint32_t data)
        : type(cType), mcuID(mid), commandID(cid), payload(data) {}

    static Result<CommandMessagePayload> fromRaw(RawCommsMessage message) {
        Option<MessageInfo> infoOpt = MessageInfo::getInfo(message.id);

        if (infoOpt.isNone()) {
            return Result<CommandMessagePayload>::errorResult(
                "Unable to get command from message! Invalid ID");
        }

        MessageInfo senderInfo = infoOpt.value();

        if (senderInfo.type != MessageContentType::MT_COMMAND) {
            return Result<CommandMessagePayload>::errorResult(
                "Unable to get command from message! Not a command message");
        }

        return Result<CommandMessagePayload>::ok(CommandMessagePayload(message.payload));
    }

   private:
    CommandMessagePayload(uint64_t raw) : raw(raw) {}
};

enum MotorControlCommandType : uint8_t { MC_CMD_POS, MC_CMD_VEL };

/// @brief Exact same memory footprint as the CommandMessagePayload::Payload, used for the "specific
/// things"
// in side the payload -- this is the inner most layer
struct MotorControlCommandOpt {
    union {
        uint32_t payload;
        struct {
            MCUID targetID;
            uint8_t motorNumber;
            MotorControlCommandType controlType;
            uint8_t value;
        };
    };

    MotorControlCommandOpt() : payload(0) {}
    MotorControlCommandOpt(MCUID targetID, uint8_t motorNumber, MotorControlCommandType type,
                           uint8_t value)
        : targetID(targetID), motorNumber(motorNumber), controlType(type), value(value) {}
};

struct BeginExecutionCommandOpt {
    union {
        uint32_t payload;
        struct {
            MCUID targetID;
        };
    };

    BeginExecutionCommandOpt() : payload(0) {}
    BeginExecutionCommandOpt(MCUID targetID) : targetID(targetID) {}
};

struct EndExecutionCommandOpt {
    union {
        uint32_t payload;
        struct {
            MCUID targetID;
        };
    };

    EndExecutionCommandOpt() : payload(0) {}
    EndExecutionCommandOpt(MCUID targetID) : targetID(targetID) {}
};

class CommandBuilder {
   public:
    static uint16_t __cmdCounter;

    static CommandMessagePayload motorControl(MCUID sender, MotorControlCommandOpt motorCmd) {
        return CommandMessagePayload(CommandType::CMD_MOTOR_CONTROL, sender, __cmdCounter++,
                                     motorCmd.payload);
    }

    static CommandMessagePayload endExecution(MCUID sender, BeginExecutionCommandOpt beginCmd) {
        return CommandMessagePayload(CommandType::CMD_BEGIN, sender, __cmdCounter++,
                                     beginCmd.payload);
    }
};

/// @brief Handles specific commands, determines if events are parallizable, etc.
/// Used to specify "when I recieve this type of command, what should happen?"
class CommandHandler {
   public:
    virtual void start(const CommandMessagePayload& payload) {}
    virtual void update(const CommandMessagePayload& payload) {}
    virtual void end(const CommandMessagePayload& payload) {}
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
        static bool isEmpty(const CommandSlice& slice);

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

    std::vector<std::function<void(ExecutionStats)>>
        _onExecutionCompleteCallbacks;  ///< Callbacks to call when execution is complete.
    std::array<std::shared_ptr<CommandHandler>, CommandType::CMD_COUNT> _handlers;

    /// @brief Finds the next slice of commands to execute.
    /// @param currentSlice The current command slice.
    /// @return The next CommandSlice.
    CommandSlice findNextSlice(const CommandSlice& currentSlice);
};

struct CommandAcknowledgementInfo {
    RawCommsMessage message;
    uint32_t lastSent;
    uint8_t numRetries;
};

class CommandManager {
   public:
    CommandManager(CommsDriver* driver, MCUID me);

    void sendCommand(CommandMessagePayload payload);
    void handleCommandMessage(MessageInfo info, RawCommsMessage message);

    void tick();

   private:
    std::unordered_map<uint16_t, CommandAcknowledgementInfo> _unackedCommands;
    std::vector<uint16_t> _toRemoveUnackedCommands;

    bool _startCommandEnqueued = false;
    RawCommsMessage _startCommandMessage;

    CommsDriver* _driver;
    MCUID _me;

    CommandBuffer _cmdBuf;
};

}  // namespace comms

#endif  // __COMMAND_H__
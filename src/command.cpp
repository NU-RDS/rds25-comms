#include "impl/command.hpp"

#include <Arduino.h>
#include <stdint.h>

#include <iostream>

#include "impl/debug.hpp"

using namespace comms;

uint16_t CommandBuilder::__cmdCounter = 0;

CommandBuffer::CommandBuffer() : _currentSlice(CommandSlice::empty()) {}

void CommandBuffer::addCommand(CommandMessagePayload command) {
    _commands.push_back(command);
}

void CommandBuffer::tick() {
    if (_isExecuting == false) {
        return;
    }

    if (CommandSlice::isEmpty(_currentSlice)) {
        _currentSlice = findNextSlice(_currentSlice);
    }

    if (CommandSlice::isEmpty(_currentSlice)) {
        _isExecuting = false;
        return;
    }

    for (size_t i = _currentSlice.start(); i < _currentSlice.end(); i++) {
        // find the handler
        CommandMessagePayload commandPayload = _commands[i];

        std::shared_ptr<CommandHandler> handler = _handlers[commandPayload.commandID];
        if (handler == nullptr) continue;  // no handler
    }

    if (_numCompletedCommands == _currentSlice.size()) {
        _currentSlice = CommandSlice::empty();
        _numCompletedCommands = 0;

        ExecutionStats stats = {
            .time = millis() - _startTime,
            .executed = static_cast<uint8_t>(_currentSlice.size()),
            .success = true,
        };

        for (auto& callback : _onExecutionCompleteCallbacks) {
            callback(stats);
        }

        _isExecuting = false;
    }
}

void CommandBuffer::setHandler(CommandType type, std::shared_ptr<CommandHandler> handler) {
    _handlers[type] = handler;
}

void CommandBuffer::startExecution() {
    if (_isExecuting) {
        std::cerr << "Command buffer is already executing" << std::endl;
        return;
    }

    this->_startTime = millis();

    _isExecuting = true;
}

void CommandBuffer::clear() {
    _commands.clear();
    _currentSlice = CommandBuffer::CommandSlice::empty();
}

void CommandBuffer::reset() {
    _currentSlice = CommandBuffer::CommandSlice(0, 0);
}

CommandBuffer::CommandSlice::CommandSlice(std::size_t start, std::size_t end)
    : _start(start), _end(end) {}

std::size_t CommandBuffer::CommandSlice::start() const {
    return _start;
}

std::size_t CommandBuffer::CommandSlice::end() const {
    return _end;
}

std::size_t CommandBuffer::CommandSlice::size() const {
    return _end - _start;
}

CommandBuffer::CommandSlice CommandBuffer::CommandSlice::empty() {
    return CommandSlice(10, 0);
}

bool CommandBuffer::CommandSlice::isEmpty(const CommandSlice& slice) {
    return slice.start() >= slice.end();
}

CommandBuffer::CommandSlice CommandBuffer::findNextSlice(const CommandSlice& currentSlice) {
    std::size_t start = currentSlice.end();
    std::size_t end = currentSlice.end();

    if (start >= _commands.size()) {
        return CommandSlice::empty();
    }

    std::vector<CommandMessagePayload> slice(_commands.size());

    for (std::size_t i = currentSlice.end(); i < _commands.size(); i++) {
        slice.push_back(_commands[i]);
        CommandMessagePayload commandPayload = _commands[i];

        std::shared_ptr<CommandHandler> handler = _handlers[commandPayload.commandID];
        if (handler == nullptr) continue;  // no handler

        if (!handler->isParallelizable(slice)) {
            end = i + 1;
            break;
        }
    }

    return CommandSlice(start, end);
}

CommandManager::CommandManager(CommsDriver* driver, MCUID me) : _driver(driver), _me(me) {}

void CommandManager::tick() {
    if (_startCommandEnqueued && _unackedCommands.size() == 0) {
        // we are good to go!
        _driver->sendMessage(_startCommandMessage);
        _startCommandEnqueued = false;
    }

    // figure out if we need to retransmit
    uint32_t now = millis();
    for (auto& pair : _unackedCommands) {
        if (now - pair.second.lastSent > 1000) {
            // retransmit
            if (pair.second.numRetries <= 3) {
                _driver->sendMessage(pair.second.message);
                pair.second.numRetries++;
                COMMS_DEBUG_PRINT("Retransmitting command...");
            } else {
                // mark for removal
                CommandMessagePayload payload =
                    CommandMessagePayload::fromRaw(pair.second.message).value();
                _toRemoveUnackedCommands.push_back(payload.commandID);
            }
        }
    }

    // remove the commands
    for (uint16_t commandID : _toRemoveUnackedCommands) {
        _unackedCommands.erase(commandID);
    }
    _toRemoveUnackedCommands.clear();
}

void CommandManager::sendCommand(CommandMessagePayload payload) {
    if (_me != MCUID::MCU_HIGH_LEVEL) {
        // we should not be able to send the command
        COMMS_DEBUG_PRINT_ERRORLN("Unable to send a command! We are not high level!");
        return;
    }

    RawCommsMessage raw;
    raw.payload = payload.raw;

    Option<uint32_t> idOpt = MessageInfo::getMessageID(_me, MessageContentType::MT_COMMAND);
    if (idOpt.isNone()) {
        COMMS_DEBUG_PRINT_ERRORLN(
            "Unable to send a command! No ID found for command messages for me\n");
        return;
    }
    raw.id = idOpt.value();

    if (payload.type == CommandType::CMD_BEGIN) {
        // enqueue the command
        _startCommandEnqueued = true;
        _startCommandMessage = raw;
        COMMS_DEBUG_PRINTLN("Enqueuing start command!");
        return;
    }

    _driver->sendMessage(raw);

    // add this to the list of unacknowledged commands
    CommandAcknowledgementInfo ackInfo;
    ackInfo.lastSent = millis();
    ackInfo.numRetries = 0;
    ackInfo.message = raw;

    _unackedCommands[payload.commandID] = ackInfo;
}

void CommandManager::handleCommandMessage(MessageInfo info, RawCommsMessage message) {
    Result<CommandMessagePayload> cmdRes = CommandMessagePayload::fromRaw(message);
    if (cmdRes.isError()) {
        COMMS_DEBUG_PRINT_ERROR("Unable to handle command: %s,", cmdRes.error());
        return;
    }

    CommandMessagePayload cmd = cmdRes.value();
    if (_me != MCUID::MCU_HIGH_LEVEL) {
        // acknoweldge the command by copying the payload
        RawCommsMessage ack;
        ack.payload = cmd.raw;
        _driver->sendMessage(ack);

        switch (cmd.type) {
            case CMD_BEGIN:
                _cmdBuf.startExecution();
                break;
            case CMD_STOP:
                COMMS_DEBUG_PRINT_ERROR("Command stop unimplemented!!!");
                break;
            case CMD_MOTOR_CONTROL:
                _cmdBuf.addCommand(cmd);
                break;
            default:
                COMMS_DEBUG_PRINT_ERRORLN("Invalid command recieved!");
                break;
        }
    } else {
        // we are recieving an acknowledgement
        // check if it's true
        if (_unackedCommands.find(cmd.commandID) == _unackedCommands.end()) {
            // we recieved an ack for a command that we never sent
            COMMS_DEBUG_PRINT_ERRORLN("Received acknowledgement for command %d but don't need one!",
                                      cmd.commandID);
            return;
        }

        // erase it from the unacked commdns
        _unackedCommands.erase(cmd.commandID);
    }
}
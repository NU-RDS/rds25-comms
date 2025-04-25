#include "impl/command.hpp"

#include <Arduino.h>
#include <stdint.h>
#include <iostream>

using namespace comms;

CommandBuffer::CommandBuffer() : _currentSlice(CommandSlice::empty()) {
}

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
        if (handler == nullptr) continue; // no handler

    }

    if (_numCompletedCommands == _currentSlice.size()) {
        _currentSlice = CommandSlice::empty();
        _numCompletedCommands = 0;

        ExecutionStats stats = {
            .time = millis() - _startTime,
            .executed = static_cast<uint8_t>(_currentSlice.size()),
            .success = true,
        };

        for (auto &callback : _onExecutionCompleteCallbacks) {
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

/**------------------------------------------------------------------------
 *           CommandBuffer::CommandSlice Implementation
 *------------------------------------------------------------------------**/

CommandBuffer::CommandSlice::CommandSlice(std::size_t start, std::size_t end)
    : _start(start), _end(end) {
}

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

bool CommandBuffer::CommandSlice::isEmpty(const CommandSlice &slice) {
    return slice.start() >= slice.end();
}

CommandBuffer::CommandSlice CommandBuffer::findNextSlice(const CommandSlice &currentSlice) {
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
        if (handler == nullptr) continue; // no handler

        if (!handler->isParallelizable(slice)) {
            end = i + 1;
            break;
        }
    }

    return CommandSlice(start, end);
}
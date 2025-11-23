#ifndef HELIX_BUILTIN_HANDLER_H
#define HELIX_BUILTIN_HANDLER_H

#include "shell/interfaces.h"
#include "types.h"
#include "shell/shell_state.h"
#include <string>
#include <memory>
#include <map>

namespace helix {

// Forward declaration
struct ShellState;

// BuiltinCommandHandler - Base class for all built-in command handlers
// Implements IBuiltinCommandHandler interface (Dependency Inversion Principle)
// Uses Strategy pattern to allow different implementations for each builtin
class BuiltinCommandHandler : public IBuiltinCommandHandler {
public:
    virtual ~BuiltinCommandHandler() = default;

    // Handle a command
    // Returns true on success, false on error
    virtual bool handle(const ParsedCommand& cmd, ShellState& state) override = 0;

    // Check if this handler can handle the given command
    virtual bool canHandle(const std::string& command) const override = 0;
};

// CdCommandHandler - Handles 'cd' command
class CdCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

// ExitCommandHandler - Handles 'exit' command
class ExitCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

// HistoryCommandHandler - Handles 'history' command
class HistoryCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

// JobsCommandHandler - Handles 'jobs' command
class JobsCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

// FgCommandHandler - Handles 'fg' command
class FgCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

// BgCommandHandler - Handles 'bg' command
class BgCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

// BuiltinCommandDispatcher - Dispatches commands to appropriate handlers
// Implements IBuiltinDispatcher interface (Dependency Inversion Principle)
class BuiltinCommandDispatcher : public IBuiltinDispatcher {
public:
    BuiltinCommandDispatcher();
    ~BuiltinCommandDispatcher() override = default;

    // Dispatch command to appropriate handler
    // Returns true if command was handled, false otherwise
    bool dispatch(const ParsedCommand& cmd, ShellState& state) override;

    // Check if a command is a builtin
    bool isBuiltin(const std::string& command) const override;

private:
    std::map<std::string, std::unique_ptr<BuiltinCommandHandler>> handlers;
};

} // namespace helix

#endif // HELIX_BUILTIN_HANDLER_H

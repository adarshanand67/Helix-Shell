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

// PwdCommandHandler - Handles 'pwd' command
class PwdCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

// ExportCommandHandler - Handles 'export' command
class ExportCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

// EchoCommandHandler - Handles 'echo' command
class EchoCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

// HelpCommandHandler - Handles 'help' command
class HelpCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

// AliasCommandHandler - Handles 'alias' command
class AliasCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

// UnaliasCommandHandler - Handles 'unalias' command
class UnaliasCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

// UnsetCommandHandler - Handles 'unset' command
class UnsetCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

// TypeCommandHandler - Handles 'type' command
class TypeCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

// ReadCommandHandler - Handles 'read' command
class ReadCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

// PushdCommandHandler - Handles 'pushd' command
class PushdCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

// PopdCommandHandler - Handles 'popd' command
class PopdCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

// DirsCommandHandler - Handles 'dirs' command
class DirsCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

// WaitCommandHandler - Handles 'wait' command
class WaitCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

// TrueCommandHandler / FalseCommandHandler
class TrueCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

class FalseCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

// SetCommandHandler - Handles 'set' command (set -e, set -x, set -u, set +e …)
class SetCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

// TestCommandHandler - Handles 'test' and '[' commands
class TestCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

// AiCommandHandler - AI assistant: suggest, explain, fix, run
class AiCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

class PrintfCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

class KillCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

class TrapCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

class UmaskCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

class UlimitCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

class DeclareCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

class ReadonlyCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

class GetoptsCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

class ShiftCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

class CommandCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

class BuiltinCommandCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

class ExecCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

class EvalCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

class TimesCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

class HashCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

class SuspendCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

class DisownCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

class LetCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

class ReturnCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

class BreakCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

class ContinueCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

class LocalCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

// SourceCommandHandler - Handles 'source' and '.' commands
class SourceCommandHandler : public BuiltinCommandHandler {
public:
    bool handle(const ParsedCommand& cmd, ShellState& state) override;
    bool canHandle(const std::string& command) const override;
};

// WhichCommandHandler - Handles 'which' command
class WhichCommandHandler : public BuiltinCommandHandler {
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

#ifndef HELIX_SHELL_H
#define HELIX_SHELL_H

#include "types.h"
#include "tokenizer.h"
#include "parser.h"
#include "executor.h"
#include "readline_support.h"
#include "prompt.h"
#include "shell/shell_state.h"
#include "shell/builtin_handler.h"
#include "shell/job_manager.h"
#include <string>
#include <vector>
#include <memory>

namespace helix {

// Main shell class managing the REPL loop using composition
// Delegates specialized tasks to focused components:
// - BuiltinCommandDispatcher for built-in commands
// - JobManager for job control
// - ShellState for environment and state management
class Shell {
public:
    Shell();
    ~Shell();

    // Main REPL loop - runs until exit
    int run();

    // Public version for testing
    bool processInputString(const std::string& input) { return processInput(input); }

private:
    // Core REPL components
    void showPrompt();
    std::string readInput();
    bool processInput(const std::string& input);

    // Autocompletion
    std::vector<std::string> getCommandCompletions(const std::string& partial) const;
    std::vector<std::string> getPathCompletions(const std::string& partial) const;
    std::string handleTabCompletion(const std::string& currentInput, size_t cursorPos);

    // Shell state (centralized)
    ShellState state;

    // Command processing components
    Tokenizer tokenizer;
    Parser parser;
    Executor executor;

    // Prompt generator
    Prompt prompt;

    // Component instances (composition)
    std::unique_ptr<BuiltinCommandDispatcher> builtin_dispatcher;
    std::unique_ptr<JobManager> job_manager;
};

} // namespace helix

#endif // HELIX_SHELL_H

#ifndef HSHELL_SHELL_H
#define HSHELL_SHELL_H

#include "types.h"
#include "tokenizer.h"
#include "parser.h"
#include "executor.h"
#include <string>
#include <vector>

namespace hshell {

// Main shell class managing the REPL loop and overall shell state
class Shell {
public:
    Shell();
    ~Shell();

    // Main REPL loop - runs until exit
    void run();

private:
    // Core REPL components
    void showPrompt();
    std::string readInput();
    bool processInput(const std::string& input);

    // State
    std::string current_directory;
    std::string home_directory;
    std::string prompt_format;
    bool running;

    // Parser for commands
    Tokenizer tokenizer;
    Parser parser;
    Executor executor;

    // Shell environment
    std::map<std::string, std::string> environment;

    // History (will be implemented later)
    std::vector<std::string> command_history;

    // Job management (will be implemented later)
    std::map<int, Job> jobs;
    int next_job_id;
};

} // namespace hshell

#endif // HSHELL_SHELL_H

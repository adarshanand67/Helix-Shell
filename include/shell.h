#ifndef HELIX_SHELL_H
#define HELIX_SHELL_H

#include "types.h"
#include "tokenizer.h"
#include "parser.h"
#include "executor.h"
#include "readline_support.h"
#include <string>
#include <vector>

namespace helix {

// Main shell class managing the REPL loop and overall shell state
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

    // Prompt enhancement
    std::string getGitBranch() const;
    std::string getColoredPrompt();

    // Autocompletion
    std::vector<std::string> getCommandCompletions(const std::string& partial) const;
    std::vector<std::string> getPathCompletions(const std::string& partial) const;
    std::string handleTabCompletion(const std::string& currentInput, size_t cursorPos);

    // State
    std::string current_directory;
    std::string home_directory;
    std::string prompt_format;
    bool running;
    int last_exit_status = 0;

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

    // Job control methods
    void printJobs();
    void bringToForeground(int job_id);
    void resumeInBackground(int job_id);
};

} // namespace helix

#endif // HELIX_SHELL_H

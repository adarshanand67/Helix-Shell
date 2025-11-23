#ifndef HSHELL_SHELL_H
#define HSHELL_SHELL_H

#include "types.h" // Includes type definitions: ParsedCommand, TokenType, Job, JobStatus structs, Command structures for shell operations.
#include "tokenizer.h" // Includes Tokenizer class for tokenizing input strings into tokens.
#include "parser.h" // Includes Parser class for converting token sequences into ParsedCommand structures.
#include "executor.h" // Includes Executor class for executing parsed commands and pipelines.
#include <string> // Provides std::string for directory paths, prompts, and command history storage.
#include <vector> // Provides std::vector for storing command history and future collections.

namespace hshell {

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

} // namespace hshell

#endif // HSHELL_SHELL_H

#ifndef HSHELL_EXECUTOR_H
#define HSHELL_EXECUTOR_H

#include "types.h" // Includes project-wide type definitions for ParsedCommand, Command, Pipeline structures, and Token enums used in command execution.
#include <string> // Provides std::string for executable paths, command arguments, and error messages.
#include <vector> // Provides std::vector for storing command arguments and pipeline commands.
#include <optional> // Provides std::optional for potential future use in optional execution parameters.

namespace hshell {

// Command Executor - handles execution of parsed commands
class Executor {
public:
    Executor();
    ~Executor();

    // Execute a parsed command (pipeline)
    // Returns exit status or error information
    int execute(const ParsedCommand& cmd);

private:
    // Execute a single command (may be part of a pipeline)
    int executeSingleCommand(const Command& cmd, int input_fd = -1, int output_fd = -1);

    // Handle redirections for a command
    bool setupRedirections(const Command& cmd, int& input_fd, int& output_fd);

    // Restore file descriptors after redirection
    void restoreFileDescriptors(int original_stdin, int original_stdout, int original_stderr);

    // Find executable in PATH
    std::string findExecutable(const std::string& command);

    // Convert command args to C-style array for exec
    std::vector<char*> buildArgv(const std::vector<std::string>& args);

    // Error handling
    void reportError(const std::string& message);

    // Original file descriptors for restoration
    int original_stdin;
    int original_stdout;
    int original_stderr;
};

} // namespace hshell

#endif // HSHELL_EXECUTOR_H

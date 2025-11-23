#ifndef HSHELL_EXECUTOR_H
#define HSHELL_EXECUTOR_H

#include "types.h"
#include <string>
#include <vector>
#include <optional>

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

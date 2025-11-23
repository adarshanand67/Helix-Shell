#ifndef HELIX_EXECUTOR_H
#define HELIX_EXECUTOR_H

#include "types.h"
#include "executor/interfaces.h"
#include <string>
#include <vector>
#include <memory>

namespace helix {

// Command Executor - handles execution of parsed commands using composition
// Follows Dependency Inversion Principle (depends on interfaces, not implementations)
// Delegates specialized tasks to focused components:
// - IExecutableResolver for PATH searching
// - IEnvironmentExpander for variable substitution
// - IFileDescriptorManager for FD redirections
// - IPipelineManager for multi-command pipelines
class Executor {
public:
    // Constructor with default implementations (can be swapped for testing)
    Executor();

    // Constructor with dependency injection (for testing/flexibility)
    Executor(
        std::unique_ptr<IExecutableResolver> resolver,
        std::unique_ptr<IEnvironmentExpander> expander,
        std::unique_ptr<IFileDescriptorManager> fd_mgr,
        std::unique_ptr<IPipelineManager> pipe_mgr);

    ~Executor();

    // Execute a parsed command (pipeline or single command)
    // Returns exit status or error information
    int execute(const ParsedCommand& cmd);

    // Get the PID of the last background job (if any)
    // Returns 0 if no background job was started
    pid_t getLastBackgroundPid() const { return last_background_pid; }

private:
    // Execute a single command (may be part of a pipeline)
    // If background=true, doesn't wait for process and returns 0
    int executeSingleCommand(const Command& cmd, int input_fd = -1, int output_fd = -1, bool background = false);

    // Execute command directly in child process (no fork, for pipelines)
    void executeCommandInChild(const Command& cmd);

    // Convert command args to C-style array for exec
    std::vector<char*> buildArgv(const std::vector<std::string>& args);

    // Error handling
    void reportError(const std::string& message);

    // Component instances (composition via interfaces - Dependency Inversion Principle)
    std::unique_ptr<IExecutableResolver> exe_resolver;
    std::unique_ptr<IEnvironmentExpander> env_expander;
    std::unique_ptr<IFileDescriptorManager> fd_manager;
    std::unique_ptr<IPipelineManager> pipeline_manager;

    // Track last background job PID
    pid_t last_background_pid = 0;
};

} // namespace helix

#endif // HELIX_EXECUTOR_H

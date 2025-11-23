#ifndef HELIX_EXECUTOR_H
#define HELIX_EXECUTOR_H

#include "types.h"
#include "executor/executable_resolver.h"
#include "executor/environment_expander.h"
#include "executor/fd_manager.h"
#include "executor/pipeline_manager.h"
#include <string>
#include <vector>
#include <memory>

namespace helix {

// Command Executor - handles execution of parsed commands using composition
// Delegates specialized tasks to focused components:
// - ExecutableResolver for PATH searching
// - EnvironmentVariableExpander for variable substitution
// - FileDescriptorManager for FD redirections
// - PipelineManager for multi-command pipelines
class Executor {
public:
    Executor();
    ~Executor();

    // Execute a parsed command (pipeline or single command)
    // Returns exit status or error information
    int execute(const ParsedCommand& cmd);

private:
    // Execute a single command (may be part of a pipeline)
    int executeSingleCommand(const Command& cmd, int input_fd = -1, int output_fd = -1);

    // Execute command directly in child process (no fork, for pipelines)
    void executeCommandInChild(const Command& cmd);

    // Convert command args to C-style array for exec
    std::vector<char*> buildArgv(const std::vector<std::string>& args);

    // Error handling
    void reportError(const std::string& message);

    // Component instances (composition)
    std::unique_ptr<ExecutableResolver> exe_resolver;
    std::unique_ptr<EnvironmentVariableExpander> env_expander;
    std::unique_ptr<FileDescriptorManager> fd_manager;
    std::unique_ptr<PipelineManager> pipeline_manager;
};

} // namespace helix

#endif // HELIX_EXECUTOR_H

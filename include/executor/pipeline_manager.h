#ifndef HELIX_PIPELINE_MANAGER_H
#define HELIX_PIPELINE_MANAGER_H

#include "executor/interfaces.h"
#include "types.h"
#include <vector>
#include <utility>
#include <functional>
#include <sys/types.h>

namespace helix {

// PipelineManager - Manages execution of command pipelines
// Implements IPipelineManager interface (Dependency Inversion Principle)
// Responsibilities:
// - Create pipes between commands
// - Fork processes for each command in pipeline
// - Setup proper pipe connections
// - Wait for all pipeline processes to complete
// - Return exit status of last command
class PipelineManager : public IPipelineManager {
public:
    PipelineManager() = default;
    ~PipelineManager() override = default;

    // Execute a pipeline of commands
    // The executor_func is called for each command in the pipeline
    // Returns exit status of the last command
    int executePipeline(
        const ParsedCommand& cmd,
        std::function<void(const Command&)> executor_func) override;

private:
    // Create pipes for pipeline
    std::vector<std::pair<int, int>> createPipes(size_t count);

    // Clean up all pipes
    void cleanupPipes(std::vector<std::pair<int, int>>& pipes);

    // Wait for all child processes and return exit status of last command
    int waitForPipeline(const std::vector<pid_t>& pids);
};

} // namespace helix

#endif // HELIX_PIPELINE_MANAGER_H

#include "executor/pipeline_manager.h"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

namespace helix {

int PipelineManager::executePipeline(
    const ParsedCommand& cmd,
    std::function<void(const Command&)> executor_func) {

    size_t num_commands = cmd.pipeline.commands.size();

    if (num_commands == 0) {
        return 0;
    }

    if (num_commands == 1) {
        std::cerr << "PipelineManager: Use executeSingleCommand for single commands\n";
        return -1;
    }

    // Create pipes between commands
    std::vector<std::pair<int, int>> pipes = createPipes(num_commands - 1);
    if (pipes.size() != num_commands - 1) {
        return -1;
    }

    // Fork and execute each command in the pipeline
    std::vector<pid_t> pids;
    pids.reserve(num_commands);

    for (size_t i = 0; i < num_commands; ++i) {
        pid_t pid = fork();
        if (pid == -1) {
            std::cerr << "Fork failed for pipeline command\n";
            cleanupPipes(pipes);
            return -1;
        }

        if (pid == 0) { // Child process
            // Setup input redirection for this command
            if (i > 0) {
                // Read from previous pipe
                if (dup2(pipes[i-1].first, STDIN_FILENO) == -1) {
                    std::cerr << "Failed to redirect stdin from pipe\n";
                    exit(1);
                }
            }

            // Setup output redirection for this command
            if (i < num_commands - 1) {
                // Write to next pipe
                if (dup2(pipes[i].second, STDOUT_FILENO) == -1) {
                    std::cerr << "Failed to redirect stdout to pipe\n";
                    exit(1);
                }
            }

            // Close all pipe file descriptors in child
            for (auto& pipe_fds : pipes) {
                close(pipe_fds.first);
                close(pipe_fds.second);
            }

            // Execute the command (this will call executeCommandInChild)
            executor_func(cmd.pipeline.commands[i]);
            exit(1); // Should never reach here if exec succeeds
        } else {
            // Parent process
            pids.push_back(pid);

            // Close pipe ends that are not needed in parent
            if (i > 0) {
                close(pipes[i-1].first);
                close(pipes[i-1].second);
            }
        }
    }

    // Close remaining pipe ends in parent
    cleanupPipes(pipes);

    // Wait for all commands in the pipeline to complete
    return waitForPipeline(pids);
}

std::vector<std::pair<int, int>> PipelineManager::createPipes(size_t count) {
    std::vector<std::pair<int, int>> pipes;
    pipes.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        int pipe_fds[2];
        if (pipe(pipe_fds) == -1) {
            std::cerr << "Failed to create pipe\n";
            // Clean up already created pipes
            for (auto& p : pipes) {
                close(p.first);
                close(p.second);
            }
            return {};
        }
        pipes.emplace_back(pipe_fds[0], pipe_fds[1]); // [read_fd, write_fd]
    }

    return pipes;
}

void PipelineManager::cleanupPipes(std::vector<std::pair<int, int>>& pipes) {
    for (auto& pipe_fds : pipes) {
        close(pipe_fds.first);
        close(pipe_fds.second);
    }
}

int PipelineManager::waitForPipeline(const std::vector<pid_t>& pids) {
    int exit_status = -1;

    for (size_t i = 0; i < pids.size(); ++i) {
        int status;
        if (waitpid(pids[i], &status, 0) == -1) {
            std::cerr << "Wait failed for pipeline process " << i << "\n";
            continue;
        }

        if (i == pids.size() - 1) { // Last command
            if (WIFEXITED(status)) {
                exit_status = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                std::cerr << "Pipeline last command terminated by signal "
                         << WTERMSIG(status) << "\n";
                exit_status = 128 + WTERMSIG(status);
            }
        }
    }

    return exit_status;
}

} // namespace helix

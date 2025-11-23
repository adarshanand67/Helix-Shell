#include "executor.h"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstring>

namespace helix {

Executor::Executor()
    : exe_resolver(std::make_unique<ExecutableResolver>()),
      env_expander(std::make_unique<EnvironmentVariableExpander>()),
      fd_manager(std::make_unique<FileDescriptorManager>()),
      pipeline_manager(std::make_unique<PipelineManager>()) {
}

Executor::~Executor() = default;

int Executor::execute(const ParsedCommand& cmd) {
    size_t num_commands = cmd.pipeline.commands.size();

    // Handle empty command
    if (num_commands == 0) {
        return 0;
    }

    // Handle single command (no pipeline)
    if (num_commands == 1) {
        if (cmd.background) {
            reportError("Background execution (&) not yet implemented");
            return -1;
        }
        return executeSingleCommand(cmd.pipeline.commands[0]);
    }

    // Handle pipeline
    if (cmd.background) {
        reportError("Background execution (&) not supported for pipelines yet");
        return -1;
    }

    // Use PipelineManager to execute pipeline
    // The lambda handles file redirections before executing each command
    auto executor_func = [this](const Command& command) {
        // Setup file redirections for this pipeline command
        int file_input_fd = -1, file_output_fd = -1;
        if (!fd_manager->setupRedirections(command, file_input_fd, file_output_fd)) {
            exit(1);
        }

        // Execute the command
        this->executeCommandInChild(command);
    };

    return pipeline_manager->executePipeline(cmd, executor_func);
}

int Executor::executeSingleCommand(const Command& cmd, int input_fd, int output_fd) {
    if (cmd.args.empty()) {
        reportError("No command to execute");
        return -1;
    }

    // Handle built-in commands (should be handled at shell level)
    if (cmd.args[0] == "cd" || cmd.args[0] == "jobs" ||
        cmd.args[0] == "fg" || cmd.args[0] == "bg") {
        reportError("Built-in commands should be handled at shell level");
        return -1;
    }

    // Fork child process
    pid_t pid = fork();
    if (pid == -1) {
        reportError("Fork failed");
        return -1;
    }

    if (pid == 0) { // Child process
        // Setup file redirections using FileDescriptorManager
        int file_input_fd = -1, file_output_fd = -1;
        if (!fd_manager->setupRedirections(cmd, file_input_fd, file_output_fd)) {
            exit(1);
        }

        // If no file redirection, use the provided fd (from pipes)
        if (file_input_fd == -1 && input_fd != -1) {
            if (dup2(input_fd, STDIN_FILENO) == -1) {
                std::cerr << "Failed to redirect stdin from pipe\n";
                exit(1);
            }
            if (input_fd > 2) close(input_fd);
        }

        if (file_output_fd == -1 && output_fd != -1) {
            if (dup2(output_fd, STDOUT_FILENO) == -1) {
                std::cerr << "Failed to redirect stdout to pipe\n";
                exit(1);
            }
            if (output_fd > 2) close(output_fd);
        }

        // Execute the command
        executeCommandInChild(cmd);
        exit(1); // Should never reach here if exec succeeds
    } else { // Parent process
        // Close the input/output fds that we provided to the child
        if (input_fd > 2) close(input_fd);
        if (output_fd > 2) close(output_fd);

        // Wait for child process completion
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            reportError("Wait failed");
            return -1;
        }

        // Return exit status
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            std::cerr << "Command terminated by signal " << WTERMSIG(status) << "\n";
            return 128 + WTERMSIG(status);
        }

        return -1;
    }
}

void Executor::executeCommandInChild(const Command& cmd) {
    if (cmd.args.empty()) {
        exit(1);
    }

    // Expand environment variables using EnvironmentVariableExpander
    std::vector<std::string> expanded_args = cmd.args;
    for (auto& arg : expanded_args) {
        arg = env_expander->expand(arg);
    }

    // Find executable using ExecutableResolver
    std::string executable = exe_resolver->findExecutable(expanded_args[0]);
    if (executable.empty()) {
        std::cerr << "Command not found: " << expanded_args[0] << "\n";
        exit(127); // Command not found exit code
    }

    // Update the command path
    std::vector<std::string> exec_args = expanded_args;
    exec_args[0] = executable;

    // Close extra file descriptors that might be from pipes
    for (int fd = 3; fd < 1024; ++fd) {
        close(fd);
    }

    // Build arguments for exec
    std::vector<char*> argv = buildArgv(exec_args);

    // Debug: print the command being executed
    std::cerr << "Executing: " << executable;
    for (size_t i = 1; i < exec_args.size(); ++i) {
        std::cerr << " '" << exec_args[i] << "'";
    }
    std::cerr << std::endl;

    // Execute the command
    execvp(executable.c_str(), &argv[0]);

    // If we reach here, exec failed
    std::cerr << "Exec failed: " << strerror(errno) << "\n";
    exit(1);
}

std::vector<char*> Executor::buildArgv(const std::vector<std::string>& args) {
    std::vector<char*> argv;
    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr); // Null terminate
    return argv;
}

void Executor::reportError(const std::string& message) {
    std::cerr << "Executor error: " << message << "\n";
}

} // namespace helix

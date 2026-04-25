#include "executor.h"
#include "executor/executable_resolver.h"
#include "executor/environment_expander.h"
#include "executor/fd_manager.h"
#include "executor/pipeline_manager.h"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstring>
#include <glob.h>

namespace helix {

// Default constructor - creates standard implementations
Executor::Executor()
    : exe_resolver(std::make_unique<ExecutableResolver>()),
      env_expander(std::make_unique<EnvironmentVariableExpander>()),
      fd_manager(std::make_unique<FileDescriptorManager>()),
      pipeline_manager(std::make_unique<PipelineManager>()) {
}

// Dependency injection constructor - allows custom implementations (for testing)
Executor::Executor(
    std::unique_ptr<IExecutableResolver> resolver,
    std::unique_ptr<IEnvironmentExpander> expander,
    std::unique_ptr<IFileDescriptorManager> fd_mgr,
    std::unique_ptr<IPipelineManager> pipe_mgr)
    : exe_resolver(std::move(resolver)),
      env_expander(std::move(expander)),
      fd_manager(std::move(fd_mgr)),
      pipeline_manager(std::move(pipe_mgr)) {
}

Executor::~Executor() = default;

int Executor::execute(const ParsedCommand& cmd) {
    size_t num_commands = cmd.pipeline.commands.size();

    // Reset background PID
    last_background_pid = 0;

    // Handle empty command
    if (num_commands == 0) {
        return 0;
    }

    // Handle single command (no pipeline)
    if (num_commands == 1) {
        return executeSingleCommand(cmd.pipeline.commands[0], -1, -1, cmd.background);
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

int Executor::executeSingleCommand(const Command& cmd, int input_fd, int output_fd, bool background) {
    if (cmd.args.empty()) {
        reportError("No command to execute");
        return -1;
    }

    // Handle built-in commands (should be handled at shell level)
    if (cmd.args[0] == "cd" || cmd.args[0] == "jobs" ||
        cmd.args[0] == "fg" || cmd.args[0] == "bg" ||
        cmd.args[0] == "pwd" || cmd.args[0] == "export" ||
        cmd.args[0] == "exit" || cmd.args[0] == "history") {
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

        // Handle background vs foreground execution
        if (background) {
            // For background jobs, create a new process group and don't wait
            if (setpgid(pid, 0) == -1) {
                std::cerr << "Warning: Failed to create process group for background job\n";
            }
            last_background_pid = pid;
            std::cout << "[Background job started with PID " << pid << "]\n";
            return 0;
        } else {
            // Wait for child process completion (foreground)
            // ECHILD means SIGCHLD handler already reaped it — not an error
            int status;
            if (waitpid(pid, &status, 0) == -1) {
                if (errno == ECHILD) return 0;
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
}

// Expand globs in a single argument — returns 1+ args (or the original if no match)
static std::vector<std::string> expandGlob(const std::string& arg) {
    // Only bother if the arg contains a glob metacharacter
    if (arg.find_first_of("*?[") == std::string::npos) return {arg};

    glob_t g;
    int rc = glob(arg.c_str(), GLOB_TILDE | GLOB_NOCHECK, nullptr, &g);
    std::vector<std::string> result;
    if (rc == 0) {
        for (size_t i = 0; i < g.gl_pathc; ++i)
            result.emplace_back(g.gl_pathv[i]);
    } else {
        result.push_back(arg); // no match — pass through unchanged
    }
    globfree(&g);
    return result;
}

void Executor::executeCommandInChild(const Command& cmd) {
    if (cmd.args.empty()) {
        exit(1);
    }

    // Expand environment variables (including $(...) command substitution)
    std::vector<std::string> expanded_args;
    for (size_t i = 0; i < cmd.args.size(); ++i) {
        std::string arg = env_expander->expand(cmd.args[i]);
        if (i == 0) {
            // Don't glob-expand the command name
            expanded_args.push_back(std::move(arg));
        } else {
            auto globs = expandGlob(arg);
            for (auto& g : globs) expanded_args.push_back(std::move(g));
        }
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

    std::vector<char*> argv = buildArgv(exec_args);
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

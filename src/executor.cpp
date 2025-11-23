#include "executor.h"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstring>
#include <sys/stat.h>
#include <cstdlib>

namespace hshell {

Executor::Executor() {
    // Save original file descriptors
    original_stdin = dup(STDIN_FILENO);
    original_stdout = dup(STDOUT_FILENO);
    original_stderr = dup(STDERR_FILENO);

    if (original_stdin == -1 || original_stdout == -1 || original_stderr == -1) {
        reportError("Failed to save original file descriptors");
    }
}

Executor::~Executor() {
    // Restore original file descriptors
    if (original_stdin != -1) {
        dup2(original_stdin, STDIN_FILENO);
        close(original_stdin);
    }
    if (original_stdout != -1) {
        dup2(original_stdout, STDOUT_FILENO);
        close(original_stdout);
    }
    if (original_stderr != -1) {
        dup2(original_stderr, STDERR_FILENO);
        close(original_stderr);
    }
}

int Executor::execute(const ParsedCommand& cmd) {
    // For now, handle only single commands (no pipelines)
    if (cmd.pipeline.commands.size() != 1) {
        reportError("Pipelines not yet implemented");
        return -1;
    }

    // Handle background execution
    if (cmd.background) {
        // Create background job (not implemented yet)
        reportError("Background execution (&) not yet implemented");
        return -1;
    }

    return executeSingleCommand(cmd.pipeline.commands[0]);
}

int Executor::executeSingleCommand(const Command& cmd, int input_fd, int output_fd) {
    if (cmd.args.empty()) {
        reportError("No command to execute");
        return -1;
    }

    // Handle built-in commands first (only interactive ones like cd, jobs - exit is handled by shell)
    if (cmd.args[0] == "cd" || cmd.args[0] == "jobs" || cmd.args[0] == "fg" || cmd.args[0] == "bg") {
        reportError("Built-in commands should be handled at shell level");
        return -1;
    }

    // Find the executable
    std::string executable = findExecutable(cmd.args[0]);
    if (executable.empty()) {
        std::cerr << "Command not found: " << cmd.args[0] << "\n";
        return 127; // Command not found exit code
    }

    // Update the command path
    std::vector<std::string> exec_args = cmd.args;
    exec_args[0] = executable;

    // Fork child process
    pid_t pid = fork();
    if (pid == -1) {
        reportError("Fork failed");
        return -1;
    }

    if (pid == 0) { // Child process
        // Setup redirections
        if (!setupRedirections(cmd, input_fd, output_fd)) {
            exit(1);
        }

        // Build arguments for exec
        std::vector<char*> argv = buildArgv(exec_args);

        // Execute the command
        execvp(executable.c_str(), &argv[0]);

        // If we reach here, exec failed
        std::cerr << "Exec failed: " << strerror(errno) << "\n";
        exit(1);
    } else { // Parent process
        // Wait for child to complete
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
            return 128 + WTERMSIG(status); // Signal exit codes
        }

        return -1;
    }
}

bool Executor::setupRedirections(const Command& cmd, int& input_fd, int& output_fd) {
    // Input redirection
    if (!cmd.input_file.empty()) {
        input_fd = open(cmd.input_file.c_str(), O_RDONLY);
        if (input_fd == -1) {
            std::cerr << "Failed to open input file: " << cmd.input_file << " - " << strerror(errno) << "\n";
            return false;
        }
        if (dup2(input_fd, STDIN_FILENO) == -1) {
            std::cerr << "Failed to redirect stdin: " << strerror(errno) << "\n";
            close(input_fd);
            return false;
        }
        close(input_fd);
    }

    // Output redirection
    if (!cmd.output_file.empty()) {
        int flags = O_WRONLY | O_CREAT;
        flags |= cmd.append_mode ? O_APPEND : O_TRUNC;

        output_fd = open(cmd.output_file.c_str(), flags, 0644);
        if (output_fd == -1) {
            std::cerr << "Failed to open output file: " << cmd.output_file << " - " << strerror(errno) << "\n";
            return false;
        }
        if (dup2(output_fd, STDOUT_FILENO) == -1) {
            std::cerr << "Failed to redirect stdout: " << strerror(errno) << "\n";
            close(output_fd);
            return false;
        }
        close(output_fd);
    }

    // Error redirection
    if (!cmd.error_file.empty()) {
        int flags = O_WRONLY | O_CREAT;
        flags |= cmd.error_append_mode ? O_APPEND : O_TRUNC;

        int error_fd = open(cmd.error_file.c_str(), flags, 0644);
        if (error_fd == -1) {
            std::cerr << "Failed to open error file: " << cmd.error_file << " - " << strerror(errno) << "\n";
            return false;
        }
        if (dup2(error_fd, STDERR_FILENO) == -1) {
            std::cerr << "Failed to redirect stderr: " << strerror(errno) << "\n";
            close(error_fd);
            return false;
        }
        close(error_fd);
    }

    return true;
}

void Executor::restoreFileDescriptors(int original_stdin, int original_stdout, int original_stderr) {
    if (original_stdin != -1) dup2(original_stdin, STDIN_FILENO);
    if (original_stdout != -1) dup2(original_stdout, STDOUT_FILENO);
    if (original_stderr != -1) dup2(original_stderr, STDERR_FILENO);
}

std::string Executor::findExecutable(const std::string& command) {
    // Check if absolute or relative path
    if (command.find('/') != std::string::npos) {
        // Absolute or relative path
        struct stat st;
        if (stat(command.c_str(), &st) == 0 && S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR)) {
            return command;
        }
        return "";
    }

    // Search in PATH
    char* path_env = getenv("PATH");
    if (!path_env) {
        return "";
    }

    std::string path_str(path_env);
    size_t start = 0, end = 0;
    while ((end = path_str.find(':', start)) != std::string::npos) {
        std::string dir_path = path_str.substr(start, end - start);
        std::string full_path = dir_path + "/" + command;

        struct stat st;
        if (stat(full_path.c_str(), &st) == 0 && S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR)) {
            return full_path;
        }

        start = end + 1;
    }

    // Check last directory
    if (start < path_str.length()) {
        std::string dir_path = path_str.substr(start);
        std::string full_path = dir_path + "/" + command;

        struct stat st;
        if (stat(full_path.c_str(), &st) == 0 && S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR)) {
            return full_path;
        }
    }

    return "";
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

} // namespace hshell

#include "executor.h" // Includes the Executor class definition, providing methods for command execution, redirection setup, and error handling.
#include <iostream> // Provides standard I/O streams: std::cerr for error output - used for reporting execution failures and debug messages.
#include <unistd.h> // Provides Unix system calls: dup, dup2 for file descriptor duplication during redirection; fork for creating child processes; execvp for executing programs; close for managing file descriptors - core for pipeline and process management.
#include <sys/wait.h> // Provides process waiting and status functions: waitpid, WIFEXITED, WEXITSTATUS, WIFSIGNALED, WTERMSIG - used for waiting on child processes and interpreting their exit statuses in pipelines.
#include <fcntl.h> // Provides file control definitions: O_RDONLY, O_WRONLY, O_CREAT, O_TRUNC, O_APPEND - flags used for opening input/output/error files during redirection setup.
#include <cstring> // Provides C-style string functions: strerror for converting errno codes to readable error messages - used in file operation error reporting.
#include <sys/stat.h> // Provides file status functions: stat for retrieving file information, S_ISREG and S_IXUSR for checking if a file is a regular executable - used in PATH searching.
#include <cstdlib> // Provides general utilities: getenv for accessing environment variables like PATH - essential for searching for executable commands.
#include <regex> // Provides regular expressions for environment variable expansion.

namespace helix {

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
    size_t num_commands = cmd.pipeline.commands.size();

    // Handle empty command
    if (num_commands == 0) {
        // Empty command should succeed
        return 0;
    }

    // Handle single command (no pipeline)
    if (num_commands == 1) {
        // Handle background execution
        if (cmd.background) {
            // Create background job (not implemented yet)
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

    // Create pipes between commands
    std::vector<std::pair<int, int>> pipes; // [read_fd, write_fd] pairs
    pipes.reserve(num_commands - 1);

    for (size_t i = 0; i < num_commands - 1; ++i) {
        int pipe_fds[2];
        if (pipe(pipe_fds) == -1) {
            reportError("Failed to create pipe");
            return -1;
        }
        pipes.emplace_back(pipe_fds[0], pipe_fds[1]); // [read_fd, write_fd]
    }

    // Fork and execute each command in the pipeline
    std::vector<pid_t> pids;
    pids.reserve(num_commands);

    for (size_t i = 0; i < num_commands; ++i) {
        pid_t pid = fork();
        if (pid == -1) {
            reportError("Fork failed for pipeline command");
            // Clean up pipes
            for (auto& pipe_fds : pipes) {
                close(pipe_fds.first);
                close(pipe_fds.second);
            }
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

            // Setup any file redirections for this command
            int dummy_input_fd = -1, dummy_output_fd = -1;
            if (!setupRedirections(cmd.pipeline.commands[i], dummy_input_fd, dummy_output_fd)) {
                exit(1);
            }

            // Execute the command directly in this child process
            executeCommandInChild(cmd.pipeline.commands[i]);
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
    for (auto& pipe_fds : pipes) {
        close(pipe_fds.first);
        close(pipe_fds.second);
    }

    // Wait for all commands in the pipeline to complete
    int exit_status = -1;
    for (size_t i = 0; i < pids.size(); ++i) {
        int status;
        if (waitpid(pids[i], &status, 0) == -1) {
            reportError("Wait failed for pipeline process " + std::to_string(i));
            return -1;
        }

        if (i == pids.size() - 1) { // Last command
            if (WIFEXITED(status)) {
                exit_status = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                std::cerr << "Pipeline last command terminated by signal " << WTERMSIG(status) << "\n";
                exit_status = 128 + WTERMSIG(status);
            }
        }
    }

    // Return exit status of the last command
    return exit_status;
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

    // Fork child process
    pid_t pid = fork();
    if (pid == -1) {
        reportError("Fork failed");
        return -1;
    }

    if (pid == 0) { // Child process
        // Setup file redirections first (important: this sets input_fd, output_fd)
        int file_input_fd = -1, file_output_fd = -1;
        if (!setupRedirections(cmd, file_input_fd, file_output_fd)) {
            exit(1);
        }

        // If no file redirection, use the provided fd (from pipes)
        if (file_input_fd == -1 && input_fd != -1) {
            if (dup2(input_fd, STDIN_FILENO) == -1) {
                std::cerr << "Failed to redirect stdin from pipe\n";
                exit(1);
            }
            if (input_fd > 2) close(input_fd); // Don't close stdin/stdout/stderr
        }

        if (file_output_fd == -1 && output_fd != -1) {
            if (dup2(output_fd, STDOUT_FILENO) == -1) {
                std::cerr << "Failed to redirect stdout to pipe\n";
                exit(1);
            }
            if (output_fd > 2) close(output_fd); // Don't close stdin/stdout/stderr
        }

        // Execute the command directly in this child process
        executeCommandInChild(cmd);
        exit(1); // Should never reach here if exec succeeds
    } else { // Parent process
        // Close the input/output fds that we provided to the child (they were duplicated)
        if (input_fd > 2) close(input_fd);
        if (output_fd > 2) close(output_fd);

        // For single commands, wait for completion
        // For pipeline commands, this will be handled at pipeline level
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

std::string Executor::expandEnvironmentVariables(const std::string& input) {
    std::string result = input;

    // Regular expression to match $VAR or ${VAR}
    std::regex var_regex(R"(\$\{([^}]+)\}|\$([A-Za-z_][A-Za-z0-9_]*))");

    std::smatch match;
    std::string::const_iterator search_start(input.cbegin());

    while (std::regex_search(search_start, input.cend(), match, var_regex)) {
        std::string var_name;
        if (!match[1].str().empty()) {
            // ${VAR} format
            var_name = match[1].str();
        } else if (!match[2].str().empty()) {
            // $VAR format
            var_name = match[2].str();
        }

        // Get the environment variable value
        const char* var_value = getenv(var_name.c_str());
        std::string replacement = var_value ? var_value : "";

        // Replace the variable in the result
        size_t pos = result.find(match[0].str());
        if (pos != std::string::npos) {
            result.replace(pos, match[0].str().length(), replacement);
        }

        // Move search position
        search_start = match.suffix().first;
    }

    return result;
}

void Executor::executeCommandInChild(const Command& cmd) {
    if (cmd.args.empty()) {
        exit(1);
    }

    // Expand environment variables in arguments
    std::vector<std::string> expanded_args = cmd.args;
    for (auto& arg : expanded_args) {
        arg = expandEnvironmentVariables(arg);
    }

    // Find the executable
    std::string executable = findExecutable(expanded_args[0]);
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

} // namespace helix

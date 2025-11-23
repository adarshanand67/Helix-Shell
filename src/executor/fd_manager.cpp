#include "executor/fd_manager.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

namespace helix {

FileDescriptorManager::FileDescriptorManager() {
    // Save original file descriptors
    original_stdin = dup(STDIN_FILENO);
    original_stdout = dup(STDOUT_FILENO);
    original_stderr = dup(STDERR_FILENO);

    if (original_stdin == -1 || original_stdout == -1 || original_stderr == -1) {
        std::cerr << "FD Manager: Failed to save original file descriptors\n";
    }
}

FileDescriptorManager::~FileDescriptorManager() {
    // Restore original file descriptors
    restoreFileDescriptors();

    // Close saved descriptors
    if (original_stdin != -1) close(original_stdin);
    if (original_stdout != -1) close(original_stdout);
    if (original_stderr != -1) close(original_stderr);
}

bool FileDescriptorManager::setupRedirections(const Command& cmd, int& input_fd, int& output_fd) {
    // Setup input redirection
    if (!setupInputRedirection(cmd, input_fd)) {
        return false;
    }

    // Setup output redirection
    if (!setupOutputRedirection(cmd, output_fd)) {
        return false;
    }

    // Setup error redirection
    if (!setupErrorRedirection(cmd)) {
        return false;
    }

    return true;
}

bool FileDescriptorManager::setupInputRedirection(const Command& cmd, int& input_fd) {
    if (cmd.input_file.empty()) {
        return true;
    }

    input_fd = open(cmd.input_file.c_str(), O_RDONLY);
    if (input_fd == -1) {
        std::cerr << "Failed to open input file: " << cmd.input_file
                  << " - " << strerror(errno) << "\n";
        return false;
    }

    if (dup2(input_fd, STDIN_FILENO) == -1) {
        std::cerr << "Failed to redirect stdin: " << strerror(errno) << "\n";
        close(input_fd);
        return false;
    }

    close(input_fd);
    return true;
}

bool FileDescriptorManager::setupOutputRedirection(const Command& cmd, int& output_fd) {
    if (cmd.output_file.empty()) {
        return true;
    }

    int flags = O_WRONLY | O_CREAT;
    flags |= cmd.append_mode ? O_APPEND : O_TRUNC;

    output_fd = open(cmd.output_file.c_str(), flags, 0644);
    if (output_fd == -1) {
        std::cerr << "Failed to open output file: " << cmd.output_file
                  << " - " << strerror(errno) << "\n";
        return false;
    }

    if (dup2(output_fd, STDOUT_FILENO) == -1) {
        std::cerr << "Failed to redirect stdout: " << strerror(errno) << "\n";
        close(output_fd);
        return false;
    }

    close(output_fd);
    return true;
}

bool FileDescriptorManager::setupErrorRedirection(const Command& cmd) {
    if (cmd.error_file.empty()) {
        return true;
    }

    int flags = O_WRONLY | O_CREAT;
    flags |= cmd.error_append_mode ? O_APPEND : O_TRUNC;

    int error_fd = open(cmd.error_file.c_str(), flags, 0644);
    if (error_fd == -1) {
        std::cerr << "Failed to open error file: " << cmd.error_file
                  << " - " << strerror(errno) << "\n";
        return false;
    }

    if (dup2(error_fd, STDERR_FILENO) == -1) {
        std::cerr << "Failed to redirect stderr: " << strerror(errno) << "\n";
        close(error_fd);
        return false;
    }

    close(error_fd);
    return true;
}

void FileDescriptorManager::restoreFileDescriptors() {
    if (original_stdin != -1) {
        dup2(original_stdin, STDIN_FILENO);
    }
    if (original_stdout != -1) {
        dup2(original_stdout, STDOUT_FILENO);
    }
    if (original_stderr != -1) {
        dup2(original_stderr, STDERR_FILENO);
    }
}

} // namespace helix

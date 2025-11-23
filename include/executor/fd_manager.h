#ifndef HELIX_FD_MANAGER_H
#define HELIX_FD_MANAGER_H

#include "types.h"

namespace helix {

// FileDescriptorManager - Manages file descriptor redirections
// Responsibilities:
// - Save and restore original file descriptors
// - Setup input/output/error redirections for commands
// - Handle file opening with appropriate flags (append, truncate, etc.)
class FileDescriptorManager {
public:
    FileDescriptorManager();
    ~FileDescriptorManager();

    // Disable copy and move to prevent FD management issues
    FileDescriptorManager(const FileDescriptorManager&) = delete;
    FileDescriptorManager& operator=(const FileDescriptorManager&) = delete;
    FileDescriptorManager(FileDescriptorManager&&) = delete;
    FileDescriptorManager& operator=(FileDescriptorManager&&) = delete;

    // Setup all redirections for a command
    // Sets input_fd and output_fd if file redirections are used
    // Returns true on success, false on error
    bool setupRedirections(const Command& cmd, int& input_fd, int& output_fd);

    // Restore original file descriptors
    void restoreFileDescriptors();

private:
    // Setup individual redirection types
    bool setupInputRedirection(const Command& cmd, int& input_fd);
    bool setupOutputRedirection(const Command& cmd, int& output_fd);
    bool setupErrorRedirection(const Command& cmd);

    // Original file descriptors for restoration
    int original_stdin;
    int original_stdout;
    int original_stderr;
};

} // namespace helix

#endif // HELIX_FD_MANAGER_H

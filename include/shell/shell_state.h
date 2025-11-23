#ifndef HELIX_SHELL_STATE_H
#define HELIX_SHELL_STATE_H

#include "shell/job_manager.h"
#include "prompt.h"
#include <string>
#include <vector>
#include <map>

namespace helix {

// Forward declaration
class JobManager;

// ShellState - Encapsulates all shell state
// Responsibilities:
// - Store current directory, home directory
// - Track last exit status
// - Manage command history
// - Store environment variables
// - Manage jobs
// - Control shell running state
struct ShellState {
    std::string current_directory;
    std::string home_directory;
    int last_exit_status = 0;
    bool running = true;

    std::vector<std::string> command_history;
    std::map<std::string, std::string> environment;

    // Job management
    JobManager* job_manager = nullptr;

    // Prompt
    Prompt* prompt = nullptr;
};

} // namespace helix

#endif // HELIX_SHELL_STATE_H

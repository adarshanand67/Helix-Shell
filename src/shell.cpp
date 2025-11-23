#include "shell.h"
#include "tokenizer.h"
#include "parser.h"
#include "readline_support.h"
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <pwd.h>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <csignal>
#include <sys/wait.h>

namespace helix {

// Global pointer to JobManager for signal handler access
// This is necessary because signal handlers must be static/global functions
static IJobManager* g_job_manager = nullptr;

// SIGCHLD signal handler - called when a child process terminates or stops
// Must be signal-safe (avoid malloc, I/O operations, etc.)
static void sigchld_handler(int /* sig */) {
    // Save and restore errno to be signal-safe
    int saved_errno = errno;

    // Check for completed jobs using the global job manager
    if (g_job_manager) {
        static_cast<JobManager*>(g_job_manager)->checkCompletedJobs();
    }

    errno = saved_errno;
}

Shell::Shell()
    : builtin_dispatcher(std::make_unique<BuiltinCommandDispatcher>()),
      job_manager(std::make_unique<JobManager>()) {

    // Initialize state
    state.running = true;
    state.job_manager = job_manager.get();
    state.prompt = &prompt;

    // Set up global job manager pointer for signal handler
    g_job_manager = job_manager.get();

    // Set up SIGCHLD handler to automatically track background job completion
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP; // Restart interrupted syscalls, ignore stopped children
    if (sigaction(SIGCHLD, &sa, nullptr) == -1) {
        std::cerr << "Warning: Failed to set up SIGCHLD handler\n";
    }

    // Initialize readline for advanced autocompletion
    ReadlineSupport::initialize();

    // Initialize environment
    char* home = getenv("HOME");
    if (home) {
        state.home_directory = home;
    } else {
        // Fallback to getpwuid
        struct passwd* pw = getpwuid(getuid());
        if (pw) {
            state.home_directory = pw->pw_dir;
        }
    }

    // Set current directory
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd))) {
        state.current_directory = cwd;
    }

    // Set up prompt with user and host info
    char* user = getenv("USER");
    if (!user) {
        user = getenv("LOGNAME");
    }
    if (!user && getpwuid(getuid())) {
        user = getpwuid(getuid())->pw_name;
    }

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        prompt.setUserHost(user ? user : "user", hostname);
    } else {
        prompt.setUserHost(user ? user : "user", "helix");
    }

    prompt.setHomeDirectory(state.home_directory);
    prompt.setCurrentDirectory(state.current_directory);
    prompt.setLastExitStatus(state.last_exit_status);
}

Shell::~Shell() {
    // Clear global job manager pointer
    g_job_manager = nullptr;

    // Clean up readline
    ReadlineSupport::cleanup();
}

int Shell::run() {
    std::cout << "Helix Shell (helix) v2.0 - Type 'exit' to quit\n";

    while (state.running) {
        // Check for and print completed background job notifications
        if (job_manager) {
            static_cast<JobManager*>(job_manager.get())->printAndCleanCompletedJobs();
        }

        showPrompt();

        std::string input = readInput();

        if (!processInput(input)) {
            break;
        }
    }

    std::cout << "Goodbye!\n";

    return state.last_exit_status;
}

void Shell::showPrompt() {
    // Update the prompt with current state
    prompt.setCurrentDirectory(state.current_directory);
    prompt.setLastExitStatus(state.last_exit_status);

    // Generate and display the prompt
    std::cout << prompt.generate();
    std::cout.flush(); // Ensure prompt is displayed immediately
}

std::string Shell::readInput() {
    // Use readline for advanced autocompletion and history
    std::string line = ReadlineSupport::readLineWithCompletion("");

    if (line.empty() && std::cin.eof()) {
        // EOF (Ctrl+D) detected
        state.running = false;
        return "exit";
    }

    return line;
}

bool Shell::processInput(const std::string& input) {
    // Skip empty lines
    if (input.empty()) {
        return true;
    }

    // Add to history
    state.command_history.push_back(input);

    // Basic command processing
    std::string trimmed = input;
    // Remove leading/trailing whitespace
    size_t start = trimmed.find_first_not_of(" \t");
    if (start == std::string::npos) return true;
    size_t end = trimmed.find_last_not_of(" \t");
    trimmed = trimmed.substr(start, end - start + 1);

    // Tokenize and parse the command
    auto tokens = tokenizer.tokenize(trimmed);

    // Parse the tokens into structured command
    ParsedCommand parsed_cmd = parser.parse(tokens);

    // Try to handle as builtin command first
    bool handled = builtin_dispatcher->dispatch(parsed_cmd, state);

    // Check if it was the exit command (regardless of handler return value)
    if (!state.running) {
        return false;
    }

    // If handled as builtin, we're done
    if (handled) {
        return true;
    }

    // Execute the command using the executor
    state.last_exit_status = executor.execute(parsed_cmd);

    // If this was a background job, register it with the job manager
    pid_t bg_pid = executor.getLastBackgroundPid();
    if (bg_pid > 0 && job_manager) {
        job_manager->addJob(bg_pid, input);
    }

    return true;
}

// Get command completions from PATH
std::vector<std::string> Shell::getCommandCompletions(const std::string& partial) const {
    std::vector<std::string> completions;

    // Get PATH environment variable
    const char* path_env = getenv("PATH");
    if (!path_env) {
        return completions;
    }

    std::string path_str(path_env);
    std::stringstream ss(path_str);
    std::string dir;

    // Search each directory in PATH
    while (std::getline(ss, dir, ':')) {
        DIR* dirp = opendir(dir.c_str());
        if (!dirp) continue;

        struct dirent* entry;
        while ((entry = readdir(dirp)) != nullptr) {
            std::string name(entry->d_name);

            // Skip hidden files and directories
            if (name[0] == '.') continue;

            // Check if it matches the partial input
            if (name.find(partial) == 0) {
                // Check if it's executable
                std::string full_path = dir + "/" + name;
                struct stat st;
                if (stat(full_path.c_str(), &st) == 0 && (st.st_mode & S_IXUSR)) {
                    completions.push_back(name);
                }
            }
        }
        closedir(dirp);
    }

    // Also check built-in commands
    std::vector<std::string> builtins = {"cd", "pwd", "history", "exit", "jobs", "fg", "bg"};
    for (const auto& builtin : builtins) {
        if (builtin.find(partial) == 0) {
            completions.push_back(builtin);
        }
    }

    return completions;
}

// Get path completions for files and directories
std::vector<std::string> Shell::getPathCompletions(const std::string& partial) const {
    std::vector<std::string> completions;

    // Determine directory and prefix
    std::string dir_path = ".";
    std::string prefix = partial;

    size_t last_slash = partial.rfind('/');
    if (last_slash != std::string::npos) {
        dir_path = partial.substr(0, last_slash + 1);
        prefix = partial.substr(last_slash + 1);

        // Handle absolute paths
        if (dir_path[0] != '/') {
            dir_path = state.current_directory + "/" + dir_path;
        }
    }

    // Handle ~ expansion
    if (!prefix.empty() && prefix[0] == '~') {
        dir_path = state.home_directory;
        prefix = prefix.substr(1);
    }

    DIR* dirp = opendir(dir_path.c_str());
    if (!dirp) {
        return completions;
    }

    struct dirent* entry;
    while ((entry = readdir(dirp)) != nullptr) {
        std::string name(entry->d_name);

        // Skip . and ..
        if (name == "." || name == "..") continue;

        // Check if it matches the prefix
        if (prefix.empty() || name.find(prefix) == 0) {
            std::string completion = name;

            // Add trailing slash for directories
            std::string full_path = dir_path + "/" + name;
            struct stat st;
            if (stat(full_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
                completion += "/";
            }

            completions.push_back(completion);
        }
    }
    closedir(dirp);

    return completions;
}

// Handle tab completion (basic implementation)
std::string Shell::handleTabCompletion(const std::string& currentInput, size_t /* cursorPos */) {
    // Simple implementation: complete at end of input only
    // cursorPos not yet used but reserved for future inline completion feature
    if (currentInput.empty()) {
        return currentInput;
    }

    // Split into tokens
    std::vector<std::string> tokens;
    std::stringstream ss(currentInput);
    std::string token;
    while (ss >> token) {
        tokens.push_back(token);
    }

    if (tokens.empty()) {
        return currentInput;
    }

    std::vector<std::string> completions;

    // First token: complete commands
    if (tokens.size() == 1) {
        completions = getCommandCompletions(tokens[0]);
    } else {
        // Other tokens: complete paths
        completions = getPathCompletions(tokens.back());
    }

    if (completions.empty()) {
        return currentInput; // No completions
    }

    if (completions.size() == 1) {
        // Single completion: auto-complete
        std::string result = currentInput;
        if (tokens.size() == 1) {
            return completions[0];
        } else {
            // Replace last token with completion
            size_t last_space = currentInput.rfind(' ');
            return currentInput.substr(0, last_space + 1) + completions[0];
        }
    }

    // Multiple completions: show options
    std::cout << "\n";
    for (const auto& completion : completions) {
        std::cout << completion << "  ";
    }
    std::cout << "\n";

    return currentInput;
}

} // namespace helix

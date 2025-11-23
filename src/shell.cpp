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

namespace helix {

Shell::Shell() : running(true) {
    // Initialize readline for advanced autocompletion
    ReadlineSupport::initialize();

    // Initialize environment
    char* home = getenv("HOME");
    if (home) {
        home_directory = home;
    } else {
        // Fallback to getpwuid
        struct passwd* pw = getpwuid(getuid());
        if (pw) {
            home_directory = pw->pw_dir;
        }
    }

    // Set current directory
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd))) {
        current_directory = cwd;
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

    prompt.setHomeDirectory(home_directory);
    prompt.setCurrentDirectory(current_directory);
    prompt.setLastExitStatus(last_exit_status);
}

Shell::~Shell() {
    // Clean up readline
    ReadlineSupport::cleanup();
}

int Shell::run() {
    std::cout << "Helix Shell (helix) v2.0 - Type 'exit' to quit\n";

    while (running) {
        showPrompt();

        std::string input = readInput();

        if (!processInput(input)) {
            break;
        }
    }

    std::cout << "Goodbye!\n";

    return last_exit_status;
}

void Shell::showPrompt() {
    // Update the prompt with current state
    prompt.setCurrentDirectory(current_directory);
    prompt.setLastExitStatus(last_exit_status);

    // Generate and display the prompt
    std::cout << prompt.generate();

    std::cout.flush(); // Ensure prompt is displayed immediately
}

std::string Shell::readInput() {
    // Use readline for advanced autocompletion and history
    std::string line = ReadlineSupport::readLineWithCompletion("");

    if (line.empty() && std::cin.eof()) {
        // EOF (Ctrl+D) detected
        running = false;
        return "exit";
    }

    return line;
}

bool Shell::processInput(const std::string& input) {
    // Skip empty lines
    if (input.empty()) {
        return true;
    }

    // Add to history (basic implementation for now)
    command_history.push_back(input);

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

    // Handle built-in commands at shell level
    if (!parsed_cmd.pipeline.commands.empty() && !parsed_cmd.pipeline.commands[0].args.empty()) {
        const std::string& command = parsed_cmd.pipeline.commands[0].args[0];

        if (command == "cd") {
            // Handle cd command
            std::string new_dir;
            if (parsed_cmd.pipeline.commands[0].args.size() > 1) {
                new_dir = parsed_cmd.pipeline.commands[0].args[1];

                // Handle cd -
                if (new_dir == "-") {
                    char* oldpwd = getenv("OLDPWD");
                    if (oldpwd) {
                        new_dir = oldpwd;
                    } else {
                        std::cerr << "cd: OLDPWD not set\n";
                        return true;
                    }
                }
            } else {
                // Default to home directory
                new_dir = home_directory;
            }

            // Store the old directory
            std::string old_cwd = current_directory;

            if (chdir(new_dir.c_str()) == -1) {
                std::cerr << "cd: " << strerror(errno) << ": " << new_dir << "\n";
            } else {
                // Update current directory
                char cwd[1024];
                if (getcwd(cwd, sizeof(cwd))) {
                    current_directory = cwd;
                }

                // Update environment variables
                setenv("OLDPWD", old_cwd.c_str(), 1);
                setenv("PWD", current_directory.c_str(), 1);

                // For cd -, print the directory we changed to
                if (parsed_cmd.pipeline.commands[0].args.size() > 1 && parsed_cmd.pipeline.commands[0].args[1] == "-") {
                    std::cout << current_directory << "\n";
                }
            }
            return true;
        } else if (command == "history") {
            // Handle history command
            for (size_t i = 0; i < command_history.size(); ++i) {
                std::cout << std::setw(4) << std::setfill(' ') << (i + 1) << "  " << command_history[i] << "\n";
            }
            return true;
        } else if (command == "jobs") {
            // Handle jobs command
            printJobs();
            return true;
        } else if (command == "fg") {
            // Handle fg command
            if (parsed_cmd.pipeline.commands[0].args.size() < 2) {
                std::cerr << "fg: job specification missing\n";
                return true;
            }
            int job_id = std::stoi(parsed_cmd.pipeline.commands[0].args[1]);
            bringToForeground(job_id);
            return true;
        } else if (command == "bg") {
            // Handle bg command
            if (parsed_cmd.pipeline.commands[0].args.size() < 2) {
                std::cerr << "bg: job specification missing\n";
                return true;
            }
            int job_id = std::stoi(parsed_cmd.pipeline.commands[0].args[1]);
            resumeInBackground(job_id);
            return true;
        } else if (command == "exit") {
            // Handle exit command
            last_exit_status = 0;
            if (parsed_cmd.pipeline.commands[0].args.size() > 1) {
                try {
                    last_exit_status = std::stoi(parsed_cmd.pipeline.commands[0].args[1]);
                } catch (const std::exception&) {
                    std::cerr << "exit: numeric argument required\n";
                    return true;
                }
            }
            running = false;
            return false;
        }
    }

    // Execute the command using the executor
    last_exit_status = executor.execute(parsed_cmd);

    return true;
}

void Shell::printJobs() {
    for (const auto& pair : jobs) {
        const Job& job = pair.second;
        std::string status_str = "Running";
        if (job.status == JobStatus::STOPPED) status_str = "Stopped";
        else if (job.status == JobStatus::DONE) status_str = "Done";
        else if (job.status == JobStatus::TERMINATED) status_str = "Terminated";
        std::cout << "[" << job.job_id << "] " << status_str << " " << job.command << "\n";
    }
}

void Shell::bringToForeground(int job_id) {
    auto it = jobs.find(job_id);
    if (it == jobs.end()) {
        std::cerr << "fg: job " << job_id << " not found\n";
        return;
    }
    // Placeholder for now - full implementation needs tcsetpgrp and SIGCONT
    std::cout << "Bringing job " << job_id << " to foreground\n";
}

void Shell::resumeInBackground(int job_id) {
    auto it = jobs.find(job_id);
    if (it == jobs.end()) {
        std::cerr << "bg: job " << job_id << " not found\n";
        return;
    }
    // Placeholder for now - full implementation needs SIGCONT
    std::cout << "Resuming job " << job_id << " in background\n";
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
            dir_path = current_directory + "/" + dir_path;
        }
    }

    // Handle ~ expansion
    if (!prefix.empty() && prefix[0] == '~') {
        dir_path = home_directory;
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

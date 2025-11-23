#include "shell.h" // Includes the Shell class definition, providing run() method for the main REPL loop, prompt display, input reading, and built-in command handling.
#include "tokenizer.h" // Includes the Tokenizer class header, providing tokenize() method to split input strings into tokens (words, pipes, redirections, etc.).
#include "parser.h" // Includes the Parser class header, providing parse() method to convert token sequences into ParsedCommand structures with pipelines and redirections.
#include <iostream> // Provides standard I/O streams: std::cout for displaying prompt, history, and debug output; std::cin for reading user input - essential for REPL interaction.
#include <iomanip> // Provides stream manipulators: std::setw, std::setfill for formatted output, specifically used in history command display.
#include <unistd.h> // Provides Unix system calls: getuid, getcwd, chdir for user and directory management; gethostname for system hostname retrieval - used in prompt building and cd command.
#include <pwd.h> // Provides password database functions: getpwuid for accessing user information struct - used to determine username and home directory.
#include <cstring> // Provides C-style string functions: strerror for converting errno to readable strings - used in error messages for cd command failures.
#include <cstdlib> // Provides general utilities: getenv, setenv for environment variable access and setting (USER, LOGNAME, OLDPWD, PWD) - crucial for prompt and cd functionality.
#include <sys/stat.h> // Provides file status functions: (currently unused directly, but included for potential file operations or directory checks).

namespace hshell {

Shell::Shell() : running(true) {
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

    // Set up basic prompt
    char* user = getenv("USER");
    if (!user) {
        user = getenv("LOGNAME");
    }
    if (!user && getpwuid(getuid())) {
        user = getpwuid(getuid())->pw_name;
    }

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        prompt_format = std::string(user ? user : "user") + "@" + hostname + ":";
    } else {
        prompt_format = std::string(user ? user : "user") + "@hsh:";
    }
}

Shell::~Shell() {
    // Cleanup will be added later
}

int Shell::run() {
    std::cout << "Helix Shell (hsh) v2.0 - Type 'exit' to quit\n";

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
    std::string cwd_display = current_directory;

    // Replace home directory with ~
    if (!home_directory.empty() && current_directory.find(home_directory) == 0) {
        cwd_display = "~" + current_directory.substr(home_directory.length());
    }

    // Shorten long paths (simple truncation for now)
    if (cwd_display.length() > 30) {
        cwd_display = "..." + cwd_display.substr(cwd_display.length() - 27);
    }

    std::cout << prompt_format << cwd_display << "$ ";
    std::cout.flush(); // Ensure prompt is displayed immediately
}

std::string Shell::readInput() {
    std::string line;
    if (!std::getline(std::cin, line)) {
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

    // Debug: print the input we're processing
    std::cout << "Processing input: '" << input << "'\n";
    std::cout << "Trimmed: '" << trimmed << "'\n";

    // Tokenize and parse the command
    auto tokens = tokenizer.tokenize(trimmed);

    std::cout << "Tokens count: " << tokens.size() << "\n";

    // For now, just print the tokens for debugging
    std::cout << "Tokens: ";
    for (size_t idx = 0; idx < tokens.size(); ++idx) {
        const auto& token = tokens[idx];
        if (token.type == TokenType::END_OF_INPUT) {
            std::cout << "[" << idx << ":END]";
            break;
        }
        std::cout << "[" << idx << ":" << (int)token.type << ":" << token.value << "] ";
    }
    std::cout << "\n";



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
    int exit_status = executor.execute(parsed_cmd);

    // For debugging, show exit status
    std::cout << "Command exited with status: " << exit_status << "\n";

    // Future: Execute commands here

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

} // namespace hshell

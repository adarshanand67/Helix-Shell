#include "shell.h"
#include "tokenizer.h"
#include "parser.h"
#include <iostream>
#include <unistd.h>
#include <pwd.h>
#include <cstring>
#include <sys/stat.h>

namespace hshell {

Shell::Shell() : running(true), next_job_id(1) {
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

void Shell::run() {
    std::cout << "Helix Shell (hsh) v2.0 - Type 'exit' to quit\n";

    while (running) {
        showPrompt();

        std::string input = readInput();

        if (!processInput(input)) {
            break;
        }
    }

    std::cout << "Goodbye!\n";
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

    // Check for shell built-ins that need special handling
    if (trimmed == "exit") {
        running = false;
        return false;
    }

    // Parse the tokens into structured command
    ParsedCommand parsed_cmd = parser.parse(tokens);

    // Handle built-in commands at shell level
    if (!parsed_cmd.pipeline.commands.empty() && !parsed_cmd.pipeline.commands[0].args.empty()) {
        const std::string& command = parsed_cmd.pipeline.commands[0].args[0];

        if (command == "cd") {
            // Handle cd command
            std::string new_dir = ".";
            if (parsed_cmd.pipeline.commands[0].args.size() > 1) {
                new_dir = parsed_cmd.pipeline.commands[0].args[1];
            }

            if (chdir(new_dir.c_str()) == -1) {
                std::cerr << "cd: " << strerror(errno) << ": " << new_dir << "\n";
            } else {
                // Update current directory
                char cwd[1024];
                if (getcwd(cwd, sizeof(cwd))) {
                    current_directory = cwd;
                }
            }
            return true;
        }
    }

    // Execute the command using the executor
    int exit_status = executor.execute(parsed_cmd);

    // For debugging, show exit status
    std::cout << "Command exited with status: " << exit_status << "\n";

    // Future: Execute commands here

    return true;
}

} // namespace hshell

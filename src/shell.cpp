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

    // Parse the tokens into structured command
    ParsedCommand parsed_cmd = parser.parse(tokens);

    // Debug: print the parsed command
    std::cout << "Parsed command:\n";
    std::cout << "  Background: " << (parsed_cmd.background ? "true" : "false") << "\n";
    std::cout << "  Pipeline commands: " << parsed_cmd.pipeline.commands.size() << "\n";
    for (size_t i = 0; i < parsed_cmd.pipeline.commands.size(); ++i) {
        const auto& cmd = parsed_cmd.pipeline.commands[i];
        std::cout << "    Command " << i << ":\n";
        std::cout << "      Args: ";
        for (const auto& arg : cmd.args) {
            std::cout << "'" << arg << "' ";
        }
        std::cout << "\n";
        if (!cmd.input_file.empty()) {
            std::cout << "      Input: '" << cmd.input_file << "'\n";
        }
        if (!cmd.output_file.empty()) {
            std::cout << "      Output: '" << cmd.output_file << "' (append: " << cmd.append_mode << ")\n";
        }
        if (!cmd.error_file.empty()) {
            std::cout << "      Error: '" << cmd.error_file << "' (append: " << cmd.error_append_mode << ")\n";
        }
    }
    std::cout << "\n";

    // Handle basic commands
    if (trimmed == "exit") {
        running = false;
        return false;
    }

    // Future: Execute commands here

    return true;
}

} // namespace hshell

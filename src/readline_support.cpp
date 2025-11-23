#include "readline_support.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <cstring>
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

namespace hshell {

// Static member initialization
std::vector<std::string> ReadlineSupport::available_commands;
int ReadlineSupport::completion_index = 0;

// Forward declarations for readline callbacks
static char** completion_function(const char* text, int start, int end);
static char* command_generator(const char* text, int state);
static char* path_generator(const char* text, int state);

void ReadlineSupport::initialize() {
    // Set up readline completion
    rl_attempted_completion_function = completion_function;

    // Set default commands (built-ins)
    available_commands = {"cd", "pwd", "exit", "history", "jobs", "fg", "bg", "help"};

    // Enable tab completion
    rl_bind_key('\t', rl_complete);

    // Configure readline behavior
    rl_completion_append_character = ' ';
    rl_filename_completion_desired = 1;
}

void ReadlineSupport::cleanup() {
    // Clean up readline history
    clear_history();
}

std::string ReadlineSupport::readLineWithCompletion(const std::string& prompt) {
    char* line = readline(prompt.c_str());

    if (!line) {
        return "";  // EOF
    }

    std::string result(line);

    // Add non-empty lines to history
    if (!result.empty() && result != " ") {
        add_history(line);
    }

    free(line);
    return result;
}

void ReadlineSupport::setCommands(const std::vector<std::string>& commands) {
    available_commands = commands;
}

// Completion callback - called by readline
char** completion_function(const char* text, int start, int /* end */) {
    char** matches = nullptr;

    // If at the beginning of the line, complete commands
    if (start == 0) {
        matches = rl_completion_matches(text, command_generator);
    } else {
        // Otherwise, complete file paths
        matches = rl_completion_matches(text, path_generator);
    }

    return matches;
}

// Generate command completions
char* command_generator(const char* text, int state) {
    static size_t list_index, len;

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    // Check built-in commands
    while (list_index < ReadlineSupport::available_commands.size()) {
        const std::string& cmd = ReadlineSupport::available_commands[list_index++];
        if (cmd.substr(0, len) == text) {
            return strdup(cmd.c_str());
        }
    }

    // Check PATH commands
    static std::vector<std::string> path_commands;
    if (state == 0) {
        path_commands.clear();

        const char* path_env = getenv("PATH");
        if (path_env) {
            std::string path_str(path_env);
            size_t start_pos = 0;

            while (start_pos < path_str.length()) {
                size_t colon = path_str.find(':', start_pos);
                if (colon == std::string::npos) {
                    colon = path_str.length();
                }

                std::string dir = path_str.substr(start_pos, colon - start_pos);
                DIR* dirp = opendir(dir.c_str());

                if (dirp) {
                    struct dirent* entry;
                    while ((entry = readdir(dirp)) != nullptr) {
                        std::string name(entry->d_name);
                        if (name[0] != '.' && name.substr(0, len) == text) {
                            std::string full_path = dir + "/" + name;
                            struct stat st;
                            if (stat(full_path.c_str(), &st) == 0 && (st.st_mode & S_IXUSR)) {
                                path_commands.push_back(name);
                            }
                        }
                    }
                    closedir(dirp);
                }

                start_pos = colon + 1;
            }
        }

        ReadlineSupport::completion_index = 0;
    }

    if (ReadlineSupport::completion_index < static_cast<int>(path_commands.size())) {
        return strdup(path_commands[ReadlineSupport::completion_index++].c_str());
    }

    return nullptr;
}

// Generate path completions
char* path_generator(const char* text, int state) {
    static std::vector<std::string> completions;
    static size_t index;

    if (!state) {
        completions.clear();
        index = 0;

        std::string partial(text);
        std::string dir_path = ".";
        std::string prefix = partial;

        size_t last_slash = partial.rfind('/');
        if (last_slash != std::string::npos) {
            dir_path = partial.substr(0, last_slash + 1);
            prefix = partial.substr(last_slash + 1);

            if (dir_path.empty()) {
                dir_path = "/";
            }
        }

        // Handle ~ expansion
        if (!dir_path.empty() && dir_path[0] == '~') {
            const char* home = getenv("HOME");
            if (home) {
                dir_path = std::string(home) + dir_path.substr(1);
            }
        }

        DIR* dirp = opendir(dir_path.c_str());
        if (dirp) {
            struct dirent* entry;
            while ((entry = readdir(dirp)) != nullptr) {
                std::string name(entry->d_name);

                if (name == "." || name == "..") continue;
                if (prefix.empty() || name.substr(0, prefix.length()) == prefix) {
                    std::string full_path = dir_path + "/" + name;
                    struct stat st;

                    std::string completion;
                    if (last_slash != std::string::npos) {
                        completion = partial.substr(0, last_slash + 1) + name;
                    } else {
                        completion = name;
                    }

                    // Add trailing slash for directories
                    if (stat(full_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
                        completion += "/";
                    }

                    completions.push_back(completion);
                }
            }
            closedir(dirp);
        }

        std::sort(completions.begin(), completions.end());
    }

    if (index < completions.size()) {
        return strdup(completions[index++].c_str());
    }

    return nullptr;
}

} // namespace hshell

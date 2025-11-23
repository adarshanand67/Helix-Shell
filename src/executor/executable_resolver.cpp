#include "executor/executable_resolver.h"
#include <sys/stat.h>
#include <cstdlib>

namespace helix {

std::string ExecutableResolver::findExecutable(const std::string& command) const {
    // Check if absolute or relative path
    if (command.find('/') != std::string::npos) {
        // Absolute or relative path - validate directly
        if (isExecutable(command)) {
            return command;
        }
        return "";
    }

    // Search in PATH
    return searchInPath(command);
}

bool ExecutableResolver::isExecutable(const std::string& path) const {
    struct stat st;
    // Check if file exists, is regular file, and is executable
    return (stat(path.c_str(), &st) == 0 &&
            S_ISREG(st.st_mode) &&
            (st.st_mode & S_IXUSR));
}

std::string ExecutableResolver::searchInPath(const std::string& command) const {
    char* path_env = getenv("PATH");
    if (!path_env) {
        return "";
    }

    std::string path_str(path_env);
    size_t start = 0, end = 0;

    while ((end = path_str.find(':', start)) != std::string::npos) {
        std::string dir_path = path_str.substr(start, end - start);
        std::string full_path = dir_path + "/" + command;

        if (isExecutable(full_path)) {
            return full_path;
        }

        start = end + 1;
    }

    // Check last directory
    if (start < path_str.length()) {
        std::string dir_path = path_str.substr(start);
        std::string full_path = dir_path + "/" + command;

        if (isExecutable(full_path)) {
            return full_path;
        }
    }

    return "";
}

} // namespace helix

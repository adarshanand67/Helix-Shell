#ifndef HELIX_EXECUTABLE_RESOLVER_H
#define HELIX_EXECUTABLE_RESOLVER_H

#include <string>

namespace helix {

// ExecutableResolver - Finds and validates executables in PATH
// Responsibilities:
// - Search for executables in PATH directories
// - Validate file permissions and executability
// - Handle absolute and relative paths
class ExecutableResolver {
public:
    ExecutableResolver() = default;
    ~ExecutableResolver() = default;

    // Find executable by searching PATH or validating direct path
    // Returns full path to executable, or empty string if not found
    std::string findExecutable(const std::string& command) const;

private:
    // Check if a file is a regular executable
    bool isExecutable(const std::string& path) const;

    // Search for command in PATH environment variable
    std::string searchInPath(const std::string& command) const;
};

} // namespace helix

#endif // HELIX_EXECUTABLE_RESOLVER_H

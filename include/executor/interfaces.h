#ifndef HELIX_EXECUTOR_INTERFACES_H
#define HELIX_EXECUTOR_INTERFACES_H

#include "types.h"
#include <string>
#include <vector>
#include <functional>
#include <utility>
#include <sys/types.h>

namespace helix {

// Executor Component Interfaces (Dependency Inversion Principle)

/**
 * IExecutableResolver - Interface for executable resolution
 * Abstracts PATH searching and executable validation
 */
class IExecutableResolver {
public:
    virtual ~IExecutableResolver() = default;

    /**
     * Find executable in PATH or validate direct path
     * @param command Command name or path
     * @return Full path to executable, or empty string if not found
     */
    virtual std::string findExecutable(const std::string& command) const = 0;
};

/**
 * IEnvironmentExpander - Interface for environment variable expansion
 * Abstracts variable substitution logic
 */
class IEnvironmentExpander {
public:
    virtual ~IEnvironmentExpander() = default;

    /**
     * Expand environment variables in string
     * Supports $VAR and ${VAR} syntax
     * @param input String with variables
     * @return String with variables expanded
     */
    virtual std::string expand(const std::string& input) const = 0;
};

/**
 * IFileDescriptorManager - Interface for FD redirection management
 * Abstracts file descriptor manipulation and RAII cleanup
 */
class IFileDescriptorManager {
public:
    virtual ~IFileDescriptorManager() = default;

    /**
     * Setup redirections for a command
     * @param cmd Command with redirection specifications
     * @param input_fd Output parameter for input FD
     * @param output_fd Output parameter for output FD
     * @return true on success, false on error
     */
    virtual bool setupRedirections(const Command& cmd, int& input_fd, int& output_fd) = 0;

    /**
     * Restore original file descriptors
     */
    virtual void restoreFileDescriptors() = 0;
};

/**
 * IPipelineManager - Interface for pipeline execution
 * Abstracts multi-command pipeline coordination
 */
class IPipelineManager {
public:
    virtual ~IPipelineManager() = default;

    /**
     * Execute a pipeline of commands
     * @param cmd Parsed command with pipeline
     * @param executor_func Function to execute each command in child process
     * @return Exit status of last command
     */
    virtual int executePipeline(
        const ParsedCommand& cmd,
        std::function<void(const Command&)> executor_func) = 0;
};

} // namespace helix

#endif // HELIX_EXECUTOR_INTERFACES_H

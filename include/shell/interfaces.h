#ifndef HELIX_SHELL_INTERFACES_H
#define HELIX_SHELL_INTERFACES_H

#include "types.h"
#include "shell/shell_state.h"
#include <string>
#include <map>

namespace helix {

// Forward declarations
struct ShellState;
struct ParsedCommand;
struct Job;

// Shell Component Interfaces (Dependency Inversion Principle)

/**
 * IBuiltinCommandHandler - Interface for builtin command handling
 * Implements Strategy pattern for extensible builtins
 */
class IBuiltinCommandHandler {
public:
    virtual ~IBuiltinCommandHandler() = default;

    /**
     * Handle a builtin command
     * @param cmd Parsed command to handle
     * @param state Shell state to modify
     * @return true if command handled successfully
     */
    virtual bool handle(const ParsedCommand& cmd, ShellState& state) = 0;

    /**
     * Check if this handler can handle the command
     * @param command Command name
     * @return true if this handler handles this command
     */
    virtual bool canHandle(const std::string& command) const = 0;
};

/**
 * IBuiltinDispatcher - Interface for dispatching builtin commands
 * Abstracts command routing logic
 */
class IBuiltinDispatcher {
public:
    virtual ~IBuiltinDispatcher() = default;

    /**
     * Dispatch command to appropriate handler
     * @param cmd Parsed command
     * @param state Shell state
     * @return true if command was handled
     */
    virtual bool dispatch(const ParsedCommand& cmd, ShellState& state) = 0;

    /**
     * Check if command is a builtin
     * @param command Command name
     * @return true if builtin
     */
    virtual bool isBuiltin(const std::string& command) const = 0;
};

/**
 * IJobManager - Interface for job control
 * Abstracts background/foreground job management
 */
class IJobManager {
public:
    virtual ~IJobManager() = default;

    /**
     * Add a new job
     * @param pid Process ID
     * @param command Command string
     */
    virtual void addJob(int pid, const std::string& command) = 0;

    /**
     * Remove a job
     * @param job_id Job ID to remove
     */
    virtual void removeJob(int job_id) = 0;

    /**
     * Print all jobs
     */
    virtual void printJobs() const = 0;

    /**
     * Bring job to foreground
     * @param job_id Job ID
     */
    virtual void bringToForeground(int job_id) = 0;

    /**
     * Resume job in background
     * @param job_id Job ID
     */
    virtual void resumeInBackground(int job_id) = 0;

    /**
     * Get jobs map (read-only access)
     * @return Reference to jobs map
     */
    virtual const std::map<int, Job>& getJobs() const = 0;
};

} // namespace helix

#endif // HELIX_SHELL_INTERFACES_H

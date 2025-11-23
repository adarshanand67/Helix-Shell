#ifndef HSHELL_TYPES_H
#define HSHELL_TYPES_H

#include <vector>
#include <string>
#include <map>

namespace hshell {

// Job status enumeration
enum class JobStatus {
    RUNNING,
    STOPPED,
    DONE,
    TERMINATED
};

// Command structure representing a single command with redirections
struct Command {
    std::vector<std::string> args;  // Command arguments, args[0] is the binary
    std::string input_file;         // Input redirection file (<)
    std::string output_file;        // Output redirection file (> or >>)
    bool append_mode = false;       // True if using >> (append), false if > (overwrite)
    std::string error_file;         // Error redirection file (2>)
    bool error_append_mode = false; // True if using 2>> (append error)
    bool background = false;        // True if command should run in background (&)
};

// Job structure for tracking background/foreground processes
struct Job {
    int job_id;
    pid_t pgid = 0;                // Process group ID
    std::string command;            // Original command string
    JobStatus status = JobStatus::RUNNING;
};

// Pipeline structure for a sequence of commands connected by pipes
struct CommandPipeline {
    std::vector<Command> commands;  // Commands in the pipeline
    std::string original_command;   // Original command string
};

// Parsed command line - either a single command or a pipeline with optional background flag
struct ParsedCommand {
    CommandPipeline pipeline;       // The pipeline
    bool background = false;        // True if command should run in background
};

// Token types for the parser
enum class TokenType {
    WORD,
    PIPE,
    REDIRECT_IN,       // <
    REDIRECT_OUT,      // >
    REDIRECT_OUT_APPEND, // >>
    REDIRECT_ERR,      // 2>
    REDIRECT_ERR_APPEND, // 2>>
    BACKGROUND,        // &
    SEMICOLON,         // ;
    END_OF_INPUT       // End of input marker
};

// Token structure
struct Token {
    TokenType type;
    std::string value;
};

} // namespace hshell

#endif // HSHELL_TYPES_H

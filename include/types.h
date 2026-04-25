#ifndef HELIX_TYPES_H
#define HELIX_TYPES_H

#include <vector> // Provides std::vector for storing command arguments in Command struct and sequence of commands in pipelines.
#include <string> // Provides std::string for storing token values, filenames, job commands, and other string data.
#include <map> // Provides std::map for storing key-value pairs in shell environment variables (though currently not used directly here).

namespace helix {

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
    bool stderr_to_stdout = false;  // True if 2>&1
    bool both_to_file = false;      // True if &> or &>>
    bool both_append = false;       // True if &>>
    std::string heredoc_delim;      // Here-doc delimiter (if used)
    std::string heredoc_content;    // Accumulated here-doc content
    bool heredoc_strip = false;     // True if <<-
    std::string herestring;         // Here-string content <<<
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
    REDIRECT_IN,          // <
    REDIRECT_OUT,         // >
    REDIRECT_OUT_APPEND,  // >>
    REDIRECT_ERR,         // 2>
    REDIRECT_ERR_APPEND,  // 2>>
    REDIRECT_ERR_TO_OUT,  // 2>&1
    REDIRECT_BOTH,        // &>
    REDIRECT_BOTH_APPEND, // &>>
    HEREDOC,              // <<
    HEREDOC_STRIP,        // <<-
    HERESTRING,           // <<<
    BACKGROUND,           // &
    SEMICOLON,            // ;
    NEWLINE,              // newline inside multi-line input
    END_OF_INPUT          // End of input marker
};

// Token structure
struct Token {
    TokenType type;
    std::string value;
};

} // namespace helix

#endif // HELIX_TYPES_H

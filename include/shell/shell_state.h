#ifndef HELIX_SHELL_STATE_H
#define HELIX_SHELL_STATE_H

#include "prompt.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <chrono>

namespace helix {

// Forward declarations
class IJobManager;
class Prompt;

// Scoped variable frame for functions
struct VarFrame {
    std::map<std::string, std::string> locals;
};

// Defined shell function
struct ShellFunction {
    std::string name;
    std::vector<std::string> body; // lines
};

// ShellState - Encapsulates all shell state
struct ShellState {
    std::string current_directory;
    std::string home_directory;
    int last_exit_status = 0;
    bool running = true;

    std::vector<std::string> command_history;
    std::vector<std::string> dir_stack;   // pushd/popd stack
    std::map<std::string, std::string> environment;
    std::map<std::string, std::string> aliases;
    std::map<std::string, ShellFunction> functions;
    std::set<std::string> readonly_vars;
    std::set<std::string> integer_vars;
    // Variable stack frames for nested function calls
    std::vector<VarFrame> var_frames;

    // Trap handlers: signal name -> command string
    std::map<std::string, std::string> traps;
    // EXIT trap
    std::string exit_trap;

    // Set by `ai run` — shell REPL picks this up and executes it
    std::string pending_command;

    // Set by `source` — shell REPL executes these lines
    std::vector<std::string> source_lines;
    bool sourcing = false;

    // set -e: exit on non-zero exit status
    bool exit_on_error = false;
    // set -x: trace commands before execution
    bool xtrace = false;
    // set -u: treat unset variables as an error
    bool nounset = false;
    // set -n: noexec (parse but don't run)
    bool noexec = false;
    // set -f: disable globbing
    bool noglob = false;

    // Control flow flags (set by break/continue/return)
    bool breaking = false;
    bool continuing = false;
    bool returning = false;
    int  return_value = 0;
    int  break_depth = 1;    // how many loop levels to break
    int  continue_depth = 1;

    // Script positional parameters ($1, $2, …)
    std::vector<std::string> positional_params;
    // Script name ($0)
    std::string script_name;

    // getopts state
    int optind = 1;
    int optopt = 0;
    std::string optarg;

    // HISTCONTROL: "ignoredups" | "erasedups" | ""
    std::string histcontrol;
    std::string last_histentry;

    // Hash table: command -> full path
    std::map<std::string, std::string> cmd_hash;

    // Job management (uses interface - Dependency Inversion Principle)
    IJobManager* job_manager = nullptr;

    // Prompt
    Prompt* prompt = nullptr;
};

} // namespace helix

#endif // HELIX_SHELL_STATE_H

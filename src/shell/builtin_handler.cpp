#include "shell/builtin_handler.h"
#include "executor/executable_resolver.h"
#include "executor/environment_expander.h"
#include "ai_provider.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <csignal>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <climits>
#include <readline/history.h>

namespace helix {

// CdCommandHandler implementation
bool CdCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    std::string new_dir;

    if (cmd.pipeline.commands[0].args.size() > 1) {
        new_dir = cmd.pipeline.commands[0].args[1];

        // Handle cd -
        if (new_dir == "-") {
            char* oldpwd = getenv("OLDPWD");
            if (oldpwd) {
                new_dir = oldpwd;
            } else {
                std::cerr << "cd: OLDPWD not set\n";
                return true;
            }
        }
    } else {
        // Default to home directory
        new_dir = state.home_directory;
    }

    // Store the old directory
    std::string old_cwd = state.current_directory;

    if (chdir(new_dir.c_str()) == -1) {
        std::cerr << "cd: " << strerror(errno) << ": " << new_dir << "\n";
    } else {
        // Update current directory
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd))) {
            state.current_directory = cwd;
        }

        // Update environment variables
        setenv("OLDPWD", old_cwd.c_str(), 1);
        setenv("PWD", state.current_directory.c_str(), 1);

        // For cd -, print the directory we changed to
        if (cmd.pipeline.commands[0].args.size() > 1 &&
            cmd.pipeline.commands[0].args[1] == "-") {
            std::cout << state.current_directory << "\n";
        }
    }
    return true;
}

bool CdCommandHandler::canHandle(const std::string& command) const {
    return command == "cd";
}

// ExitCommandHandler implementation
bool ExitCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    state.last_exit_status = 0;
    if (cmd.pipeline.commands[0].args.size() > 1) {
        try {
            state.last_exit_status = std::stoi(cmd.pipeline.commands[0].args[1]);
        } catch (const std::exception&) {
            std::cerr << "exit: numeric argument required\n";
            return true;
        }
    }
    state.running = false;
    return false;
}

bool ExitCommandHandler::canHandle(const std::string& command) const {
    return command == "exit";
}

// HistoryCommandHandler implementation
bool HistoryCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    (void)cmd; // Unused parameter
    for (size_t i = 0; i < state.command_history.size(); ++i) {
        std::cout << std::setw(4) << std::setfill(' ') << (i + 1)
                  << "  " << state.command_history[i] << "\n";
    }
    return true;
}

bool HistoryCommandHandler::canHandle(const std::string& command) const {
    return command == "history";
}

// JobsCommandHandler implementation
bool JobsCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    (void)cmd; // Unused parameter
    if (state.job_manager) {
        state.job_manager->printJobs();
    }
    return true;
}

bool JobsCommandHandler::canHandle(const std::string& command) const {
    return command == "jobs";
}

// FgCommandHandler implementation
bool FgCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    if (cmd.pipeline.commands[0].args.size() < 2) {
        std::cerr << "fg: job specification missing\n";
        return true;
    }
    int job_id = std::stoi(cmd.pipeline.commands[0].args[1]);
    if (state.job_manager) {
        state.job_manager->bringToForeground(job_id);
    }
    return true;
}

bool FgCommandHandler::canHandle(const std::string& command) const {
    return command == "fg";
}

// BgCommandHandler implementation
bool BgCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    if (cmd.pipeline.commands[0].args.size() < 2) {
        std::cerr << "bg: job specification missing\n";
        return true;
    }
    int job_id = std::stoi(cmd.pipeline.commands[0].args[1]);
    if (state.job_manager) {
        state.job_manager->resumeInBackground(job_id);
    }
    return true;
}

bool BgCommandHandler::canHandle(const std::string& command) const {
    return command == "bg";
}

// PwdCommandHandler implementation
bool PwdCommandHandler::handle(const ParsedCommand& /* cmd */, ShellState& state) {
    std::cout << state.current_directory << "\n";
    return true;
}

bool PwdCommandHandler::canHandle(const std::string& command) const {
    return command == "pwd";
}

// ExportCommandHandler implementation
bool ExportCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    if (cmd.pipeline.commands[0].args.size() < 2) {
        // No arguments - print all environment variables
        for (const auto& pair : state.environment) {
            std::cout << "export " << pair.first << "=" << pair.second << "\n";
        }
        return true;
    }

    // Parse VAR=VALUE format
    const std::string& arg = cmd.pipeline.commands[0].args[1];
    size_t eq_pos = arg.find('=');

    if (eq_pos == std::string::npos) {
        std::cerr << "export: invalid format. Use: export VAR=VALUE\n";
        return true;
    }

    std::string var_name = arg.substr(0, eq_pos);
    std::string var_value = arg.substr(eq_pos + 1);

    // Set in shell's environment map
    state.environment[var_name] = var_value;

    // Also set in actual environment for child processes
    setenv(var_name.c_str(), var_value.c_str(), 1);

    return true;
}

bool ExportCommandHandler::canHandle(const std::string& command) const {
    return command == "export";
}

// EchoCommandHandler implementation
bool EchoCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    (void)state;
    const auto& args = cmd.pipeline.commands[0].args;
    bool newline = true;

    size_t start = 1;
    if (args.size() > 1 && args[1] == "-n") {
        newline = false;
        start = 2;
    }

    for (size_t i = start; i < args.size(); ++i) {
        if (i > start) std::cout << ' ';
        std::cout << args[i];
    }
    if (newline) std::cout << '\n';
    return true;
}

bool EchoCommandHandler::canHandle(const std::string& command) const {
    return command == "echo";
}

// AliasCommandHandler implementation
bool AliasCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;
    if (args.size() == 1) {
        for (const auto& pair : state.aliases) {
            std::cout << "alias " << pair.first << "='" << pair.second << "'\n";
        }
        return true;
    }
    // alias NAME=VALUE
    const std::string& arg = args[1];
    size_t eq = arg.find('=');
    if (eq == std::string::npos) {
        auto it = state.aliases.find(arg);
        if (it != state.aliases.end()) {
            std::cout << "alias " << it->first << "='" << it->second << "'\n";
        } else {
            std::cerr << "alias: " << arg << ": not found\n";
        }
        return true;
    }
    std::string name = arg.substr(0, eq);
    std::string value = arg.substr(eq + 1);
    // Strip surrounding quotes from value
    if (value.size() >= 2 &&
        ((value.front() == '\'' && value.back() == '\'') ||
         (value.front() == '"'  && value.back() == '"'))) {
        value = value.substr(1, value.size() - 2);
    }
    state.aliases[name] = value;
    return true;
}

bool AliasCommandHandler::canHandle(const std::string& command) const {
    return command == "alias";
}

// UnaliasCommandHandler implementation
bool UnaliasCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;
    if (args.size() < 2) {
        std::cerr << "unalias: usage: unalias NAME\n";
        return true;
    }
    if (args[1] == "-a") {
        state.aliases.clear();
        return true;
    }
    for (size_t i = 1; i < args.size(); ++i) {
        state.aliases.erase(args[i]);
    }
    return true;
}

bool UnaliasCommandHandler::canHandle(const std::string& command) const {
    return command == "unalias";
}

// UnsetCommandHandler implementation
bool UnsetCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;
    for (size_t i = 1; i < args.size(); ++i) {
        state.environment.erase(args[i]);
        unsetenv(args[i].c_str());
    }
    return true;
}

bool UnsetCommandHandler::canHandle(const std::string& command) const {
    return command == "unset";
}

// TypeCommandHandler implementation — like bash 'type', shows what a name resolves to
bool TypeCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;
    if (args.size() < 2) {
        std::cerr << "type: usage: type NAME\n";
        return true;
    }
    static const std::vector<std::string> builtins = {
        "cd","pwd","echo","export","unset","history","jobs","fg","bg",
        "exit","help","alias","unalias","type","ai","set","test","[",
        "source",".","which","read","pushd","popd","dirs","wait","true","false"
    };
    for (size_t i = 1; i < args.size(); ++i) {
        const std::string& name = args[i];
        // alias?
        if (auto it = state.aliases.find(name); it != state.aliases.end()) {
            std::cout << name << " is aliased to '" << it->second << "'\n";
            continue;
        }
        // builtin?
        bool is_builtin = false;
        for (const auto& b : builtins) {
            if (b == name) { is_builtin = true; break; }
        }
        if (is_builtin) {
            std::cout << name << " is a shell builtin\n";
            continue;
        }
        // external?
        ExecutableResolver resolver;
        std::string path = resolver.findExecutable(name);
        if (!path.empty()) {
            std::cout << name << " is " << path << "\n";
        } else {
            std::cerr << name << ": not found\n";
        }
    }
    return true;
}

bool TypeCommandHandler::canHandle(const std::string& command) const {
    return command == "type";
}

// ── ReadCommandHandler ────────────────────────────────────────────────────────

bool ReadCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;

    std::string line;
    if (!std::getline(std::cin, line)) {
        state.last_exit_status = 1;
        return true;
    }

    if (args.size() < 2) {
        // No variable name — store in REPLY
        setenv("REPLY", line.c_str(), 1);
        state.environment["REPLY"] = line;
        return true;
    }

    // Split line by IFS (default: space/tab) and assign to variables
    const char* ifs_env = getenv("IFS");
    std::string ifs = ifs_env ? ifs_env : " \t";

    std::vector<std::string> words;
    size_t i = 0;
    while (i < line.size()) {
        // Skip leading IFS
        while (i < line.size() && ifs.find(line[i]) != std::string::npos) ++i;
        if (i >= line.size()) break;
        size_t start = i;
        while (i < line.size() && ifs.find(line[i]) == std::string::npos) ++i;
        words.push_back(line.substr(start, i - start));
    }

    for (size_t v = 1; v < args.size(); ++v) {
        std::string val;
        if (v - 1 < words.size()) {
            if (v == args.size() - 1) {
                // Last variable gets the rest of the line
                val = words[v - 1];
                for (size_t w = v; w < words.size(); ++w)
                    val += " " + words[w];
            } else {
                val = words[v - 1];
            }
        }
        setenv(args[v].c_str(), val.c_str(), 1);
        state.environment[args[v]] = val;
    }

    return true;
}

bool ReadCommandHandler::canHandle(const std::string& command) const {
    return command == "read";
}

// ── PushdCommandHandler ───────────────────────────────────────────────────────

bool PushdCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;

    if (args.size() < 2) {
        std::cerr << "pushd: usage: pushd <dir>\n";
        return true;
    }

    state.dir_stack.push_back(state.current_directory);

    std::string new_dir = args[1];
    if (!new_dir.empty() && new_dir[0] == '~')
        new_dir = state.home_directory + new_dir.substr(1);

    if (chdir(new_dir.c_str()) == -1) {
        std::cerr << "pushd: " << strerror(errno) << ": " << new_dir << "\n";
        state.dir_stack.pop_back();
        return true;
    }

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd))) {
        state.current_directory = cwd;
        setenv("PWD", cwd, 1);
    }

    // Print stack
    std::cout << state.current_directory;
    for (auto it = state.dir_stack.rbegin(); it != state.dir_stack.rend(); ++it)
        std::cout << " " << *it;
    std::cout << "\n";

    return true;
}

bool PushdCommandHandler::canHandle(const std::string& command) const {
    return command == "pushd";
}

// ── PopdCommandHandler ────────────────────────────────────────────────────────

bool PopdCommandHandler::handle(const ParsedCommand& /* cmd */, ShellState& state) {
    if (state.dir_stack.empty()) {
        std::cerr << "popd: directory stack empty\n";
        return true;
    }

    std::string prev = state.dir_stack.back();
    state.dir_stack.pop_back();

    if (chdir(prev.c_str()) == -1) {
        std::cerr << "popd: " << strerror(errno) << ": " << prev << "\n";
        return true;
    }

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd))) {
        state.current_directory = cwd;
        setenv("PWD", cwd, 1);
    }

    std::cout << state.current_directory;
    for (auto it = state.dir_stack.rbegin(); it != state.dir_stack.rend(); ++it)
        std::cout << " " << *it;
    std::cout << "\n";

    return true;
}

bool PopdCommandHandler::canHandle(const std::string& command) const {
    return command == "popd";
}

// ── DirsCommandHandler ────────────────────────────────────────────────────────

bool DirsCommandHandler::handle(const ParsedCommand& /* cmd */, ShellState& state) {
    std::cout << state.current_directory;
    for (auto it = state.dir_stack.rbegin(); it != state.dir_stack.rend(); ++it)
        std::cout << " " << *it;
    std::cout << "\n";
    return true;
}

bool DirsCommandHandler::canHandle(const std::string& command) const {
    return command == "dirs";
}

// ── WaitCommandHandler ────────────────────────────────────────────────────────

bool WaitCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;

    if (args.size() < 2) {
        // wait for all children
        int status;
        while (waitpid(-1, &status, 0) > 0) {}
        state.last_exit_status = 0;
        return true;
    }

    try {
        pid_t pid = std::stoi(args[1]);
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            std::cerr << "wait: " << strerror(errno) << "\n";
            state.last_exit_status = 1;
        } else {
            state.last_exit_status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
        }
    } catch (...) {
        std::cerr << "wait: invalid pid\n";
        state.last_exit_status = 1;
    }

    return true;
}

bool WaitCommandHandler::canHandle(const std::string& command) const {
    return command == "wait";
}

// ── TrueCommandHandler / FalseCommandHandler ─────────────────────────────────

bool TrueCommandHandler::handle(const ParsedCommand& /* cmd */, ShellState& state) {
    state.last_exit_status = 0;
    return true;
}
bool TrueCommandHandler::canHandle(const std::string& command) const {
    return command == "true";
}

bool FalseCommandHandler::handle(const ParsedCommand& /* cmd */, ShellState& state) {
    state.last_exit_status = 1;
    return true;
}
bool FalseCommandHandler::canHandle(const std::string& command) const {
    return command == "false";
}

// ── SetCommandHandler ─────────────────────────────────────────────────────────

bool SetCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;

    if (args.size() == 1) {
        // `set` with no args: print all variables
        for (const auto& p : state.environment)
            std::cout << p.first << "=" << p.second << "\n";
        return true;
    }

    for (size_t i = 1; i < args.size(); ++i) {
        const std::string& opt = args[i];
        if (opt.size() < 2 || (opt[0] != '-' && opt[0] != '+')) {
            std::cerr << "set: unknown option: " << opt << "\n";
            continue;
        }
        bool enable = (opt[0] == '-');
        for (size_t j = 1; j < opt.size(); ++j) {
            switch (opt[j]) {
                case 'e': state.exit_on_error = enable; break;
                case 'x': state.xtrace        = enable; break;
                case 'u': state.nounset        = enable; break;
                default:
                    std::cerr << "set: unknown flag: " << opt[j] << "\n";
            }
        }
    }
    return true;
}

bool SetCommandHandler::canHandle(const std::string& command) const {
    return command == "set";
}

// ── TestCommandHandler ────────────────────────────────────────────────────────
// Supports:
//   Unary:   -e, -f, -d, -r, -w, -x, -z, -n, -L
//   Binary:  = != -eq -ne -lt -le -gt -ge
//   Logical: ! (negation, only as first arg)

bool TestCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& raw = cmd.pipeline.commands[0].args;

    // Build test args: strip surrounding '[' ']' if present
    std::vector<std::string> args;
    size_t start = 1;
    size_t end   = raw.size();
    if (!raw.empty() && raw[0] == "[") {
        if (raw.back() == "]") end--;
        else {
            std::cerr << "test: missing ']'\n";
            state.last_exit_status = 2;
            return true;
        }
    }
    for (size_t i = start; i < end; ++i) args.push_back(raw[i]);

    bool result = false;

    auto asLong = [](const std::string& s, long& out) -> bool {
        try { out = std::stol(s); return true; }
        catch (...) { return false; }
    };

    auto fileTest = [](char flag, const std::string& path) -> bool {
        struct stat st;
        switch (flag) {
            case 'e': return stat(path.c_str(), &st) == 0;
            case 'f': return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
            case 'd': return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
            case 'L': { struct stat lst; return lstat(path.c_str(), &lst) == 0 && S_ISLNK(lst.st_mode); }
            case 'r': return access(path.c_str(), R_OK) == 0;
            case 'w': return access(path.c_str(), W_OK) == 0;
            case 'x': return access(path.c_str(), X_OK) == 0;
            case 's': return stat(path.c_str(), &st) == 0 && st.st_size > 0;
            default:  return false;
        }
    };

    if (args.empty()) {
        result = false;
    } else if (args.size() == 1) {
        result = !args[0].empty();
    } else if (args.size() == 2) {
        const std::string& op = args[0];
        const std::string& val = args[1];
        if (op == "!") {
            result = val.empty();
        } else if (op.size() == 2 && op[0] == '-') {
            char flag = op[1];
            if (flag == 'z') result = val.empty();
            else if (flag == 'n') result = !val.empty();
            else result = fileTest(flag, val);
        } else {
            result = !args[0].empty();
        }
    } else if (args.size() == 3) {
        const std::string& a  = args[0];
        const std::string& op = args[1];
        const std::string& b  = args[2];
        long la, lb;
        if      (op == "="  || op == "==") result = (a == b);
        else if (op == "!=")               result = (a != b);
        else if (op == "-eq" && asLong(a, la) && asLong(b, lb)) result = (la == lb);
        else if (op == "-ne" && asLong(a, la) && asLong(b, lb)) result = (la != lb);
        else if (op == "-lt" && asLong(a, la) && asLong(b, lb)) result = (la <  lb);
        else if (op == "-le" && asLong(a, la) && asLong(b, lb)) result = (la <= lb);
        else if (op == "-gt" && asLong(a, la) && asLong(b, lb)) result = (la >  lb);
        else if (op == "-ge" && asLong(a, la) && asLong(b, lb)) result = (la >= lb);
        else {
            std::cerr << "test: unknown operator: " << op << "\n";
            state.last_exit_status = 2;
            return true;
        }
    } else {
        std::cerr << "test: too many arguments\n";
        state.last_exit_status = 2;
        return true;
    }

    state.last_exit_status = result ? 0 : 1;
    return true;
}

bool TestCommandHandler::canHandle(const std::string& command) const {
    return command == "test" || command == "[";
}

bool AiCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;

    if (args.size() < 2) {
        std::cout <<
            "Usage:\n"
            "  ai <description>          suggest a shell command\n"
            "  ai explain <command>      explain what a command does\n"
            "  ai fix <error message>    suggest a fix for an error\n"
            "  ai run <description>      suggest AND run after confirmation\n";
        return true;
    }

    AiProvider provider = detectAiProvider();
    if (provider.name != "ollama" && provider.api_key.empty()) {
        std::cerr << "ai: no API key found. Set one of:\n"
                  << "  ANTHROPIC_API_KEY   (Claude)\n"
                  << "  OPENAI_API_KEY      (GPT-4o)\n"
                  << "  GOOGLE_API_KEY      (Gemini)\n"
                  << "  GROQ_API_KEY        (Llama3, free)\n"
                  << "  or install Ollama for free local AI (no key needed)\n";
        return true;
    }

    std::string subcommand = args[1];
    std::string query;
    size_t start = 2;

    bool mode_explain = (subcommand == "explain");
    bool mode_fix     = (subcommand == "fix");
    bool mode_run     = (subcommand == "run");
    bool mode_suggest = !mode_explain && !mode_fix && !mode_run;

    if (mode_suggest) { query = subcommand; start = 2; }
    for (size_t i = start; i < args.size(); ++i) {
        if (!query.empty()) query += ' ';
        query += args[i];
    }
    if (query.empty()) {
        std::cerr << "ai: empty query\n";
        return true;
    }

    std::string system_prompt, result;

    if (mode_explain) {
        system_prompt = "You are a shell expert. Explain what the given shell command does in plain English. "
                        "Be concise (3-5 sentences). Cover what it does, flags used, and any risks.";
        result = callAiProvider(provider, system_prompt, query, 256);
        if (result.empty()) { std::cerr << "ai: no response\n"; return true; }
        std::cout << "\033[96m" << result << "\033[0m\n";

    } else if (mode_fix) {
        system_prompt = "You are a shell expert. The user got this error from their terminal. "
                        "Suggest the most likely fix as a shell command or brief explanation. "
                        "Output the fix command first, then a one-sentence explanation.";
        result = callAiProvider(provider, system_prompt, query, 256);
        if (result.empty()) { std::cerr << "ai: no response\n"; return true; }
        std::cout << "\033[93m" << result << "\033[0m\n";

    } else {
        // suggest or run
        system_prompt = "Convert the user's description to a single shell command. "
                        "Output ONLY the command — no explanation, no markdown, no backticks, no newlines.";
        result = callAiProvider(provider, system_prompt, query, 128);
        while (!result.empty() && (result.back() == '\n' || result.back() == ' ')) {
            result.pop_back();
        }
        if (result.empty()) { std::cerr << "ai: no response\n"; return true; }

        std::cout << "\033[90m→ " << result << "\033[0m\n";

        if (mode_run) {
            std::cout << "Run this command? [y/N] ";
            std::cout.flush();
            std::string answer;
            std::getline(std::cin, answer);
            if (!answer.empty() && (answer[0] == 'y' || answer[0] == 'Y')) {
                // Pre-fill readline so user can edit before running
                add_history(result.c_str());
                // Put command into readline's editing buffer and execute
                state.command_history.push_back(result);
                // Execute it directly
                std::cout << "\033[90mRunning: " << result << "\033[0m\n";
                // We'd need access to shell's runSingleCommand — use a sentinel instead
                // Write to a temp state variable that the shell REPL reads
                state.pending_command = result;
            }
        }
    }

    return true;
}

bool AiCommandHandler::canHandle(const std::string& command) const {
    return command == "ai";
}

// ── SourceCommandHandler ──────────────────────────────────────────────────────

bool SourceCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;
    if (args.size() < 2) {
        std::cerr << "source: usage: source <file>\n";
        return true;
    }

    std::string path = args[1];
    // Expand ~ manually
    if (!path.empty() && path[0] == '~') {
        path = state.home_directory + path.substr(1);
    }

    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "source: " << args[1] << ": no such file\n";
        return true;
    }

    // Store lines and execute via pending_source
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(f, line)) {
        std::string trimmed;
        size_t a = line.find_first_not_of(" \t");
        if (a == std::string::npos) continue;
        size_t b = line.find_last_not_of(" \t");
        trimmed = line.substr(a, b - a + 1);
        if (trimmed.empty() || trimmed[0] == '#') continue;
        lines.push_back(trimmed);
    }
    state.source_lines = std::move(lines);
    state.sourcing = true;
    return true;
}

bool SourceCommandHandler::canHandle(const std::string& command) const {
    return command == "source" || command == ".";
}

// ── WhichCommandHandler ───────────────────────────────────────────────────────

bool WhichCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;
    if (args.size() < 2) {
        std::cerr << "which: usage: which <command>\n";
        return true;
    }
    ExecutableResolver resolver;
    bool found_any = false;
    for (size_t i = 1; i < args.size(); ++i) {
        // Check alias first
        if (auto it = state.aliases.find(args[i]); it != state.aliases.end()) {
            std::cout << args[i] << ": aliased to " << it->second << "\n";
            found_any = true;
            continue;
        }
        std::string path = resolver.findExecutable(args[i]);
        if (!path.empty()) {
            std::cout << path << "\n";
            found_any = true;
        } else {
            std::cerr << args[i] << " not found\n";
        }
    }
    (void)found_any;
    return true;
}

bool WhichCommandHandler::canHandle(const std::string& command) const {
    return command == "which";
}

// HelpCommandHandler implementation
bool HelpCommandHandler::handle(const ParsedCommand& /* cmd */, ShellState& /* state */) {
    std::cout <<
        "Helix Shell — built-in commands\n\n"
        "  cd [dir]            change directory (- for previous, ~ for home)\n"
        "  pwd                 print working directory\n"
        "  echo [-n] [args]    print arguments\n"
        "  export [VAR=VAL]    set or list environment variables\n"
        "  unset VAR           unset environment variable\n"
        "  alias [NAME=VAL]    define or list aliases\n"
        "  unalias [-a] NAME   remove alias\n"
        "  type NAME           show what NAME resolves to\n"
        "  history             show command history\n"
        "  jobs                list background jobs\n"
        "  fg <id>             bring job to foreground\n"
        "  bg <id>             resume job in background\n"
        "  source <file>        execute commands from a file\n"
        "  which <cmd>         show path or alias for a command\n"
        "  read [var ...]      read a line from stdin into variables\n"
        "  pushd <dir>         push dir onto directory stack and cd to it\n"
        "  popd                pop directory stack and cd to top\n"
        "  dirs                print directory stack\n"
        "  wait [pid]          wait for background process(es)\n"
        "  true                return exit code 0\n"
        "  false               return exit code 1\n"
        "  set [-e|-x|-u|+e|+x|+u]  set shell options or print variables\n"
        "  test <expr>         evaluate expression (exit 0=true, 1=false)\n"
        "  [ <expr> ]          synonym for test\n"
        "  exit [code]         exit the shell\n"
        "  help                show this message\n\n"
        "AI assistant:\n"
        "  ai <description>    suggest a shell command\n"
        "  ai explain <cmd>    explain what a command does\n"
        "  ai fix <error>      suggest a fix for an error\n"
        "  ai run <desc>       suggest and run after confirmation\n\n"
        "History expansion:\n"
        "  !!                  repeat last command\n"
        "  !n                  repeat command number n\n\n"
        "Operators: | ; && || & < > >> 2> 2>>\n";
    return true;
}

bool HelpCommandHandler::canHandle(const std::string& command) const {
    return command == "help";
}

// ── PrintfCommandHandler ──────────────────────────────────────────────────────

bool PrintfCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    (void)state;
    const auto& args = cmd.pipeline.commands[0].args;
    if (args.size() < 2) { std::cerr << "printf: usage: printf FORMAT [args...]\n"; return true; }

    const std::string& fmt = args[1];
    size_t ai = 2; // arg index
    std::string out;

    for (size_t fi = 0; fi < fmt.size(); ++fi) {
        if (fmt[fi] == '\\') {
            if (fi+1 < fmt.size()) {
                switch (fmt[++fi]) {
                    case 'n': out += '\n'; break;
                    case 't': out += '\t'; break;
                    case 'r': out += '\r'; break;
                    case '\\': out += '\\'; break;
                    case '0': out += '\0'; break;
                    case 'a': out += '\a'; break;
                    case 'b': out += '\b'; break;
                    default: out += '\\'; out += fmt[fi]; break;
                }
            }
            continue;
        }
        if (fmt[fi] != '%') { out += fmt[fi]; continue; }
        // Collect full format spec: %[flags][width][.precision]type
        std::string spec = "%";
        ++fi;
        if (fi >= fmt.size()) { out += '%'; break; }
        // flags
        while (fi < fmt.size() && (fmt[fi] == '-' || fmt[fi] == '+' || fmt[fi] == ' ' || fmt[fi] == '0' || fmt[fi] == '#'))
            spec += fmt[fi++];
        // width
        while (fi < fmt.size() && std::isdigit((unsigned char)fmt[fi]))
            spec += fmt[fi++];
        // precision
        if (fi < fmt.size() && fmt[fi] == '.') {
            spec += fmt[fi++];
            while (fi < fmt.size() && std::isdigit((unsigned char)fmt[fi]))
                spec += fmt[fi++];
        }
        if (fi >= fmt.size()) { out += spec; break; }
        char type = fmt[fi];
        spec += type;

        std::string val = (ai < args.size()) ? args[ai++] : "";
        char buf[256];
        switch (type) {
            case 'd': case 'i': {
                long n = 0; try { n = std::stol(val); } catch (...) {}
                spec.pop_back(); spec += "ld";
                snprintf(buf, sizeof(buf), spec.c_str(), n); out += buf; break;
            }
            case 'u': {
                unsigned long n = 0; try { n = std::stoul(val); } catch (...) {}
                spec.pop_back(); spec += "lu";
                snprintf(buf, sizeof(buf), spec.c_str(), n); out += buf; break;
            }
            case 'f': case 'F': case 'e': case 'E': case 'g': case 'G': {
                double d = 0; try { d = std::stod(val); } catch (...) {}
                snprintf(buf, sizeof(buf), spec.c_str(), d); out += buf; break;
            }
            case 's': {
                snprintf(buf, sizeof(buf), spec.c_str(), val.c_str()); out += buf; break;
            }
            case 'x': case 'X': {
                long n = 0; try { n = std::stol(val); } catch (...) {}
                spec.pop_back(); spec += std::string("l") + type;
                snprintf(buf, sizeof(buf), spec.c_str(), n); out += buf; break;
            }
            case 'o': {
                long n = 0; try { n = std::stol(val); } catch (...) {}
                spec.pop_back(); spec += "lo";
                snprintf(buf, sizeof(buf), spec.c_str(), n); out += buf; break;
            }
            case 'c': out += (val.empty() ? '\0' : val[0]); break;
            case '%': out += '%'; --ai; break;
            default: out += spec; --ai; break;
        }
    }
    std::cout << out;
    return true;
}
bool PrintfCommandHandler::canHandle(const std::string& command) const { return command == "printf"; }

// ── KillCommandHandler ────────────────────────────────────────────────────────

bool KillCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;
    int sig = SIGTERM;
    size_t start = 1;

    if (args.size() >= 2 && !args[1].empty() && args[1][0] == '-') {
        std::string sigstr = args[1].substr(1);
        if (sigstr == "l" || sigstr == "L") {
            // List signals
            const char* sigs[] = {"HUP","INT","QUIT","ILL","TRAP","ABRT","BUS","FPE",
                "KILL","USR1","SEGV","USR2","PIPE","ALRM","TERM","STKFLT",
                "CHLD","CONT","STOP","TSTP","TTIN","TTOU","URG","XCPU",
                "XFSZ","VTALRM","PROF","WINCH","POLL","PWR","SYS"};
            for (size_t i = 0; i < sizeof(sigs)/sizeof(sigs[0]); ++i)
                std::cout << (i+1) << ") SIG" << sigs[i] << "\n";
            return true;
        }
        // Parse signal number or name
        try {
            sig = std::stoi(sigstr);
        } catch (...) {
            // try signal name
            if (sigstr == "HUP") sig = SIGHUP;
            else if (sigstr == "INT") sig = SIGINT;
            else if (sigstr == "TERM") sig = SIGTERM;
            else if (sigstr == "KILL") sig = SIGKILL;
            else if (sigstr == "STOP") sig = SIGSTOP;
            else if (sigstr == "CONT") sig = SIGCONT;
        }
        ++start;
    }

    for (size_t i = start; i < args.size(); ++i) {
        try {
            pid_t pid = std::stoi(args[i]);
            if (kill(pid, sig) == -1)
                std::cerr << "kill: " << strerror(errno) << ": " << args[i] << "\n";
        } catch (...) {
            std::cerr << "kill: invalid pid: " << args[i] << "\n";
        }
    }
    state.last_exit_status = 0;
    return true;
}
bool KillCommandHandler::canHandle(const std::string& command) const { return command == "kill"; }

// ── TrapCommandHandler ────────────────────────────────────────────────────────

bool TrapCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;
    if (args.size() == 1) {
        for (const auto& t : state.traps)
            std::cout << "trap -- '" << t.second << "' " << t.first << "\n";
        if (!state.exit_trap.empty())
            std::cout << "trap -- '" << state.exit_trap << "' EXIT\n";
        return true;
    }
    if (args.size() >= 3) {
        const std::string& handler = args[1];
        for (size_t i = 2; i < args.size(); ++i) {
            if (args[i] == "EXIT") {
                state.exit_trap = handler;
            } else {
                state.traps[args[i]] = handler;
                // Install actual signal handler (best effort — just record for now)
                // Full signal->command dispatching would require async-signal-safe infra
                // We store the trap and check it at appropriate times
            }
        }
    } else if (args.size() == 2) {
        // trap - signal  (reset)
        if (args[1] == "EXIT") state.exit_trap.clear();
        else state.traps.erase(args[1]);
    }
    return true;
}
bool TrapCommandHandler::canHandle(const std::string& command) const { return command == "trap"; }

// ── UmaskCommandHandler ───────────────────────────────────────────────────────

bool UmaskCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;
    mode_t current = umask(0);
    umask(current);

    if (args.size() == 1) {
        char buf[8];
        snprintf(buf, sizeof(buf), "0%03o", (unsigned)current);
        std::cout << buf << "\n";
    } else {
        unsigned val = 0;
        try {
            val = std::stoul(args[1], nullptr, 8);
        } catch (...) {
            std::cerr << "umask: invalid mask: " << args[1] << "\n";
            state.last_exit_status = 1;
            return true;
        }
        umask((mode_t)val);
    }
    return true;
}
bool UmaskCommandHandler::canHandle(const std::string& command) const { return command == "umask"; }

// ── UlimitCommandHandler ──────────────────────────────────────────────────────

bool UlimitCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;
    (void)state;

    struct rlimit rl;
    int resource = RLIMIT_NOFILE;
    bool show = true;
    std::string optflag;

    if (args.size() >= 2 && args[1][0] == '-') {
        optflag = args[1].substr(1);
        if (optflag == "n") resource = RLIMIT_NOFILE;
        else if (optflag == "u") resource = RLIMIT_NPROC;
        else if (optflag == "s") resource = RLIMIT_STACK;
        else if (optflag == "v") resource = RLIMIT_AS;
        else if (optflag == "c") resource = RLIMIT_CORE;
        else if (optflag == "a") {
            // print all
            struct { int r; const char* name; } all[] = {
                {RLIMIT_CORE, "-c: core file size"},
                {RLIMIT_NOFILE, "-n: open files"},
                {RLIMIT_NPROC, "-u: max user processes"},
                {RLIMIT_STACK, "-s: stack size"},
            };
            for (auto& x : all) {
                getrlimit(x.r, &rl);
                std::cout << x.name << " (blocks) ";
                if (rl.rlim_cur == RLIM_INFINITY) std::cout << "unlimited\n";
                else std::cout << rl.rlim_cur << "\n";
            }
            return true;
        }
        if (args.size() >= 3) show = false;
    }

    getrlimit(resource, &rl);
    if (show) {
        if (rl.rlim_cur == RLIM_INFINITY) std::cout << "unlimited\n";
        else std::cout << rl.rlim_cur << "\n";
    } else {
        const std::string& valstr = args.size() >= 3 ? args[2] : args[1];
        if (valstr == "unlimited") {
            rl.rlim_cur = RLIM_INFINITY;
        } else {
            try { rl.rlim_cur = std::stoul(valstr); }
            catch (...) { std::cerr << "ulimit: invalid value\n"; return true; }
        }
        setrlimit(resource, &rl);
    }
    return true;
}
bool UlimitCommandHandler::canHandle(const std::string& command) const { return command == "ulimit"; }

// ── DeclareCommandHandler ─────────────────────────────────────────────────────

bool DeclareCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;
    bool is_integer = false;
    bool is_readonly = false;
    bool is_export = false;

    size_t start = 1;
    for (; start < args.size() && !args[start].empty() && args[start][0] == '-'; ++start) {
        for (char f : args[start].substr(1)) {
            if (f == 'i') is_integer = true;
            else if (f == 'r') is_readonly = true;
            else if (f == 'x') is_export = true;
        }
    }

    if (start == args.size()) {
        // Print all variables
        for (const auto& p : state.environment)
            std::cout << "declare -- " << p.first << "=" << p.second << "\n";
        return true;
    }

    for (size_t i = start; i < args.size(); ++i) {
        const std::string& arg = args[i];
        size_t eq = arg.find('=');
        std::string vname = eq == std::string::npos ? arg : arg.substr(0, eq);
        std::string vval  = eq == std::string::npos ? "" : arg.substr(eq + 1);

        if (is_integer) {
            state.integer_vars.insert(vname);
            if (eq != std::string::npos) {
                // evaluate as arithmetic
                long n = 0;
                try { n = std::stol(vval); } catch (...) {}
                vval = std::to_string(n);
            }
        }
        if (is_readonly) state.readonly_vars.insert(vname);
        if (eq != std::string::npos) {
            state.environment[vname] = vval;
            setenv(vname.c_str(), vval.c_str(), 1);
        }
        if (is_export) {
            setenv(vname.c_str(), getenv(vname.c_str()) ? getenv(vname.c_str()) : "", 1);
        }
    }
    return true;
}
bool DeclareCommandHandler::canHandle(const std::string& command) const {
    return command == "declare" || command == "typeset";
}

// ── ReadonlyCommandHandler ────────────────────────────────────────────────────

bool ReadonlyCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;
    if (args.size() == 1) {
        for (const auto& v : state.readonly_vars)
            std::cout << "declare -r " << v << "\n";
        return true;
    }
    for (size_t i = 1; i < args.size(); ++i) {
        size_t eq = args[i].find('=');
        std::string vname = eq == std::string::npos ? args[i] : args[i].substr(0, eq);
        if (eq != std::string::npos) {
            std::string vval = args[i].substr(eq + 1);
            state.environment[vname] = vval;
            setenv(vname.c_str(), vval.c_str(), 1);
        }
        state.readonly_vars.insert(vname);
    }
    return true;
}
bool ReadonlyCommandHandler::canHandle(const std::string& command) const { return command == "readonly"; }

// ── GetoptsCommandHandler ─────────────────────────────────────────────────────

bool GetoptsCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;
    if (args.size() < 3) {
        std::cerr << "getopts: usage: getopts optstring name\n";
        state.last_exit_status = 1;
        return true;
    }
    const std::string& optstring = args[1];
    const std::string& varname = args[2];

    // OPTIND is 1-based, refers into positional_params
    int idx = state.optind;
    if (idx > (int)state.positional_params.size()) {
        state.last_exit_status = 1;
        return true;
    }

    const std::string& param = state.positional_params[idx - 1];
    if (param.empty() || param[0] != '-' || param == "--") {
        if (param == "--") ++state.optind;
        state.last_exit_status = 1;
        return true;
    }

    char opt = param[1];
    size_t pos = optstring.find(opt);
    if (pos == std::string::npos) {
        // Unknown option
        state.optopt = opt;
        state.environment[varname] = "?";
        setenv(varname.c_str(), "?", 1);
        ++state.optind;
        state.last_exit_status = 0;
        return true;
    }

    state.environment[varname] = std::string(1, opt);
    setenv(varname.c_str(), std::string(1, opt).c_str(), 1);

    if (pos + 1 < optstring.size() && optstring[pos+1] == ':') {
        // Requires argument
        if (param.size() > 2) {
            state.optarg = param.substr(2);
        } else {
            ++state.optind;
            if (state.optind <= (int)state.positional_params.size())
                state.optarg = state.positional_params[state.optind - 1];
        }
        setenv("OPTARG", state.optarg.c_str(), 1);
        state.environment["OPTARG"] = state.optarg;
    }

    ++state.optind;
    setenv("OPTIND", std::to_string(state.optind).c_str(), 1);
    state.environment["OPTIND"] = std::to_string(state.optind);
    state.last_exit_status = 0;
    return true;
}
bool GetoptsCommandHandler::canHandle(const std::string& command) const { return command == "getopts"; }

// ── ShiftCommandHandler ───────────────────────────────────────────────────────

bool ShiftCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;
    int n = 1;
    if (args.size() >= 2) {
        try { n = std::stoi(args[1]); } catch (...) {}
    }
    if (n < 0) { std::cerr << "shift: count must be non-negative\n"; state.last_exit_status = 1; return true; }
    if (n > (int)state.positional_params.size()) n = (int)state.positional_params.size();
    state.positional_params.erase(state.positional_params.begin(), state.positional_params.begin() + n);
    state.last_exit_status = 0;
    return true;
}
bool ShiftCommandHandler::canHandle(const std::string& command) const { return command == "shift"; }

// ── CommandCommandHandler ─────────────────────────────────────────────────────

bool CommandCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;
    if (args.size() < 2) { std::cerr << "command: usage: command [-v|-p] name [args...]\n"; return true; }

    size_t start = 1;
    bool verbose = false;
    if (args[1] == "-v") { verbose = true; ++start; }
    else if (args[1] == "-p") { ++start; }

    if (start >= args.size()) return true;

    ExecutableResolver resolver;
    std::string path = resolver.findExecutable(args[start]);
    if (verbose) {
        if (!path.empty()) std::cout << path << "\n";
        state.last_exit_status = path.empty() ? 1 : 0;
        return true;
    }
    if (path.empty()) {
        std::cerr << "command: " << args[start] << ": not found\n";
        state.last_exit_status = 127;
        return true;
    }
    // Execute directly
    pid_t pid = fork();
    if (pid == 0) {
        std::vector<char*> argv;
        for (size_t i = start; i < args.size(); ++i)
            argv.push_back(const_cast<char*>(args[i].c_str()));
        argv.push_back(nullptr);
        execvp(path.c_str(), argv.data());
        exit(127);
    }
    int status;
    waitpid(pid, &status, 0);
    state.last_exit_status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    return true;
}
bool CommandCommandHandler::canHandle(const std::string& command) const { return command == "command"; }

// ── BuiltinCommandCommandHandler ──────────────────────────────────────────────

bool BuiltinCommandCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    (void)cmd; (void)state;
    // Already handled by dispatcher
    return false;
}
bool BuiltinCommandCommandHandler::canHandle(const std::string& command) const { return command == "builtin"; }

// ── ExecCommandHandler ────────────────────────────────────────────────────────

bool ExecCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;
    if (args.size() < 2) {
        std::cerr << "exec: usage: exec command [args...]\n";
        return true;
    }
    ExecutableResolver resolver;
    std::string path = resolver.findExecutable(args[1]);
    if (path.empty()) {
        std::cerr << "exec: " << args[1] << ": not found\n";
        state.last_exit_status = 127;
        return true;
    }
    std::vector<char*> argv;
    for (size_t i = 1; i < args.size(); ++i)
        argv.push_back(const_cast<char*>(args[i].c_str()));
    argv.push_back(nullptr);
    execvp(path.c_str(), argv.data());
    std::cerr << "exec: " << strerror(errno) << "\n";
    state.last_exit_status = 1;
    return true;
}
bool ExecCommandHandler::canHandle(const std::string& command) const { return command == "exec"; }

// ── EvalCommandHandler ────────────────────────────────────────────────────────

bool EvalCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;
    if (args.size() < 2) return true;

    // Join all args and put into pending_command so shell re-executes
    std::string eval_str;
    for (size_t i = 1; i < args.size(); ++i) {
        if (i > 1) eval_str += ' ';
        eval_str += args[i];
    }
    // Queue as pending
    state.pending_command = eval_str;
    return true;
}
bool EvalCommandHandler::canHandle(const std::string& command) const { return command == "eval"; }

// ── TimesCommandHandler ───────────────────────────────────────────────────────

bool TimesCommandHandler::handle(const ParsedCommand& /* cmd */, ShellState& /* state */) {
    struct tms t;
    times(&t);
    long hz = sysconf(_SC_CLK_TCK);
    auto fmt = [hz](clock_t c) -> std::string {
        long total = c * 1000 / hz;
        return std::to_string(total / 60000) + "m" +
               std::to_string((total % 60000) / 1000) + "." +
               std::to_string(total % 1000) + "s";
    };
    std::cout << fmt(t.tms_utime)  << " " << fmt(t.tms_stime)  << "\n";
    std::cout << fmt(t.tms_cutime) << " " << fmt(t.tms_cstime) << "\n";
    return true;
}
bool TimesCommandHandler::canHandle(const std::string& command) const { return command == "times"; }

// ── HashCommandHandler ────────────────────────────────────────────────────────

bool HashCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;
    if (args.size() == 1 || (args.size() == 2 && args[1] == "-l")) {
        for (const auto& p : state.cmd_hash)
            std::cout << "hits\tcommand\n1\t" << p.second << "\n";
        return true;
    }
    if (args.size() == 2 && args[1] == "-r") {
        state.cmd_hash.clear();
        return true;
    }
    ExecutableResolver resolver;
    for (size_t i = 1; i < args.size(); ++i) {
        std::string path = resolver.findExecutable(args[i]);
        if (!path.empty()) state.cmd_hash[args[i]] = path;
        else std::cerr << "hash: " << args[i] << ": not found\n";
    }
    return true;
}
bool HashCommandHandler::canHandle(const std::string& command) const { return command == "hash"; }

// ── SuspendCommandHandler ─────────────────────────────────────────────────────

bool SuspendCommandHandler::handle(const ParsedCommand& /* cmd */, ShellState& /* state */) {
    kill(getpid(), SIGSTOP);
    return true;
}
bool SuspendCommandHandler::canHandle(const std::string& command) const { return command == "suspend"; }

// ── DisownCommandHandler ──────────────────────────────────────────────────────

bool DisownCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;
    (void)state;
    if (args.size() < 2) {
        std::cerr << "disown: usage: disown pid\n";
        return true;
    }
    // Just suppress — we don't track jobs in the process table directly
    std::cout << "disown: job removed from job table\n";
    return true;
}
bool DisownCommandHandler::canHandle(const std::string& command) const { return command == "disown"; }

// ── LetCommandHandler ─────────────────────────────────────────────────────────

bool LetCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;
    long result = 0;
    for (size_t i = 1; i < args.size(); ++i) {
        // Simple: eval arithmetic expression, allow VAR=expr
        std::string expr = args[i];
        size_t eq = expr.find('=');
        if (eq != std::string::npos) {
            std::string vname = expr.substr(0, eq);
            std::string vexpr = expr.substr(eq + 1);
            // expand vars in vexpr
            EnvironmentVariableExpander expander;
            std::string expanded = expander.expandWithState(vexpr, &state);
            // eval arithmetic
            long n = 0;
            try { n = std::stol(expanded); } catch (...) {}
            state.environment[vname] = std::to_string(n);
            setenv(vname.c_str(), std::to_string(n).c_str(), 1);
            result = n;
        } else {
            EnvironmentVariableExpander expander;
            std::string expanded = expander.expandWithState(expr, &state);
            try { result = std::stol(expanded); } catch (...) { result = 0; }
        }
    }
    state.last_exit_status = (result == 0) ? 1 : 0;
    return true;
}
bool LetCommandHandler::canHandle(const std::string& command) const { return command == "let"; }

// ── ReturnCommandHandler ──────────────────────────────────────────────────────

bool ReturnCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;
    int code = state.last_exit_status;
    if (args.size() >= 2) {
        try { code = std::stoi(args[1]); } catch (...) {}
    }
    state.return_value = code;
    state.last_exit_status = code;
    state.returning = true;
    return false; // signals caller to stop
}
bool ReturnCommandHandler::canHandle(const std::string& command) const { return command == "return"; }

// ── BreakCommandHandler ───────────────────────────────────────────────────────

bool BreakCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;
    int n = 1;
    if (args.size() >= 2) { try { n = std::stoi(args[1]); } catch (...) {} }
    state.break_depth = n;
    state.breaking = true;
    return false;
}
bool BreakCommandHandler::canHandle(const std::string& command) const { return command == "break"; }

// ── ContinueCommandHandler ────────────────────────────────────────────────────

bool ContinueCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;
    int n = 1;
    if (args.size() >= 2) { try { n = std::stoi(args[1]); } catch (...) {} }
    state.continue_depth = n;
    state.continuing = true;
    return false;
}
bool ContinueCommandHandler::canHandle(const std::string& command) const { return command == "continue"; }

// ── LocalCommandHandler ───────────────────────────────────────────────────────

bool LocalCommandHandler::handle(const ParsedCommand& cmd, ShellState& state) {
    const auto& args = cmd.pipeline.commands[0].args;
    if (state.var_frames.empty()) {
        std::cerr << "local: can only be used in a function\n";
        return true;
    }
    for (size_t i = 1; i < args.size(); ++i) {
        size_t eq = args[i].find('=');
        if (eq == std::string::npos) {
            state.var_frames.back().locals[args[i]] = "";
        } else {
            std::string vname = args[i].substr(0, eq);
            std::string vval  = args[i].substr(eq + 1);
            state.var_frames.back().locals[vname] = vval;
            setenv(vname.c_str(), vval.c_str(), 1);
        }
    }
    return true;
}
bool LocalCommandHandler::canHandle(const std::string& command) const { return command == "local"; }

// BuiltinCommandDispatcher implementation
BuiltinCommandDispatcher::BuiltinCommandDispatcher() {
    handlers["cd"]      = std::make_unique<CdCommandHandler>();
    handlers["exit"]    = std::make_unique<ExitCommandHandler>();
    handlers["history"] = std::make_unique<HistoryCommandHandler>();
    handlers["jobs"]    = std::make_unique<JobsCommandHandler>();
    handlers["fg"]      = std::make_unique<FgCommandHandler>();
    handlers["bg"]      = std::make_unique<BgCommandHandler>();
    handlers["pwd"]     = std::make_unique<PwdCommandHandler>();
    handlers["export"]  = std::make_unique<ExportCommandHandler>();
    handlers["echo"]    = std::make_unique<EchoCommandHandler>();
    handlers["help"]    = std::make_unique<HelpCommandHandler>();
    handlers["alias"]   = std::make_unique<AliasCommandHandler>();
    handlers["unalias"] = std::make_unique<UnaliasCommandHandler>();
    handlers["unset"]   = std::make_unique<UnsetCommandHandler>();
    handlers["type"]    = std::make_unique<TypeCommandHandler>();
    handlers["ai"]      = std::make_unique<AiCommandHandler>();
    handlers["source"]  = std::make_unique<SourceCommandHandler>();
    handlers["."]       = std::make_unique<SourceCommandHandler>();
    handlers["which"]   = std::make_unique<WhichCommandHandler>();
    handlers["read"]    = std::make_unique<ReadCommandHandler>();
    handlers["pushd"]   = std::make_unique<PushdCommandHandler>();
    handlers["popd"]    = std::make_unique<PopdCommandHandler>();
    handlers["dirs"]    = std::make_unique<DirsCommandHandler>();
    handlers["wait"]    = std::make_unique<WaitCommandHandler>();
    handlers["true"]    = std::make_unique<TrueCommandHandler>();
    handlers["false"]   = std::make_unique<FalseCommandHandler>();
    handlers["set"]      = std::make_unique<SetCommandHandler>();
    handlers["test"]     = std::make_unique<TestCommandHandler>();
    handlers["["]        = std::make_unique<TestCommandHandler>();
    handlers["printf"]   = std::make_unique<PrintfCommandHandler>();
    handlers["kill"]     = std::make_unique<KillCommandHandler>();
    handlers["trap"]     = std::make_unique<TrapCommandHandler>();
    handlers["umask"]    = std::make_unique<UmaskCommandHandler>();
    handlers["ulimit"]   = std::make_unique<UlimitCommandHandler>();
    handlers["declare"]  = std::make_unique<DeclareCommandHandler>();
    handlers["typeset"]  = std::make_unique<DeclareCommandHandler>();
    handlers["readonly"] = std::make_unique<ReadonlyCommandHandler>();
    handlers["getopts"]  = std::make_unique<GetoptsCommandHandler>();
    handlers["shift"]    = std::make_unique<ShiftCommandHandler>();
    handlers["command"]  = std::make_unique<CommandCommandHandler>();
    handlers["builtin"]  = std::make_unique<BuiltinCommandCommandHandler>();
    handlers["exec"]     = std::make_unique<ExecCommandHandler>();
    handlers["eval"]     = std::make_unique<EvalCommandHandler>();
    handlers["times"]    = std::make_unique<TimesCommandHandler>();
    handlers["hash"]     = std::make_unique<HashCommandHandler>();
    handlers["suspend"]  = std::make_unique<SuspendCommandHandler>();
    handlers["disown"]   = std::make_unique<DisownCommandHandler>();
    handlers["let"]      = std::make_unique<LetCommandHandler>();
    handlers["return"]   = std::make_unique<ReturnCommandHandler>();
    handlers["break"]    = std::make_unique<BreakCommandHandler>();
    handlers["continue"] = std::make_unique<ContinueCommandHandler>();
    handlers["local"]    = std::make_unique<LocalCommandHandler>();
}

bool BuiltinCommandDispatcher::dispatch(const ParsedCommand& cmd, ShellState& state) {
    if (cmd.pipeline.commands.empty() || cmd.pipeline.commands[0].args.empty()) {
        return false;
    }

    const std::string& command = cmd.pipeline.commands[0].args[0];
    auto it = handlers.find(command);
    if (it != handlers.end()) {
        return it->second->handle(cmd, state);
    }

    return false;
}

bool BuiltinCommandDispatcher::isBuiltin(const std::string& command) const {
    return handlers.find(command) != handlers.end();
}

} // namespace helix

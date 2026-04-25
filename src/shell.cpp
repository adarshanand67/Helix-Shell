#include "shell.h"
#include "tokenizer.h"
#include "parser.h"
#include "readline_support.h"
#include "executor/environment_expander.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <pwd.h>
#include <cstring>
#include <cstdlib>
#include <csignal>
#include <chrono>
#include <sys/wait.h>
#include <fnmatch.h>

namespace helix {

static IJobManager* g_job_manager = nullptr;

static void sigchld_handler(int /* sig */) {
    int saved_errno = errno;
    if (g_job_manager) {
        static_cast<JobManager*>(g_job_manager)->checkCompletedJobs();
    }
    errno = saved_errno;
}

Shell::Shell()
    : builtin_dispatcher(std::make_unique<BuiltinCommandDispatcher>()),
      job_manager(std::make_unique<JobManager>()) {

    state.running = true;
    state.job_manager = job_manager.get();
    state.prompt = &prompt;

    g_job_manager = job_manager.get();

    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, nullptr) == -1) {
        std::cerr << "Warning: Failed to set up SIGCHLD handler\n";
    }

    ReadlineSupport::initialize();

    char* home = getenv("HOME");
    if (home) {
        state.home_directory = home;
    } else {
        struct passwd* pw = getpwuid(getuid());
        if (pw) state.home_directory = pw->pw_dir;
    }

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd))) state.current_directory = cwd;

    char* user = getenv("USER");
    if (!user) user = getenv("LOGNAME");
    if (!user && getpwuid(getuid())) user = getpwuid(getuid())->pw_name;

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        prompt.setUserHost(user ? user : "user", hostname);
    } else {
        prompt.setUserHost(user ? user : "user", "helix");
    }

    prompt.setHomeDirectory(state.home_directory);
    prompt.setCurrentDirectory(state.current_directory);
    prompt.setLastExitStatus(state.last_exit_status);

    // HISTCONTROL from environment
    const char* hc = getenv("HISTCONTROL");
    if (hc) state.histcontrol = hc;

    loadHistory();
    loadRcFile();
}

Shell::~Shell() {
    // Run EXIT trap if set
    if (!state.exit_trap.empty()) {
        runSingleCommand(state.exit_trap);
    }
    saveHistory();
    g_job_manager = nullptr;
    ReadlineSupport::cleanup();
}

// ── History persistence ──────────────────────────────────────────────────────

void Shell::loadHistory() {
    std::string path = state.home_directory + "/.helix_history";
    std::ifstream f(path);
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty()) {
            state.command_history.push_back(line);
            add_history(line.c_str());
        }
    }
}

void Shell::saveHistory() {
    std::string path = state.home_directory + "/.helix_history";
    std::ofstream f(path, std::ios::app);
    for (const auto& cmd : session_history_) {
        f << cmd << '\n';
    }
}

// ── RC file ──────────────────────────────────────────────────────────────────

void Shell::loadRcFile() {
    std::string path = state.home_directory + "/.helixrc";
    std::ifstream f(path);
    if (!f.is_open()) return;

    std::string line;
    while (std::getline(f, line)) {
        std::string trimmed = trimWhitespace(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;
        runSingleCommand(trimmed);
    }
}

// ── REPL ─────────────────────────────────────────────────────────────────────

int Shell::run() {
    std::cout << "Helix Shell v1.0.0  (type 'help' for commands, 'ai <query>' for AI assist)\n";

    while (state.running) {
        if (job_manager) {
            static_cast<JobManager*>(job_manager.get())->printAndCleanCompletedJobs();
        }

        // PROMPT_COMMAND — run a command before showing prompt
        const char* pc = getenv("PROMPT_COMMAND");
        if (pc && *pc) {
            runSingleCommand(std::string(pc));
        }

        showPrompt();

        std::string input = readInput();

        auto t0 = std::chrono::steady_clock::now();
        bool ok = processInput(input);
        auto t1 = std::chrono::steady_clock::now();
        last_duration_ = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0);

        if (state.sourcing) {
            state.sourcing = false;
            for (const auto& line : state.source_lines) {
                if (!runSingleCommand(line)) break;
                if (!state.running) break;
            }
            state.source_lines.clear();
        }

        if (!state.pending_command.empty()) {
            std::string pending = state.pending_command;
            state.pending_command.clear();
            runSingleCommand(pending);
        }

        if (!ok) break;
    }

    std::cout << "Goodbye!\n";
    return state.last_exit_status;
}

void Shell::showPrompt() {
    prompt.setCurrentDirectory(state.current_directory);
    prompt.setLastExitStatus(state.last_exit_status);
    prompt.setLastCommandDuration(last_duration_);
    std::cout << prompt.generate();
    std::cout.flush();
}

std::string Shell::readInput() {
    // Check if we're inside a multi-line block (accumulating lines)
    if (!block_stack_.empty()) {
        std::string line = ReadlineSupport::readLineWithCompletion("> ");
        if (line.empty() && std::cin.eof()) {
            state.running = false;
            return "";
        }
        return line;
    }

    std::string line = ReadlineSupport::readLineWithCompletion("");
    if (line.empty() && std::cin.eof()) {
        state.running = false;
        return "exit";
    }
    return line;
}

// ── Input helpers ────────────────────────────────────────────────────────────

std::string Shell::trimWhitespace(const std::string& s) {
    size_t a = s.find_first_not_of(" \t");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t");
    return s.substr(a, b - a + 1);
}

enum class SplitOp { SEQ, AND, OR };
struct Fragment { std::string text; SplitOp op; };

static std::vector<Fragment> splitLogical(const std::string& input) {
    std::vector<Fragment> frags;
    std::string current;
    bool in_single = false, in_double = false;
    int brace_depth = 0; // {} depth — don't split inside function bodies
    size_t i = 0;

    auto push = [&](SplitOp op) {
        frags.push_back({current, op});
        current.clear();
    };

    while (i < input.size()) {
        char c = input[i];
        if      (c == '\'' && !in_double) { in_single = !in_single; current += c; ++i; }
        else if (c == '"'  && !in_single) { in_double = !in_double; current += c; ++i; }
        else if (!in_single && !in_double && c == '{') { ++brace_depth; current += c; ++i; }
        else if (!in_single && !in_double && c == '}') { --brace_depth; if (brace_depth < 0) brace_depth = 0; current += c; ++i; }
        else if (!in_single && !in_double && brace_depth == 0 && c == '&' && i+1 < input.size() && input[i+1] == '&') {
            push(SplitOp::AND); i += 2;
        } else if (!in_single && !in_double && brace_depth == 0 && c == '|' && i+1 < input.size() && input[i+1] == '|') {
            push(SplitOp::OR); i += 2;
        } else if (!in_single && !in_double && brace_depth == 0 && c == ';') {
            push(SplitOp::SEQ); ++i;
        } else {
            current += c; ++i;
        }
    }
    frags.push_back({current, SplitOp::SEQ});
    return frags;
}

// ── Brace expansion {a,b,c} and {1..5} ───────────────────────────────────────

static std::vector<std::string> braceExpand(const std::string& word) {
    // Find outermost unquoted { ... }
    size_t open = std::string::npos;
    int depth = 0;
    bool in_sq = false, in_dq = false;
    for (size_t i = 0; i < word.size(); ++i) {
        char c = word[i];
        if (c == '\'' && !in_dq) { in_sq = !in_sq; continue; }
        if (c == '"' && !in_sq) { in_dq = !in_dq; continue; }
        if (in_sq || in_dq) continue;
        if (c == '{') { if (depth == 0) open = i; ++depth; }
        else if (c == '}') { --depth; if (depth == 0 && open != std::string::npos) break; }
    }
    if (open == std::string::npos || depth != 0) return {word};

    size_t close = std::string::npos;
    depth = 0;
    in_sq = in_dq = false;
    for (size_t i = open; i < word.size(); ++i) {
        char c = word[i];
        if (c == '\'' && !in_dq) { in_sq = !in_sq; continue; }
        if (c == '"' && !in_sq) { in_dq = !in_dq; continue; }
        if (in_sq || in_dq) continue;
        if (c == '{') ++depth;
        else if (c == '}') { --depth; if (depth == 0) { close = i; break; } }
    }
    if (close == std::string::npos) return {word};

    std::string pre  = word.substr(0, open);
    std::string inner = word.substr(open + 1, close - open - 1);
    std::string post = word.substr(close + 1);

    // Check for sequence {a..b} or {1..5}
    size_t dotdot = inner.find("..");
    if (dotdot != std::string::npos) {
        std::string a = inner.substr(0, dotdot);
        std::string b = inner.substr(dotdot + 2);
        std::vector<std::string> result;
        if (a.size() == 1 && b.size() == 1 && std::isalpha((unsigned char)a[0]) && std::isalpha((unsigned char)b[0])) {
            char ca = a[0], cb = b[0];
            int dir = ca <= cb ? 1 : -1;
            for (char c = ca; c != cb + dir; c += dir)
                result.push_back(pre + c + post);
        } else {
            try {
                long ia = std::stol(a), ib = std::stol(b);
                int dir = ia <= ib ? 1 : -1;
                for (long n = ia; n != ib + dir; n += dir)
                    result.push_back(pre + std::to_string(n) + post);
            } catch (...) { return {word}; }
        }
        return result;
    }

    // Split on unquoted commas inside inner
    std::vector<std::string> parts;
    std::string cur;
    depth = 0;
    in_sq = in_dq = false;
    for (size_t i = 0; i < inner.size(); ++i) {
        char c = inner[i];
        if (c == '\'' && !in_dq) { in_sq = !in_sq; cur += c; continue; }
        if (c == '"' && !in_sq) { in_dq = !in_dq; cur += c; continue; }
        if (in_sq || in_dq) { cur += c; continue; }
        if (c == '{') { ++depth; cur += c; }
        else if (c == '}') { --depth; cur += c; }
        else if (c == ',' && depth == 0) { parts.push_back(cur); cur.clear(); }
        else cur += c;
    }
    parts.push_back(cur);

    if (parts.size() == 1) return {word}; // no comma — not a list

    std::vector<std::string> result;
    for (const auto& p : parts) {
        auto expanded = braceExpand(pre + p + post);
        for (auto& e : expanded) result.push_back(e);
    }
    return result;
}

// ── History expansion ─────────────────────────────────────────────────────────

std::string Shell::expandHistory(const std::string& line) const {
    if (line.empty()) return line;

    // ^old^new substitution on previous command
    if (line[0] == '^') {
        size_t second = line.find('^', 1);
        if (second == std::string::npos) return line;
        size_t third = line.find('^', second + 1);
        std::string old_str = line.substr(1, second - 1);
        std::string new_str = (third == std::string::npos)
            ? line.substr(second + 1)
            : line.substr(second + 1, third - second - 1);
        if (state.command_history.empty()) {
            std::cerr << "helix: no previous command\n"; return "";
        }
        std::string prev = state.command_history.back();
        size_t pos = prev.find(old_str);
        if (pos == std::string::npos) { std::cerr << "helix: substitution failed\n"; return ""; }
        std::string result = prev.substr(0, pos) + new_str + prev.substr(pos + old_str.size());
        std::cout << result << "\n";
        return result;
    }

    if (line[0] != '!') return line;

    if (line == "!!") {
        if (state.command_history.empty()) {
            std::cerr << "helix: !!: no previous command\n"; return "";
        }
        const std::string& prev = state.command_history.back();
        std::cout << prev << "\n";
        return prev;
    }

    if (line.size() > 1 && std::isdigit(static_cast<unsigned char>(line[1]))) {
        try {
            size_t n = std::stoul(line.substr(1));
            if (n == 0 || n > state.command_history.size()) {
                std::cerr << "helix: !" << n << ": event not found\n"; return "";
            }
            const std::string& cmd = state.command_history[n - 1];
            std::cout << cmd << "\n";
            return cmd;
        } catch (...) {}
    }

    // !string — find last command matching prefix
    if (line.size() > 1) {
        std::string prefix = line.substr(1);
        for (int i = (int)state.command_history.size() - 1; i >= 0; --i) {
            if (state.command_history[i].find(prefix) == 0) {
                std::cout << state.command_history[i] << "\n";
                return state.command_history[i];
            }
        }
        std::cerr << "helix: !" << prefix << ": event not found\n";
        return "";
    }

    return line;
}

// ── Alias expansion ───────────────────────────────────────────────────────────

std::string Shell::expandAliases(const std::string& line) const {
    if (state.aliases.empty()) return line;

    size_t word_end = line.find_first_of(" \t");
    std::string cmd = (word_end == std::string::npos) ? line : line.substr(0, word_end);
    std::string rest = (word_end == std::string::npos) ? "" : line.substr(word_end);

    auto it = state.aliases.find(cmd);
    if (it == state.aliases.end()) return line;
    return it->second + rest;
}

// ── Block execution helpers ───────────────────────────────────────────────────

// Run a sequence of lines as a sub-block, returns exit status of last command
int Shell::runBlock(const std::vector<std::string>& lines) {
    for (const auto& line : lines) {
        std::string trimmed = trimWhitespace(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;
        if (!runSingleCommand(trimmed)) break;
        if (!state.running) break;
        if (state.breaking || state.continuing || state.returning) break;
    }
    return state.last_exit_status;
}

// ── Function invocation ───────────────────────────────────────────────────────

bool Shell::invokeFunction(const std::string& name, const std::vector<std::string>& callArgs) {
    auto it = state.functions.find(name);
    if (it == state.functions.end()) return false;

    // Push a new variable frame
    state.var_frames.push_back(VarFrame{});

    // Save and replace positional params
    auto saved_params = state.positional_params;
    auto saved_script  = state.script_name;
    state.positional_params = callArgs;
    state.script_name = name;

    bool saved_returning = state.returning;
    state.returning = false;

    runBlock(it->second.body);

    // Restore
    state.positional_params = saved_params;
    state.script_name = saved_script;
    state.var_frames.pop_back();

    if (state.returning) {
        state.returning = false;
        state.last_exit_status = state.return_value;
    }
    state.returning = saved_returning && !state.returning;

    return true;
}

// ── If/elif/else/fi execution ─────────────────────────────────────────────────

int Shell::executeIf(const std::vector<std::string>& lines) {
    // lines[0] is "if condition; then" or "if condition"
    // We need to find then/elif/else/fi sections

    struct Clause { std::string cond; std::vector<std::string> body; bool is_else = false; };
    std::vector<Clause> clauses;
    std::string else_body_lines;
    std::vector<std::string> else_body;

    std::string cond_acc;
    std::vector<std::string> body_acc;
    bool in_body = false;
    bool in_else = false;
    int depth = 0; // nested if depth

    for (const auto& raw_line : lines) {
        std::string line = trimWhitespace(raw_line);
        if (line.empty() || line[0] == '#') continue;

        // Split on semicolons to handle "if cond; then"
        // We use the first word to detect keywords
        std::string first_word;
        size_t sp = line.find_first_of(" \t;");
        first_word = (sp == std::string::npos) ? line : line.substr(0, sp);

        if (first_word == "if" && depth > 0) { ++depth; }
        else if (first_word == "fi" && depth > 0) { --depth; }

        if (depth > 0) {
            if (in_body) body_acc.push_back(line);
            else if (in_else) else_body.push_back(line);
            continue;
        }

        if (first_word == "if" && depth == 0 && !in_body) {
            // Extract condition after "if " and before "then" or ";"
            std::string rest = (sp == std::string::npos) ? "" : line.substr(sp + 1);
            // strip leading/trailing whitespace from rest
            size_t ts = rest.find_first_not_of(" \t");
            if (ts != std::string::npos) rest = rest.substr(ts);
            // remove trailing ; then
            size_t semi = rest.rfind(';');
            if (semi != std::string::npos) rest = rest.substr(0, semi);
            // remove "then" suffix
            if (rest.size() >= 4 && rest.substr(rest.size() - 4) == "then") {
                rest = rest.substr(0, rest.size() - 4);
            }
            cond_acc = trimWhitespace(rest);
            in_body = true;
            continue;
        }
        if (first_word == "then") {
            in_body = true;
            // Handle "then cmd" inline
            if (sp != std::string::npos) {
                std::string rest = trimWhitespace(line.substr(sp + 1));
                if (!rest.empty() && rest != "fi" && rest != "else") body_acc.push_back(rest);
            }
            continue;
        }
        if (first_word == "elif") {
            // save previous clause
            clauses.push_back({cond_acc, body_acc, false});
            body_acc.clear();
            std::string rest = line.substr(4);
            size_t ts2 = rest.find_first_not_of(" \t");
            if (ts2 != std::string::npos) rest = rest.substr(ts2);
            size_t semi = rest.rfind(';');
            if (semi != std::string::npos) rest = rest.substr(0, semi);
            if (rest.size() >= 4 && rest.substr(rest.size() - 4) == "then")
                rest = rest.substr(0, rest.size() - 4);
            cond_acc = trimWhitespace(rest);
            in_body = true;
            continue;
        }
        if (first_word == "else") {
            clauses.push_back({cond_acc, body_acc, false});
            body_acc.clear();
            in_else = true; in_body = false;
            continue;
        }
        if (first_word == "fi") {
            if (in_else) {
                // nothing else to do
            } else {
                clauses.push_back({cond_acc, body_acc, false});
            }
            break;
        }

        if (in_body)  body_acc.push_back(line);
        else if (in_else) else_body.push_back(line);
        else cond_acc += " " + line;
    }

    // Execute
    for (const auto& clause : clauses) {
        if (clause.cond.empty()) continue;
        runSingleCommand(clause.cond);
        if (state.last_exit_status == 0) {
            runBlock(clause.body);
            return state.last_exit_status;
        }
    }
    if (!else_body.empty()) runBlock(else_body);
    return state.last_exit_status;
}

// ── While/until loop ──────────────────────────────────────────────────────────

int Shell::executeWhile(const std::vector<std::string>& lines, bool until) {
    // lines[0]: "while condition" / "until condition"
    // body between do/done
    std::string cond;
    std::vector<std::string> body;
    bool in_cond = true;
    bool in_body = false;

    for (const auto& raw : lines) {
        std::string line = trimWhitespace(raw);
        if (line.empty() || line[0] == '#') continue;
        std::string first;
        size_t sp = line.find_first_of(" \t;");
        first = (sp == std::string::npos) ? line : line.substr(0, sp);

        if ((first == "while" || first == "until") && in_cond && cond.empty()) {
            std::string rest = sp == std::string::npos ? "" : line.substr(sp + 1);
            size_t ts = rest.find_first_not_of(" \t");
            if (ts != std::string::npos) rest = rest.substr(ts);
            size_t semi = rest.rfind(';');
            if (semi != std::string::npos) rest = rest.substr(0, semi);
            if (rest.size() >= 2 && rest.substr(rest.size() - 2) == "do")
                rest = rest.substr(0, rest.size() - 2);
            cond = trimWhitespace(rest);
            continue;
        }
        if (first == "do") {
            in_cond = false; in_body = true;
            // inline body after do
            if (sp != std::string::npos) {
                std::string rest = trimWhitespace(line.substr(sp + 1));
                if (!rest.empty() && rest != "done") body.push_back(rest);
            }
            continue;
        }
        if (first == "done") break;
        if (in_cond) { cond += " " + line; continue; }
        if (in_body) body.push_back(line);
    }

    while (state.running) {
        if (!cond.empty()) {
            runSingleCommand(cond);
        }
        bool cond_ok = (state.last_exit_status == 0);
        if (until ? cond_ok : !cond_ok) break;

        runBlock(body);

        if (state.breaking) {
            --state.break_depth;
            state.breaking = (state.break_depth > 0);
            break;
        }
        if (state.continuing) {
            --state.continue_depth;
            state.continuing = (state.continue_depth > 0);
            if (state.continuing) break;
            // continue this loop
            continue;
        }
        if (state.returning) break;
    }
    return state.last_exit_status;
}

// ── For loop ──────────────────────────────────────────────────────────────────

int Shell::executeFor(const std::vector<std::string>& lines) {
    // "for VAR in word1 word2 ...; do ... done"
    std::string varname;
    std::vector<std::string> words;
    std::vector<std::string> body;
    bool in_body = false;

    for (const auto& raw : lines) {
        std::string line = trimWhitespace(raw);
        if (line.empty() || line[0] == '#') continue;
        std::string first;
        size_t sp = line.find_first_of(" \t;");
        first = (sp == std::string::npos) ? line : line.substr(0, sp);

        if (first == "for" && varname.empty()) {
            // for VAR in w1 w2 ...
            std::string rest = sp == std::string::npos ? "" : trimWhitespace(line.substr(sp + 1));
            size_t sp2 = rest.find_first_of(" \t");
            varname = sp2 == std::string::npos ? rest : rest.substr(0, sp2);
            rest = sp2 == std::string::npos ? "" : trimWhitespace(rest.substr(sp2 + 1));
            // strip "in "
            if (rest.size() >= 3 && rest.substr(0, 3) == "in ") rest = trimWhitespace(rest.substr(3));
            // strip trailing "; do" or "do"
            size_t semi = rest.rfind(';');
            if (semi != std::string::npos) rest = trimWhitespace(rest.substr(0, semi));
            if (rest.size() >= 2 && rest.substr(rest.size() - 2) == "do")
                rest = trimWhitespace(rest.substr(0, rest.size() - 2));
            // Split words
            std::istringstream iss(rest);
            std::string w;
            while (iss >> w) words.push_back(w);
            continue;
        }
        if (first == "do") {
            in_body = true;
            // "do cmd" on same line
            if (sp != std::string::npos) {
                std::string rest = trimWhitespace(line.substr(sp + 1));
                if (!rest.empty() && rest != "done") body.push_back(rest);
            }
            continue;
        }
        if (first == "done") break;
        if (!in_body && varname.empty()) continue;
        if (!in_body) { continue; }
        body.push_back(line);
    }

    // Expand each word (glob, env)
    EnvironmentVariableExpander expander;
    std::vector<std::string> expanded_words;
    for (const auto& w : words) {
        std::string ew = expander.expandWithState(w, &state);
        auto be = braceExpand(ew);
        for (const auto& b : be) expanded_words.push_back(b);
    }

    for (const auto& val : expanded_words) {
        setenv(varname.c_str(), val.c_str(), 1);
        state.environment[varname] = val;

        runBlock(body);

        if (state.breaking) {
            --state.break_depth;
            state.breaking = (state.break_depth > 0);
            break;
        }
        if (state.continuing) {
            --state.continue_depth;
            state.continuing = (state.continue_depth > 0);
            if (state.continuing) break;
        }
        if (state.returning || !state.running) break;
    }
    return state.last_exit_status;
}

// ── Case/esac ─────────────────────────────────────────────────────────────────

int Shell::executeCase(const std::vector<std::string>& lines) {
    // "case WORD in"
    // "pattern1|pattern2) body ;;"
    // "esac"
    std::string word;
    bool in_body = false;
    std::string cur_pattern;
    std::vector<std::string> cur_body;
    bool matched = false;

    EnvironmentVariableExpander expander;

    for (const auto& raw : lines) {
        std::string line = trimWhitespace(raw);
        if (line.empty() || line[0] == '#') continue;
        std::string first;
        size_t sp = line.find_first_of(" \t");
        first = (sp == std::string::npos) ? line : line.substr(0, sp);

        if (first == "case" && word.empty()) {
            std::string rest = sp == std::string::npos ? "" : trimWhitespace(line.substr(sp + 1));
            // strip " in"
            if (rest.size() >= 3 && rest.substr(rest.size() - 3) == " in")
                rest = trimWhitespace(rest.substr(0, rest.size() - 3));
            else if (rest.size() >= 2 && rest.substr(rest.size() - 2) == "in")
                rest = trimWhitespace(rest.substr(0, rest.size() - 2));
            word = expander.expandWithState(rest, &state);
            in_body = false;
            continue;
        }
        if (line == "esac") break;

        if (!in_body) {
            // Expect "pattern)" line
            size_t paren = line.find(')');
            if (paren == std::string::npos) continue;
            cur_pattern = trimWhitespace(line.substr(0, paren));
            in_body = true;
            cur_body.clear();
            // Handle single-line: "pattern) cmd ;;"
            std::string rest = trimWhitespace(line.substr(paren + 1));
            if (rest == ";;") { in_body = false; }
            else if (!rest.empty()) {
                if (rest.size() >= 2 && rest.substr(rest.size() - 2) == ";;") {
                    cur_body.push_back(rest.substr(0, rest.size() - 2));
                    in_body = false;
                } else {
                    cur_body.push_back(rest);
                }
            }
        } else {
            if (line == ";;" || (line.size() >= 2 && line.substr(line.size() - 2) == ";;")) {
                if (line != ";;") cur_body.push_back(line.substr(0, line.size() - 2));
                in_body = false;
            } else {
                cur_body.push_back(line);
            }
        }

        if (!in_body && !matched) {
            // Check if word matches any pattern (| separated)
            std::istringstream pss(cur_pattern);
            std::string pat;
            while (std::getline(pss, pat, '|')) {
                pat = trimWhitespace(pat);
                if (pat == "*" || fnmatch(pat.c_str(), word.c_str(), 0) == 0) {
                    matched = true;
                    runBlock(cur_body);
                    break;
                }
            }
        }
    }
    return state.last_exit_status;
}

// ── VAR=value assignment detection ────────────────────────────────────────────

static bool isAssignment(const std::string& token) {
    if (token.empty()) return false;
    if (!std::isalpha((unsigned char)token[0]) && token[0] != '_') return false;
    size_t eq = token.find('=');
    if (eq == std::string::npos) return false;
    for (size_t i = 0; i < eq; ++i) {
        if (!std::isalnum((unsigned char)token[i]) && token[i] != '_') return false;
    }
    return true;
}

// ── processInput / runSingleCommand ──────────────────────────────────────────

bool Shell::processInput(const std::string& input) {
    if (input.empty()) return true;

    std::string expanded_hist = expandHistory(input);
    if (expanded_hist.empty() && !input.empty() && (input[0] == '!' || input[0] == '^')) return true;
    const std::string& effective = (input[0] == '!' || input[0] == '^') ? expanded_hist : input;

    // HISTCONTROL
    bool skip_hist = false;
    if (state.histcontrol == "ignoredups" && effective == state.last_histentry) skip_hist = true;
    if (state.histcontrol == "erasedups") {
        auto& h = state.command_history;
        h.erase(std::remove(h.begin(), h.end(), effective), h.end());
    }
    if (!skip_hist) {
        state.command_history.push_back(effective);
        session_history_.push_back(effective);
        add_history(effective.c_str());
        state.last_histentry = effective;
    }

    // ── Check if compound statement spans the full line (semicolon-separated) ──
    // e.g. "X=0; while ...; do ...; done" — run prefix assignments then block
    {
        // Quick pre-scan: does effective contain a block opener AND closer?
        auto hasWord = [&](const std::string& s, const std::string& w) {
            size_t pos = s.find(w);
            while (pos != std::string::npos) {
                bool before_ok = (pos == 0 || !std::isalnum((unsigned char)s[pos-1]));
                bool after_ok  = (pos + w.size() >= s.size() || !std::isalnum((unsigned char)s[pos + w.size()]));
                if (before_ok && after_ok) return true;
                pos = s.find(w, pos + 1);
            }
            return false;
        };
        bool has_for   = hasWord(effective, "for");
        bool has_while = hasWord(effective, "while");
        bool has_until = hasWord(effective, "until");
        bool has_if    = hasWord(effective, "if");
        bool has_case  = hasWord(effective, "case");
        bool has_done  = hasWord(effective, "done");
        bool has_fi    = hasWord(effective, "fi");
        bool has_esac  = hasWord(effective, "esac");

        bool needs_block = (has_for && has_done) || (has_while && has_done) ||
                           (has_until && has_done) || (has_if && has_fi) ||
                           (has_case && has_esac);

        if (needs_block) {
            // Split by semicolons into "lines" and execute as block
            std::vector<std::string> seg_lines;
            std::string seg;
            bool in_sq = false, in_dq = false;
            for (size_t ci = 0; ci <= effective.size(); ++ci) {
                if (ci == effective.size()) {
                    seg = trimWhitespace(seg);
                    if (!seg.empty()) seg_lines.push_back(seg);
                    break;
                }
                char ch = effective[ci];
                if (ch == '\'' && !in_dq) { in_sq = !in_sq; seg += ch; continue; }
                if (ch == '"' && !in_sq) { in_dq = !in_dq; seg += ch; continue; }
                if (ch == ';' && !in_sq && !in_dq) {
                    seg = trimWhitespace(seg);
                    if (!seg.empty()) seg_lines.push_back(seg);
                    seg.clear();
                } else {
                    seg += ch;
                }
            }

            // Find the first block opener in seg_lines
            size_t block_start = 0;
            for (; block_start < seg_lines.size(); ++block_start) {
                std::string fw;
                size_t fsp = seg_lines[block_start].find_first_of(" \t;");
                fw = fsp == std::string::npos ? seg_lines[block_start] : seg_lines[block_start].substr(0, fsp);
                if (fw == "for" || fw == "while" || fw == "until" || fw == "if" || fw == "case") break;
            }

            // Run prefix assignments/commands
            for (size_t pi = 0; pi < block_start; ++pi) {
                runSingleCommand(seg_lines[pi]);
            }

            // Execute the block
            std::vector<std::string> block_only(seg_lines.begin() + block_start, seg_lines.end());
            std::string opener_fw;
            if (!block_only.empty()) {
                size_t fsp = block_only[0].find_first_of(" \t;");
                opener_fw = fsp == std::string::npos ? block_only[0] : block_only[0].substr(0, fsp);
            }
            if (opener_fw == "for") executeFor(block_only);
            else if (opener_fw == "while") executeWhile(block_only, false);
            else if (opener_fw == "until") executeWhile(block_only, true);
            else if (opener_fw == "if") executeIf(block_only);
            else if (opener_fw == "case") executeCase(block_only);

            return state.running;
        }
    }

    // ── Multi-line block accumulation ────────────────────────────────────────
    // Keywords that open blocks: if, while, until, for, case, { (function)
    // Keywords that close them: fi, done, esac, }

    std::string trimmed = trimWhitespace(effective);

    // Check if we're already accumulating a block
    if (!block_stack_.empty()) {
        block_lines_.push_back(trimmed);

        // Check for keywords that increment nesting depth
        std::string first;
        size_t sp = trimmed.find_first_of(" \t;");
        first = (sp == std::string::npos) ? trimmed : trimmed.substr(0, sp);

        if (first == "if" || first == "while" || first == "until" ||
            first == "for" || first == "case") {
            block_stack_.push_back(first);
        }
        if (first == "fi") {
            if (!block_stack_.empty()) block_stack_.pop_back();
            if (block_stack_.empty()) {
                auto lines = block_lines_; block_lines_.clear();
                executeIf(lines);
                return state.running;
            }
        }
        if (first == "done") {
            if (!block_stack_.empty()) {
                std::string opener = block_stack_.back();
                block_stack_.pop_back();
                if (block_stack_.empty()) {
                    auto lines = block_lines_; block_lines_.clear();
                    if (opener == "for") executeFor(lines);
                    else executeWhile(lines, opener == "until");
                    return state.running;
                }
            }
        }
        if (first == "esac") {
            if (!block_stack_.empty()) block_stack_.pop_back();
            if (block_stack_.empty()) {
                auto lines = block_lines_; block_lines_.clear();
                executeCase(lines);
                return state.running;
            }
        }
        if (trimmed == "}") {
            if (!block_stack_.empty()) block_stack_.pop_back();
            if (block_stack_.empty()) {
                // Function definition complete
                auto lines = block_lines_; block_lines_.clear();
                // lines[0] is "name() {" or "function name {", rest is body
                if (!pending_func_name_.empty()) {
                    ShellFunction fn;
                    fn.name = pending_func_name_;
                    fn.body.assign(lines.begin() + 1, lines.end());
                    if (!fn.body.empty() && trimWhitespace(fn.body.back()) == "}")
                        fn.body.pop_back();
                    state.functions[fn.name] = fn;
                    pending_func_name_.clear();
                }
                return state.running;
            }
        }
        return true; // still accumulating
    }

    // ── Detect block openers ─────────────────────────────────────────────────
    {
        std::string first;
        size_t sp = trimmed.find_first_of(" \t;");
        first = (sp == std::string::npos) ? trimmed : trimmed.substr(0, sp);

        // Single-line for: "for x in ...; do ...; done"
        if (first == "for" && trimmed.find("done") != std::string::npos) {
            std::vector<std::string> lines;
            std::string seg;
            for (size_t ci = 0; ci <= trimmed.size(); ++ci) {
                if (ci == trimmed.size() || trimmed[ci] == ';') {
                    seg = trimWhitespace(seg);
                    if (!seg.empty()) lines.push_back(seg);
                    seg.clear();
                } else {
                    seg += trimmed[ci];
                }
            }
            executeFor(lines);
            return state.running;
        }
        // Single-line while/until: "while cond; do ...; done"
        if ((first == "while" || first == "until") && trimmed.find("done") != std::string::npos) {
            std::vector<std::string> lines;
            std::string seg;
            for (size_t ci = 0; ci <= trimmed.size(); ++ci) {
                if (ci == trimmed.size() || trimmed[ci] == ';') {
                    seg = trimWhitespace(seg);
                    if (!seg.empty()) lines.push_back(seg);
                    seg.clear();
                } else {
                    seg += trimmed[ci];
                }
            }
            executeWhile(lines, first == "until");
            return state.running;
        }

        // Function definition: "name() {" or "name()" or "function name {"
        if (first == "function") {
            // Multi-line "function NAME {": accumulate; single-line falls through to splitLogical
            std::string rest = trimmed.size() > 8 ? trimWhitespace(trimmed.substr(8)) : "";
            size_t sp2 = rest.find_first_of(" \t({");
            std::string fname = (sp2 == std::string::npos) ? rest : rest.substr(0, sp2);
            size_t open_brace = trimmed.rfind('{');
            size_t close_brace = trimmed.rfind('}');
            if (open_brace != std::string::npos && (close_brace == std::string::npos || close_brace <= open_brace)) {
                pending_func_name_ = fname;
                block_stack_.push_back("function");
                block_lines_.push_back(trimmed);
                return true;
            }
            // Single-line complete "function f { body }": fall through to splitLogical
        }
        // "name() {" multi-line only; single-line falls through to splitLogical
        if (trimmed.find("()") != std::string::npos) {
            size_t paren = trimmed.find("()");
            if (paren > 0 && std::isalnum((unsigned char)trimmed[paren-1])) {
                std::string fname = trimmed.substr(0, paren);
                size_t ns = fname.find_first_not_of(" \t");
                if (ns != std::string::npos) fname = fname.substr(ns);
                if (!fname.empty()) {
                    size_t open_brace = trimmed.find('{', paren + 2);
                    size_t close_brace = trimmed.rfind('}');
                    if (open_brace != std::string::npos && (close_brace == std::string::npos || close_brace <= open_brace)) {
                        // Multi-line: open brace only
                        pending_func_name_ = fname;
                        block_stack_.push_back("function");
                        block_lines_.push_back(trimmed);
                        return true;
                    }
                    // Single-line or bare "name()": fall through to splitLogical
                }
            }
        }
        if (first == "if") {
            block_stack_.push_back("if");
            block_lines_.push_back(trimmed);
            // Check for single-line "if cond; then cmd; fi"
            if (trimmed.find("fi") != std::string::npos) {
                auto lines = block_lines_; block_lines_.clear(); block_stack_.clear();
                executeIf(lines);
                return state.running;
            }
            return true;
        }
        if (first == "while" || first == "until") {
            block_stack_.push_back(first);
            block_lines_.push_back(trimmed);
            return true;
        }
        if (first == "for") {
            block_stack_.push_back("for");
            block_lines_.push_back(trimmed);
            return true;
        }
        if (first == "case") {
            block_stack_.push_back("case");
            block_lines_.push_back(trimmed);
            return true;
        }
    }

    auto frags = splitLogical(effective);
    for (const auto& frag : frags) {
        std::string t = trimWhitespace(frag.text);
        if (t.empty()) continue;

        if (!runSingleCommand(t)) return false;
        if (!state.running) return false;

        if (frag.op == SplitOp::AND && state.last_exit_status != 0) break;
        if (frag.op == SplitOp::OR  && state.last_exit_status == 0) break;
    }

    return true;
}

bool Shell::runSingleCommand(const std::string& input) {
    if (state.noexec) return true;

    std::string trimmed_input = trimWhitespace(input);
    if (trimmed_input.empty()) return true;

    // ── Function definition in a single fragment ─────────────────────────────
    // "name() { body }" or "function name { body }"
    {
        std::string first;
        size_t sp = trimmed_input.find_first_of(" \t;");
        first = (sp == std::string::npos) ? trimmed_input : trimmed_input.substr(0, sp);

        if (first == "function" || (trimmed_input.find("()") != std::string::npos &&
            trimmed_input.find('{') != std::string::npos &&
            trimmed_input.find('}') != std::string::npos)) {
            // Let processInput handle function definition
            std::string fname;
            std::string body_str;
            size_t open_brace  = trimmed_input.rfind('{');
            size_t close_brace = trimmed_input.rfind('}');
            if (open_brace != std::string::npos && close_brace != std::string::npos && close_brace > open_brace) {
                if (first == "function") {
                    std::string rest = trimmed_input.size() > 8 ? trimWhitespace(trimmed_input.substr(8)) : "";
                    size_t sp2 = rest.find_first_of(" \t({");
                    fname = (sp2 == std::string::npos) ? rest : rest.substr(0, sp2);
                } else {
                    size_t paren = trimmed_input.find("()");
                    if (paren != std::string::npos) {
                        fname = trimmed_input.substr(0, paren);
                        size_t ns = fname.find_first_not_of(" \t");
                        if (ns != std::string::npos) fname = fname.substr(ns);
                    }
                }
                body_str = trimWhitespace(trimmed_input.substr(open_brace + 1, close_brace - open_brace - 1));
                if (!fname.empty()) {
                    ShellFunction fn; fn.name = fname;
                    std::string bseg;
                    for (size_t ci = 0; ci <= body_str.size(); ++ci) {
                        if (ci == body_str.size() || body_str[ci] == ';') {
                            bseg = trimWhitespace(bseg);
                            if (!bseg.empty()) fn.body.push_back(bseg);
                            bseg.clear();
                        } else { bseg += body_str[ci]; }
                    }
                    state.functions[fname] = fn;
                    return true;
                }
            }
        }
    }

    std::string expanded = expandAliases(trimmed_input);
    const std::string& trimmed = trimmed_input;

    if (state.xtrace) std::cerr << "+ " << expanded << "\n";

    // Check for VAR=value assignment (possibly prefixed before a command)
    // e.g. "FOO=bar" or "FOO=bar cmd args"
    {
        EnvironmentVariableExpander expander;
        std::istringstream iss(expanded);
        std::string first_tok;
        iss >> first_tok;
        if (!first_tok.empty() && isAssignment(first_tok)) {
            // Apply all leading assignments
            std::string rest_of_line;
            std::getline(iss, rest_of_line);
            rest_of_line = trimWhitespace(rest_of_line);

            // Apply this assignment
            size_t eq = first_tok.find('=');
            std::string vname = first_tok.substr(0, eq);
            std::string vval  = expander.expandWithState(first_tok.substr(eq + 1), &state);

            if (state.readonly_vars.count(vname)) {
                std::cerr << vname << ": readonly variable\n";
                state.last_exit_status = 1;
                return true;
            }

            state.environment[vname] = vval;
            setenv(vname.c_str(), vval.c_str(), 1);

            if (!rest_of_line.empty() && !isAssignment(rest_of_line.substr(0, rest_of_line.find(' ')))) {
                // There's a command after the assignment(s) — re-run with it
                return runSingleCommand(rest_of_line);
            } else if (!rest_of_line.empty()) {
                return runSingleCommand(first_tok + " " + rest_of_line);
            }

            state.last_exit_status = 0;
            // Update $? env
            setenv("?", "0", 1);
            return true;
        }
    }

    // Check for function invocation
    {
        std::string first_word;
        size_t sp = expanded.find_first_of(" \t");
        first_word = (sp == std::string::npos) ? expanded : expanded.substr(0, sp);

        if (state.functions.count(first_word)) {
            std::vector<std::string> call_args;
            if (sp != std::string::npos) {
                // tokenize args
                EnvironmentVariableExpander expander;
                std::istringstream iss(expanded.substr(sp + 1));
                std::string w;
                while (iss >> w) call_args.push_back(expander.expandWithState(w, &state));
            }
            invokeFunction(first_word, call_args);
            setenv("?", std::to_string(state.last_exit_status).c_str(), 1);
            return state.running;
        }
    }

    // Pre-expand the whole line before tokenizing so $(()), $(), ${}, $VAR
    // are resolved before the tokenizer sees them (avoids glob-splitting inside $(()))
    {
        EnvironmentVariableExpander pre_exp;
        expanded = pre_exp.expandWithState(expanded, &state);
    }

    auto tokens = tokenizer.tokenize(expanded);
    ParsedCommand parsed_cmd = parser.parse(tokens);

    // Brace-expand args
    for (auto& cmd_entry : parsed_cmd.pipeline.commands) {
        std::vector<std::string> expanded_args;
        for (auto& arg : cmd_entry.args) {
            auto be = braceExpand(arg);
            for (auto& b : be) expanded_args.push_back(std::move(b));
        }
        cmd_entry.args = std::move(expanded_args);
    }

    bool handled = builtin_dispatcher->dispatch(parsed_cmd, state);

    // Update $? env var
    setenv("?", std::to_string(state.last_exit_status).c_str(), 1);

    if (!state.running) return false;
    if (!handled) {
        state.last_exit_status = executor.execute(parsed_cmd);
        setenv("?", std::to_string(state.last_exit_status).c_str(), 1);

        pid_t bg_pid = executor.getLastBackgroundPid();
        if (bg_pid > 0) {
            setenv("!", std::to_string(bg_pid).c_str(), 1);
            if (job_manager) job_manager->addJob(bg_pid, trimmed);
        }
    }

    if (state.exit_on_error && state.last_exit_status != 0) {
        state.running = false;
        return false;
    }

    return true;
}

// ── Non-interactive entry points ─────────────────────────────────────────────

int Shell::runCommand(const std::string& cmd) {
    processInput(cmd);
    return state.last_exit_status;
}

int Shell::runStdin() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::string trimmed = trimWhitespace(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;
        if (!processInput(trimmed)) break;
        if (!state.running) break;
        if (state.exit_on_error && state.last_exit_status != 0) break;
    }
    return state.last_exit_status;
}

int Shell::runScript(const char* path, int argc, char* argv[]) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "helix: " << path << ": No such file or directory\n";
        return 127;
    }

    state.script_name = path;
    for (int i = 0; i < argc; ++i)
        state.positional_params.push_back(argv[i]);

    std::string line;
    while (std::getline(f, line)) {
        std::string trimmed = trimWhitespace(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;
        if (!processInput(trimmed)) break;
        if (!state.running) break;
        if (state.exit_on_error && state.last_exit_status != 0) break;
    }
    return state.last_exit_status;
}

} // namespace helix

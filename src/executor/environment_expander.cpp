#include "executor/environment_expander.h"
#include "shell/shell_state.h"
#include <array>
#include <cstdlib>
#include <memory>
#include <string>
#include <sstream>
#include <iostream>
#include <unistd.h>

namespace helix {

// Run $(cmd) and return stdout
static std::string captureSubshell(const std::string& cmd) {
    std::string result;
    std::array<char, 256> buf;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) return "";
    while (fgets(buf.data(), buf.size(), pipe.get())) result += buf.data();
    while (!result.empty() && result.back() == '\n') result.pop_back();
    return result;
}

// Simple arithmetic evaluator (handles + - * / % with operator precedence)
static long evalArith(const std::string& expr);

static long evalAtom(const std::string& expr, size_t& i) {
    while (i < expr.size() && expr[i] == ' ') ++i;
    if (i >= expr.size()) return 0;

    bool neg = false;
    if (expr[i] == '-') { neg = true; ++i; }
    else if (expr[i] == '+') { ++i; }
    else if (expr[i] == '!') { ++i; return !evalAtom(expr, i); }
    else if (expr[i] == '~') { ++i; return ~evalAtom(expr, i); }

    long val = 0;
    if (i < expr.size() && expr[i] == '(') {
        ++i;
        val = evalArith(expr.substr(i));
        // find matching )
        int depth = 1;
        while (i < expr.size() && depth > 0) {
            if (expr[i] == '(') ++depth;
            else if (expr[i] == ')') --depth;
            ++i;
        }
    } else if (i < expr.size() && expr[i] == '$') {
        ++i;
        std::string vname;
        while (i < expr.size() && (std::isalnum((unsigned char)expr[i]) || expr[i] == '_'))
            vname += expr[i++];
        const char* v = getenv(vname.c_str());
        val = v ? std::atol(v) : 0;
    } else if (i < expr.size() && (std::isalpha((unsigned char)expr[i]) || expr[i] == '_')) {
        // Bare variable name inside arithmetic (bash-compatible)
        std::string vname;
        while (i < expr.size() && (std::isalnum((unsigned char)expr[i]) || expr[i] == '_'))
            vname += expr[i++];
        const char* v = getenv(vname.c_str());
        val = v ? std::atol(v) : 0;
    } else {
        while (i < expr.size() && std::isdigit((unsigned char)expr[i]))
            val = val * 10 + (expr[i++] - '0');
    }
    return neg ? -val : val;
}

static long evalMul(const std::string& expr, size_t& i) {
    long left = evalAtom(expr, i);
    while (i < expr.size()) {
        while (i < expr.size() && expr[i] == ' ') ++i;
        if (i < expr.size() && expr[i] == '*') { ++i; left *= evalAtom(expr, i); }
        else if (i < expr.size() && expr[i] == '/' && (i+1 >= expr.size() || expr[i+1] != '/')) {
            ++i; long r = evalAtom(expr, i); left = r ? left / r : 0;
        }
        else if (i < expr.size() && expr[i] == '%') { ++i; long r = evalAtom(expr, i); left = r ? left % r : 0; }
        else break;
    }
    return left;
}

static long evalAdd(const std::string& expr, size_t& i) {
    long left = evalMul(expr, i);
    while (i < expr.size()) {
        while (i < expr.size() && expr[i] == ' ') ++i;
        if (i < expr.size() && expr[i] == '+' && (i+1 >= expr.size() || expr[i+1] != '+')) { ++i; left += evalMul(expr, i); }
        else if (i < expr.size() && expr[i] == '-' && (i+1 >= expr.size() || expr[i+1] != '-')) { ++i; left -= evalMul(expr, i); }
        else break;
    }
    return left;
}

static long evalShift(const std::string& expr, size_t& i) {
    long left = evalAdd(expr, i);
    while (i + 1 < expr.size()) {
        while (i < expr.size() && expr[i] == ' ') ++i;
        if (expr[i] == '<' && expr[i+1] == '<') { i += 2; left <<= evalAdd(expr, i); }
        else if (expr[i] == '>' && expr[i+1] == '>') { i += 2; left >>= evalAdd(expr, i); }
        else break;
    }
    return left;
}

static long evalCmp(const std::string& expr, size_t& i) {
    long left = evalShift(expr, i);
    while (i < expr.size()) {
        while (i < expr.size() && expr[i] == ' ') ++i;
        if (i + 1 < expr.size() && expr[i] == '<' && expr[i+1] == '=') { i += 2; left = left <= evalShift(expr, i); }
        else if (i + 1 < expr.size() && expr[i] == '>' && expr[i+1] == '=') { i += 2; left = left >= evalShift(expr, i); }
        else if (expr[i] == '<') { ++i; left = left < evalShift(expr, i); }
        else if (expr[i] == '>') { ++i; left = left > evalShift(expr, i); }
        else break;
    }
    return left;
}

static long evalEq(const std::string& expr, size_t& i) {
    long left = evalCmp(expr, i);
    while (i + 1 < expr.size()) {
        while (i < expr.size() && expr[i] == ' ') ++i;
        if (expr[i] == '=' && expr[i+1] == '=') { i += 2; left = left == evalCmp(expr, i); }
        else if (expr[i] == '!' && expr[i+1] == '=') { i += 2; left = left != evalCmp(expr, i); }
        else break;
    }
    return left;
}

static long evalBitAnd(const std::string& expr, size_t& i) {
    long left = evalEq(expr, i);
    while (i < expr.size()) {
        while (i < expr.size() && expr[i] == ' ') ++i;
        if (expr[i] == '&' && (i+1 >= expr.size() || expr[i+1] != '&')) { ++i; left &= evalEq(expr, i); }
        else break;
    }
    return left;
}

static long evalBitXor(const std::string& expr, size_t& i) {
    long left = evalBitAnd(expr, i);
    while (i < expr.size()) {
        while (i < expr.size() && expr[i] == ' ') ++i;
        if (expr[i] == '^') { ++i; left ^= evalBitAnd(expr, i); }
        else break;
    }
    return left;
}

static long evalBitOr(const std::string& expr, size_t& i) {
    long left = evalBitXor(expr, i);
    while (i < expr.size()) {
        while (i < expr.size() && expr[i] == ' ') ++i;
        if (expr[i] == '|' && (i+1 >= expr.size() || expr[i+1] != '|')) { ++i; left |= evalBitXor(expr, i); }
        else break;
    }
    return left;
}

static long evalLogAnd(const std::string& expr, size_t& i) {
    long left = evalBitOr(expr, i);
    while (i + 1 < expr.size()) {
        while (i < expr.size() && expr[i] == ' ') ++i;
        if (expr[i] == '&' && expr[i+1] == '&') { i += 2; long r = evalBitOr(expr, i); left = left && r; }
        else break;
    }
    return left;
}

static long evalLogOr(const std::string& expr, size_t& i) {
    long left = evalLogAnd(expr, i);
    while (i + 1 < expr.size()) {
        while (i < expr.size() && expr[i] == ' ') ++i;
        if (expr[i] == '|' && expr[i+1] == '|') { i += 2; long r = evalLogAnd(expr, i); left = left || r; }
        else break;
    }
    return left;
}

static long evalArith(const std::string& expr) {
    size_t i = 0;
    return evalLogOr(expr, i);
}

// Parameter expansion modifiers: ${VAR:-default} etc.
static std::string applyParamModifier(const std::string& var_name, const std::string& modifier,
                                       const std::string& word, const ShellState* state) {
    const char* raw = getenv(var_name.c_str());
    std::string val = raw ? raw : "";
    bool is_set = (raw != nullptr);
    bool is_nonempty = is_set && !val.empty();

    if (modifier == ":-") {
        return is_nonempty ? val : word;
    } else if (modifier == "-") {
        return is_set ? val : word;
    } else if (modifier == ":=") {
        if (!is_nonempty) {
            setenv(var_name.c_str(), word.c_str(), 1);
            if (state) const_cast<ShellState*>(state)->environment[var_name] = word;
            return word;
        }
        return val;
    } else if (modifier == "=") {
        if (!is_set) {
            setenv(var_name.c_str(), word.c_str(), 1);
            if (state) const_cast<ShellState*>(state)->environment[var_name] = word;
            return word;
        }
        return val;
    } else if (modifier == ":?") {
        if (!is_nonempty) {
            std::string msg = var_name + ": " + (word.empty() ? "parameter null or not set" : word);
            std::cerr << msg << "\n";
            exit(1);
        }
        return val;
    } else if (modifier == "?") {
        if (!is_set) {
            std::string msg = var_name + ": " + (word.empty() ? "parameter not set" : word);
            std::cerr << msg << "\n";
            exit(1);
        }
        return val;
    } else if (modifier == ":+") {
        return is_nonempty ? word : "";
    } else if (modifier == "+") {
        return is_set ? word : "";
    } else if (modifier == "#") {
        // ${VAR#pattern} — strip shortest matching prefix (simplified: literal prefix)
        if (val.find(word) == 0) return val.substr(word.size());
        return val;
    } else if (modifier == "##") {
        // Longest matching prefix
        size_t pos = val.rfind(word);
        if (pos != std::string::npos && pos == 0) return val.substr(word.size());
        return val;
    } else if (modifier == "%") {
        // Strip shortest suffix
        if (val.size() >= word.size() && val.compare(val.size() - word.size(), word.size(), word) == 0)
            return val.substr(0, val.size() - word.size());
        return val;
    } else if (modifier == "%%") {
        if (val.size() >= word.size() && val.compare(val.size() - word.size(), word.size(), word) == 0)
            return val.substr(0, val.size() - word.size());
        return val;
    }
    return val;
}

std::string EnvironmentVariableExpander::expand(const std::string& input) const {
    return expandWithState(input, nullptr);
}

std::string EnvironmentVariableExpander::expandWithState(const std::string& input, const ShellState* state) const {
    std::string result;
    result.reserve(input.size());
    size_t i = 0;

    // Leading ~ expansion
    if (!input.empty() && input[0] == '~') {
        if (input.size() == 1 || input[1] == '/') {
            const char* home = getenv("HOME");
            if (home) { result += home; i = 1; }
        } else {
            // ~username expansion
            size_t end = 1;
            while (end < input.size() && input[end] != '/') ++end;
            std::string uname = input.substr(1, end - 1);
            struct passwd* pw = getpwnam(uname.c_str());
            if (pw) { result += pw->pw_dir; i = end; }
        }
    }

    while (i < input.size()) {
        char c = input[i];

        if (c == '$') {
            ++i;
            if (i >= input.size()) { result += '$'; break; }

            if (input[i] == '(') {
                if (i + 1 < input.size() && input[i+1] == '(') {
                    // $(( arithmetic ))
                    i += 2;
                    std::string expr;
                    int depth = 1;
                    while (i < input.size() && depth > 0) {
                        if (input[i] == '(') { ++depth; expr += input[i]; }
                        else if (input[i] == ')') { --depth; if (depth == 0) { ++i; break; } else expr += input[i]; }
                        else { expr += input[i]; }
                        ++i;
                    }
                    // skip closing )
                    if (i < input.size() && input[i] == ')') ++i;
                    // Pre-expand variable names in the arithmetic expression
                    std::string arith_expanded;
                    arith_expanded.reserve(expr.size());
                    for (size_t ai = 0; ai < expr.size(); ) {
                        if (expr[ai] == '$') {
                            ++ai;
                            std::string vn;
                            while (ai < expr.size() && (std::isalnum((unsigned char)expr[ai]) || expr[ai] == '_'))
                                vn += expr[ai++];
                            arith_expanded += getVariableValueWithState(vn, state);
                        } else {
                            arith_expanded += expr[ai++];
                        }
                    }
                    result += std::to_string(evalArith(arith_expanded));
                } else {
                    // $(...) command substitution
                    ++i;
                    int depth = 1;
                    std::string subcmd;
                    while (i < input.size() && depth > 0) {
                        if      (input[i] == '(') { ++depth; subcmd += input[i]; }
                        else if (input[i] == ')') { --depth; if (depth > 0) subcmd += input[i]; }
                        else    { subcmd += input[i]; }
                        ++i;
                    }
                    result += captureSubshell(subcmd);
                }

            } else if (input[i] == '{') {
                ++i;
                // Check for #VAR (length)
                if (i < input.size() && input[i] == '#') {
                    ++i;
                    std::string var_name;
                    while (i < input.size() && input[i] != '}') var_name += input[i++];
                    if (i < input.size()) ++i;
                    const char* v = getenv(var_name.c_str());
                    result += std::to_string(v ? std::string(v).size() : 0);
                } else {
                    std::string var_name;
                    // collect var name chars
                    while (i < input.size() && input[i] != '}' &&
                           input[i] != ':' && input[i] != '#' &&
                           input[i] != '%' && input[i] != '+' &&
                           input[i] != '-' && input[i] != '=' &&
                           input[i] != '?') {
                        var_name += input[i++];
                    }
                    // Check for modifier
                    if (i < input.size() && input[i] != '}') {
                        std::string mod;
                        if (input[i] == ':' && i+1 < input.size()) {
                            mod += ':';
                            ++i;
                            mod += input[i++];
                        } else {
                            mod += input[i++];
                            if (i < input.size() && input[i] == '#') { mod += '#'; ++i; }
                            else if (i < input.size() && input[i] == '%') { mod += '%'; ++i; }
                        }
                        std::string word;
                        while (i < input.size() && input[i] != '}') word += input[i++];
                        if (i < input.size()) ++i;
                        result += applyParamModifier(var_name, mod, word, state);
                    } else {
                        if (i < input.size()) ++i;
                        result += getVariableValueWithState(var_name, state);
                    }
                }

            } else if (input[i] == '?') {
                result += std::to_string(state ? state->last_exit_status : 0);
                ++i;
            } else if (input[i] == '$') {
                // $$  — shell PID
                result += std::to_string(getpid());
                ++i;
            } else if (input[i] == '!') {
                // $! — last background PID
                const char* v = getenv("!");
                result += v ? v : "0";
                ++i;
            } else if (input[i] == '#') {
                // $# — positional param count
                result += std::to_string(state ? state->positional_params.size() : 0);
                ++i;
            } else if (input[i] == '@' || input[i] == '*') {
                // $@ / $* — all positional params
                if (state) {
                    for (size_t k = 0; k < state->positional_params.size(); ++k) {
                        if (k) result += ' ';
                        result += state->positional_params[k];
                    }
                }
                ++i;
            } else if (std::isdigit((unsigned char)input[i])) {
                int n = input[i] - '0';
                ++i;
                if (n == 0) {
                    result += state ? state->script_name : "";
                } else if (state && n <= (int)state->positional_params.size()) {
                    result += state->positional_params[n - 1];
                }
            } else if (std::isalpha(static_cast<unsigned char>(input[i])) || input[i] == '_') {
                std::string var_name;
                while (i < input.size() &&
                       (std::isalnum(static_cast<unsigned char>(input[i])) || input[i] == '_')) {
                    var_name += input[i++];
                }
                result += getVariableValueWithState(var_name, state);
            } else {
                result += '$';
            }
        } else {
            result += c;
            ++i;
        }
    }

    return result;
}

std::string EnvironmentVariableExpander::getVariableValue(const std::string& name) const {
    return getVariableValueWithState(name, nullptr);
}

std::string EnvironmentVariableExpander::getVariableValueWithState(const std::string& name, const ShellState* state) const {
    // Check function-local variable frames first
    if (state && !state->var_frames.empty()) {
        for (int f = (int)state->var_frames.size() - 1; f >= 0; --f) {
            auto it = state->var_frames[f].locals.find(name);
            if (it != state->var_frames[f].locals.end()) return it->second;
        }
    }
    const char* val = getenv(name.c_str());
    if (!val && state && state->nounset && !name.empty()) {
        std::cerr << "helix: " << name << ": unbound variable\n";
    }
    return val ? val : "";
}

} // namespace helix

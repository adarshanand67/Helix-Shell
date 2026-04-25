#ifndef HELIX_SHELL_H
#define HELIX_SHELL_H

#include "types.h"
#include "tokenizer.h"
#include "parser.h"
#include "executor.h"
#include "readline_support.h"
#include "prompt.h"
#include "shell/shell_state.h"
#include "shell/builtin_handler.h"
#include "shell/job_manager.h"
#include <readline/history.h>
#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace helix {

class Shell {
public:
    Shell();
    ~Shell();

    int run();
    int runCommand(const std::string& cmd);
    int runStdin();
    int runScript(const char* path, int argc, char* argv[]);

    bool processInputString(const std::string& input) { return processInput(input); }

private:
    void showPrompt();
    std::string readInput();
    bool processInput(const std::string& input);
    bool runSingleCommand(const std::string& trimmed);

    int runBlock(const std::vector<std::string>& lines);
    bool invokeFunction(const std::string& name, const std::vector<std::string>& args);
    int executeIf(const std::vector<std::string>& lines);
    int executeWhile(const std::vector<std::string>& lines, bool until = false);
    int executeFor(const std::vector<std::string>& lines);
    int executeCase(const std::vector<std::string>& lines);

    std::string expandAliases(const std::string& line) const;
    std::string expandHistory(const std::string& line) const;

    void loadHistory();
    void saveHistory();
    void loadRcFile();

    static std::string trimWhitespace(const std::string& s);

    ShellState state;
    Tokenizer tokenizer;
    Parser parser;
    Executor executor;
    Prompt prompt;

    std::unique_ptr<IBuiltinDispatcher> builtin_dispatcher;
    std::unique_ptr<IJobManager> job_manager;

    std::vector<std::string> session_history_;
    std::chrono::milliseconds last_duration_{0};

    // Multi-line block accumulation
    std::vector<std::string> block_stack_;  // stack of opener keywords
    std::vector<std::string> block_lines_;  // accumulated lines
    std::string pending_func_name_;         // function name being defined
};

} // namespace helix

#endif // HELIX_SHELL_H

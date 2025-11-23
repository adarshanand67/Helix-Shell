#include "shell/builtin_handler.h"
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <cstring>
#include <cstdlib>

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

// BuiltinCommandDispatcher implementation
BuiltinCommandDispatcher::BuiltinCommandDispatcher() {
    handlers["cd"] = std::make_unique<CdCommandHandler>();
    handlers["exit"] = std::make_unique<ExitCommandHandler>();
    handlers["history"] = std::make_unique<HistoryCommandHandler>();
    handlers["jobs"] = std::make_unique<JobsCommandHandler>();
    handlers["fg"] = std::make_unique<FgCommandHandler>();
    handlers["bg"] = std::make_unique<BgCommandHandler>();
    handlers["pwd"] = std::make_unique<PwdCommandHandler>();
    handlers["export"] = std::make_unique<ExportCommandHandler>();
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

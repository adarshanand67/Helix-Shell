#include "prompt.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace helix {

Prompt::Prompt() : last_exit_status_(0) {}

void Prompt::setUserHost(const std::string& user, const std::string& host) {
    user_ = user;
    host_ = host;
}

void Prompt::setCurrentDirectory(const std::string& cwd) {
    current_directory_ = cwd;
}

void Prompt::setHomeDirectory(const std::string& home) {
    home_directory_ = home;
}

void Prompt::setLastExitStatus(int status) {
    last_exit_status_ = status;
}

std::string Prompt::generate() const {
    std::string git_info = getGitInfo();

    return getStatusIcon() + " " +
           getUserHost() +
           getDirectoryDisplay() +
           git_info + "\n" +
           getPromptCharacter();
}

std::string Prompt::getStatusIcon() const {
    if (last_exit_status_ == 0) {
        return Colors::GREEN + "✓" + Colors::RESET;
    } else {
        return Colors::RED + "✗" + Colors::RESET;
    }
}

std::string Prompt::getUserHost() const {
    std::string user_host = user_;
    if (!host_.empty()) {
        user_host += "@" + host_;
    }
    return Colors::BRIGHT_CYAN + Colors::BOLD + user_host + Colors::RESET;
}

std::string Prompt::getDirectoryDisplay() const {
    std::string display_path = shortenPath(current_directory_);
    return Colors::BRIGHT_BLUE + display_path + Colors::RESET;
}

std::string Prompt::getGitInfo() const {
    std::string git_branch = getGitBranch();
    if (!git_branch.empty()) {
        return " " + Colors::BRIGHT_BLACK + "on" + Colors::RESET +
               " " + Colors::BRIGHT_MAGENTA + "±" + Colors::RESET +
               " " + Colors::YELLOW + git_branch + Colors::RESET;
    }
    return "";
}

std::string Prompt::getPromptCharacter() const {
    return Colors::BRIGHT_GREEN + "❯" + Colors::RESET + " ";
}

std::string Prompt::getGitBranch() const {
    // Check if .git directory exists
    std::string git_dir = current_directory_ + "/.git";
    struct stat st;
    if (stat(git_dir.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
        // Not a git repo, check parent directories
        return "";
    }

    // Read .git/HEAD file to get current branch
    std::string head_file = git_dir + "/HEAD";
    std::ifstream file(head_file);
    if (!file.is_open()) {
        return "";
    }

    std::string line;
    if (std::getline(file, line)) {
        // Format: ref: refs/heads/branch-name
        if (line.find("ref: refs/heads/") == 0) {
            return line.substr(16); // Extract branch name
        }
        // Detached HEAD state
        if (line.length() >= 7) {
            return line.substr(0, 7); // Show short commit hash
        }
    }

    return "";
}

std::string Prompt::shortenPath(const std::string& path) const {
    std::string cwd_display = path;

    // Replace home directory with ~
    if (!home_directory_.empty() && path.find(home_directory_) == 0) {
        cwd_display = "~" + path.substr(home_directory_.length());
    }

    // Shorten long paths (keep first and last parts)
    if (cwd_display.length() > 40) {
        size_t slash_pos = cwd_display.find('/', 1);
        if (slash_pos != std::string::npos && slash_pos < cwd_display.length() - 20) {
            std::string start = cwd_display.substr(0, slash_pos + 1);
            std::string end = cwd_display.substr(cwd_display.length() - 20);
            cwd_display = start + "..." + end;
        }
    }

    return cwd_display;
}

} // namespace helix

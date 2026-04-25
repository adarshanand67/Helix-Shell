#include "prompt.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <array>
#include <memory>
#include <cstdio>

namespace helix {

Prompt::Prompt() = default;

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

void Prompt::setLastCommandDuration(std::chrono::milliseconds ms) {
    last_duration_ = ms;
}

std::string Prompt::generate() const {
    std::string right;
    std::string dur = getDurationDisplay();
    if (!dur.empty()) right = " " + dur;

    return getStatusIcon() + " " +
           getUserHost() + " " +
           getDirectoryDisplay() +
           getGitInfo() +
           right + "\n" +
           getPromptCharacter();
}

std::string Prompt::getStatusIcon() const {
    if (last_exit_status_ == 0) {
        return Colors::GREEN + "✓" + Colors::RESET;
    }
    return Colors::RED + "✗" + Colors::RESET + Colors::RED +
           "(" + std::to_string(last_exit_status_) + ")" + Colors::RESET;
}

std::string Prompt::getUserHost() const {
    std::string user_host = user_;
    if (!host_.empty()) user_host += "@" + host_;
    return Colors::BRIGHT_CYAN + Colors::BOLD + user_host + Colors::RESET;
}

std::string Prompt::getDirectoryDisplay() const {
    return Colors::BRIGHT_BLUE + shortenPath(current_directory_) + Colors::RESET;
}

std::string Prompt::getDurationDisplay() const {
    long ms = last_duration_.count();
    if (ms < 2000) return "";  // don't show for fast commands

    std::string s;
    if (ms >= 60000) {
        s = std::to_string(ms / 60000) + "m" + std::to_string((ms % 60000) / 1000) + "s";
    } else {
        s = std::to_string(ms / 1000) + "s";
    }
    return Colors::BRIGHT_BLACK + "took " + s + Colors::RESET;
}

std::string Prompt::getGitInfo() const {
    std::string branch = getGitBranch();
    if (branch.empty()) return "";

    std::string status = getGitStatus();

    return " " + Colors::BRIGHT_BLACK + "on" + Colors::RESET +
           " " + Colors::BRIGHT_MAGENTA + "±" + Colors::RESET +
           " " + Colors::YELLOW + branch + Colors::RESET +
           status;
}

std::string Prompt::getPromptCharacter() const {
    return Colors::BRIGHT_GREEN + "❯" + Colors::RESET + " ";
}

std::string Prompt::getGitBranch() const {
    std::string dir = current_directory_;
    struct stat st;

    while (!dir.empty()) {
        std::string git_dir = dir + "/.git";
        if (stat(git_dir.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            std::ifstream file(git_dir + "/HEAD");
            std::string line;
            if (file && std::getline(file, line)) {
                if (line.find("ref: refs/heads/") == 0) return line.substr(16);
                if (line.length() >= 7) return line.substr(0, 7);
            }
            return "";
        }
        size_t slash = dir.rfind('/');
        if (slash == std::string::npos || slash == 0) break;
        dir = dir.substr(0, slash);
    }

    return "";
}

// Parse `git status --porcelain` output to show dirty/staged/untracked icons
std::string Prompt::getGitStatus() const {
    // Use popen so we don't need to fork/exec ourselves
    std::array<char, 512> buf;
    std::string output;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(
        popen("git -C . status --porcelain 2>/dev/null", "r"), pclose);
    if (!pipe) return "";

    while (fgets(buf.data(), buf.size(), pipe.get())) {
        output += buf.data();
    }

    if (output.empty()) return "";

    bool staged = false, dirty = false, untracked = false;
    std::istringstream ss(output);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.size() < 2) continue;
        char index = line[0];
        char work  = line[1];
        if (index == '?' && work == '?') { untracked = true; continue; }
        if (index != ' ' && index != '?') staged = true;
        if (work  != ' ' && work  != '?') dirty  = true;
    }

    std::string result;
    if (staged)    result += " " + Colors::GREEN    + "●" + Colors::RESET;
    if (dirty)     result += " " + Colors::ORANGE   + "●" + Colors::RESET;
    if (untracked) result += " " + Colors::BRIGHT_BLACK + "●" + Colors::RESET;
    return result;
}

std::string Prompt::shortenPath(const std::string& path) const {
    std::string display = path;

    if (!home_directory_.empty() && display.find(home_directory_) == 0) {
        display = "~" + display.substr(home_directory_.length());
    }

    if (display.length() > 40) {
        size_t slash = display.find('/', 1);
        if (slash != std::string::npos && slash < display.length() - 20) {
            display = display.substr(0, slash + 1) + "..." +
                      display.substr(display.length() - 20);
        }
    }

    return display;
}

} // namespace helix

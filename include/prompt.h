#ifndef HELIX_PROMPT_H
#define HELIX_PROMPT_H

#include <string>
#include <chrono>

namespace helix {

namespace Colors {
    const std::string RESET          = "\033[0m";
    const std::string BOLD           = "\033[1m";
    const std::string GREEN          = "\033[32m";
    const std::string RED            = "\033[31m";
    const std::string YELLOW         = "\033[33m";
    const std::string BRIGHT_BLACK   = "\033[90m";
    const std::string BRIGHT_GREEN   = "\033[92m";
    const std::string BRIGHT_BLUE    = "\033[94m";
    const std::string BRIGHT_MAGENTA = "\033[95m";
    const std::string BRIGHT_CYAN    = "\033[96m";
    const std::string ORANGE         = "\033[38;5;208m";
}

class Prompt {
public:
    Prompt();

    void setUserHost(const std::string& user, const std::string& host);
    void setCurrentDirectory(const std::string& cwd);
    void setHomeDirectory(const std::string& home);
    void setLastExitStatus(int status);
    void setLastCommandDuration(std::chrono::milliseconds ms);

    std::string generate() const;

    // Exposed for testing
    std::string getStatusIcon() const;
    std::string getUserHost() const;
    std::string getDirectoryDisplay() const;
    std::string getGitInfo() const;
    std::string getPromptCharacter() const;
    std::string getDurationDisplay() const;

private:
    std::string getGitBranch() const;
    std::string getGitStatus() const;   // dirty / staged / untracked indicators
    std::string shortenPath(const std::string& path) const;

    std::string user_;
    std::string host_;
    std::string current_directory_;
    std::string home_directory_;
    int last_exit_status_ = 0;
    std::chrono::milliseconds last_duration_{0};
};

} // namespace helix

#endif // HELIX_PROMPT_H

#ifndef HELIX_PROMPT_H
#define HELIX_PROMPT_H

#include <string>
#include <map>

namespace helix {

// ANSI color codes for beautiful prompts
namespace Colors {
    const std::string RESET = "\033[0m";
    const std::string BOLD = "\033[1m";
    const std::string GREEN = "\033[32m";
    const std::string RED = "\033[31m";
    const std::string YELLOW = "\033[33m";
    const std::string BRIGHT_BLACK = "\033[90m";
    const std::string BRIGHT_GREEN = "\033[92m";
    const std::string BRIGHT_BLUE = "\033[94m";
    const std::string BRIGHT_MAGENTA = "\033[95m";
    const std::string BRIGHT_CYAN = "\033[96m";
}

// Prompt class for generating beautiful, informative prompts
class Prompt {
public:
    Prompt();

    // Configure prompt elements
    void setUserHost(const std::string& user, const std::string& host);
    void setCurrentDirectory(const std::string& cwd);
    void setHomeDirectory(const std::string& home);
    void setLastExitStatus(int status);

    // Generate the full prompt string
    std::string generate() const;

    // Get individual prompt components (for testing)
    std::string getStatusIcon() const;
    std::string getUserHost() const;
    std::string getDirectoryDisplay() const;
    std::string getGitInfo() const;
    std::string getPromptCharacter() const;

private:
    // Git branch detection
    std::string getGitBranch() const;

    // Path shortening logic
    std::string shortenPath(const std::string& path) const;

private:
    std::string user_;
    std::string host_;
    std::string current_directory_;
    std::string home_directory_;
    int last_exit_status_;
};

} // namespace helix

#endif // HELIX_PROMPT_H

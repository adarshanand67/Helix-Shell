#ifndef HELIX_READLINE_SUPPORT_H
#define HELIX_READLINE_SUPPORT_H

#include <string>
#include <vector>

namespace helix {

// Readline-based autocompletion support
class ReadlineSupport {
public:
    static void initialize();
    static void cleanup();
    static std::string readLineWithCompletion(const std::string& prompt);

    // Completion callbacks
    static char** completionCallback(const char* text, int start, int end);
    static char* commandGenerator(const char* text, int state);
    static char* pathGenerator(const char* text, int state);

    // Set available commands for completion
    static void setCommands(const std::vector<std::string>& commands);

    // Public for friend functions
    static std::vector<std::string> available_commands;
    static int completion_index;
};

} // namespace helix

#endif // HELIX_READLINE_SUPPORT_H

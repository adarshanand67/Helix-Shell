#ifndef HSHELL_PARSER_H
#define HSHELL_PARSER_H

#include "types.h" // Includes type definitions: Token, TokenType, ParsedCommand, Command structs, and Pipeline structures used in parsing.
#include <vector> // Provides std::vector for storing sequences of parsed commands, tokens, and command arguments.
#include <string> // Provides std::string for storing token values, filenames in redirections, and command arguments.

namespace hshell {

// Command Parser - converts tokens into structured commands
class Parser {
public:
    // Parse a sequence of tokens into a ParsedCommand
    // Handles pipelines, redirections, and command arguments
    ParsedCommand parse(const std::vector<Token>& tokens);

private:
    // Helper methods for parsing different parts
    Command parseSingleCommand(size_t& pos, const std::vector<Token>& tokens);
    void parseArguments(size_t& pos, const std::vector<Token>& tokens, Command& cmd);
    void parseRedirections(size_t& pos, const std::vector<Token>& tokens, Command& cmd);
    std::string extractFilename(const Token& token);
};

} // namespace hshell

#endif // HSHELL_PARSER_H

#include "parser.h"
#include <iostream>

namespace hshell {

ParsedCommand Parser::parse(const std::vector<Token>& tokens) {
    ParsedCommand result;
    size_t pos = 0;

    // Parse the pipeline of commands
    while (pos < tokens.size() && tokens[pos].type != TokenType::END_OF_INPUT) {
        // Parse a single command in the pipeline
        Command cmd = parseSingleCommand(pos, tokens);
        result.pipeline.commands.push_back(cmd);

        // Check if there's a pipe to continue the pipeline
        if (pos < tokens.size() && tokens[pos].type == TokenType::PIPE) {
            pos++; // Skip the pipe token
            // Continue to next command in pipeline
        } else {
            break; // End of pipeline
        }
    }

    // Check for background execution (&)
    if (pos < tokens.size() && tokens[pos].type == TokenType::BACKGROUND) {
        result.background = true;
        pos++;
    }

    // The pipeline should end at END_OF_INPUT
    if (pos >= tokens.size() || tokens[pos].type != TokenType::END_OF_INPUT) {
        // Error: unexpected tokens
        std::cerr << "Parse error: unexpected tokens at end of command\n";
    }

    return result;
}

Command Parser::parseSingleCommand(size_t& pos, const std::vector<Token>& tokens) {
    Command cmd;

    // Parse command arguments
    parseArguments(pos, tokens, cmd);

    // Parse redirections
    parseRedirections(pos, tokens, cmd);

    return cmd;
}

void Parser::parseArguments(size_t& pos, const std::vector<Token>& tokens, Command& cmd) {
    // Collect all WORD tokens until we hit a redirection, pipe, background, semicolon, or end
    while (pos < tokens.size()) {
        TokenType type = tokens[pos].type;

        if (type == TokenType::WORD) {
            cmd.args.push_back(tokens[pos].value);
            pos++;
        } else if (type == TokenType::PIPE || type == TokenType::BACKGROUND ||
                   type == TokenType::SEMICOLON || type == TokenType::END_OF_INPUT) {
            // Stop parsing arguments
            break;
        } else {
            // Hit a redirection, parse it later
            break;
        }
    }
}

void Parser::parseRedirections(size_t& pos, const std::vector<Token>& tokens, Command& cmd) {
    while (pos < tokens.size()) {
        TokenType type = tokens[pos].type;

        if (type == TokenType::REDIRECT_IN) {
            // Input redirection: < filename
            pos++; // Skip <
            if (pos < tokens.size() && tokens[pos].type == TokenType::WORD) {
                cmd.input_file = tokens[pos].value;
                pos++;
            } else {
                std::cerr << "Parse error: expected filename after <\n";
                break;
            }
        } else if (type == TokenType::REDIRECT_OUT) {
            // Output redirection: > filename
            pos++; // Skip >
            if (pos < tokens.size() && tokens[pos].type == TokenType::WORD) {
                cmd.output_file = tokens[pos].value;
                cmd.append_mode = false;
                pos++;
            } else {
                std::cerr << "Parse error: expected filename after >\n";
                break;
            }
        } else if (type == TokenType::REDIRECT_OUT_APPEND) {
            // Append output redirection: >> filename
            pos++; // Skip >>
            if (pos < tokens.size() && tokens[pos].type == TokenType::WORD) {
                cmd.output_file = tokens[pos].value;
                cmd.append_mode = true;
                pos++;
            } else {
                std::cerr << "Parse error: expected filename after >>\n";
                break;
            }
        } else if (type == TokenType::REDIRECT_ERR) {
            // Error redirection: 2> filename
            pos++; // Skip 2>
            if (pos < tokens.size() && tokens[pos].type == TokenType::WORD) {
                cmd.error_file = tokens[pos].value;
                cmd.error_append_mode = false;
                pos++;
            } else {
                std::cerr << "Parse error: expected filename after 2>\n";
                break;
            }
        } else if (type == TokenType::REDIRECT_ERR_APPEND) {
            // Append error redirection: 2>> filename
            pos++; // Skip 2>>
            if (pos < tokens.size() && tokens[pos].type == TokenType::WORD) {
                cmd.error_file = tokens[pos].value;
                cmd.error_append_mode = true;
                pos++;
            } else {
                std::cerr << "Parse error: expected filename after 2>>\n";
                break;
            }
        } else {
            // Not a redirection token, stop parsing redirections
            break;
        }
    }
}

std::string Parser::extractFilename(const Token& token) {
    // This method might be extended for more complex filename handling
    return token.value;
}

} // namespace hshell

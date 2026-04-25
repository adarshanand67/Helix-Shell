#include "parser.h"
#include <iostream>

namespace helix {

ParsedCommand Parser::parse(const std::vector<Token>& tokens) {
    ParsedCommand result;
    size_t pos = 0;

    while (pos < tokens.size() && tokens[pos].type != TokenType::END_OF_INPUT) {
        // Skip leading semicolons / newlines
        if (tokens[pos].type == TokenType::SEMICOLON || tokens[pos].type == TokenType::NEWLINE) {
            ++pos; continue;
        }

        Command cmd = parseSingleCommand(pos, tokens);
        result.pipeline.commands.push_back(cmd);

        if (pos < tokens.size() && tokens[pos].type == TokenType::PIPE) {
            ++pos;
        } else {
            break;
        }
    }

    if (pos < tokens.size() && tokens[pos].type == TokenType::BACKGROUND) {
        result.background = true;
        ++pos;
    }

    return result;
}

Command Parser::parseSingleCommand(size_t& pos, const std::vector<Token>& tokens) {
    Command cmd;
    parseArguments(pos, tokens, cmd);
    parseRedirections(pos, tokens, cmd);
    return cmd;
}

void Parser::parseArguments(size_t& pos, const std::vector<Token>& tokens, Command& cmd) {
    while (pos < tokens.size()) {
        TokenType type = tokens[pos].type;

        if (type == TokenType::WORD) {
            cmd.args.push_back(tokens[pos].value);
            ++pos;
        } else if (type == TokenType::PIPE || type == TokenType::BACKGROUND ||
                   type == TokenType::SEMICOLON || type == TokenType::NEWLINE ||
                   type == TokenType::END_OF_INPUT) {
            break;
        } else {
            break; // redirection follows
        }
    }
}

void Parser::parseRedirections(size_t& pos, const std::vector<Token>& tokens, Command& cmd) {
    while (pos < tokens.size()) {
        TokenType type = tokens[pos].type;

        auto needWord = [&]() -> bool {
            ++pos;
            return pos < tokens.size() && tokens[pos].type == TokenType::WORD;
        };

        if (type == TokenType::REDIRECT_IN) {
            if (needWord()) { cmd.input_file = tokens[pos].value; ++pos; }
            else { std::cerr << "Parse error: expected filename after <\n"; break; }

        } else if (type == TokenType::REDIRECT_OUT) {
            if (needWord()) { cmd.output_file = tokens[pos].value; cmd.append_mode = false; ++pos; }
            else { std::cerr << "Parse error: expected filename after >\n"; break; }

        } else if (type == TokenType::REDIRECT_OUT_APPEND) {
            if (needWord()) { cmd.output_file = tokens[pos].value; cmd.append_mode = true; ++pos; }
            else { std::cerr << "Parse error: expected filename after >>\n"; break; }

        } else if (type == TokenType::REDIRECT_ERR) {
            if (needWord()) { cmd.error_file = tokens[pos].value; cmd.error_append_mode = false; ++pos; }
            else { std::cerr << "Parse error: expected filename after 2>\n"; break; }

        } else if (type == TokenType::REDIRECT_ERR_APPEND) {
            if (needWord()) { cmd.error_file = tokens[pos].value; cmd.error_append_mode = true; ++pos; }
            else { std::cerr << "Parse error: expected filename after 2>>\n"; break; }

        } else if (type == TokenType::REDIRECT_ERR_TO_OUT) {
            cmd.stderr_to_stdout = true;
            ++pos;

        } else if (type == TokenType::REDIRECT_BOTH) {
            if (needWord()) { cmd.output_file = tokens[pos].value; cmd.both_to_file = true; cmd.both_append = false; ++pos; }
            else { std::cerr << "Parse error: expected filename after &>\n"; break; }

        } else if (type == TokenType::REDIRECT_BOTH_APPEND) {
            if (needWord()) { cmd.output_file = tokens[pos].value; cmd.both_to_file = true; cmd.both_append = true; ++pos; }
            else { std::cerr << "Parse error: expected filename after &>>\n"; break; }

        } else if (type == TokenType::HEREDOC || type == TokenType::HEREDOC_STRIP) {
            bool strip = (type == TokenType::HEREDOC_STRIP);
            ++pos;
            if (pos < tokens.size() && tokens[pos].type == TokenType::WORD) {
                cmd.heredoc_delim = tokens[pos].value;
                cmd.heredoc_strip = strip;
                ++pos;
            } else {
                std::cerr << "Parse error: expected delimiter after <<\n"; break;
            }

        } else if (type == TokenType::HERESTRING) {
            ++pos;
            if (pos < tokens.size() && tokens[pos].type == TokenType::WORD) {
                cmd.herestring = tokens[pos].value;
                ++pos;
            } else {
                std::cerr << "Parse error: expected word after <<<\n"; break;
            }

        } else {
            break;
        }
    }
}

std::string Parser::extractFilename(const Token& token) {
    return token.value;
}

} // namespace helix

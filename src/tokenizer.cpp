#include "tokenizer.h"
#include <iostream>

namespace helix {

std::vector<Token> Tokenizer::tokenize(const std::string &input) {
    std::vector<Token> tokens;
    std::string current;
    TokenizerState state = TokenizerState::NORMAL;
    size_t i = 0;

    while (i < input.length()) {
        switch (state) {
        case TokenizerState::NORMAL:
            i += processNormalState(input, i, tokens, current, state);
            break;
        case TokenizerState::IN_DOUBLE_QUOTE:
            i += processDoubleQuoteState(input, i, current, state);
            break;
        case TokenizerState::IN_SINGLE_QUOTE:
            i += processSingleQuoteState(input, i, current, state);
            break;
        }
    }

    if (!current.empty()) {
        tokens.push_back({TokenType::WORD, current});
    }

    tokens.push_back({TokenType::END_OF_INPUT, ""});
    return tokens;
}

size_t Tokenizer::processNormalState(const std::string &input, size_t i,
                                     std::vector<Token> &tokens,
                                     std::string &current,
                                     TokenizerState &state) {
    char c = input[i];

    if (std::isspace(c) && c != '\n') {
        if (!current.empty()) {
            tokens.push_back({TokenType::WORD, current});
            current.clear();
        }
        return 1;
    }

    if (c == '\n') {
        if (!current.empty()) {
            tokens.push_back({TokenType::WORD, current});
            current.clear();
        }
        tokens.push_back({TokenType::NEWLINE, "\n"});
        return 1;
    }

    if (c == '"') {
        if (!current.empty()) {
            tokens.push_back({TokenType::WORD, current});
            current.clear();
        }
        state = TokenizerState::IN_DOUBLE_QUOTE;
        return 1;
    }

    if (c == '\'') {
        if (!current.empty()) {
            tokens.push_back({TokenType::WORD, current});
            current.clear();
        }
        state = TokenizerState::IN_SINGLE_QUOTE;
        return 1;
    }

    if (c == '\\') {
        if (i + 1 < input.length()) {
            current += input[i + 1];
            return 2;
        }
        return 1;
    }

    // Multi-character special tokens
    if (c == '2' && i + 1 < input.length() && input[i+1] == '>') {
        if (!current.empty()) { tokens.push_back({TokenType::WORD, current}); current.clear(); }
        if (i + 2 < input.length() && input[i+2] == '>') {
            tokens.push_back({TokenType::REDIRECT_ERR_APPEND, "2>>"});
            return 3;
        }
        // 2>&1
        if (i + 3 < input.length() && input[i+2] == '&' && input[i+3] == '1') {
            tokens.push_back({TokenType::REDIRECT_ERR_TO_OUT, "2>&1"});
            return 4;
        }
        tokens.push_back({TokenType::REDIRECT_ERR, "2>"});
        return 2;
    }

    if (c == '&') {
        if (!current.empty()) { tokens.push_back({TokenType::WORD, current}); current.clear(); }
        if (i + 1 < input.length() && input[i+1] == '>') {
            if (i + 2 < input.length() && input[i+2] == '>') {
                tokens.push_back({TokenType::REDIRECT_BOTH_APPEND, "&>>"});
                return 3;
            }
            tokens.push_back({TokenType::REDIRECT_BOTH, "&>"});
            return 2;
        }
        tokens.push_back({TokenType::BACKGROUND, "&"});
        return 1;
    }

    if (c == '<') {
        if (!current.empty()) { tokens.push_back({TokenType::WORD, current}); current.clear(); }
        if (i + 1 < input.length() && input[i+1] == '<') {
            if (i + 2 < input.length() && input[i+2] == '<') {
                tokens.push_back({TokenType::HERESTRING, "<<<"});
                return 3;
            }
            if (i + 2 < input.length() && input[i+2] == '-') {
                tokens.push_back({TokenType::HEREDOC_STRIP, "<<-"});
                return 3;
            }
            tokens.push_back({TokenType::HEREDOC, "<<"});
            return 2;
        }
        tokens.push_back({TokenType::REDIRECT_IN, "<"});
        return 1;
    }

    if (c == '>') {
        if (!current.empty()) { tokens.push_back({TokenType::WORD, current}); current.clear(); }
        if (i + 1 < input.length() && input[i+1] == '>') {
            tokens.push_back({TokenType::REDIRECT_OUT_APPEND, ">>"});
            return 2;
        }
        tokens.push_back({TokenType::REDIRECT_OUT, ">"});
        return 1;
    }

    if (c == '|') {
        if (!current.empty()) { tokens.push_back({TokenType::WORD, current}); current.clear(); }
        tokens.push_back({TokenType::PIPE, "|"});
        return 1;
    }

    if (c == ';') {
        if (!current.empty()) { tokens.push_back({TokenType::WORD, current}); current.clear(); }
        tokens.push_back({TokenType::SEMICOLON, ";"});
        return 1;
    }

    current += c;
    return 1;
}

size_t Tokenizer::processDoubleQuoteState(const std::string &input, size_t i,
                                          std::string &current,
                                          TokenizerState &state) {
    char c = input[i];

    if (c == '"') {
        state = TokenizerState::NORMAL;
        return 1;
    }

    if (c == '\\' && i + 1 < input.length()) {
        char next = input[i + 1];
        if (next == '"' || next == '\\' || next == '$' || next == '`') {
            current += next;
            return 2;
        }
    }

    current += c;
    return 1;
}

size_t Tokenizer::processSingleQuoteState(const std::string &input, size_t i,
                                          std::string &current,
                                          TokenizerState &state) {
    if (input[i] == '\'') {
        state = TokenizerState::NORMAL;
        return 1;
    }

    current += input[i];
    return 1;
}

bool Tokenizer::handleSpecialTokens(const std::string &input, size_t i,
                                    std::vector<Token> &tokens,
                                    std::string &current) {
    (void)input; (void)i; (void)tokens; (void)current;
    return false;
}

size_t Tokenizer::getTokenLength(const std::string &input, size_t i) {
    (void)input; (void)i;
    return 1;
}

} // namespace helix

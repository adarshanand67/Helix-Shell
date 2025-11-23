#include "tokenizer.h"
#include <iostream>

namespace hshell {

// Simple tokenizer implementation based on the PRD requirements
// The algorithm uses a state machine approach similar to what might be found
// in standard C libraries but implemented with modern C++ features.

std::vector<Token> Tokenizer::tokenize(const std::string& input) {
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

    // Flush any remaining token
    if (!current.empty()) {
        tokens.push_back({TokenType::WORD, current});
    }

    // Add end marker
    tokens.push_back({TokenType::END_OF_INPUT, ""});

    return tokens;
}

size_t Tokenizer::processNormalState(const std::string& input, size_t i, std::vector<Token>& tokens,
                                    std::string& current, TokenizerState& state) {
    char c = input[i];

    if (std::isspace(c)) {
        // Flush current token on space
        if (!current.empty()) {
            tokens.push_back({TokenType::WORD, current});
            current.clear();
        }
        return 1;
    }

    if (c == '"') {
        // Flush current and start double quote
        if (!current.empty()) {
            tokens.push_back({TokenType::WORD, current});
            current.clear();
        }
        state = TokenizerState::IN_DOUBLE_QUOTE;
        return 1;
    }

    if (c == '\'') {
        // Flush current and start single quote
        if (!current.empty()) {
            tokens.push_back({TokenType::WORD, current});
            current.clear();
        }
        state = TokenizerState::IN_SINGLE_QUOTE;
        return 1;
    }

    if (c == '\\') {
        // Handle escape sequence
        if (i + 1 < input.length()) {
            current += input[i + 1];
            return 2;
        }
        return 1;
    }

    // Check for special tokens only if this character could start one
    if (c == '|' || c == '<' || c == '>' || c == '&' || c == ';' || c == '2' ||
        (c == '2' && i + 1 < input.length() && input[i+1] == '>')) {
        if (handleSpecialTokens(input, i, tokens, current)) {
            return getTokenLength(input, i);
        }
    }

    // Regular character
    current += c;
    return 1;
}

size_t Tokenizer::processDoubleQuoteState(const std::string& input, size_t i,
                                         std::string& current, TokenizerState& state) {
    char c = input[i];

    if (c == '"') {
        state = TokenizerState::NORMAL;
        return 1;
    }

    if (c == '\\' && i + 1 < input.length()) {
        char next = input[i + 1];
        if (next == '"' || next == '\\' || next == '$') {
            current += next;
            return 2;
        }
    }

    current += c;
    return 1;
}

size_t Tokenizer::processSingleQuoteState(const std::string& input, size_t i,
                                         std::string& current, TokenizerState& state) {
    if (input[i] == '\'') {
        state = TokenizerState::NORMAL;
        return 1;
    }

    current += input[i];
    return 1;
}

bool Tokenizer::handleSpecialTokens(const std::string& input, size_t i, std::vector<Token>& tokens, std::string& current) {
    // Flush current token
    if (!current.empty()) {
        tokens.push_back({TokenType::WORD, current});
        current.clear();
    }

    // Check for multi-character tokens first
    if (i + 1 < input.length()) {
        std::string two = input.substr(i, 2);

        if (two == ">>") {
            tokens.push_back({TokenType::REDIRECT_OUT_APPEND, ">>"});
            return true;
        }
        if (two == "2>") {
            tokens.push_back({TokenType::REDIRECT_ERR, "2>"});
            return true;
        }
        if (two == "2>>") {
            tokens.push_back({TokenType::REDIRECT_ERR_APPEND, "2>>"});
            return true;
        }
    }

    // Single character tokens
    char c = input[i];
    switch (c) {
        case '|':
            tokens.push_back({TokenType::PIPE, "|"});
            return true;
        case '<':
            tokens.push_back({TokenType::REDIRECT_IN, "<"});
            return true;
        case '>':
            tokens.push_back({TokenType::REDIRECT_OUT, ">"});
            return true;
        case '&':
            tokens.push_back({TokenType::BACKGROUND, "&"});
            return true;
        case ';':
            tokens.push_back({TokenType::SEMICOLON, ";"});
            return true;
        default:
            return false;
    }
}

size_t Tokenizer::getTokenLength(const std::string& input, size_t i) {
    char c = input[i];

    if (c == '2') {
        if (i + 2 < input.length()) {
            std::string three = input.substr(i, 3);
            if (three == "2>>") return 3;
            if (three == "2>") return 2;
        }
        // Just "2" by itself - not a special token
        return 0;
    }

    if (i + 1 < input.length()) {
        std::string two = input.substr(i, 2);
        if (two == ">>") return 2;
    }

    // Single character tokens
    return 1;
}

} // namespace hshell

#ifndef HELIX_TOKENIZER_H
#define HELIX_TOKENIZER_H

#include "types.h" // Includes type definitions: Token, TokenType enum for token classification and tokenizer states.
#include <vector> // Provides std::vector for storing the sequence of parsed tokens.
#include <string> // Provides std::string for storing token values and manipulating substrings during tokenization.

namespace helix {

// State machine states for the tokenizer
enum class TokenizerState {
    NORMAL,
    IN_DOUBLE_QUOTE,
    IN_SINGLE_QUOTE
};

class Tokenizer {
public:
    // Tokenize input string into tokens
    // Handles quoting with state machine: Normal, InDoubleQuote, InSingleQuote
    // Treats |, <, >, >>, 2>, 2>>, &, ; as separate tokens
    // Returns vector of Token objects
    std::vector<Token> tokenize(const std::string& input);

private:
    // Main processing methods for each state
    size_t processNormalState(const std::string& input, size_t i, std::vector<Token>& tokens,
                             std::string& current, TokenizerState& state);
    size_t processDoubleQuoteState(const std::string& input, size_t i,
                                  std::string& current, TokenizerState& state);
    size_t processSingleQuoteState(const std::string& input, size_t i,
                                  std::string& current, TokenizerState& state);

    // Helper methods for token handling
    bool handleSpecialTokens(const std::string& input, size_t i, std::vector<Token>& tokens, std::string& current);
    size_t getTokenLength(const std::string& input, size_t i);
};

} // namespace helix

#endif // HELIX_TOKENIZER_H

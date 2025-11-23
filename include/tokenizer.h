#ifndef HSHELL_TOKENIZER_H
#define HSHELL_TOKENIZER_H

#include "types.h"
#include <vector>
#include <string>

namespace hshell {

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
    size_t processDoubleQuoteState(const std::string& input, size_t i, std::vector<Token>& tokens,
                                  std::string& current, TokenizerState& state);
    size_t processSingleQuoteState(const std::string& input, size_t i, std::vector<Token>& tokens,
                                  std::string& current, TokenizerState& state);

    // Helper methods for token handling
    bool handleSpecialTokens(const std::string& input, size_t i, std::vector<Token>& tokens, std::string& current);
    size_t getTokenLength(const std::string& input, size_t i);
};

} // namespace hshell

#endif // HSHELL_TOKENIZER_H

#ifndef HELIX_TEST_HELPERS_H
#define HELIX_TEST_HELPERS_H

#include "../include/parser.h"
#include "../include/tokenizer.h"
#include "../include/types.h"
#include <chrono>
#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <vector>

// Custom assertion macros for better error messages
#define CPPUNIT_ASSERT_TOKENS_EQUAL(expected, actual)                          \
  do {                                                                         \
    std::ostringstream msg;                                                    \
    msg << "Token sequence mismatch\nExpected: ";                              \
    for (const auto &t : expected) {                                           \
      msg << TokenTypeToString(t.type) << ":'" << t.value << "' ";             \
    }                                                                          \
    msg << "\nActual: ";                                                       \
    for (const auto &t : actual) {                                             \
      msg << TokenTypeToString(t.type) << ":'" << t.value << "' ";             \
    }                                                                          \
    CPPUNIT_ASSERT_MESSAGE(msg.str(), areTokensEqual(expected, actual));       \
  } while (0)

#define CPPUNIT_ASSERT_TOKEN_AT(index, expected_type, expected_value, tokens)  \
  do {                                                                         \
    std::ostringstream msg;                                                    \
    msg << "Token at index " << index << " mismatch\n";                        \
    if (static_cast<size_t>(index) >= tokens.size()) {                         \
      msg << "Expected: " << TokenTypeToString(expected_type) << ":'"          \
          << expected_value << "' but token list too short";                   \
      CPPUNIT_FAIL(msg.str());                                                 \
    } else {                                                                   \
      const auto &actual = tokens[index];                                      \
      msg << "Expected: " << TokenTypeToString(expected_type) << ":'"          \
          << expected_value << "'\n";                                          \
      msg << "Actual: " << TokenTypeToString(actual.type) << ":'"              \
          << actual.value << "'";                                              \
      CPPUNIT_ASSERT_EQUAL_MESSAGE(msg.str(), expected_type, actual.type);     \
      CPPUNIT_ASSERT_EQUAL_MESSAGE(msg.str(), expected_value, actual.value);   \
    }                                                                          \
  } while (0)

#define CPPUNIT_ASSERT_NO_PARSE_ERRORS(tokens)                                 \
  do {                                                                         \
    std::vector<Token> processed = tokens;                                     \
    bool hasErrors = false;                                                    \
    std::string errorMsg;                                                      \
    for (const auto &token : processed) {                                      \
      if (!isValidToken(token)) {                                              \
        hasErrors = true;                                                      \
        errorMsg.append(TokenTypeToString(token.type) + " ");                  \
      }                                                                        \
    }                                                                          \
    if (hasErrors) {                                                           \
      std::ostringstream msg;                                                  \
      msg << "Invalid tokens found: " << errorMsg;                             \
      CPPUNIT_FAIL(msg.str());                                                 \
    }                                                                          \
  } while (0)

#define CPPUNIT_TEST_EXCEPTION(exception_type, expression)                     \
  do {                                                                         \
    bool threwException = false;                                               \
    try {                                                                      \
      expression;                                                              \
    } catch (const exception_type &) {                                         \
      threwException = true;                                                   \
    } catch (...) {                                                            \
      std::ostringstream msg;                                                  \
      msg << "Expected exception of type " << #exception_type                  \
          << ", but got different exception type";                             \
      CPPUNIT_FAIL(msg.str());                                                 \
    }                                                                          \
    if (!threwException) {                                                     \
      std::ostringstream msg;                                                  \
      msg << "Expected exception of type " << #exception_type                  \
          << ", but no exception was thrown";                                  \
      CPPUNIT_FAIL(msg.str());                                                 \
    }                                                                          \
  } while (0)

#define PERFORMANCE_TEST(test_name, code_block, max_ms)                        \
  do {                                                                         \
    auto start = std::chrono::high_resolution_clock::now();                    \
    code_block auto end = std::chrono::high_resolution_clock::now();           \
    auto duration =                                                            \
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start);    \
    std::ostringstream msg;                                                    \
    msg << #test_name << " took " << duration.count()                          \
        << "ms (max allowed: " << max_ms << "ms)";                             \
    if (duration.count() > max_ms) {                                           \
      msg << " - PERFORMANCE REGRESSION DETECTED!";                            \
      std::cout << msg.str() << std::endl;                                     \
      /* Don't fail, just warn */                                              \
    } else {                                                                   \
      CPPUNIT_ASSERT_MESSAGE(msg.str(), duration.count() <= max_ms);           \
    }                                                                          \
  } while (0)

// Helper functions
inline std::string TokenTypeToString(TokenType type) {
  switch (type) {
  case TokenType::WORD:
    return "WORD";
  case TokenType::PIPE:
    return "PIPE";
  case TokenType::REDIRECT_IN:
    return "REDIRECT_IN";
  case TokenType::REDIRECT_OUT:
    return "REDIRECT_OUT";
  case TokenType::REDIRECT_OUT_APPEND:
    return "REDIRECT_OUT_APPEND";
  case TokenType::REDIRECT_ERR:
    return "REDIRECT_ERR";
  case TokenType::REDIRECT_ERR_APPEND:
    return "REDIRECT_ERR_APPEND";
  case TokenType::BACKGROUND:
    return "&";
  case TokenType::SEMICOLON:
    return ";";
  case TokenType::END_OF_INPUT:
    return "END_OF_INPUT";
  default:
    return "UNKNOWN";
  }
}

inline bool areTokensEqual(const std::vector<Token> &a,
                           const std::vector<Token> &b) {
  if (a.size() != b.size())
    return false;
  for (size_t i = 0; i < a.size(); ++i) {
    if (a[i].type != b[i].type || a[i].value != b[i].value)
      return false;
  }
  return true;
}

inline bool isValidToken(const Token &token) {
  // Basic validation - can be extended
  return !token.value.empty() || token.type == TokenType::END_OF_INPUT;
}

// Enhanced fixture base class
class EnhancedTestFixture : public CppUnit::TestFixture {
protected:
  void setUp() override {
    tokenizer = std::make_unique<helix::Tokenizer>();
    parser = std::make_unique<helix::Parser>();
  }

  void tearDown() override {
    tokenizer.reset();
    parser.reset();
  }

  std::unique_ptr<helix::Tokenizer> tokenizer;
  std::unique_ptr<helix::Parser> parser;

  // Helper for creating test tokens
  std::vector<helix::Token>
  createTokens(std::initializer_list<std::pair<helix::TokenType, std::string>>
                   token_specs) {
    std::vector<helix::Token> tokens;
    for (const auto &spec : token_specs) {
      tokens.emplace_back(spec.first, spec.second);
    }
    return tokens;
  }

  // Helper for validating parsed commands
  void validateCommand(const helix::Command &cmd,
                       const std::vector<std::string> &expected_args,
                       const std::string &expected_input = "",
                       const std::string &expected_output = "",
                       bool expected_append = false) {
    std::ostringstream msg;
    msg << "Command validation failed";

    CPPUNIT_ASSERT_EQUAL_MESSAGE(msg.str() + " - args count",
                                 expected_args.size(), cmd.args.size());

    for (size_t i = 0; i < expected_args.size(); ++i) {
      if (i < cmd.args.size()) {
        CPPUNIT_ASSERT_EQUAL_MESSAGE(msg.str() + " - arg " + std::to_string(i),
                                     expected_args[i], cmd.args[i]);
      }
    }

    CPPUNIT_ASSERT_EQUAL_MESSAGE(msg.str() + " - input file", expected_input,
                                 cmd.input_file);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(msg.str() + " - output file", expected_output,
                                 cmd.output_file);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(msg.str() + " - append mode", expected_append,
                                 cmd.append_mode);
  }
};

// Test data structure for parameterized testing
struct TokenizerTestCase {
  std::string input;
  std::vector<helix::Token> expected_tokens;
  std::string description;
};

struct ParserTestCase {
  std::vector<helix::Token> input_tokens;
  std::vector<std::string> expected_command_args;
  std::string expected_input_file;
  std::string expected_output_file;
  bool expected_append_mode;
  std::string description;
};

#endif // HELIX_TEST_HELPERS_H

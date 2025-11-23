#include "../include/tokenizer.h" // Includes the Tokenizer class and its tokenize() method for testing various tokenization scenarios.
#include "../include/types.h" // Includes Token, TokenType, and other type definitions needed for creating test tokens and checking types.
#include <iostream> // Provides std::cout for displaying test progress and completion messages.
#include <cassert> // Provides assert() macro for verifying tokenization results in unit tests.

// Simple test functions for basic functionality testing
using namespace hshell;

void test_simple_tokenization() {
    Tokenizer tokenizer;
    auto tokens = tokenizer.tokenize("ls -la");

    assert(tokens.size() == 3);
    assert(tokens[0].type == TokenType::WORD);
    assert(tokens[0].value == "ls");
    assert(tokens[1].type == TokenType::WORD);
    assert(tokens[1].value == "-la");
    assert(tokens[2].type == TokenType::END_OF_INPUT);

    std::cout << "✓ Simple tokenization test passed\n";
}

void test_pipe_tokenization() {
    Tokenizer tokenizer;
    auto tokens = tokenizer.tokenize("cat file | grep text");

    assert(tokens.size() == 6);
    assert(tokens[0].type == TokenType::WORD && tokens[0].value == "cat");
    assert(tokens[1].type == TokenType::WORD && tokens[1].value == "file");
    assert(tokens[2].type == TokenType::PIPE && tokens[2].value == "|");
    assert(tokens[3].type == TokenType::WORD && tokens[3].value == "grep");
    assert(tokens[4].type == TokenType::WORD && tokens[4].value == "text");
    assert(tokens[5].type == TokenType::END_OF_INPUT);

    std::cout << "✓ Pipe tokenization test passed\n";
}

void test_redirection_tokenization() {
    Tokenizer tokenizer;
    auto tokens = tokenizer.tokenize("echo hello > output.txt");

    assert(tokens.size() == 5);
    assert(tokens[0].type == TokenType::WORD && tokens[0].value == "echo");
    assert(tokens[1].type == TokenType::WORD && tokens[1].value == "hello");
    assert(tokens[2].type == TokenType::REDIRECT_OUT && tokens[2].value == ">");
    assert(tokens[3].type == TokenType::WORD && tokens[3].value == "output.txt");
    assert(tokens[4].type == TokenType::END_OF_INPUT);

    std::cout << "✓ Redirection tokenization test passed\n";
}

void test_quoting() {
    Tokenizer tokenizer;

    // Double quotes
    auto tokens = tokenizer.tokenize("echo \"hello world\"");
    assert(tokens.size() == 3);
    assert(tokens[1].type == TokenType::WORD && tokens[1].value == "hello world");

    // Single quotes
    tokens = tokenizer.tokenize("echo 'single quotes'");
    assert(tokens.size() == 3);
    assert(tokens[1].type == TokenType::WORD && tokens[1].value == "single quotes");

    // Escaped characters
    tokens = tokenizer.tokenize("echo hello\\ world");
    assert(tokens.size() == 3);
    assert(tokens[1].type == TokenType::WORD && tokens[1].value == "hello world");

    std::cout << "✓ Quoting tests passed\n";
}

void test_edge_cases() {
    Tokenizer tokenizer;

    // Empty input
    auto tokens = tokenizer.tokenize("");
    assert(tokens.size() == 1 && tokens[0].type == TokenType::END_OF_INPUT);

    // Whitespace only
    tokens = tokenizer.tokenize("   \t   ");
    assert(tokens.size() == 1 && tokens[0].type == TokenType::END_OF_INPUT);

    // Trailing whitespace
    tokens = tokenizer.tokenize("ls   ");
    assert(tokens.size() == 2);
    assert(tokens[0].type == TokenType::WORD && tokens[0].value == "ls");
    assert(tokens[1].type == TokenType::END_OF_INPUT);

    std::cout << "✓ Edge case tests passed\n";
}

void test_complex_command() {
    Tokenizer tokenizer;
    auto tokens = tokenizer.tokenize("cat \"my file.txt\" | grep -i \"search pattern\" > results.txt &");

    assert(tokens.size() == 10);
    assert(tokens[0].type == TokenType::WORD); // cat
    assert(tokens[1].type == TokenType::WORD && tokens[1].value == "my file.txt");
    assert(tokens[2].type == TokenType::PIPE);
    assert(tokens[3].type == TokenType::WORD && tokens[3].value == "grep");
    assert(tokens[4].type == TokenType::WORD && tokens[4].value == "-i");
    assert(tokens[5].type == TokenType::WORD && tokens[5].value == "search pattern");
    assert(tokens[6].type == TokenType::REDIRECT_OUT);
    assert(tokens[7].type == TokenType::WORD && tokens[7].value == "results.txt");
    assert(tokens[8].type == TokenType::BACKGROUND);

    std::cout << "✓ Complex command test passed\n";
}

// Main test runner
void run_tokenizer_tests() {
    std::cout << "Running Tokenizer Tests...\n";

    test_simple_tokenization();
    test_pipe_tokenization();
    test_redirection_tokenization();
    test_quoting();
    test_edge_cases();
    test_complex_command();

    std::cout << "All tokenizer tests passed! ✓\n";
}

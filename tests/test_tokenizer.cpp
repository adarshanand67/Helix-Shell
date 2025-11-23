#include "../include/tokenizer.h" // Includes the Tokenizer class and its tokenize() method for testing various tokenization scenarios.
#include "../include/types.h" // Includes Token, TokenType, and other type definitions needed for creating test tokens and checking types.
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

class TokenizerTest : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(TokenizerTest);
    CPPUNIT_TEST(testSimpleTokenization);
    CPPUNIT_TEST(testPipeTokenization);
    CPPUNIT_TEST(testRedirectionTokenization);
    CPPUNIT_TEST(testQuoting);
    CPPUNIT_TEST(testEdgeCases);
    CPPUNIT_TEST(testComplexCommand);
    CPPUNIT_TEST_SUITE_END();

private:
    hshell::Tokenizer* tokenizer;

public:
    void setUp() override {
        tokenizer = new hshell::Tokenizer();
    }

    void tearDown() override {
        delete tokenizer;
    }

    void testSimpleTokenization() {
        auto tokens = tokenizer->tokenize("ls -la");

        CPPUNIT_ASSERT_EQUAL(size_t(3), tokens.size());
        CPPUNIT_ASSERT_EQUAL(hshell::TokenType::WORD, tokens[0].type);
        CPPUNIT_ASSERT_EQUAL(std::string("ls"), tokens[0].value);
        CPPUNIT_ASSERT_EQUAL(hshell::TokenType::WORD, tokens[1].type);
        CPPUNIT_ASSERT_EQUAL(std::string("-la"), tokens[1].value);
        CPPUNIT_ASSERT_EQUAL(hshell::TokenType::END_OF_INPUT, tokens[2].type);
    }

    void testPipeTokenization() {
        auto tokens = tokenizer->tokenize("cat file | grep text");

        CPPUNIT_ASSERT_EQUAL(size_t(6), tokens.size());
        CPPUNIT_ASSERT(tokens[0].type == hshell::TokenType::WORD && tokens[0].value == "cat");
        CPPUNIT_ASSERT(tokens[1].type == hshell::TokenType::WORD && tokens[1].value == "file");
        CPPUNIT_ASSERT(tokens[2].type == hshell::TokenType::PIPE && tokens[2].value == "|");
        CPPUNIT_ASSERT(tokens[3].type == hshell::TokenType::WORD && tokens[3].value == "grep");
        CPPUNIT_ASSERT(tokens[4].type == hshell::TokenType::WORD && tokens[4].value == "text");
        CPPUNIT_ASSERT(tokens[5].type == hshell::TokenType::END_OF_INPUT);
    }

    void testRedirectionTokenization() {
        auto tokens = tokenizer->tokenize("echo hello > output.txt");

        CPPUNIT_ASSERT_EQUAL(size_t(5), tokens.size());
        CPPUNIT_ASSERT(tokens[0].type == hshell::TokenType::WORD && tokens[0].value == "echo");
        CPPUNIT_ASSERT(tokens[1].type == hshell::TokenType::WORD && tokens[1].value == "hello");
        CPPUNIT_ASSERT(tokens[2].type == hshell::TokenType::REDIRECT_OUT && tokens[2].value == ">");
        CPPUNIT_ASSERT(tokens[3].type == hshell::TokenType::WORD && tokens[3].value == "output.txt");
        CPPUNIT_ASSERT(tokens[4].type == hshell::TokenType::END_OF_INPUT);
    }

    void testQuoting() {
        // Double quotes
        auto tokens = tokenizer->tokenize("echo \"hello world\"");
        CPPUNIT_ASSERT_EQUAL(size_t(3), tokens.size());
        CPPUNIT_ASSERT(tokens[1].type == hshell::TokenType::WORD && tokens[1].value == "hello world");

        // Single quotes
        tokens = tokenizer->tokenize("echo 'single quotes'");
        CPPUNIT_ASSERT_EQUAL(size_t(3), tokens.size());
        CPPUNIT_ASSERT(tokens[1].type == hshell::TokenType::WORD && tokens[1].value == "single quotes");

        // Escaped characters
        tokens = tokenizer->tokenize("echo hello\\ world");
        CPPUNIT_ASSERT_EQUAL(size_t(3), tokens.size());
        CPPUNIT_ASSERT(tokens[1].type == hshell::TokenType::WORD && tokens[1].value == "hello world");
    }

    void testEdgeCases() {
        // Empty input
        auto tokens = tokenizer->tokenize("");
        CPPUNIT_ASSERT_EQUAL(size_t(1), tokens.size());
        CPPUNIT_ASSERT_EQUAL(hshell::TokenType::END_OF_INPUT, tokens[0].type);

        // Whitespace only
        tokens = tokenizer->tokenize("   \t   ");
        CPPUNIT_ASSERT_EQUAL(size_t(1), tokens.size());
        CPPUNIT_ASSERT_EQUAL(hshell::TokenType::END_OF_INPUT, tokens[0].type);

        // Trailing whitespace
        tokens = tokenizer->tokenize("ls   ");
        CPPUNIT_ASSERT_EQUAL(size_t(2), tokens.size());
        CPPUNIT_ASSERT(tokens[0].type == hshell::TokenType::WORD && tokens[0].value == "ls");
        CPPUNIT_ASSERT(tokens[1].type == hshell::TokenType::END_OF_INPUT);
    }

    void testComplexCommand() {
        auto tokens = tokenizer->tokenize("cat \"my file.txt\" | grep -i \"search pattern\" > results.txt &");

        CPPUNIT_ASSERT_EQUAL(size_t(10), tokens.size());
        CPPUNIT_ASSERT_EQUAL(hshell::TokenType::WORD, tokens[0].type); // cat
        CPPUNIT_ASSERT(tokens[1].type == hshell::TokenType::WORD && tokens[1].value == "my file.txt");
        CPPUNIT_ASSERT_EQUAL(hshell::TokenType::PIPE, tokens[2].type);
        CPPUNIT_ASSERT(tokens[3].type == hshell::TokenType::WORD && tokens[3].value == "grep");
        CPPUNIT_ASSERT(tokens[4].type == hshell::TokenType::WORD && tokens[4].value == "-i");
        CPPUNIT_ASSERT(tokens[5].type == hshell::TokenType::WORD && tokens[5].value == "search pattern");
        CPPUNIT_ASSERT_EQUAL(hshell::TokenType::REDIRECT_OUT, tokens[6].type);
        CPPUNIT_ASSERT(tokens[7].type == hshell::TokenType::WORD && tokens[7].value == "results.txt");
        CPPUNIT_ASSERT_EQUAL(hshell::TokenType::BACKGROUND, tokens[8].type);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TokenizerTest);

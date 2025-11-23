#include "../include/parser.h" // Includes the Parser class definition, providing parse() method to convert token sequences into ParsedCommand structures.
#include "../include/tokenizer.h" // Includes the Tokenizer class for generating tokens to test parsing.
#include "../include/types.h" // Includes ParsedCommand, TokenType, and related type definitions.
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

class ParserTest : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(ParserTest);
    CPPUNIT_TEST(testSimpleCommandParsing);
    CPPUNIT_TEST(testPipelineParsing);
    CPPUNIT_TEST(testRedirectionParsing);
    CPPUNIT_TEST(testBackgroundParsing);
    CPPUNIT_TEST(testComplexPipeline);
    CPPUNIT_TEST(testParserErrorRecovery);
    CPPUNIT_TEST_SUITE_END();

private:
    hshell::Tokenizer* tokenizer;
    hshell::Parser* parser;

public:
    void setUp() override {
        tokenizer = new hshell::Tokenizer();
        parser = new hshell::Parser();
    }

    void tearDown() override {
        delete tokenizer;
        delete parser;
    }

    void testSimpleCommandParsing() {
        auto tokens = tokenizer->tokenize("ls -la");
        hshell::ParsedCommand cmd = parser->parse(tokens);

        CPPUNIT_ASSERT_EQUAL(size_t(1), cmd.pipeline.commands.size());
        CPPUNIT_ASSERT_EQUAL(size_t(2), cmd.pipeline.commands[0].args.size());
        CPPUNIT_ASSERT_EQUAL(std::string("ls"), cmd.pipeline.commands[0].args[0]);
        CPPUNIT_ASSERT_EQUAL(std::string("-la"), cmd.pipeline.commands[0].args[1]);
        CPPUNIT_ASSERT_EQUAL(false, cmd.background);
        CPPUNIT_ASSERT(cmd.pipeline.commands[0].input_file.empty());
        CPPUNIT_ASSERT(cmd.pipeline.commands[0].output_file.empty());
    }

    void testPipelineParsing() {
        auto tokens = tokenizer->tokenize("cat file.txt | grep \"search term\" | sort");
        hshell::ParsedCommand cmd = parser->parse(tokens);

        CPPUNIT_ASSERT_EQUAL(size_t(3), cmd.pipeline.commands.size());

        // First command: cat file.txt
        CPPUNIT_ASSERT_EQUAL(size_t(2), cmd.pipeline.commands[0].args.size());
        CPPUNIT_ASSERT_EQUAL(std::string("cat"), cmd.pipeline.commands[0].args[0]);
        CPPUNIT_ASSERT_EQUAL(std::string("file.txt"), cmd.pipeline.commands[0].args[1]);

        // Second command: grep "search term"
        CPPUNIT_ASSERT_EQUAL(size_t(2), cmd.pipeline.commands[1].args.size());
        CPPUNIT_ASSERT_EQUAL(std::string("grep"), cmd.pipeline.commands[1].args[0]);
        CPPUNIT_ASSERT_EQUAL(std::string("search term"), cmd.pipeline.commands[1].args[1]);

        // Third command: sort
        CPPUNIT_ASSERT_EQUAL(size_t(1), cmd.pipeline.commands[2].args.size());
        CPPUNIT_ASSERT_EQUAL(std::string("sort"), cmd.pipeline.commands[2].args[0]);
    }

    void testRedirectionParsing() {
        // Input redirection
        auto tokens = tokenizer->tokenize("cat < input.txt");
        hshell::ParsedCommand cmd = parser->parse(tokens);
        CPPUNIT_ASSERT_EQUAL(std::string("input.txt"), cmd.pipeline.commands[0].input_file);
        CPPUNIT_ASSERT(cmd.pipeline.commands[0].output_file.empty());

        // Output redirection
        tokens = tokenizer->tokenize("echo hello > output.txt");
        cmd = parser->parse(tokens);
        CPPUNIT_ASSERT_EQUAL(std::string("output.txt"), cmd.pipeline.commands[0].output_file);
        CPPUNIT_ASSERT_EQUAL(false, cmd.pipeline.commands[0].append_mode);

        // Append redirection
        tokens = tokenizer->tokenize("echo hello >> output.txt");
        cmd = parser->parse(tokens);
        CPPUNIT_ASSERT_EQUAL(std::string("output.txt"), cmd.pipeline.commands[0].output_file);
        CPPUNIT_ASSERT_EQUAL(true, cmd.pipeline.commands[0].append_mode);

        // Error redirection
        tokens = tokenizer->tokenize("command 2> error.log");
        cmd = parser->parse(tokens);
        CPPUNIT_ASSERT_EQUAL(std::string("error.log"), cmd.pipeline.commands[0].error_file);
        CPPUNIT_ASSERT_EQUAL(false, cmd.pipeline.commands[0].error_append_mode);

        // Error append
        tokens = tokenizer->tokenize("command 2>> error.log");
        cmd = parser->parse(tokens);
        CPPUNIT_ASSERT_EQUAL(std::string("error.log"), cmd.pipeline.commands[0].error_file);
        CPPUNIT_ASSERT_EQUAL(true, cmd.pipeline.commands[0].error_append_mode);
    }

    void testBackgroundParsing() {
        auto tokens = tokenizer->tokenize("sleep 10 &");
        hshell::ParsedCommand cmd = parser->parse(tokens);

        CPPUNIT_ASSERT_EQUAL(true, cmd.background);
        CPPUNIT_ASSERT_EQUAL(std::string("sleep"), cmd.pipeline.commands[0].args[0]);
    }

    void testComplexPipeline() {
        auto tokens = tokenizer->tokenize("cat input.txt | grep \"pattern\" > results.txt &");
        hshell::ParsedCommand cmd = parser->parse(tokens);

        CPPUNIT_ASSERT_EQUAL(size_t(2), cmd.pipeline.commands.size());
        CPPUNIT_ASSERT_EQUAL(true, cmd.background);

        // First command
        CPPUNIT_ASSERT_EQUAL(std::string("cat"), cmd.pipeline.commands[0].args[0]);
        CPPUNIT_ASSERT_EQUAL(std::string("input.txt"), cmd.pipeline.commands[0].args[1]);

        // Second command with redirection
        CPPUNIT_ASSERT_EQUAL(std::string("grep"), cmd.pipeline.commands[1].args[0]);
        CPPUNIT_ASSERT_EQUAL(std::string("pattern"), cmd.pipeline.commands[1].args[1]);
        CPPUNIT_ASSERT_EQUAL(std::string("results.txt"), cmd.pipeline.commands[1].output_file);
    }

    void testParserErrorRecovery() {
        // Parse normally formed command
        auto tokens = tokenizer->tokenize("ls -l");
        hshell::ParsedCommand cmd = parser->parse(tokens);
        CPPUNIT_ASSERT_EQUAL(size_t(1), cmd.pipeline.commands.size());

        // Another command should parse correctly after the first
        tokens = tokenizer->tokenize("pwd");
        cmd = parser->parse(tokens);
        CPPUNIT_ASSERT_EQUAL(size_t(1), cmd.pipeline.commands.size());
        CPPUNIT_ASSERT_EQUAL(std::string("pwd"), cmd.pipeline.commands[0].args[0]);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(ParserTest);

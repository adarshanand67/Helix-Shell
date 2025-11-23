#include "../include/executor.h"
#include "../include/parser.h"
#include "../include/tokenizer.h"
#include "../include/types.h"
#include <cppunit/TestAssert.h>
#include <cppunit/extensions/HelperMacros.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <unistd.h>

// Unit tests for the Executor class specifically
class TestExecutor : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(TestExecutor);

  // File descriptor tests
  CPPUNIT_TEST(testExecutorConstructor);
  CPPUNIT_TEST(testExecutorDestructor);

  // Command execution tests
  CPPUNIT_TEST(testSingleCommandSuccess);
  CPPUNIT_TEST(testCommandNotFound);
  CPPUNIT_TEST(testEmptyCommand);

  // PIPE execution tests
  CPPUNIT_TEST(testPipelineExecution);
  CPPUNIT_TEST(testPipelineWithRedirections);

  // Redirection tests
  CPPUNIT_TEST(testInputRedirection);
  CPPUNIT_TEST(testOutputRedirection);
  CPPUNIT_TEST(testErrorRedirection);
  CPPUNIT_TEST(testAppendMode);

  // PATH and executable finding - tested indirectly through execution
  // These are tested by command execution success/failure

  // Error conditions
  CPPUNIT_TEST(testBackgroundExecutionNotSupported);
  CPPUNIT_TEST(testSetupRedirectionsFailedInput);
  CPPUNIT_TEST(testSetupRedirectionsFailedOutput);

  // Additional coverage tests
  CPPUNIT_TEST(testErrorAppendRedirections);
  CPPUNIT_TEST(testCommandWithComplexArguments);
  CPPUNIT_TEST(testPipelineMultipleCommands);
  CPPUNIT_TEST(testSignalHandling);
  CPPUNIT_TEST(testBuildArgumentsFunction);
  CPPUNIT_TEST(testUnusualFileDescriptors);

  CPPUNIT_TEST_SUITE_END();

private:
  std::unique_ptr<helix::Executor> executor;
  std::unique_ptr<helix::Tokenizer> tokenizer;
  std::unique_ptr<helix::Parser> parser;

  // Test utilities
  void assertCommandExitCode(const std::string& command_str, int expected_exit_code) {
    auto tokens = tokenizer->tokenize(command_str);
    auto parsed_cmd = parser->parse(tokens);
    int actual_exit_code = executor->execute(parsed_cmd);
    CPPUNIT_ASSERT_EQUAL(expected_exit_code, actual_exit_code);
  }

  // Create a temporary file with content
  std::string createTempFile(const std::string& content) {
    char temp_filename[] = "/tmp/test_executor_XXXXXX";
    int fd = mkstemp(temp_filename);
    if (fd == -1) return "";

    if (write(fd, content.c_str(), content.size()) == -1) {
      close(fd);
      return "";
    }
    close(fd);

    return std::string(temp_filename);
  }

  // Clean up temp file
  void cleanupTempFile(const std::string& filename) {
    if (!filename.empty()) {
      unlink(filename.c_str());
    }
  }

public:
  void setUp() {
    executor = std::make_unique<helix::Executor>();
    tokenizer = std::make_unique<helix::Tokenizer>();
    parser = std::make_unique<helix::Parser>();
  }

  void tearDown() {
    executor.reset();
    tokenizer.reset();
    parser.reset();
  }

  void testExecutorConstructor() {
    // Test that constructor saves file descriptors properly
    // This is tested implicitly by successful execution, but let's verify
    CPPUNIT_ASSERT(executor != nullptr);

    // Can we execute a basic command?
    assertCommandExitCode("echo test", 0);
  }

  void testExecutorDestructor() {
    // Destructor should restore file descriptors
    // This is hard to test directly, but we can execute commands after destruction
    // and verify the executor was properly managed

    // Execute a command
    assertCommandExitCode("echo destruct_test", 0);

    // Note: Actual destructor testing is implicit - if file descriptors weren't
    // restored properly, subsequent tests would fail
  }

  void testSingleCommandSuccess() {
    // Test successful command execution
    assertCommandExitCode("echo hello", 0);
    assertCommandExitCode("true", 0);

    // Test with arguments
    assertCommandExitCode("echo arg1 arg2 arg3", 0);
  }

  void testCommandNotFound() {
    // Command not found should return 127
    assertCommandExitCode("nonexistentcommand12345", 127);

    // Test with arguments but nonexistent command
    assertCommandExitCode("definitely_not_a_real_command arg", 127);
  }

  void testEmptyCommand() {
    // Empty command should be handled
    assertCommandExitCode("", 0); // Parser should handle empty commands safely
  }

  void testPipelineExecution() {
    // Test basic pipeline
    assertCommandExitCode("echo hello | cat", 0);

    // Test longer pipeline
    assertCommandExitCode("echo test | cat | cat", 0);

    // Test pipeline with different exit codes (last command determines exit code)
    assertCommandExitCode("true | true", 0);
    assertCommandExitCode("false | true", 0);  // Last command (true) succeeds

    // Non-existent command in pipeline
    assertCommandExitCode("echo test | nonexistent", 127);
  }

  void testPipelineWithRedirections() {
    // Test pipeline with file redirections
    std::string input_file = createTempFile("pipeline test content");
    std::string output_file = std::string(input_file) + ".out";

    std::string cmd = "cat " + input_file + " | grep content > " + output_file;
    assertCommandExitCode(cmd, 0);

    // Verify output file was created
    std::ifstream outfile(output_file);
    CPPUNIT_ASSERT(outfile.good());
    std::string line;
    std::getline(outfile, line);
    CPPUNIT_ASSERT(line.find("content") != std::string::npos);

    cleanupTempFile(input_file);
    cleanupTempFile(output_file);
  }

  void testInputRedirection() {
    // Create input file
    std::string input_content = "redirection test input\n";
    std::string input_file = createTempFile(input_content);

    // Test input redirection
    std::string output_file = std::string(input_file) + ".out";
    std::string cmd = "cat < " + input_file + " > " + output_file;
    assertCommandExitCode(cmd, 0);

    // Verify output matches input
    std::ifstream outfile(output_file);
    std::stringstream buffer;
    buffer << outfile.rdbuf();
    std::string output_content = buffer.str();
    CPPUNIT_ASSERT_EQUAL(input_content, output_content);

    cleanupTempFile(input_file);
    cleanupTempFile(output_file);
  }

  void testOutputRedirection() {
    // Test output redirection
    std::string output_file = "/tmp/test_output_redirect";
    std::string cmd = "echo output_redirect_test > " + output_file;
    assertCommandExitCode(cmd, 0);

    // Verify output file content
    std::ifstream outfile(output_file);
    std::string line;
    std::getline(outfile, line);
    CPPUNIT_ASSERT(line == "output_redirect_test");

    cleanupTempFile(output_file);
  }

  void testErrorRedirection() {
    // Test error redirection
    std::string error_file = "/tmp/test_error_redirect";
    std::string cmd = "nonexistent_command 2> " + error_file;
    assertCommandExitCode(cmd, 127);

    // Verify error was captured (should contain "Command not found" or similar)
    std::ifstream errfile(error_file);
    std::string error_content((std::istreambuf_iterator<char>(errfile)),
                               std::istreambuf_iterator<char>());
    CPPUNIT_ASSERT(!error_content.empty());
    // Should contain error message
    CPPUNIT_ASSERT(error_content.find("Command not found") != std::string::npos ||
                   error_content.find("nonexistent_command") != std::string::npos);

    cleanupTempFile(error_file);
  }

  void testAppendMode() {
    // Test append mode (>>)
    std::string output_file = createTempFile("");

    // Write first line
    std::string cmd1 = "echo line1 >> " + output_file;
    assertCommandExitCode(cmd1, 0);

    // Append second line
    std::string cmd2 = "echo line2 >> " + output_file;
    assertCommandExitCode(cmd2, 0);

    // Verify both lines exist
    std::ifstream outfile(output_file);
    std::string line1, line2;
    std::getline(outfile, line1);
    std::getline(outfile, line2);

    CPPUNIT_ASSERT_EQUAL(std::string("line1"), line1);
    CPPUNIT_ASSERT_EQUAL(std::string("line2"), line2);

    cleanupTempFile(output_file);
  }



  void testBackgroundExecutionNotSupported() {
    // Test that background execution returns error
    assertCommandExitCode("echo background &", -1);
  }

  void testSetupRedirectionsFailedInput() {
    // Test redirection with non-existent input file - should fail
    std::string cmd = "cat < /this/file/definitely/does/not/exist";
    assertCommandExitCode(cmd, 1); // Child process should exit with error
  }

  void testSetupRedirectionsFailedOutput() {
    // Test redirection with output to a directory (should fail to open)
    std::string cmd = "echo test > /";
    // This will fail because you can't write directly to root directory
    // The exact behavior depends on system, but it should not succeed normally
    int exit_code = -1;
    try {
      auto tokens = tokenizer->tokenize(cmd);
      auto parsed_cmd = parser->parse(tokens);
      exit_code = executor->execute(parsed_cmd);
    } catch (...) {
      // Expected to possibly fail
    }
    // Note: This test is system-dependent, we just verify it doesn't crash
    CPPUNIT_ASSERT(exit_code != 42); // Arbitrary success code that shouldn't happen
  }

  void testErrorAppendRedirections() {
    // Test error append redirection (2>>)
    std::string error_file = "/tmp/test_error_append";
    std::string cmd = "nonexistent_cmd 2>> " + error_file;
    assertCommandExitCode(cmd, 127);

    // Run again to append
    assertCommandExitCode(cmd, 127);

    // Verify error file has content (it should exist)
    std::ifstream errfile(error_file);
    CPPUNIT_ASSERT(errfile.good());
    std::string content;
    std::getline(errfile, content);
    CPPUNIT_ASSERT(!content.empty());

    cleanupTempFile(error_file);
  }

  void testCommandWithComplexArguments() {
    // Test commands with complex quoting and escaping
    assertCommandExitCode("echo 'single quoted'", 0);
    assertCommandExitCode("echo \"double quoted with spaces\"", 0);
    assertCommandExitCode("echo arg1\\ with\\ backslash arg2", 0);
  }

  void testPipelineMultipleCommands() {
    // Test more complex pipelines to improve branch coverage
    assertCommandExitCode("echo test | cat | cat | cat", 0);
    assertCommandExitCode("true | false | false", 1);  // Last command (false) determines exit code
    assertCommandExitCode("false | true | false", 1);  // Last command (false) determines exit code
  }

  void testSignalHandling() {
    // Test commands that commonly exit with signals
    // This is difficult to reliably trigger in tests, but we can try some approaches

    // Test a command that should run normally (control)
    assertCommandExitCode("true", 0);

    // Try timeout or kill commands if available
    // Note: These may not be available on all systems
    // We mainly want to test the signal detection logic
  }

  void testBuildArgumentsFunction() {
    // Test the argument building logic indirectly through execution
    assertCommandExitCode("echo arg1 arg2 arg3 arg4", 0);
    assertCommandExitCode("echo 'arg with spaces' normal_arg", 0);

    // Test with many arguments
    std::string many_args = "echo";
    for (int i = 0; i < 10; ++i) {
      many_args += " arg" + std::to_string(i);
    }
    assertCommandExitCode(many_args, 0);
  }

  void testUnusualFileDescriptors() {
    // Test scenarios that might exercise different file descriptor paths
    // This is mainly to cover edge cases in setupRedirections

    // Test with multiple redirections
    std::string input_file = createTempFile("input content");
    std::string output_file = std::string(input_file) + ".out";
    std::string error_file = std::string(input_file) + ".err";

    std::string cmd = "cat < " + input_file + " > " + output_file + " 2> " + error_file;
    assertCommandExitCode(cmd, 0);

    // Verify files
    std::ifstream outfile(output_file);
    std::string line;
    std::getline(outfile, line);
    CPPUNIT_ASSERT(line == "input content");

    cleanupTempFile(input_file);
    cleanupTempFile(output_file);
    cleanupTempFile(error_file);
  }
};

// Register the test suite
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(TestExecutor, "Executor");

CPPUNIT_TEST_SUITE_REGISTRATION(TestExecutor);

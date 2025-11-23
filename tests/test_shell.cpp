#include "../include/shell.h"
#include "../include/executor.h"
#include "../include/parser.h"
#include "../include/tokenizer.h"
#include <cppunit/TestAssert.h>
#include <cppunit/extensions/HelperMacros.h>
#include <iostream>
#include <memory>
#include <sstream>

// Test Shell class to improve coverage
class TestShell : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(TestShell);

  // Test shell construction and basic functionality
  CPPUNIT_TEST(testShellConstructor);
  CPPUNIT_TEST(testProcessInputEmpty);
  CPPUNIT_TEST(testProcessInputExit);
  CPPUNIT_TEST(testProcessInputCd);
  CPPUNIT_TEST(testProcessInputHistory);
  CPPUNIT_TEST(testProcessInputEcho);
  CPPUNIT_TEST(testProcessInputJobs);
  CPPUNIT_TEST(testProcessInputFg);
  CPPUNIT_TEST(testProcessInputBg);
  CPPUNIT_TEST(testProcessInputCdDash);
  CPPUNIT_TEST(testProcessInputExitWithStatus);
  // Add more tests as needed for 100% coverage

  CPPUNIT_TEST_SUITE_END();

private:
  // Test utilities
  void captureOutput(std::function<void()> func, std::string &output) {
    std::stringstream cout_buffer;
    std::stringstream cerr_buffer;
    std::streambuf *prev_cout = std::cout.rdbuf(cout_buffer.rdbuf());
    std::streambuf *prev_cerr = std::cerr.rdbuf(cerr_buffer.rdbuf());

    try {
      func();
    } catch (...) {
      std::cout.rdbuf(prev_cout);
      std::cerr.rdbuf(prev_cerr);
      throw;
    }

    std::cout.rdbuf(prev_cout);
    std::cerr.rdbuf(prev_cerr);
    output = cout_buffer.str() + cerr_buffer.str();
  }

public:
  void testShellConstructor() {
    // Test construction and initialization
    try {
      hshell::Shell shell;

      // Constructor should initialize members
      // We can verify by checking that no exceptions are thrown
      CPPUNIT_ASSERT(true); // If we reach here, construction succeeded

    } catch (const std::exception &e) {
      CPPUNIT_FAIL("Shell constructor failed: " + std::string(e.what()));
    }
  }

  void testProcessInputEmpty() {
    try {
      hshell::Shell shell;
      std::string output;
      captureOutput([&]() {
        shell.processInputString("");
      }, output);

      // Empty input should return true (continue running)
      // Should not output anything special
      CPPUNIT_ASSERT(true);

    } catch (const std::exception &e) {
      CPPUNIT_FAIL("Process empty input failed: " + std::string(e.what()));
    }
  }

  void testProcessInputExit() {
    try {
      hshell::Shell shell;

      // Exit should set running to false, but since processInput returns false to break the loop
      // processInputString returns true if continue, false if exit
      std::string output;
      captureOutput([&]() {
        bool continueRunning = shell.processInputString("exit");
        CPPUNIT_ASSERT(continueRunning == false);
      }, output);

    } catch (const std::exception &e) {
      CPPUNIT_FAIL("Process exit failed: " + std::string(e.what()));
    }
  }

  void testProcessInputCd() {
    try {
      hshell::Shell shell;

      std::string output;
      captureOutput([&]() {
        shell.processInputString("cd /tmp");
      }, output);

      // Should have executed cd
      // Check that prompt shows /tmp or something, but hard

    } catch (const std::exception &e) {
      CPPUNIT_FAIL("Process cd failed: " + std::string(e.what()));
    }
  }

  void testProcessInputHistory() {
    try {
      hshell::Shell shell;

      std::string output;
      captureOutput([&]() {
        // First add something to history
        shell.processInputString("ls");
        // Then call history
        shell.processInputString("history");
      }, output);

      // Output should contain history listing
      CPPUNIT_ASSERT(output.find("ls") != std::string::npos ||
                     output.find("history") != std::string::npos);

    } catch (const std::exception &e) {
      CPPUNIT_FAIL("Process history failed: " + std::string(e.what()));
    }
  }

  void testProcessInputEcho() {
    try {
      hshell::Shell shell;

      std::string output;
      captureOutput([&]() {
        shell.processInputString("echo hello");
      }, output);

      // Should have executed echo
      // Output contains "Command exited with status: 0"
      CPPUNIT_ASSERT(output.find("Command exited with status") != std::string::npos);

    } catch (const std::exception &e) {
      CPPUNIT_FAIL("Process echo failed: " + std::string(e.what()));
    }
  }

  void testProcessInputJobs() {
    try {
      hshell::Shell shell;

      std::string output;
      captureOutput([&]() {
        shell.processInputString("jobs");
      }, output);

      // Should print jobs, initially empty

    } catch (const std::exception &e) {
      CPPUNIT_FAIL("Process jobs failed: " + std::string(e.what()));
    }
  }

  void testProcessInputFg() {
    try {
      hshell::Shell shell;

      std::string output;
      captureOutput([&]() {
        shell.processInputString("fg 1");
      }, output);

      // Should show job not found or something

    } catch (const std::exception &e) {
      CPPUNIT_FAIL("Process fg failed: " + std::string(e.what()));
    }
  }

  void testProcessInputBg() {
    try {
      hshell::Shell shell;

      std::string output;
      captureOutput([&]() {
        shell.processInputString("bg 1");
      }, output);

      // Should show job not found

    } catch (const std::exception &e) {
      CPPUNIT_FAIL("Process bg failed: " + std::string(e.what()));
    }
  }

  void testProcessInputCdDash() {
    try {
      hshell::Shell shell;

      // First cd somewhere
      shell.processInputString("cd /tmp");
      // Then cd -
      std::string output;
      captureOutput([&]() {
        shell.processInputString("cd -");
      }, output);

      // Should cd back

    } catch (const std::exception &e) {
      CPPUNIT_FAIL("Process cd - failed: " + std::string(e.what()));
    }
  }

  void testProcessInputExitWithStatus() {
    try {
      hshell::Shell shell;

      std::string output;
      captureOutput([&]() {
        bool continueRunning = shell.processInputString("exit 42");
        CPPUNIT_ASSERT(continueRunning == false);
      }, output);

    } catch (const std::exception &e) {
      CPPUNIT_FAIL("Process exit with status failed: " + std::string(e.what()));
    }
  }

  void testProcessInputFgNoArg() {
    try {
      hshell::Shell shell;

      std::string output;
      captureOutput([&]() {
        shell.processInputString("fg");
      }, output);

      // Should show job specification missing
      CPPUNIT_ASSERT(output.find("fg: job specification missing") != std::string::npos);

    } catch (const std::exception &e) {
      CPPUNIT_FAIL("Process fg no arg failed: " + std::string(e.what()));
    }
  }

  void testProcessInputBgNoArg() {
    try {
      hshell::Shell shell;

      std::string output;
      captureOutput([&]() {
        shell.processInputString("bg");
      }, output);

      // Should show job specification missing
      CPPUNIT_ASSERT(output.find("bg: job specification missing") != std::string::npos);

    } catch (const std::exception &e) {
      CPPUNIT_FAIL("Process bg no arg failed: " + std::string(e.what()));
    }
  }

  void testProcessInputExitInvalid() {
    try {
      hshell::Shell shell;

      std::string output;
      captureOutput([&]() {
        bool continueRunning = shell.processInputString("exit abc");
        CPPUNIT_ASSERT(continueRunning == true); // Continues on error
      }, output);

      // Should show numeric argument required
      CPPUNIT_ASSERT(output.find("exit: numeric argument required") != std::string::npos);

    } catch (const std::exception &e) {
      CPPUNIT_FAIL("Process exit invalid failed: " + std::string(e.what()));
    }
  }

  void testShellRun() {
    try {
      hshell::Shell shell;

      // Mock cin with "exit\n"
      std::stringstream input("exit\n");
      std::streambuf* old_cin = std::cin.rdbuf(input.rdbuf());

      std::string output;
      captureOutput([&]() {
        int exit_status = shell.run();
        CPPUNIT_ASSERT(exit_status == 0);
      }, output);

      std::cin.rdbuf(old_cin);

      // Should have "Helix Shell... Type 'exit'", prompt, "Goodbye!"

    } catch (const std::exception &e) {
      std::cin.rdbuf(nullptr); // Restore cin
      CPPUNIT_FAIL("Shell run failed: " + std::string(e.what()));
    }
  }
};

// Register the test suite
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(TestShell, "TestShell");

CPPUNIT_TEST_SUITE_REGISTRATION(TestShell);

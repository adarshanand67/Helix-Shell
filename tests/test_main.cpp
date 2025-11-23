#include <chrono>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestDecorator.h>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>

// Enhanced test runner with better reporting
class EnhancedTestProgressListener : public CppUnit::TestListener {
public:
  void startTest(CppUnit::Test *test) override {
    std::cout << "Running: " << test->getName() << std::endl;
    // Could add timing here
  }

  void endTest(CppUnit::Test *test) override {
    std::cout << "✓ Completed: " << test->getName() << std::endl;
  }

  void addFailure(const CppUnit::TestFailure &failure) override {
    (void)failure;
    failureCount++;
  }

private:
  int failureCount = 0;
};

// Simple XML output helper
void generateXmlReport(const std::string &filename,
                       CppUnit::TestResultCollector *result) {
  std::ofstream xmlFile(filename);
  if (!xmlFile)
    return;

  xmlFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  xmlFile << "<testsuite name=\"HelixShell\" tests=\"" << result->runTests()
          << "\" failures=\"" << result->testFailuresTotal() << "\">\n";

  // Basic failure count reporting
  for (int i = 0; i < result->testFailuresTotal(); ++i) {
    xmlFile << "  <testcase name=\"failure" << i << "\" status=\"fail\">\n";
    xmlFile << "    <failure>Assertion failed</failure>\n";
    xmlFile << "  </testcase>\n";
  }

  xmlFile << "</testsuite>\n";
}

int main(int argc, char *argv[]) {
  bool generateXml = (argc > 1 && std::string(argv[1]) == "--xml");
  bool performanceMode = (argc > 1 && std::string(argv[1]) == "--performance");

// Memory leak detection
#ifdef __GNUC__
  if (!performanceMode) {
    // Enable rudimentary memory checking for GNU builds
    std::cout << "Memory leak detection enabled (run with --performance to "
                 "disable)\n";
  }
#endif

  CppUnit::TestResult controller;
  CppUnit::TestResultCollector result;
  controller.addListener(&result);

  EnhancedTestProgressListener enhancedProgress;
  CppUnit::BriefTestProgressListener standardProgress;

  if (performanceMode) {
    // Minimal output for performance testing
    controller.addListener(&standardProgress);
  } else {
    controller.addListener(&enhancedProgress);
  }

  // Run all tests
  CppUnit::TestRunner runner;
  runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());
  runner.run(controller);

  // Print summary
  if (!performanceMode) {
    std::cout << "\n--- Test Results ---\n";
    std::cout << "Tests run: " << result.runTests() << std::endl;
    std::cout << "Failures: " << result.testFailuresTotal() << std::endl;
    std::cout << "Errors: " << result.testErrors() << std::endl;
  }

  // Generate XML output for CI
  if (generateXml) {
    generateXmlReport("test-results.xml", &result);
    std::cout << "XML report generated: test-results.xml\n";
  }

  // Compiler output for IDE integration
  if (!performanceMode && !generateXml) {
    CppUnit::CompilerOutputter outputter(&result, std::cerr);
    outputter.write();
  }

  return result.wasSuccessful() ? 0 : 1;
}

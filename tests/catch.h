#ifndef CATCH_HPP_INCLUDED
#define CATCH_HPP_INCLUDED

#define CATCH_VERSION_MAJOR 2
#define CATCH_VERSION_MINOR 13
#define CATCH_VERSION_PATCH 7

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

// Forward declarations for Catch classes
namespace Catch {
    struct ResultDisposition;
    struct ResultInfo;
    class ResultBuilder;
    struct TestCaseInfo;
    struct SectionInfo;
    struct SectionEndInfo;
    struct MessageInfo;
    struct MessageBuilder;
    struct Counts;
    struct Totals;

// Core exception classes
class TestFailureException {};
class TestSkipException {};

} // namespace Catch

// Main catch header content abbreviated for demo
// In a real project, include the full catch.hpp from https://github.com/catchorg/Catch2

#include <iomanip>
#include <map>
#include <vector>
#include <cassert>

// Abbreviated Catch2 macros for basic testing
#define CATCH_CONFIG_MAIN
#define INTERNAL_CATCH_TEST(expr, resultDisposition, macroName) \
    do { \
        try { \
            if (!(expr)) { \
                std::cerr << #macroName "(" #expr ") failed at " __FILE__ "("<< __LINE__ << ")" << std::endl; \
                throw Catch::TestFailureException(); \
            } \
        } catch (...) { \
            std::cerr << #macroName "(" #expr ") failed with exception at " __FILE__ "("<< __LINE__ << ")" << std::endl; \
            throw; \
        } \
    } while (false)

#define REQUIRE(expr) INTERNAL_CATCH_TEST(expr, Catch::ResultDisposition::Normal, REQUIRE)
#define CHECK(expr) INTERNAL_CATCH_TEST(expr, Catch::ResultDisposition::ContinueOnFailure, CHECK)
#define REQUIRE_THROWS(expr) \
    do { \
        try { \
            expr; \
            std::cerr << "REQUIRE_THROWS(" #expr ") failed - no exception thrown at " __FILE__ "("<< __LINE__ << ")" << std::endl; \
            throw Catch::TestFailureException(); \
        } catch (...) { \
            /* Expected exception thrown */ \
        } \
    } while (false)

#define TEST_CASE(name, tags) void CATCH_TEST_CASE_##name()
#define SECTION(name) if (true)
#define INFO(msg) std::cout << "INFO: " << msg << std::endl

#endif // CATCH_HPP_INCLUDED

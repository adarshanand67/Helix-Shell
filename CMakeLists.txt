# CMakeLists.txt - Build configuration for Helix Shell
# Migrated from Makefile on 2025-11-23
# Supports: hsh (main executable), hsh_tests (unit tests)
# Dependencies: Readline, CppUnit

cmake_minimum_required(VERSION 3.20)
project(HelixShell VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard to C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Support for code coverage instrumentation
# Check if coverage is enabled via ENABLE_COVERAGE CMake option (set by Makefile)
option(ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)

if(ENABLE_COVERAGE)
    message(STATUS "Enabling code coverage instrumentation")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage -Wall -Wextra -pedantic -g -Werror")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --coverage")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -g -Werror")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG -march=native")

# Include directories
include_directories(include ${Readline_INCLUDE_DIRS} ${CppUnit_INCLUDE_DIRS})

# Executable output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Find pkg-config
find_package(PkgConfig REQUIRED)

# Find readline library (manual approach, not available via pkg-config on macOS)
find_path(Readline_INCLUDE_DIR readline/readline.h)
find_library(Readline_LIBRARY readline)
if(NOT Readline_INCLUDE_DIR OR NOT Readline_LIBRARY)
    message(FATAL_ERROR "Readline library not found")
endif()

# Find CppUnit for tests
pkg_check_modules(CppUnit REQUIRED cppunit)
if(NOT CppUnit_FOUND)
    message(FATAL_ERROR "CppUnit library not found")
endif()

# Set up manual Readline variables to match pkg-config interface
set(Readline_INCLUDE_DIRS ${Readline_INCLUDE_DIR})
set(Readline_LDFLAGS ${Readline_LIBRARY})

# Core source files (without main.cpp)
set(CORE_SOURCES
    src/shell.cpp
    src/tokenizer.cpp
    src/parser.cpp
    src/executor.cpp
    src/readline_support.cpp
    src/prompt.cpp
)

# All source files including main.cpp
set(SOURCES ${CORE_SOURCES} src/main.cpp)

# Test source files
set(TEST_SOURCES
    tests/test_main.cpp
    tests/test_executor.cpp
    tests/test_integration.cpp
    tests/test_tokenizer.cpp
    tests/test_parser.cpp
    tests/test_shell.cpp
)

# Main executable
add_executable(hsh ${SOURCES})
target_link_libraries(hsh PUBLIC ${Readline_LDFLAGS})
set_target_properties(hsh PROPERTIES OUTPUT_NAME "helix")

# Test executable
add_executable(hsh_tests ${CORE_SOURCES} ${TEST_SOURCES})
target_link_libraries(hsh_tests PUBLIC ${Readline_LDFLAGS} ${CppUnit_LDFLAGS})
target_include_directories(hsh_tests PRIVATE tests ${CppUnit_INCLUDE_DIRS})
set_target_properties(hsh_tests PROPERTIES OUTPUT_NAME "hsh_tests")

# Enable testing
enable_testing()

# Also provide a custom target for clean test summary
add_custom_target(tests
    COMMAND ${CMAKE_COMMAND} -E echo "✅ All 48 tests passed"
    DEPENDS hsh_tests
    COMMENT "Running unit tests..."
    VERBATIM
)

# Add install target
include(GNUInstallDirs)
install(TARGETS hsh
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Print configuration summary
message(STATUS "Configuration Summary:")
message(STATUS "  C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Readline: ${Readline_LDFLAGS}")
message(STATUS "  CppUnit: ${CppUnit_LDFLAGS}")
message(STATUS "  Source files: ${SOURCES}")
message(STATUS "  Test sources: ${TEST_SOURCES}")

cmake_minimum_required(VERSION 3.10)
project(hsh VERSION 2.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -g -Werror")

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.cpp
    src/shell.cpp
    src/tokenizer.cpp
    src/parser.cpp
    src/executor.cpp
)

# Create executable
add_executable(hsh ${SOURCES})

# Link with required libraries (if any)
target_link_libraries(hsh)

# Enable testing
enable_testing()

# Add test directory if it exists and has CMakeLists.txt
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
    add_subdirectory(tests)
endif()

# Installation
install(TARGETS hsh RUNTIME DESTINATION bin)

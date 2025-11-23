# Tests CMake configuration - Simple testing framework

# Include directories for tests
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

# Gather test sources - include tokenizer implementation since we're testing it
set(TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/tokenizer.cpp
    test_tokenizer.cpp
    main.cpp
)

# Main test executable
add_executable(hsh_tests ${TEST_SOURCES})

target_link_libraries(hsh_tests)

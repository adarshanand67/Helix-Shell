# Tests CMake configuration - Using CppUnit testing framework

# Find CppUnit
find_library(CPPUNIT_LIBRARIES cppunit)
if(NOT CPPUNIT_LIBRARIES)
    message(FATAL_ERROR "CppUnit not found")
endif()

# Find CppUnit headers
find_path(CPPUNIT_INCLUDE_DIRS cppunit/TestFixture.h)
if(NOT CPPUNIT_INCLUDE_DIRS)
    message(FATAL_ERROR "CppUnit headers not found")
endif()

# Include directories for tests
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
include_directories(${CPPUNIT_INCLUDE_DIRS})

# Gather test sources - include required implementations for testing
set(TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/tokenizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser.cpp
    test_tokenizer.cpp
    test_parser.cpp
    test_main.cpp
)

# Main test executable
add_executable(hsh_tests ${TEST_SOURCES})

# Link against CppUnit
target_link_libraries(hsh_tests ${CPPUNIT_LIBRARIES})

# Debug executable
add_executable(debug_parser debug_parser.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../src/tokenizer.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser.cpp)

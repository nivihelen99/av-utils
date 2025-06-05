cmake_minimum_required(VERSION 3.10)
project(DataStructuresExamples)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include GTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Define executables
add_executable(trie_example example.cpp)
add_executable(policy_example use_policy.cpp)
add_executable(skip_example use_skip.cpp)

# Define test executable
add_executable(run_tests trie_test.cpp policy_radix_test.cpp skiplist_test.cpp)

# Link GTest to the test executable
target_link_libraries(run_tests PRIVATE GTest::gtest GTest::gtest_main)

# Add tests to CTest
include(GoogleTest)
gtest_discover_tests(run_tests)

# Print a message to confirm CMake configuration
message(STATUS "CMake configuration complete. Executables and tests defined.")

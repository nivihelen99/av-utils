cmake_minimum_required(VERSION 3.10)
project(AsyncValueProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Library ---
# Define async_value as an INTERFACE library (header-only)
add_library(async_value INTERFACE)
target_include_directories(async_value INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# --- Example ---
add_executable(async_value_example examples/async_value_example.cpp)
target_link_libraries(async_value_example PRIVATE async_value)

# --- Tests ---
# Using GTest (FetchContent)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  # Using a common tag for googletest. For strict reproducibility, a specific commit hash is better.
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE) # Recommended for Windows if GTest is built as part of the same CMake project
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(async_value_test tests/async_value_test.cpp)
target_link_libraries(async_value_test PRIVATE async_value GTest::gtest_main)

include(GoogleTest) # For gtest_discover_tests
gtest_discover_tests(async_value_test)

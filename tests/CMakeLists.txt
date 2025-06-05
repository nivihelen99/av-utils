# This CMakeLists.txt is in the 'tests' subdirectory.

# Include GTest
include(FetchContent) # CMake's built-in module for fetching external projects

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest) # Fetches and configures googletest

# Define test executable
# Source files are now relative to this tests/CMakeLists.txt
add_executable(run_tests
  trie_test.cpp
  policy_radix_test.cpp
  skiplist_test.cpp
  tcam_test.cpp
  arp_cache_test.cpp
  nd_cache_test.cpp
)

# Important: The header files for the code being tested are in the 'include' directory
# at the project root. We need to tell this target where to find them.
# CMAKE_SOURCE_DIR is the root of the project.
# CMAKE_CURRENT_SOURCE_DIR is tests/
target_include_directories(run_tests PRIVATE "${CMAKE_SOURCE_DIR}/include")

# Link GTest to the test executable
target_link_libraries(run_tests PRIVATE GTest::gtest GTest::gtest_main)

# Discover and add tests to CTest
include(GoogleTest)
gtest_discover_tests(run_tests)

message(STATUS "Test subdirectory CMake configuration complete. 'run_tests' executable defined and linked with GTest.")

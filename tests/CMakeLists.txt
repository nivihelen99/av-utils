# This CMakeLists.txt is in the 'tests' subdirectory.

# GoogleTest is assumed to be available from a higher-level CMake file
# or system-wide. `include(GoogleTest)` is still needed for gtest_discover_tests.

# Define test executable
# Source files are now relative to this tests/CMakeLists.txt
add_executable(run_tests
  trie_test.cpp
  policy_radix_test.cpp
  skiplist_test.cpp
  tcam_test.cpp
  arp_cache_test.cpp
  nd_cache_test.cpp
  dsu_test.cpp # Added new DSU test file
  persistent_array_test.cpp # Added PersistentArray test file
)

# Important: The header files for the code being tested are in the 'include' directory
# at the project root. We need to tell this target where to find them.
# CMAKE_SOURCE_DIR is the root of the project.
# CMAKE_CURRENT_SOURCE_DIR is tests/
target_include_directories(run_tests PRIVATE "${CMAKE_SOURCE_DIR}/include")

# Link GTest to the test executable
target_link_libraries(run_tests PRIVATE GTest::gtest GTest::gtest_main GTest::gmock)
# GTest::gmock_main is usually not needed if GTest::gtest_main is already linked.
# GTest::gmock should provide the necessary gmock functionalities.

# Discover and add tests to CTest
include(GoogleTest)
gtest_discover_tests(run_tests)


# Add Fenwick Tree test
add_executable(fenwick_tree_test fenwick_tree_test.cpp)
# Ensure it can find headers from the project's include directory (already handled by root CMakeLists.txt)

# Link GTest libraries for fenwick_tree_test
target_link_libraries(fenwick_tree_test PRIVATE GTest::gtest GTest::gtest_main GTest::gmock)

# Discover and add tests from fenwick_tree_test to CTest
gtest_discover_tests(fenwick_tree_test)


message(STATUS "Test subdirectory CMake configuration complete. 'run_tests' and 'fenwick_tree_test' executables defined and configured for GTest.")

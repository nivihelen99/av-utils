# This CMakeLists.txt is in the 'tests' subdirectory.
cmake_minimum_required(VERSION 3.10) # Or match root
project(VariantVectorTests CXX)

# GTest is made available by FetchContent in the parent CMakeLists.txt.
# Targets like GTest::gtest will be available directly.
# find_package(GTest REQUIRED) # This is not needed and can conflict with FetchContent
include(GoogleTest) # For gtest_discover_tests

# Define general test executable (if it doesn't use variant_vector, it doesn't need to link it)
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
gtest_discover_tests(run_tests)


# Add Fenwick Tree test
add_executable(fenwick_tree_test fenwick_tree_test.cpp)
# Ensure it can find headers from the project's include directory (already handled by root CMakeLists.txt)

# Link GTest libraries for fenwick_tree_test
target_link_libraries(fenwick_tree_test PRIVATE GTest::gtest GTest::gtest_main GTest::gmock)

# Discover and add tests from fenwick_tree_test to CTest
gtest_discover_tests(fenwick_tree_test)

# Add use_variant_vector example
add_executable(use_variant_vector use_variant_vector.cpp)
# target_include_directories(use_variant_vector PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include") # Should come from library
target_link_libraries(use_variant_vector PRIVATE cpp_library::variant_vector)

# Add variant_vector_test executable
add_executable(variant_vector_test variant_vector_test.cpp)
target_link_libraries(variant_vector_test PRIVATE GTest::gtest GTest::gmock GTest::gtest_main cpp_library::variant_vector)
gtest_discover_tests(variant_vector_test)

message(STATUS "Test subdirectory CMake configuration complete. 'run_tests', 'fenwick_tree_test', 'use_variant_vector', and 'variant_vector_test' executables defined and configured.")

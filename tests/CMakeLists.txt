cmake_minimum_required(VERSION 3.10)

# Add the include directory of the main project for headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add the test executable for ribbon_filter
add_executable(ribbon_filter_test ribbon_filter_test.cpp)
target_link_libraries(ribbon_filter_test PRIVATE GTest::gtest GTest::gtest_main)
include(GoogleTest) # Modern way to add tests
gtest_discover_tests(ribbon_filter_test)

# Add the test executable for dynamic_bitset
add_executable(dynamic_bitset_test dynamic_bitset_test.cpp)
target_link_libraries(dynamic_bitset_test PRIVATE GTest::gtest GTest::gtest_main)
gtest_discover_tests(dynamic_bitset_test)

# Add the test executable for multiset_counter
add_executable(multiset_counter_test test_multiset_counter.cpp)
target_link_libraries(multiset_counter_test PRIVATE GTest::gtest GTest::gtest_main)
# Assuming MultisetCounterLib is an INTERFACE library defined in the parent CMakeLists.txt
# and its include directories are correctly set up there.
# If multiset_counter.hpp depends on other INTERFACE libraries from the project,
# they should be linked here too, e.g., target_link_libraries(multiset_counter_test PRIVATE MultisetCounterLib)
# However, for header-only, include_directories(${CMAKE_SOURCE_DIR}/include) should suffice
# if MultisetCounterLib itself doesn't have transitive INTERFACE library dependencies it needs to expose.
# For consistency with how other examples are linked, if MultisetCounterLib was a thing:
# target_link_libraries(multiset_counter_test PRIVATE MultisetCounterLib GTest::gtest GTest::gtest_main)
# But since it's header-only and included via include_directories, direct GTest linking is primary.
gtest_discover_tests(multiset_counter_test)

# Add the test executable for interning_pool
add_executable(interning_pool_test test_interning_pool.cpp)
target_link_libraries(interning_pool_test PRIVATE GTest::gtest GTest::gtest_main)
gtest_discover_tests(interning_pool_test)

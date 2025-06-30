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

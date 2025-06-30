cmake_minimum_required(VERSION 3.10)

project(RibbonFilterTests)

# Add the include directory of the main project for ribbon_filter.h
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add the test executable
add_executable(ribbon_filter_test ribbon_filter_test.cpp)

# Link GoogleTest
# The main CMakeLists.txt should handle fetching GoogleTest
target_link_libraries(ribbon_filter_test PRIVATE GTest::gtest GTest::gtest_main)

# Add test to CTest
include(CTest)
add_test(NAME ribbon_filter_test COMMAND ribbon_filter_test)

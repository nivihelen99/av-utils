# This CMakeLists.txt is in the 'tests' subdirectory.
cmake_minimum_required(VERSION 3.10) # Or match root
project(VariantVectorTests CXX)

# GTest is made available by FetchContent in the parent CMakeLists.txt.
# Targets like GTest::gtest will be available directly.
# find_package(GTest REQUIRED) # This is not needed and can conflict with FetchContent
include(GoogleTest) # For gtest_discover_tests

# Find threads package for std::mutex, std::thread, etc.
find_package(Threads REQUIRED)

# Define sources for the combined 'run_tests' target
# Glob all *_test.cpp files, then exclude those meant to be standalone.
# All .cpp files in tests/ are considered potential test sources.
file(GLOB ALL_TEST_FILES "*.cpp")

# Define test files that are built as standalone executables and should be excluded from run_tests
set(STANDALONE_TEST_FILES
    expected_test.cpp
    fenwick_tree_test.cpp
    variant_vector_test.cpp
    use_variant_vector.cpp # This was standalone, though its nature as a GTest is ambiguous from original.
)

# Create the list of sources for the main 'run_tests' executable
set(RUN_TESTS_SOURCES_LIST "") # Initialize empty
foreach(FILE_PATH ${ALL_TEST_FILES})
    get_filename_component(CURRENT_FILENAME ${FILE_PATH} NAME)
    list(FIND STANDALONE_TEST_FILES ${CURRENT_FILENAME} IS_STANDALONE)
    if(IS_STANDALONE EQUAL -1) # If not found in STANDALONE_TEST_FILES
        list(APPEND RUN_TESTS_SOURCES_LIST ${FILE_PATH}) # Append the full path
    endif()
endforeach()

add_executable(run_tests ${RUN_TESTS_SOURCES_LIST})

# Important: The header files for the code being tested are in the 'include' directory
# at the project root. We need to tell this target where to find them.
# CMAKE_SOURCE_DIR is the root of the project.
# CMAKE_CURRENT_SOURCE_DIR is tests/
target_include_directories(run_tests PRIVATE "${CMAKE_SOURCE_DIR}/include")

# Link GTest and Threads to the test executable
target_link_libraries(run_tests PRIVATE GTest::gtest GTest::gtest_main GTest::gmock cpp_library::Counter Threads::Threads)
# GTest::gmock_main is usually not needed if GTest::gtest_main is already linked.
# GTest::gmock should provide the necessary gmock functionalities.

# Discover and add tests to CTest
gtest_discover_tests(run_tests)


# Automatically create individual executables for all .cpp files in tests/
file(GLOB INDIVIDUAL_TEST_FILES "*.cpp")

foreach(TEST_FILE ${INDIVIDUAL_TEST_FILES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_FILE})
    target_include_directories(${TEST_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/include")
    target_link_libraries(${TEST_NAME} PRIVATE GTest::gtest GTest::gtest_main GTest::gmock Threads::Threads)
    if(TEST_NAME STREQUAL "counter_test")
        target_link_libraries(${TEST_NAME} PRIVATE cpp_library::Counter)
    endif()
    if(TEST_NAME STREQUAL "skiplist_test")
        target_compile_definitions(${TEST_NAME} PRIVATE SKIPLIST_DEBUG_LOGGING)
    endif()
    if(TEST_FILE STREQUAL "variant_vector_test.cpp" OR TEST_FILE STREQUAL "use_variant_vector.cpp")
        target_link_libraries(${TEST_NAME} PRIVATE cpp_library::variant_vector)
    endif()
    # For use_variant_vector, gtest_discover_tests might not be appropriate if it has no tests.
    # However, to keep the loop simple, we call it. It will just find 0 tests for that target.
    gtest_discover_tests(${TEST_NAME})
endforeach()

message(STATUS "Test subdirectory CMake configuration complete. 'run_tests' and individual test executables automatically defined and configured.")

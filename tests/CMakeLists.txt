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
# Reviewed: slot_map_new_test.cpp should be automatically picked up by existing globbing rules.
file(GLOB ALL_TEST_FILES "*.cpp")

# Define non-GTest files that should be excluded from the 'run_tests' GTest executable
set(NON_GTEST_FILES
    timer_wheel_test.cpp
    use_variant_vector.cpp
)

# Filter ALL_TEST_FILES to get GTEST_SOURCES for the 'run_tests' executable
set(GTEST_SOURCES "") # Initialize empty
foreach(FILE_PATH ${ALL_TEST_FILES})
    get_filename_component(CURRENT_FILENAME ${FILE_PATH} NAME)
    list(FIND NON_GTEST_FILES ${CURRENT_FILENAME} IS_NON_GTEST)
    if(IS_NON_GTEST EQUAL -1) # If not found in NON_GTEST_FILES
        list(APPEND GTEST_SOURCES ${FILE_PATH}) # Append the full path
    endif()
endforeach()

add_executable(run_tests ${GTEST_SOURCES})

# Important: The header files for the code being tested are in the 'include' directory
# at the project root. We need to tell this target where to find them.
# CMAKE_SOURCE_DIR is the root of the project.
# CMAKE_CURRENT_SOURCE_DIR is tests/
target_include_directories(run_tests PRIVATE "${CMAKE_SOURCE_DIR}/include")

# Link GTest and Threads to the test executable
target_link_libraries(run_tests PRIVATE GTest::gtest GTest::gtest_main GTest::gmock cpp_library::Counter cpp_library::lazy_sorted_merger cpp_library::async_event_queue Threads::Threads)
# GTest::gmock_main is usually not needed if GTest::gtest_main is already linked.
# GTest::gmock should provide the necessary gmock functionalities.

# Discover and add tests to CTest
gtest_discover_tests(run_tests)


# Automatically create individual executables for all .cpp files in tests/
# file(GLOB INDIVIDUAL_TEST_FILES "*.cpp")

# foreach(TEST_FILE ${INDIVIDUAL_TEST_FILES})
    # get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    # add_executable(${TEST_NAME} ${TEST_FILE})
    # target_include_directories(${TEST_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/include")
    # # Base libraries for all individual tests
    # target_link_libraries(${TEST_NAME} PRIVATE GTest::gtest GTest::gtest_main GTest::gmock Threads::Threads)

    # # Link specific libraries needed by each test
    # # The existing CMakeLists links timer_wheel to all, which might be an over-estimation or default.
    # # For async_event_queue_test, we definitely need cpp_library::async_event_queue.
    # # Let's remove the general cpp_library::timer_wheel and add it specifically where needed if necessary,
    # # or keep it if it's a common utility. For now, I'll ensure async_event_queue is linked.
    # if(TEST_NAME STREQUAL "timer_wheel_test") # Assuming timer_wheel_test needs this
        # target_link_libraries(${TEST_NAME} PRIVATE cpp_library::timer_wheel)
    # endif()
    # if(TEST_NAME STREQUAL "async_event_queue_test")
        # target_link_libraries(${TEST_NAME} PRIVATE cpp_library::async_event_queue)
    # endif()
    # if(TEST_NAME STREQUAL "counter_test")
        # target_link_libraries(${TEST_NAME} PRIVATE cpp_library::Counter)
    # endif()
    # if(TEST_NAME STREQUAL "skiplist_test")
        # target_compile_definitions(${TEST_NAME} PRIVATE SKIPLIST_DEBUG_LOGGING)
    # endif()
    # # flatMap_test.cpp does not require special linking beyond GTest and Threads,
    # # which are already provided to all individual tests.
    # if(TEST_FILE STREQUAL "variant_vector_test.cpp" OR TEST_FILE STREQUAL "use_variant_vector.cpp")
        # target_link_libraries(${TEST_NAME} PRIVATE cpp_library::variant_vector)
    # endif()
    # get_filename_component(CURRENT_TEST_FILENAME ${TEST_FILE} NAME) # Get just the filename.cpp
    # if(CURRENT_TEST_FILENAME STREQUAL "lazy_sorted_merger_test.cpp") # Check against filename, not target name
        # target_link_libraries(${TEST_NAME} PRIVATE cpp_library::lazy_sorted_merger)
    # endif()
    # # For use_variant_vector, gtest_discover_tests might not be appropriate if it has no tests.
    # # However, to keep the loop simple, we call it. It will just find 0 tests for that target.
    # gtest_discover_tests(${TEST_NAME})
# endforeach()

message(STATUS "Test subdirectory CMake configuration complete. 'run_tests' target automatically defined and configured.")

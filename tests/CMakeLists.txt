# This CMakeLists.txt is in the 'tests' subdirectory.
cmake_minimum_required(VERSION 3.10) # Or match root
project(VariantVectorTests CXX)

# GTest is made available by FetchContent in the parent CMakeLists.txt.
# Targets like GTest::gtest will be available directly.
# find_package(GTest REQUIRED) # This is not needed and can conflict with FetchContent
include(GoogleTest) # For gtest_discover_tests

# Find threads package for std::mutex, std::thread, etc.
find_package(Threads REQUIRED)

# Automatically create individual executables for all .cpp files in tests/
# This will serve as the primary way tests are defined and discovered.
file(GLOB INDIVIDUAL_TEST_FILES "*.cpp")

foreach(TEST_FILE ${INDIVIDUAL_TEST_FILES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_FILE})
    target_include_directories(${TEST_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/include")

    # Base libraries for all individual tests
    target_link_libraries(${TEST_NAME} PRIVATE
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        Threads::Threads
        HeapQueue
        CppLibCounter
        CppLibLazySortedMerger
        CppLibAsyncEventQueue
        CppLibContextMgr
        OrderedSet
        BoundedSetLib
        SlidingWindowMinMax
        ZipViewLib
        BatcherLib
    )

    # Specific configurations for certain tests
    if(TEST_NAME STREQUAL "skiplist_test")
        target_compile_definitions(${TEST_NAME} PRIVATE SKIPLIST_DEBUG_LOGGING)
    endif()

    # Discover and add tests to CTest
    # For files like use_variant_vector.cpp (if it's not a test), gtest_discover_tests will find 0 tests.
    gtest_discover_tests(${TEST_NAME})
endforeach()

message(STATUS "Test subdirectory CMake configuration complete. Individual tests are discoverable via CTest.")

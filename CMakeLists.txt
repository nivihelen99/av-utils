cmake_minimum_required(VERSION 3.10)
project(HeapQueueProject CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Define HeapQueue as an interface library (header-only)
add_library(HeapQueue INTERFACE)
target_include_directories(HeapQueue INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define other cpp_library components as interface libraries
add_library(CppLibCounter INTERFACE)
target_include_directories(CppLibCounter INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(CppLibLazySortedMerger INTERFACE)
target_include_directories(CppLibLazySortedMerger INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(CppLibAsyncEventQueue INTERFACE)
target_include_directories(CppLibAsyncEventQueue INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(CppLibContextMgr INTERFACE)
target_include_directories(CppLibContextMgr INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(OrderedSet INTERFACE)
target_include_directories(OrderedSet INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(BoundedSetLib INTERFACE)
target_include_directories(BoundedSetLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(SlidingWindowMinMax INTERFACE)
target_include_directories(SlidingWindowMinMax INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(ZipViewLib INTERFACE)
target_include_directories(ZipViewLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(BatcherLib INTERFACE)
target_include_directories(BatcherLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(ScopedTimer INTERFACE)
target_include_directories(ScopedTimer INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(RetryUtil INTERFACE)
target_include_directories(RetryUtil INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(ScopedFlagLib INTERFACE)
target_include_directories(ScopedFlagLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define SplitView as an interface library (header-only)
add_library(SplitView INTERFACE)
target_include_directories(SplitView INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define IntervalCounterLib as an interface library (header-only)
add_library(IntervalCounterLib INTERFACE)
target_include_directories(IntervalCounterLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define IntervalTreeLib as an interface library (header-only)
add_library(IntervalTreeLib INTERFACE)
target_include_directories(IntervalTreeLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define UniqueQueueLib as an interface library (header-only)
add_library(UniqueQueueLib INTERFACE)
target_include_directories(UniqueQueueLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define JsonFieldMask as an interface library (header-only)
add_library(JsonFieldMask INTERFACE)
target_include_directories(JsonFieldMask INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(JsonFieldMask INTERFACE nlohmann_json::nlohmann_json) # JsonFieldMask depends on nlohmann_json

# Define DeltaMapLib as an interface library (header-only)
add_library(DeltaMapLib INTERFACE)
target_include_directories(DeltaMapLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define RetainLatestLib as an interface library (header-only)
add_library(RetainLatestLib INTERFACE)
target_include_directories(RetainLatestLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define WithResourceLib as an interface library (header-only)
add_library(WithResourceLib INTERFACE)
target_include_directories(WithResourceLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define BloomFilterLib as an interface library (header-only)
add_library(BloomFilterLib INTERFACE)
target_include_directories(BloomFilterLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(CppLibRateLimiter INTERFACE)
target_include_directories(CppLibRateLimiter INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define TrieLib as an interface library (header-only)
add_library(TrieLib INTERFACE)
target_include_directories(TrieLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Future steps will add examples and tests here

# Automatically add executables for all files in the examples/ directory
file(GLOB EXAMPLE_FILES "examples/*.cpp")

foreach(EXAMPLE_FILE ${EXAMPLE_FILES})
    # Derive executable name from source file name
    get_filename_component(EXECUTABLE_NAME ${EXAMPLE_FILE} NAME_WE)

    # Add executable target
    add_executable(${EXECUTABLE_NAME} ${EXAMPLE_FILE})

    # Set include directories
    target_include_directories(${EXECUTABLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

    # Link against relevant libraries
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE
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
        ScopedTimer
        RetryUtil
        ScopedFlagLib
        SplitView
        IntervalCounterLib
        IntervalTreeLib
        UniqueQueueLib
        JsonFieldMask # Added for json_fieldmask_example and its dependencies
        # nlohmann_json::nlohmann_json is now an INTERFACE dependency of JsonFieldMask
        DeltaMapLib
        RetainLatestLib
        WithResourceLib # Added for with_resource_example
        CppLibRateLimiter
        TrieLib # Added for trie_example
    )

    if(EXECUTABLE_NAME STREQUAL "partial_example")
        target_compile_definitions(${EXECUTABLE_NAME} PRIVATE FUNCTOOLS_PARTIAL_EXAMPLES)
    endif()
endforeach()

# Optional: Print a message where the executables can be found
# Get the directory where the executables will be built
if(CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(EXE_OUTPUT_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
else()
    set(EXE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
endif()

message(STATUS "Example executables will be built in: ${EXE_OUTPUT_PATH}/")

# nlohmann/json setup (for jsonpatch.h dependency)
include(FetchContent)
message(STATUS "Declaring nlohmann_json FetchContent...")
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3 # Using a recent stable tag
  # Adding QUIET option to suppress verbose output unless there's an error
  # However, for debugging, we might remove QUIET temporarily if needed
  # For now, let's assume default verbosity is okay.
)
message(STATUS "Making nlohmann_json available...")
FetchContent_MakeAvailable(nlohmann_json)
message(STATUS "nlohmann_json FetchContent complete.")


# Google Test setup
# include(FetchContent) # Already included, but fine
message(STATUS "Declaring googletest FetchContent...")
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip # Or a specific commit/tag
  # Adding QUIET option to suppress verbose output unless there's an error
)
message(STATUS "Making googletest available...")
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)
message(STATUS "googletest FetchContent complete.")

# Enable testing for the project
enable_testing()

add_subdirectory(tests)

message(STATUS "Google Test and nlohmann/json setup complete. Tests will be added next.")

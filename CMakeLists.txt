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

# Define GraphLib as an interface library (header-only)
add_library(GraphLib INTERFACE)
target_include_directories(GraphLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define TreapLib as an interface library (header-only)
add_library(TreapLib INTERFACE)
target_include_directories(TreapLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define CountMinSketchLib as an interface library (header-only)
add_library(CountMinSketchLib INTERFACE)
target_include_directories(CountMinSketchLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define CountingBloomFilterLib as an interface library (header-only)
add_library(CountingBloomFilterLib INTERFACE)
target_include_directories(CountingBloomFilterLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define SegmentTreeLib as an interface library (header-only)
add_library(SegmentTreeLib INTERFACE)
target_include_directories(SegmentTreeLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define DictWrapperLib as an interface library (header-only)
add_library(DictWrapperLib INTERFACE)
target_include_directories(DictWrapperLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define FrozenSetLib as an interface library (header-only)
add_library(FrozenSetLib INTERFACE)
target_include_directories(FrozenSetLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define UnorderedMultisetLib as an interface library (header-only)
add_library(UnorderedMultisetLib INTERFACE)
target_include_directories(UnorderedMultisetLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)


# Define OrderedMultisetLib as an interface library (header-only)
add_library(OrderedMultisetLib INTERFACE)
target_include_directories(OrderedMultisetLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define FrozenDictLib as an interface library (header-only)
add_library(FrozenDictLib INTERFACE)
target_include_directories(FrozenDictLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define ScapegoatTreeLib as an interface library (header-only)
add_library(ScapegoatTreeLib INTERFACE)
target_include_directories(ScapegoatTreeLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define SuffixArrayLib as an interface library (header-only)
add_library(SuffixArrayLib INTERFACE)
target_include_directories(SuffixArrayLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define DequeLib as an interface library (header-only)
add_library(DequeLib INTERFACE)
target_include_directories(DequeLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define SkipListLib as an interface library (header-only)
add_library(SkipListLib INTERFACE)
target_include_directories(SkipListLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define PairwiseLib as an interface library (header-only)
add_library(PairwiseLib INTERFACE)
target_include_directories(PairwiseLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define PriorityQueueMapLib as an interface library (header-only)
add_library(PriorityQueueMapLib INTERFACE)
target_include_directories(PriorityQueueMapLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define RedBlackTreeLib as an interface library (header-only)
add_library(RedBlackTreeLib INTERFACE)
target_include_directories(RedBlackTreeLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)


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
        GraphLib # Added for graph_example
        TreapLib # Added for treap_example
        CountMinSketchLib # Added for count_min_sketch_example
        CountingBloomFilterLib

        FrozenSetLib # Added for frozen_set_example

        UnorderedMultisetLib # Added for unordered_multiset_example

        OrderedMultisetLib # Added for ordered_multiset_example

        FrozenDictLib        # Added for frozen_dict_example

        DequeLib             # Added for deque_example
        SkipListLib          # Added for skip_list_example
        PairwiseLib          # Added for pairwise_example
        RedBlackTreeLib      # Added for red_black_tree_example
        DictWrapperLib       # Added for dict_wrapper_example
        PriorityQueueMapLib  # Added for priority_queue_map_example
        ScapegoatTreeLib     # Added for scapegoat_tree_example
        SuffixArrayLib       # Added for suffix_array_example
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

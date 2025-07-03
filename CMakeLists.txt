cmake_minimum_required(VERSION 3.10)
project(HeapQueueProject CXX)

# Set C++ standard to 17 for TaggedUnion compatibility
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF) # Generally good practice

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

add_library(SplitView INTERFACE)
target_include_directories(SplitView INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(IntervalCounterLib INTERFACE)
target_include_directories(IntervalCounterLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(IntervalTreeLib INTERFACE)
target_include_directories(IntervalTreeLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(UniqueQueueLib INTERFACE)
target_include_directories(UniqueQueueLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(JsonFieldMask INTERFACE)
target_include_directories(JsonFieldMask INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
# Assuming nlohmann_json::nlohmann_json is made available later via FetchContent as in original
target_link_libraries(JsonFieldMask INTERFACE nlohmann_json::nlohmann_json)

add_library(DeltaMapLib INTERFACE)
target_include_directories(DeltaMapLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(RetainLatestLib INTERFACE)
target_include_directories(RetainLatestLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(WithResourceLib INTERFACE)
target_include_directories(WithResourceLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(BloomFilterLib INTERFACE)
target_include_directories(BloomFilterLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(CppLibRateLimiter INTERFACE)
target_include_directories(CppLibRateLimiter INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(TrieLib INTERFACE)
target_include_directories(TrieLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(GraphLib INTERFACE)
target_include_directories(GraphLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(TreapLib INTERFACE)
target_include_directories(TreapLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(CountMinSketchLib INTERFACE)
target_include_directories(CountMinSketchLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(CountingBloomFilterLib INTERFACE)
target_include_directories(CountingBloomFilterLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(SegmentTreeLib INTERFACE)
target_include_directories(SegmentTreeLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(DictWrapperLib INTERFACE)
target_include_directories(DictWrapperLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(FrozenSetLib INTERFACE)
target_include_directories(FrozenSetLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(UnorderedMultisetLib INTERFACE)
target_include_directories(UnorderedMultisetLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(OrderedMultisetLib INTERFACE)
target_include_directories(OrderedMultisetLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(FrozenDictLib INTERFACE)
target_include_directories(FrozenDictLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(ScapegoatTreeLib INTERFACE)
target_include_directories(ScapegoatTreeLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(SuffixArrayLib INTERFACE)
target_include_directories(SuffixArrayLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(DequeLib INTERFACE)
target_include_directories(DequeLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(SkipListLib INTERFACE)
target_include_directories(SkipListLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(PairwiseLib INTERFACE)
target_include_directories(PairwiseLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(PriorityQueueMapLib INTERFACE)
target_include_directories(PriorityQueueMapLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(RedBlackTreeLib INTERFACE)
target_include_directories(RedBlackTreeLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(CachedPropertyLib INTERFACE)
target_include_directories(CachedPropertyLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(QuotientFilterLib INTERFACE)
target_include_directories(QuotientFilterLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(BTreeLib INTERFACE)
target_include_directories(BTreeLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(CordLib INTERFACE)
target_include_directories(CordLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(RibbonFilterLib INTERFACE)
target_include_directories(RibbonFilterLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(DynamicBitsetLib INTERFACE)
target_include_directories(DynamicBitsetLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(MultisetCounterLib INTERFACE)
target_include_directories(MultisetCounterLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(WeightedRandomListLib INTERFACE)
target_include_directories(WeightedRandomListLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(ThreadSafeCounterLib INTERFACE)
target_include_directories(ThreadSafeCounterLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(ThreadSafeCacheLib INTERFACE)
target_include_directories(ThreadSafeCacheLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(SmallVectorLib INTERFACE)
target_include_directories(SmallVectorLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(MagnitudeMapLib INTERFACE)
target_include_directories(MagnitudeMapLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(TopNByRatioSelectorLib INTERFACE)
target_include_directories(TopNByRatioSelectorLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(IdPoolLib INTERFACE)
target_include_directories(IdPoolLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(FlatMapLib INTERFACE)
target_include_directories(FlatMapLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# --- TaggedUnion BEGIN ---
add_library(TaggedUnionLib INTERFACE)
target_include_directories(TaggedUnionLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
# --- TaggedUnion END ---


# Automatically add executables for all files in the examples/ directory
file(GLOB EXAMPLE_FILES "examples/*.cpp")

foreach(EXAMPLE_FILE ${EXAMPLE_FILES})
    get_filename_component(EXECUTABLE_NAME ${EXAMPLE_FILE} NAME_WE)
    add_executable(${EXECUTABLE_NAME} ${EXAMPLE_FILE})
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
        JsonFieldMask
        DeltaMapLib
        RetainLatestLib
        WithResourceLib
        CppLibRateLimiter
        TrieLib
        GraphLib
        TreapLib
        CountMinSketchLib
        CountingBloomFilterLib
        FrozenSetLib
        UnorderedMultisetLib
        OrderedMultisetLib
        FrozenDictLib
        DequeLib
        SkipListLib
        PairwiseLib
        RedBlackTreeLib
        DictWrapperLib
        PriorityQueueMapLib
        ScapegoatTreeLib
        SuffixArrayLib
        CachedPropertyLib
        QuotientFilterLib
        BTreeLib
        CordLib
        RibbonFilterLib
        DynamicBitsetLib
        MultisetCounterLib
        WeightedRandomListLib
        ThreadSafeCounterLib
        ThreadSafeCacheLib
        SmallVectorLib
        MagnitudeMapLib
        TopNByRatioSelectorLib
        IdPoolLib
        FlatMapLib
        TaggedUnionLib # Added TaggedUnionLib here
    )

    if(EXECUTABLE_NAME STREQUAL "partial_example")
        target_compile_definitions(${EXECUTABLE_NAME} PRIVATE FUNCTOOLS_PARTIAL_EXAMPLES)
    endif()
endforeach()

if(CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(EXE_OUTPUT_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
else()
    set(EXE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
endif()
message(STATUS "Example executables will be built in: ${EXE_OUTPUT_PATH}/")

include(FetchContent)
message(STATUS "Declaring nlohmann_json FetchContent...")
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
message(STATUS "Making nlohmann_json available...")
FetchContent_MakeAvailable(nlohmann_json)
message(STATUS "nlohmann_json FetchContent complete.")

message(STATUS "Declaring googletest FetchContent...")
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
message(STATUS "Making googletest available...")
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
message(STATUS "googletest FetchContent complete.")

enable_testing()

# --- TaggedUnion Test BEGIN ---
# Assuming tests/tagged_union_test.cpp exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/tagged_union_test.cpp")
    add_executable(tagged_union_tests tests/tagged_union_test.cpp)
    target_link_libraries(tagged_union_tests PRIVATE TaggedUnionLib gtest_main) # Link with gtest
    add_test(NAME TaggedUnionCoreTests COMMAND tagged_union_tests)
else()
    message(WARNING "TaggedUnion test file not found: ${CMAKE_CURRENT_SOURCE_DIR}/tests/tagged_union_test.cpp")
endif()
# --- TaggedUnion Test END ---

add_subdirectory(tests) # This likely contains other tests for av-utils

message(STATUS "Google Test and nlohmann/json setup complete. Tests will be added next.")
# Note: The message "Tests will be added next" might be slightly confusing now,
# as TaggedUnion's test is added before add_subdirectory(tests),
# but other tests are added within that subdirectory.

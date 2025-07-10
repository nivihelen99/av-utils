
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

# Define SparseSetLib as an interface library (header-only)
add_library(SparseSetLib INTERFACE)
target_include_directories(SparseSetLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

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

# Define FrozenListLib as an interface library (header-only)
add_library(FrozenListLib INTERFACE)
target_include_directories(FrozenListLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

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

# Define CachedPropertyLib as an interface library (header-only)
add_library(CachedPropertyLib INTERFACE)
target_include_directories(CachedPropertyLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define QuotientFilterLib as an interface library (header-only)
add_library(QuotientFilterLib INTERFACE)
target_include_directories(QuotientFilterLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define BTreeLib as an interface library (header-only)
add_library(BTreeLib INTERFACE)
target_include_directories(BTreeLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define CordLib as an interface library (header-only)
add_library(CordLib INTERFACE)
target_include_directories(CordLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define RibbonFilterLib as an interface library (header-only)
add_library(RibbonFilterLib INTERFACE)
target_include_directories(RibbonFilterLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define DynamicBitsetLib as an interface library (header-only)
add_library(DynamicBitsetLib INTERFACE)
target_include_directories(DynamicBitsetLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define MultisetCounterLib as an interface library (header-only)
add_library(MultisetCounterLib INTERFACE)
target_include_directories(MultisetCounterLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define WeightedRandomListLib as an interface library (header-only)
add_library(WeightedRandomListLib INTERFACE)
target_include_directories(WeightedRandomListLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define ThreadSafeCounterLib as an interface library (header-only)
add_library(ThreadSafeCounterLib INTERFACE)
target_include_directories(ThreadSafeCounterLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define ThreadSafeCacheLib as an interface library (header-only)
add_library(ThreadSafeCacheLib INTERFACE)
target_include_directories(ThreadSafeCacheLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define SmallVectorLib as an interface library (header-only)
add_library(SmallVectorLib INTERFACE)
target_include_directories(SmallVectorLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define MagnitudeMapLib as an interface library (header-only)
add_library(MagnitudeMapLib INTERFACE)
target_include_directories(MagnitudeMapLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define TopNByRatioSelectorLib as an interface library (header-only)
add_library(TopNByRatioSelectorLib INTERFACE)
target_include_directories(TopNByRatioSelectorLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define IdPoolLib as an interface library (header-only)
add_library(IdPoolLib INTERFACE)
target_include_directories(IdPoolLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define FlatMapLib as an interface library (header-only)
add_library(FlatMapLib INTERFACE)
target_include_directories(FlatMapLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define ValueIndexMapLib as an interface library (header-only)
add_library(ValueIndexMapLib INTERFACE)
target_include_directories(ValueIndexMapLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define InvertedIndexLib as an interface library (header-only)
add_library(InvertedIndexLib INTERFACE)
target_include_directories(InvertedIndexLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define OneOfLib as an interface library (header-only)
add_library(OneOfLib INTERFACE)
target_include_directories(OneOfLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define PackedSlotMapLib as an interface library (header-only)
add_library(PackedSlotMapLib INTERFACE)
target_include_directories(PackedSlotMapLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(AsyncValueLib INTERFACE)
target_include_directories(AsyncValueLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define PredicateCacheLib as an interface library (header-only)
add_library(PredicateCacheLib INTERFACE)
target_include_directories(PredicateCacheLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(TaggedUnionLib INTERFACE)
target_include_directories(TaggedUnionLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define ChronoRingLib as an interface library (header-only)
add_library(ChronoRingLib INTERFACE)
target_include_directories(ChronoRingLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define TaggedPtrLib as an interface library (header-only)
add_library(TaggedPtrLib INTERFACE)
target_include_directories(TaggedPtrLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define ShadowCopyLib as an interface library (header-only)
add_library(ShadowCopyLib INTERFACE)
target_include_directories(ShadowCopyLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define StatBufferLib as an interface library (header-only)
add_library(StatBufferLib INTERFACE)
target_include_directories(StatBufferLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define DequeMapLib as an interface library (header-only)
add_library(DequeMapLib INTERFACE)
target_include_directories(DequeMapLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define WeightedSetLib as an interface library (header-only)
add_library(WeightedSetLib INTERFACE)
target_include_directories(WeightedSetLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define GenerationalArenaLib as an interface library (header-only)
add_library(GenerationalArenaLib INTERFACE)
target_include_directories(GenerationalArenaLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define GroupedSetLib as an interface library (header-only)
add_library(GroupedSetLib INTERFACE)
target_include_directories(GroupedSetLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define VectorBiMapLib as an interface library (header-only)
add_library(VectorBiMapLib INTERFACE)
target_include_directories(VectorBiMapLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define WeightedReservoirSamplerLib as an interface library (header-only)
add_library(WeightedReservoirSamplerLib INTERFACE)
target_include_directories(WeightedReservoirSamplerLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)


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
        CachedPropertyLib    # Added for cached_property_example
        QuotientFilterLib    # Added for quotient_filter_example (and potentially others)
        BTreeLib             # Added for btree_example
        CordLib              # Added for cord_example
        RibbonFilterLib      # Added for ribbon_filter_example
        DynamicBitsetLib     # Added for dynamic_bitset_example
        MultisetCounterLib   # Added for multiset_counter_example
        WeightedRandomListLib # Added for weighted_random_list_example
        ThreadSafeCounterLib # Added for thread_safe_counter_example
        ThreadSafeCacheLib   # Added for thread_safe_cache_example
        SmallVectorLib       # Added for small_vector_example
        MagnitudeMapLib      # Added for magnitude_map_example
        TopNByRatioSelectorLib
        IdPoolLib            # Added for id_pool_example
        FlatMapLib           # Added for flat_map_example
        ValueIndexMapLib     # Added for value_index_map_example
        InvertedIndexLib     # Added for inverted_index_example
        OneOfLib             # Added for one_of_example
        PackedSlotMapLib     # Added for packed_slot_map_example
        ChronoRingLib        # Added for chrono_ring_example
        AsyncValueLib
        TaggedPtrLib         # Added for tagged_ptr_example
        PredicateCacheLib    # Added for predicate_cache_example
        ShadowCopyLib        # Added for shadow_copy_example
        StatBufferLib        # Added for stat_buffer_example
        DequeMapLib          # Added for deque_map_example
        WeightedSetLib       # Added for weighted_set_example

        GenerationalArenaLib # Added for generational_arena_example
        GroupedSetLib        # Added for GroupedSet_example

        FrozenListLib        # Added for frozen_list_example
        SparseSetLib         # Added for sparse_set_example
        VectorBiMapLib       # Added for vector_bimap_example
        WeightedReservoirSamplerLib # Added for weighted_reservoir_sampler_example
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

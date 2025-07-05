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
        ScopedTimer
        RetryUtil
        ScopedFlagLib
        SplitView
        IntervalCounterLib
        IntervalTreeLib
        UniqueQueueLib
        JsonFieldMask # For json_fieldmask_test and its dependencies
        # nlohmann_json::nlohmann_json is an INTERFACE dependency of JsonFieldMask
        DeltaMapLib
        RetainLatestLib
        WithResourceLib # Added for with_resource_test
        CppLibRateLimiter
        TrieLib # Added for trie_test
        GraphLib # Added for graph_test
        TreapLib # Added for treap_test
        CountMinSketchLib # Added for count_min_sketch_test
        CountingBloomFilterLib

        FrozenSetLib # Added for frozen_set_test

        UnorderedMultisetLib # Added for unordered_multiset_test

        OrderedMultisetLib # Added for ordered_multiset_test

        FrozenDictLib        # Added for frozen_dict_test

        DequeLib             # Added for deque_test
        SkipListLib          # Added for skip_list_test
        PairwiseLib          # Added for pairwise_test
        RedBlackTreeLib      # Added for red_black_tree_test
        DictWrapperLib       # Added for dict_wrapper_test
        PriorityQueueMapLib  # Added for priority_queue_map_test
        ScapegoatTreeLib     # Added for scapegoat_tree_test
        SuffixArrayLib       # Added for suffix_array_test
        CachedPropertyLib    # Added for cached_property_test
        QuotientFilterLib    # Added for test_quotient_filter

        # Newly added libraries based on existing test files
        BloomFilterLib       # Added for bloom_filter_test
        CordLib              # Added for cord_test
        DynamicBitsetLib     # Added for dynamic_bitset_test
        RibbonFilterLib      # Added for ribbon_filter_test
        SegmentTreeLib       # Added for segment_tree_test
        ThreadSafeCacheLib   # Added for thread_safe_cache_test
        ThreadSafeCounterLib # Added for thread_safe_counter_test
        WeightedRandomListLib # Added for weighted_random_list_test
        SmallVectorLib       # Added for test_small_vector
        MagnitudeMapLib      # Added for magnitude_map_test
        TopNByRatioSelectorLib
        IdPoolLib            # Added for id_pool_test
        FlatMapLib           # Added for flat_map_test
        ValueIndexMapLib     # Added for value_index_map_test
        InvertedIndexLib     # Added for inverted_index_test
        OneOfLib             # Added for one_of_test
        PackedSlotMapLib     # Added for packed_slot_map_test
        ChronoRingLib        # Added for chrono_ring_test
        TaggedPtrLib         # Added for tagged_ptr_test
        PredicateCacheLib    # Added for predicate_cache_test
        ShadowCopyLib        # Added for shadow_copy_test
        StatBufferLib        # Added for stat_buffer_test
        DequeMapLib          # Added for deque_map_test
        WeightedSetLib       # Added for weighted_set_test
        GenerationalArenaLib # Added for generational_arena_test
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

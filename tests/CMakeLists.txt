cmake_minimum_required(VERSION 3.10)

# Add the include directory of the main project for headers
include_directories(${CMAKE_SOURCE_DIR}/include)
include(GoogleTest) # Modern way to add tests

# # Add the test executable for ribbon_filter
# add_executable(ribbon_filter_test ribbon_filter_test.cpp)
# target_link_libraries(ribbon_filter_test PRIVATE GTest::gtest GTest::gtest_main)
# gtest_discover_tests(ribbon_filter_test)

# # Add the test executable for dynamic_bitset
# add_executable(dynamic_bitset_test dynamic_bitset_test.cpp)
# target_link_libraries(dynamic_bitset_test PRIVATE GTest::gtest GTest::gtest_main)
# gtest_discover_tests(dynamic_bitset_test)

# # Add the test executable for multiset_counter
# add_executable(multiset_counter_test test_multiset_counter.cpp)
# target_link_libraries(multiset_counter_test PRIVATE GTest::gtest GTest::gtest_main)
# gtest_discover_tests(multiset_counter_test)

# # Add the test executable for interning_pool
# add_executable(interning_pool_test test_interning_pool.cpp)
# target_link_libraries(interning_pool_test PRIVATE GTest::gtest GTest::gtest_main)
# gtest_discover_tests(interning_pool_test)

# # Add the test executable for WeightedRandomList
# add_executable(weighted_random_list_test test_weighted_random_list.cpp)
# target_link_libraries(weighted_random_list_test PRIVATE GTest::gtest GTest::gtest_main)
# gtest_discover_tests(weighted_random_list_test)

# # Add the test executable for ThreadSafeCounter
# add_executable(thread_safe_counter_test test_thread_safe_counter.cpp)
# target_link_libraries(thread_safe_counter_test PRIVATE GTest::gtest GTest::gtest_main)
# gtest_discover_tests(thread_safe_counter_test)

# # Add the test executable for ThreadSafeCache
# add_executable(test_thread_safe_cache test_thread_safe_cache.cpp)
# target_link_libraries(test_thread_safe_cache PRIVATE GTest::gtest GTest::gtest_main)
# gtest_discover_tests(test_thread_safe_cache)

# Add the test executable for instrumented_ring_buffer
add_executable(instrumented_ring_buffer_test test_instrumented_ring_buffer.cpp)
target_link_libraries(instrumented_ring_buffer_test PRIVATE GTest::gtest GTest::gtest_main pthread) # Added pthread for std::thread
gtest_discover_tests(instrumented_ring_buffer_test)

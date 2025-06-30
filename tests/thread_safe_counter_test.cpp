#include "gtest/gtest.h"
#include "thread_safe_counter.hpp"
#include <thread>
#include <vector>
#include <string>
#include <numeric> // For std::iota
#include <set>     // For checking keys in most_common
#include <string_view> // For string literals if needed, but string should be fine

// For "s" literal
using namespace std::string_literals;

// Basic tests (single-threaded context)
TEST(ThreadSafeCounterTest, Initialization) {
    ThreadSafeCounter<std::string> c1;
    ASSERT_TRUE(c1.empty());
    ASSERT_EQ(c1.size(), 0);

    ThreadSafeCounter<int> c2({1, 2, 2, 3, 3, 3});
    ASSERT_FALSE(c2.empty());
    ASSERT_EQ(c2.size(), 3);
    ASSERT_EQ(c2.count(1), 1);
    ASSERT_EQ(c2.count(2), 2);
    ASSERT_EQ(c2.count(3), 3);
    ASSERT_EQ(c2.count(4), 0);

    ThreadSafeCounter<char> c3({{ 'a', 2 }, { 'b', 3 }});
    ASSERT_EQ(c3.count('a'), 2);
    ASSERT_EQ(c3.count('b'), 3);
    ASSERT_EQ(c3.total(), 5);
}

TEST(ThreadSafeCounterTest, AddAndCount) {
    ThreadSafeCounter<std::string> counter;
    counter.add("apple");
    ASSERT_EQ(counter.count("apple"), 1);
    counter.add("apple", 2);
    ASSERT_EQ(counter.count("apple"), 3);
    counter.add("banana");
    ASSERT_EQ(counter.count("banana"), 1);
    ASSERT_EQ(counter.size(), 2);
    ASSERT_EQ(counter.total(), 4);
}

TEST(ThreadSafeCounterTest, Subtract) {
    ThreadSafeCounter<std::string> counter;
    counter.add("apple", 5); // apple: 5
    counter.subtract("apple", 2); // apple: 3
    ASSERT_EQ(counter.count("apple"), 3);
    ASSERT_TRUE(counter.contains("apple"));

    counter.subtract("apple", 3); // apple: 3 - 3 = 0. Count becomes 0.
    ASSERT_EQ(counter.count("apple"), 0);
    ASSERT_FALSE(counter.contains("apple")); // contains checks for count > 0
    ASSERT_EQ(counter.size(), 0); // Item 'apple' exists with count 0, so not counted in size().

    counter.add("banana", 2); // banana: 2, apple: 0. size() should be 1 (for banana).
    ASSERT_EQ(counter.size(), 1);
    counter.subtract("banana", 5); // banana: 2 - 5 = -3. apple: 0. size() should be 0.
    ASSERT_EQ(counter.count("banana"), -3);
    ASSERT_FALSE(counter.contains("banana")); // count is not > 0
    ASSERT_EQ(counter.size(), 0); // apple (0), banana (-3). No positive counts.

    counter.add("orange", 3); // orange: 3, apple: 0, banana: -3. size() is 1 (for orange).
    ASSERT_EQ(counter.size(), 1);
    counter.subtract("non_existent", 2); // non_existent: -2 is created. orange: 3, apple: 0, banana: -3. size() is 1.
    ASSERT_EQ(counter.count("orange"), 3);
    ASSERT_TRUE(counter.contains("orange"));
    ASSERT_EQ(counter.count("non_existent"), -2);
    ASSERT_FALSE(counter.contains("non_existent"));
    ASSERT_EQ(counter.size(), 1); // apple (0), banana (-3), orange (3), non_existent (-2). Only orange has positive count.
}

TEST(ThreadSafeCounterTest, SetCount) {
    ThreadSafeCounter<std::string> counter;
    counter.set_count("apple", 5);
    ASSERT_EQ(counter.count("apple"), 5);
    ASSERT_TRUE(counter.contains("apple"));

    counter.set_count("apple", 0); // Setting to 0 should remove
    ASSERT_EQ(counter.count("apple"), 0);
    ASSERT_FALSE(counter.contains("apple"));

    counter.set_count("banana", 3);
    ASSERT_EQ(counter.count("banana"), 3);
    counter.set_count("apple", 2); // Add apple back
    ASSERT_EQ(counter.count("apple"), 2);
    ASSERT_EQ(counter.size(), 2);
}


TEST(ThreadSafeCounterTest, ContainsEraseClear) {
    ThreadSafeCounter<int> counter({1, 2, 2, 3});
    ASSERT_TRUE(counter.contains(1));
    ASSERT_TRUE(counter.contains(2));
    ASSERT_FALSE(counter.contains(4));

    ASSERT_EQ(counter.erase(2), 1); // Erase '2' (count was 2)
    ASSERT_FALSE(counter.contains(2));
    ASSERT_EQ(counter.count(2), 0);
    ASSERT_EQ(counter.size(), 2); // 1 and 3 remain

    ASSERT_EQ(counter.erase(5), 0); // Erase non-existent

    counter.clear();
    ASSERT_TRUE(counter.empty());
    ASSERT_EQ(counter.size(), 0);
    ASSERT_FALSE(counter.contains(1));
}

TEST(ThreadSafeCounterTest, MostCommon) {
    ThreadSafeCounter<std::string> counter({
        {"a", 1}, {"b", 5}, {"c", 2}, {"d", 5}, {"e", 3}
    });
    auto common = counter.most_common(3);
    ASSERT_EQ(common.size(), 3);
    // Order for ties (b and d both have 5) depends on internal tie-breaking (e.g. key order)
    // Assuming 'b' < 'd' for string comparison
    ASSERT_EQ(common[0].first, "b"); ASSERT_EQ(common[0].second, 5);
    ASSERT_EQ(common[1].first, "d"); ASSERT_EQ(common[1].second, 5);
    ASSERT_EQ(common[2].first, "e"); ASSERT_EQ(common[2].second, 3);

    auto all_common = counter.most_common(); // Get all
    ASSERT_EQ(all_common.size(), 5);
    // Check if all original elements are present
    std::set<std::string> keys;
    for(const auto& p : all_common) keys.insert(p.first);
    ASSERT_TRUE(keys.count("a") && keys.count("b") && keys.count("c") && keys.count("d") && keys.count("e"));


    ThreadSafeCounter<int> empty_counter;
    ASSERT_TRUE(empty_counter.most_common().empty());
    ASSERT_TRUE(empty_counter.most_common(5).empty());
}

TEST(ThreadSafeCounterTest, CopyAndAssignment) {
    ThreadSafeCounter<std::string> original;
    original.add("apple", 3);
    original.add("banana", 2);

    ThreadSafeCounter<std::string> copy_constructed = original;
    ASSERT_EQ(copy_constructed.count("apple"), 3);
    ASSERT_EQ(copy_constructed.count("banana"), 2);
    ASSERT_EQ(copy_constructed.size(), 2);

    ThreadSafeCounter<std::string> copy_assigned;
    copy_assigned.add("orange", 1); // To ensure it gets overwritten
    copy_assigned = original;
    ASSERT_EQ(copy_assigned.count("apple"), 3);
    ASSERT_EQ(copy_assigned.count("banana"), 2);
    ASSERT_EQ(copy_assigned.count("orange"), 0); // Should be gone
    ASSERT_EQ(copy_assigned.size(), 2);

    // Test self-assignment
    copy_assigned = copy_assigned;
    ASSERT_EQ(copy_assigned.count("apple"), 3);
    ASSERT_EQ(copy_assigned.count("banana"), 2);

    // Test move construction
    ThreadSafeCounter<std::string> moved_from = original; // make a copy first
    ThreadSafeCounter<std::string> moved_to(std::move(moved_from));
    ASSERT_EQ(moved_to.count("apple"), 3);
    ASSERT_EQ(moved_to.count("banana"), 2);
    // Standard doesn't guarantee source state for map after move, but our impl clears it
    ASSERT_TRUE(moved_from.empty());

    // Test move assignment
    ThreadSafeCounter<std::string> another_moved_from;
    another_moved_from.add("grape", 10);
    moved_to = std::move(another_moved_from);
    ASSERT_EQ(moved_to.count("grape"), 10);
    ASSERT_EQ(moved_to.count("apple"), 0); // Old content gone
    ASSERT_TRUE(another_moved_from.empty());
}


TEST(ThreadSafeCounterTest, ArithmeticOperators) {
    ThreadSafeCounter<char> c1({{'a', 1}, {'b', 2}});
    ThreadSafeCounter<char> c2({{'b', 3}, {'c', 4}});

    // Operator+
    ThreadSafeCounter<char> c_sum = c1 + c2;
    ASSERT_EQ(c_sum.count('a'), 1);
    ASSERT_EQ(c_sum.count('b'), 5); // 2 + 3
    ASSERT_EQ(c_sum.count('c'), 4);
    ASSERT_EQ(c_sum.size(), 3); // a(1), b(5), c(4) are all positive

    // Operator-
    ThreadSafeCounter<char> c_diff = c1 - c2;
    ASSERT_EQ(c_diff.count('a'), 1);
    ASSERT_EQ(c_diff.count('b'), -1); // 2 - 3 = -1. Our impl with set_count will remove if <=0.
                                     // The operator- itself can generate negative, then set_count cleans.
                                     // Let's check the direct result of operator-
    // The current implementation of operator- will store negative values if they result.
    // And `count` will return them. `set_count` is for explicit setting.
    // `subtract` method will remove if count <=0.
    // The `operator-` as implemented will result in `counts_` having negative values.
    // `count()` will return this. Items are only removed if their count becomes <=0 *during a subtract operation*
    // or if `set_count` is used with a non-positive value.
    // This behavior matches Python's Counter for subtraction.
    ASSERT_EQ(c_diff.count('b'), -1);
    ASSERT_EQ(c_diff.count('c'), -4); // 0 - 4
    ASSERT_EQ(c_diff.size(), 1); // Only 'a' (count 1) is positive. b(-1), c(-4) are not counted by size().

    // Operator+=
    ThreadSafeCounter<char> c_sum_assign = c1;
    c_sum_assign += c2;
    ASSERT_EQ(c_sum_assign.count('a'), 1);
    ASSERT_EQ(c_sum_assign.count('b'), 5);
    ASSERT_EQ(c_sum_assign.count('c'), 4);

    // Operator-=
    ThreadSafeCounter<char> c_diff_assign = c1; // c1 is {{'a', 1}, {'b', 2}}
    c_diff_assign -= c2; // c2 is {{'b', 3}, {'c', 4}}
    // Expected:
    // 'a': remains 1
    // 'b': 2 - 3 = -1. _subtract_nolock now allows negative counts.
    // 'c': not in c1. _subtract_nolock for ('c', 4) will create 'c' with count -4.
    ASSERT_EQ(c_diff_assign.count('a'), 1);
    ASSERT_EQ(c_diff_assign.count('b'), -1);
    // contains() checks for count > 0.
    ASSERT_FALSE(c_diff_assign.contains('b')); // count is -1
    ASSERT_EQ(c_diff_assign.count('c'), -4);
    ASSERT_FALSE(c_diff_assign.contains('c')); // count is -4
    ASSERT_EQ(c_diff_assign.size(), 1); // Only 'a' (count 1) is positive. b(-1), c(-4) are not counted by size().
}

TEST(ThreadSafeCounterTest, SetOperations) {
    ThreadSafeCounter<char> c1({{'a', 5}, {'b', 3}, {'c', 1}});
    ThreadSafeCounter<char> c2({{'b', 2}, {'c', 4}, {'d', 2}});

    // Intersection
    ThreadSafeCounter<char> intersection = c1.intersection(c2);
    ASSERT_EQ(intersection.count('a'), 0);
    ASSERT_EQ(intersection.count('b'), 2); // min(3, 2)
    ASSERT_EQ(intersection.count('c'), 1); // min(1, 4)
    ASSERT_EQ(intersection.count('d'), 0);
    ASSERT_EQ(intersection.size(), 2);

    // Union
    ThreadSafeCounter<char> uni = c1.union_with(c2);
    ASSERT_EQ(uni.count('a'), 5); // max(5, 0)
    ASSERT_EQ(uni.count('b'), 3); // max(3, 2)
    ASSERT_EQ(uni.count('c'), 4); // max(1, 4)
    ASSERT_EQ(uni.count('d'), 2); // max(0, 2)
    ASSERT_EQ(uni.size(), 4);
}

// Thread safety tests
TEST(ThreadSafeCounterTest, ConcurrentAdd) {
    ThreadSafeCounter<int> counter;
    std::vector<std::thread> threads;
    int num_threads = 10;
    int ops_per_thread = 1000;
    int key_to_increment = 1;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&counter, key_to_increment, ops_per_thread]() {
            for (int j = 0; j < ops_per_thread; ++j) {
                counter.add(key_to_increment);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(counter.count(key_to_increment), num_threads * ops_per_thread);
    ASSERT_EQ(counter.size(), 1);
}

TEST(ThreadSafeCounterTest, ConcurrentAddDifferentKeys) {
    ThreadSafeCounter<int> counter;
    std::vector<std::thread> threads;
    int num_threads = 10;
    int ops_per_thread = 100; // Reduced ops to make it faster

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&counter, i, ops_per_thread]() { // i is the key for this thread
            for (int j = 0; j < ops_per_thread; ++j) {
                counter.add(i); // Each thread increments its own key
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(counter.size(), num_threads);
    for (int i = 0; i < num_threads; ++i) {
        ASSERT_EQ(counter.count(i), ops_per_thread);
    }
}

TEST(ThreadSafeCounterTest, ConcurrentAddSubtractSameKey) {
    ThreadSafeCounter<int> counter;
    std::vector<std::thread> threads;
    int num_threads = 10;
    int ops_per_thread = 1000;
    int key = 7;

    // Each pair of threads: one adds, one subtracts
    for (int i = 0; i < num_threads / 2; ++i) {
        threads.emplace_back([&counter, key, ops_per_thread]() {
            for (int j = 0; j < ops_per_thread; ++j) {
                counter.add(key, 1);
            }
        });
        threads.emplace_back([&counter, key, ops_per_thread]() {
            for (int j = 0; j < ops_per_thread; ++j) {
                counter.subtract(key, 1);
            }
        });
    }
    // If num_threads is odd, one more adder thread
    if (num_threads % 2 != 0) {
        threads.emplace_back([&counter, key, ops_per_thread]() {
            for (int j = 0; j < ops_per_thread; ++j) {
                counter.add(key, 1);
            }
        });
    }


    for (auto& t : threads) {
        t.join();
    }

    // If num_threads is even, final count should be 0.
    // If num_threads is odd, final count should be ops_per_thread.
    int expected_count = (num_threads % 2 != 0) ? ops_per_thread : 0;
    ASSERT_EQ(counter.count(key), expected_count);
    if (expected_count == 0) {
        ASSERT_FALSE(counter.contains(key));
        ASSERT_EQ(counter.size(), 0);
    } else {
        ASSERT_TRUE(counter.contains(key));
        ASSERT_EQ(counter.size(), 1);
    }
}


TEST(ThreadSafeCounterTest, ConcurrentSetCount) {
    ThreadSafeCounter<std::string> counter;
    std::vector<std::thread> threads;
    int num_threads = 5; // Fewer threads, as it's more about last-write-wins

    std::string key = "test_key";

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&counter, key, i]() {
            // Each thread attempts to set the count to its own index + 1
            // Sleep a bit to encourage interleaving, though not guaranteed
            std::this_thread::sleep_for(std::chrono::microseconds(i * 10));
            counter.set_count(key, i + 1);
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // The final count is not strictly deterministic due to thread scheduling.
    // However, it must be one of the values set by the threads (1 to num_threads).
    int final_count = counter.count(key);
    ASSERT_GE(final_count, 1);
    ASSERT_LE(final_count, num_threads);
    if (final_count > 0) {
         ASSERT_TRUE(counter.contains(key));
    } else { // If some thread managed to set it to 0 (e.g. if values could be 0)
         ASSERT_FALSE(counter.contains(key));
    }
}


TEST(ThreadSafeCounterTest, ConcurrentMostCommon) {
    ThreadSafeCounter<int> counter;
    std::vector<std::thread> threads;
    int num_add_threads = 5;
    int items_per_thread = 10; // Each thread adds 0 to 9

    for (int i = 0; i < num_add_threads; ++i) {
        threads.emplace_back([&counter, items_per_thread, i]() {
            for (int j = 0; j < items_per_thread; ++j) {
                counter.add(j, i + 1); // Add varying amounts to make counts different
            }
        });
    }

    std::vector<std::pair<int, int>> common_items;
    // Thread to concurrently call most_common
    threads.emplace_back([&counter, &common_items]() {
        // Call most_common multiple times to increase chance of catching issues
        for(int k=0; k<10; ++k) {
            common_items = counter.most_common(5);
            // Basic check: size should be <= 5. Content can vary.
            ASSERT_LE(common_items.size(), 5);
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
    });

    for (auto& t : threads) {
        t.join();
    }

    // After all threads join, check final state from main thread
    auto final_common = counter.most_common(items_per_thread); // Get all distinct items
    ASSERT_LE(final_common.size(), items_per_thread); // Should have at most 10 distinct keys (0-9)

    int total_sum_of_counts = 0;
    for(const auto& p : final_common) {
        total_sum_of_counts += p.second;
    }
    ASSERT_EQ(counter.total(), total_sum_of_counts);
    // The sum of counts for each key `j` should be sum_{i=0 to num_add_threads-1} (i+1)
    // = items_per_thread * (sum of 1 to num_add_threads)
    // = items_per_thread * (num_add_threads * (num_add_threads + 1) / 2)
    int expected_total_sum = 0;
    for (int j=0; j < items_per_thread; ++j) { // for each key 0-9
        int expected_count_for_key_j = 0;
        for (int i=0; i < num_add_threads; ++i) { // each thread i adds (i+1) to key j
            expected_count_for_key_j += (i+1);
        }
        ASSERT_EQ(counter.count(j), expected_count_for_key_j);
        expected_total_sum += expected_count_for_key_j;
    }
    ASSERT_EQ(counter.total(), expected_total_sum);

    // The `common_items` from the concurrent thread is harder to verify precisely,
    // but we ensured its size was okay during its execution.
}

// Test for deduction guides (compile-time check)
TEST(ThreadSafeCounterTest, DeductionGuides) {
    std::vector<int> v = {1, 2, 2, 3};
    ThreadSafeCounter c_from_iter(v.begin(), v.end()); // Should deduce ThreadSafeCounter<int>
    ASSERT_EQ(c_from_iter.count(2), 2);

    ThreadSafeCounter c_from_init_list = {1, 2, 2, 3}; // Should deduce ThreadSafeCounter<int>
    ASSERT_EQ(c_from_init_list.count(2), 2);

    // Explicitly create pairs for the initializer list
    ThreadSafeCounter c_from_pair_list = {
        std::pair<std::string, int>(std::string("a"), 1),
        std::pair<std::string, int>(std::string("b"), 2)
    }; // Should deduce ThreadSafeCounter<std::string>
    ASSERT_EQ(c_from_pair_list.count("b"), 2);

    // Simpler form that should also work with CTAD for pairs if T is specified
    ThreadSafeCounter<std::string> c_from_pair_list_simpler = {{"a", 1}, {"b", 2}};
    ASSERT_EQ(c_from_pair_list_simpler.count("b"), 2);

    // Test auto-deduction with simpler pair form
    ThreadSafeCounter c_from_pair_list_auto = {std::pair("a"s, 1), std::pair("b"s, 2)};
    ASSERT_EQ(c_from_pair_list_auto.count("b"s), 2);


    // Suppress unused variable warnings for the above
    (void)c_from_iter;
    (void)c_from_init_list;
    (void)c_from_pair_list;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

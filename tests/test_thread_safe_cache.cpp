#include "thread_safe_cache.hpp"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <set>

// Basic test runner
int tests_run = 0;
int tests_failed = 0;

#define RUN_TEST(test_name) \
    do { \
        tests_run++; \
        std::cout << "Running test: " #test_name "..." << std::endl; \
        try { \
            test_name(); \
            std::cout << #test_name " PASSED." << std::endl; \
        } catch (const std::exception& e) { \
            tests_failed++; \
            std::cerr << #test_name " FAILED: " << e.what() << std::endl; \
        } catch (...) { \
            tests_failed++; \
            std::cerr << #test_name " FAILED: Unknown exception." << std::endl; \
        } \
    } while (0)

void test_constructor_and_basic_properties() {
    cpp_collections::ThreadSafeCache<int, std::string> cache(5);
    assert(cache.size() == 0);
    assert(cache.empty());

    bool thrown = false;
    try {
        cpp_collections::ThreadSafeCache<int, std::string> zero_cap_cache(0);
    } catch (const std::invalid_argument& e) {
        thrown = true;
    }
    assert(thrown);
}

void test_put_get_lru() {
    cpp_collections::ThreadSafeCache<int, std::string> cache(3, cpp_collections::EvictionPolicy::LRU);
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");

    assert(cache.size() == 3);
    assert(cache.get(1).value_or("") == "one");
    assert(cache.get(2).value_or("") == "two");
    assert(cache.get(3).value_or("") == "three");

    // Test update
    cache.put(1, "new_one");
    assert(cache.get(1).value_or("") == "new_one");
    assert(cache.size() == 3); // Size should not change

    // Test LRU eviction
    cache.get(2); // 2 is now MRU, 1 is next, 3 is LRU
    cache.put(4, "four"); // Evicts 3
    assert(cache.size() == 3);
    assert(cache.get(3) == std::nullopt);
    assert(cache.get(4).value_or("") == "four");
    assert(cache.get(1).value_or("") == "new_one");
    assert(cache.get(2).value_or("") == "two");

    // Access 1, make 4 LRU
    cache.get(1); // lru_order_ was [2,1,4] (from line 71's get(2)). Becomes [1,2,4]. (4 is LRU)

    cache.put(5, "five"); // Evicts 4 (the LRU item)
    assert(cache.get(4) == std::nullopt);       // Assert that 4 was evicted
    assert(cache.get(2).value_or("") == "two"); // Assert that 2 is still present
    assert(cache.get(1).value_or("") == "new_one"); // Assert that 1 is still present
    assert(cache.get(5).value_or("") == "five");    // Assert that 5 was added
}

void test_put_get_fifo() {
    cpp_collections::ThreadSafeCache<int, std::string> cache(3, cpp_collections::EvictionPolicy::FIFO);
    cache.put(1, "one"); // 1st in
    cache.put(2, "two"); // 2nd in
    cache.put(3, "three"); // 3rd in

    assert(cache.size() == 3);
    assert(cache.get(1).value_or("") == "one");

    cache.get(1); // Accessing does not change FIFO order
    cache.put(4, "four"); // Evicts 1 (oldest)
    assert(cache.size() == 3);
    assert(cache.get(1) == std::nullopt);
    assert(cache.get(4).value_or("") == "four");
    assert(cache.get(2).value_or("") == "two"); // 2 is now oldest

    cache.put(5, "five"); // Evicts 2
    assert(cache.get(2) == std::nullopt);
    assert(cache.get(5).value_or("") == "five");
    assert(cache.get(3).value_or("") == "three"); // 3 is now oldest
}

void test_put_get_lfu() {
    cpp_collections::ThreadSafeCache<int, std::string> cache(3, cpp_collections::EvictionPolicy::LFU);
    cache.put(1, "one");   // Freq(1)=1
    cache.put(2, "two");   // Freq(2)=1
    cache.put(3, "three"); // Freq(3)=1

    cache.get(1); // Freq(1)=2
    cache.get(1); // Freq(1)=3
    cache.get(2); // Freq(2)=2
    // Freqs: 1:3, 2:2, 3:1

    cache.put(4, "four"); // Evicts 3 (LFU)
    assert(cache.size() == 3);
    assert(cache.get(3) == std::nullopt);
    assert(cache.get(4).value_or("") == "four"); // Freq(4)=1
    // Freqs: 1:3, 2:2, 4:1

    cache.get(4); // Freq(4)=2
    cache.get(4); // Freq(4)=3
    // Freqs: 1:3, 2:2, 4:3

    cache.put(5, "five"); // Evicts 2 (LFU)
    assert(cache.get(2) == std::nullopt);
    assert(cache.get(5).value_or("") == "five"); // Freq(5)=1
    // Freqs: 1:3, 4:3, 5:1

    // Test tie-breaking: 1 and 4 have freq 3. 5 has freq 1.
    cache.get(1); // Freq(1)=4
    cache.get(5); // Freq(5)=2
    cache.get(5); // Freq(5)=3
    // Freqs: 1:4, 4:3, 5:3
    // Access 4 to make it MRU among freq 3 items (excluding 1)
    cache.get(4); // Freq(4)=4
    // Freqs: 1:4 (accessed first), 4:4 (accessed after 1), 5:3
    // LRU within freq 4 is 1.

    // Current state: (1, "one", F:4), (4, "four", F:4), (5, "five", F:3)
    // Access order for F:4 items: 1 was put then its F became 4. 4 was put, its F became 3, then accessed to F:4.
    // So 1 is LRU within F:4 group.

    cache.put(6, "six"); // Evicts 5 (LFU, F:3)
    assert(cache.get(5) == std::nullopt);
    assert(cache.get(6).value_or("") == "six"); // Freq(6)=1
    // Freqs: 1:4, 4:4, 6:1

    // Make 1 and 4 have same freq, 6 LFU
    // Access 1, 4 again
    cache.get(1); // F:5
    cache.get(4); // F:5
    // Freqs: 1:5, 4:5, 6:1
    // Within F:5, 1 was accessed before 4. So 1 is LRU within F:5.
    cache.put(7, "seven"); // Evicts 6 (LFU)
    assert(cache.get(6) == std::nullopt);
    // Freqs: 1:5, 4:5, 7:1

    // Now make 1, 4, 7 all freq 5.
    // Access 7 four times
    for(int i=0; i<4; ++i) cache.get(7); // F:5
    // Freqs: 1:5, 4:5, 7:5
    // Access order for F:5: 1, then 4, then 7. So 1 is LRU.
    cache.put(8, "eight"); // Evicts 1
    assert(cache.get(1) == std::nullopt);
    assert(cache.get(8).value_or("") == "eight"); // F:1
}


void test_erase() {
    cpp_collections::ThreadSafeCache<int, std::string> cache(3, cpp_collections::EvictionPolicy::LRU);
    cache.put(1, "one");
    cache.put(2, "two");
    assert(cache.size() == 2);

    assert(cache.erase(1));
    assert(cache.size() == 1);
    assert(cache.get(1) == std::nullopt);
    assert(cache.get(2).value_or("") == "two");

    assert(!cache.erase(1)); // Already erased
    assert(cache.erase(2));
    assert(cache.empty());

    // Test erase with LFU (more complex internal state)
    cpp_collections::ThreadSafeCache<int, std::string> lfu_cache(3, cpp_collections::EvictionPolicy::LFU);
    lfu_cache.put(10, "A"); // F:1
    lfu_cache.put(20, "B"); // F:1
    lfu_cache.get(10);      // F(10)=2, F(20)=1
    assert(lfu_cache.erase(10));
    assert(lfu_cache.get(10) == std::nullopt);
    assert(lfu_cache.size() == 1);
    lfu_cache.put(30, "C"); // F(20)=1, F(30)=1
    assert(lfu_cache.size() == 2);
    // Eviction should pick 20 if we add 2 more
    lfu_cache.put(40, "D");
    lfu_cache.put(50, "E"); // 20 should be evicted (oldest F:1)
    assert(lfu_cache.get(20) == std::nullopt);
    assert(lfu_cache.size() == 3);


    // Test erase with FIFO
    cpp_collections::ThreadSafeCache<int, std::string> fifo_cache(3, cpp_collections::EvictionPolicy::FIFO);
    fifo_cache.put(1, "a");
    fifo_cache.put(2, "b");
    fifo_cache.put(3, "c");
    assert(fifo_cache.erase(2)); // Erase middle element
    assert(fifo_cache.get(2) == std::nullopt);
    assert(fifo_cache.size() == 2);
    fifo_cache.put(4, "d"); // Should evict 1 (oldest of 1,3)
    assert(fifo_cache.get(1).value_or("") == "a"); // No, put(4) makes cache {1,3,4}. Eviction later.
                                                 // After erase(2), queue is {1,3}. Cache size 2.
                                                 // Put(4) adds 4. Queue {1,3,4}. Cache size 3. No eviction yet.
    assert(fifo_cache.get(4).value_or("") == "d");
    fifo_cache.put(5, "e"); // Now evict. 1 is front of {1,3,4}.
    assert(fifo_cache.get(1) == std::nullopt);
    assert(fifo_cache.size() == 3); // {3,4,5}
}

void test_clear() {
    cpp_collections::ThreadSafeCache<int, std::string> cache(3);
    cache.put(1, "one");
    cache.put(2, "two");
    cache.clear();
    assert(cache.size() == 0);
    assert(cache.empty());
    assert(cache.get(1) == std::nullopt);
    cache.put(3,"three"); // Should work after clear
    assert(cache.get(3).value_or("") == "three");
    assert(cache.size() == 1);
}

void test_thread_safety_concurrent_put() {
    const int num_threads = 10;
    const int items_per_thread = 100;
    const int cache_capacity = 50;
    cpp_collections::ThreadSafeCache<int, int> cache(cache_capacity, cpp_collections::EvictionPolicy::LRU);
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&cache, i, items_per_thread, num_threads]() {
            for (int j = 0; j < items_per_thread; ++j) {
                int key = (i * items_per_thread + j); // Unique keys per thread initially, then overlap by modulo
                cache.put(key % (cache_capacity * 2) , i * 1000 + j);
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    assert(cache.size() <= cache_capacity); // Must not exceed capacity
    // Further checks would require specific knowledge of eviction details or item survival.
    // The main point is no crashes/deadlocks.
}

void test_thread_safety_concurrent_put_get_erase() {
    const int num_threads = 10;
    const int operations_per_thread = 200; // Increased operations
    const int cache_capacity = 75; // Slightly larger capacity
    cpp_collections::ThreadSafeCache<int, int> cache(cache_capacity, cpp_collections::EvictionPolicy::LFU); // Test LFU
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&cache, i, operations_per_thread, cache_capacity]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                int op_type = j % 3;
                int key_val = (i * operations_per_thread + j);
                int key = key_val % (cache_capacity + 20); // Keys will overlap

                if (op_type == 0) { // PUT
                    cache.put(key, key_val);
                } else if (op_type == 1) { // GET
                    cache.get(key);
                } else { // ERASE
                    cache.erase(key % (cache_capacity)); // Erase a subset of possible keys
                }
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    assert(cache.size() <= cache_capacity);
    // Check if some items exist (probabilistic, but better than nothing)
    // This is hard to assert definitively without a deterministic scenario.
    // For now, successful completion without crash is the primary check.
    int found_items = 0;
    for(int k=0; k < cache_capacity + 20; ++k) {
        if(cache.get(k).has_value()) {
            found_items++;
        }
    }
    std::cout << "Items found after mixed operations: " << found_items << std::endl;
}


int main() {
    std::cout << "Starting ThreadSafeCache tests..." << std::endl;

    RUN_TEST(test_constructor_and_basic_properties);
    RUN_TEST(test_put_get_lru);
    RUN_TEST(test_put_get_fifo);
    RUN_TEST(test_put_get_lfu);
    RUN_TEST(test_erase);
    RUN_TEST(test_clear);
    RUN_TEST(test_thread_safety_concurrent_put);
    RUN_TEST(test_thread_safety_concurrent_put_get_erase);

    std::cout << "\nTests finished." << std::endl;
    std::cout << "Total tests run: " << tests_run << std::endl;
    std::cout << "Tests failed: " << tests_failed << std::endl;

    return tests_failed == 0 ? 0 : 1;
}

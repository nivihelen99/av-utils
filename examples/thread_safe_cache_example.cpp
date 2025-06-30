#include "thread_safe_cache.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <cassert>

// Helper function to print cache contents (for demonstration)
// Not thread-safe if called concurrently with modifications without external locking.
// For this example, it's called in controlled sections.
template <typename K, typename V>
void print_cache_status(const cpp_collections::ThreadSafeCache<K, V>& cache, const std::string& name) {
    // This is a conceptual print. Direct iteration isn't provided by the cache interface
    // for simplicity and to avoid exposing internal state management.
    // We'll rely on get() and known key sequences for demonstration.
    std::cout << "--- Cache Status: " << name << " ---" << std::endl;
    std::cout << "Size: " << cache.size() << std::endl;
    // To show actual content, one would typically try to get known keys.
    // Example: cache.get(known_key_1), cache.get(known_key_2), etc.
    // For this example, we'll infer state from operations.
}

void example_lru_cache() {
    std::cout << "\n--- LRU Cache Example ---" << std::endl;
    cpp_collections::ThreadSafeCache<int, std::string> lru_cache(3, cpp_collections::EvictionPolicy::LRU);

    lru_cache.put(1, "apple");
    lru_cache.put(2, "banana");
    lru_cache.put(3, "cherry");
    print_cache_status(lru_cache, "LRU Initial"); // Expected: 1,2,3

    lru_cache.get(1); // Access 1, making it most recently used
    print_cache_status(lru_cache, "LRU Accessed 1"); // Order: 1 (MRU), 3, 2 (LRU)

    lru_cache.put(4, "date"); // Cache is full (1,3,2), 2 should be evicted
    print_cache_status(lru_cache, "LRU Added 4, Evicted 2"); // Expected: 4,1,3

    assert(lru_cache.get(2) == std::nullopt); // 2 should be evicted
    assert(lru_cache.get(1).value_or("") == "apple");
    assert(lru_cache.get(3).value_or("") == "cherry");
    assert(lru_cache.get(4).value_or("") == "date");
    std::cout << "LRU assertions passed." << std::endl;
}

void example_fifo_cache() {
    std::cout << "\n--- FIFO Cache Example ---" << std::endl;
    cpp_collections::ThreadSafeCache<int, std::string> fifo_cache(3, cpp_collections::EvictionPolicy::FIFO);

    fifo_cache.put(1, "one");
    fifo_cache.put(2, "two");
    fifo_cache.put(3, "three");
    print_cache_status(fifo_cache, "FIFO Initial"); // Expected: 1,2,3 (1 is oldest)

    fifo_cache.get(1); // Accessing 1 does not change its FIFO order
    print_cache_status(fifo_cache, "FIFO Accessed 1");

    fifo_cache.put(4, "four"); // Cache is full, 1 (oldest) should be evicted
    print_cache_status(fifo_cache, "FIFO Added 4, Evicted 1"); // Expected: 2,3,4

    assert(fifo_cache.get(1) == std::nullopt); // 1 should be evicted
    assert(fifo_cache.get(2).value_or("") == "two");
    assert(fifo_cache.get(3).value_or("") == "three");
    assert(fifo_cache.get(4).value_or("") == "four");
    std::cout << "FIFO assertions passed." << std::endl;
}

void example_lfu_cache() {
    std::cout << "\n--- LFU Cache Example ---" << std::endl;
    cpp_collections::ThreadSafeCache<int, std::string> lfu_cache(3, cpp_collections::EvictionPolicy::LFU);

    lfu_cache.put(1, "cat");    // Freq(1)=1
    lfu_cache.put(2, "dog");    // Freq(2)=1
    lfu_cache.put(3, "emu");    // Freq(3)=1
    print_cache_status(lfu_cache, "LFU Initial");

    lfu_cache.get(1);           // Freq(1)=2
    lfu_cache.get(1);           // Freq(1)=3
    lfu_cache.get(2);           // Freq(2)=2
    print_cache_status(lfu_cache, "LFU Accessed 1 (x2), 2 (x1)");
    // Freqs: 1:3, 2:2, 3:1

    lfu_cache.put(4, "fox");    // Cache full. Evict LFU. 3 (freq 1) is LFU.
                                // If multiple LFU, evict LRU among them. Here, 3 is unique LFU.
    print_cache_status(lfu_cache, "LFU Added 4, Evicted 3");
    // Cache: 1 (freq 3), 2 (freq 2), 4 (freq 1)

    assert(lfu_cache.get(3) == std::nullopt); // 3 should be evicted
    assert(lfu_cache.get(1).value_or("") == "cat");
    assert(lfu_cache.get(2).value_or("") == "dog");
    assert(lfu_cache.get(4).value_or("") == "fox");

    lfu_cache.get(4); // Freq(4)=2. Now 2 and 4 have freq 2. 1 has freq 3.
    // Cache: 1 (freq 3, MRU), 4 (freq 2, MRU within freq 2), 2 (freq 2, LRU within freq 2)

    lfu_cache.put(5, "gnu"); // Evict. Both 2 and 4 have freq 2.
                             // 2 was put before 4, and 4 was accessed more recently than 2's last access.
                             // LFU evicts based on lowest frequency. Tie-breaking is LRU within that frequency.
                             // Key 2 was accessed (put) then its frequency became 2.
                             // Key 4 was put then its frequency became 1, then accessed, its frequency became 2.
                             // Key 2 is LRU among those with frequency 2.
    print_cache_status(lfu_cache, "LFU Added 5, Evicted 2");
    // Cache: 1 (freq 3), 4 (freq 2), 5 (freq 1)

    assert(lfu_cache.get(2) == std::nullopt); // 2 should be evicted
    assert(lfu_cache.get(1).value_or("") == "cat");
    assert(lfu_cache.get(4).value_or("") == "fox");
    assert(lfu_cache.get(5).value_or("") == "gnu");
    std::cout << "LFU assertions passed." << std::endl;
}

void thread_safety_example() {
    std::cout << "\n--- Thread Safety Example (LRU) ---" << std::endl;
    cpp_collections::ThreadSafeCache<int, int> cache(100, cpp_collections::EvictionPolicy::LRU);
    std::vector<std::thread> threads;
    int num_threads = 10;
    int operations_per_thread = 1000;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&cache, i, operations_per_thread, num_threads]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                int key = (i * operations_per_thread + j) % (cache.size() + 50); // Vary keys
                cache.put(key, i * 10000 + j);
                if (j % 10 == 0) {
                    cache.get(key - 10); // Mix in some gets
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    std::cout << "Thread safety example completed. Final cache size: " << cache.size() << std::endl;
    // Verifying exact state is complex without more introspection or specific test patterns.
    // The main goal here is to run concurrently without crashing or deadlocking.
    assert(cache.size() <= 100); // Should not exceed capacity
    std::cout << "Thread safety basic check passed (no crash, respects capacity)." << std::endl;
}


int main() {
    example_lru_cache();
    example_fifo_cache();
    example_lfu_cache();
    thread_safety_example();

    std::cout << "\nAll examples completed." << std::endl;
    return 0;
}

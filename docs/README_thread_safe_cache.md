# ThreadSafeCache

## Overview

`ThreadSafeCache` is a C++ template class providing a generic thread-safe caching mechanism. It supports key-value storage with a configurable maximum capacity and three common eviction policies: Least Recently Used (LRU), Least Frequently Used (LFU), and First-In, First-Out (FIFO).

This cache is designed for scenarios where multiple threads need to access shared cached data concurrently, ensuring data integrity and preventing race conditions through internal mutex locking.

## Features

-   **Thread Safety**: All public methods are internally synchronized using `std::mutex`, making it safe for concurrent access.
-   **Generic**: Template-based design allows for caching of any key and value types.
-   **Configurable Capacity**: The maximum number of items the cache can hold is defined at construction.
-   **Eviction Policies**: Supports LRU, LFU, and FIFO eviction strategies.
    -   **LRU (Least Recently Used)**: When the cache is full, the item that hasn't been accessed for the longest time is evicted.
    -   **LFU (Least Frequently Used)**: When the cache is full, the item that has been accessed the fewest times is evicted. Ties are broken by evicting the least recently used item among those with the same minimum frequency.
    -   **FIFO (First-In, First-Out)**: When the cache is full, the item that was added first (oldest) is evicted.
-   **Header-Only**: Implemented as a header-only library for easy integration (just include `thread_safe_cache.hpp`).

## API

The `ThreadSafeCache<Key, Value>` class provides the following public methods:

-   `ThreadSafeCache(size_t capacity, EvictionPolicy policy = EvictionPolicy::LRU)`
    -   Constructor.
    -   `capacity`: The maximum number of items the cache can hold. Must be greater than 0.
    -   `policy`: The eviction policy to use. Can be `EvictionPolicy::LRU`, `EvictionPolicy::LFU`, or `EvictionPolicy::FIFO`. Defaults to LRU.
    -   Throws `std::invalid_argument` if capacity is 0.

-   `void put(const Key& key, const Value& value)`
    -   Inserts or updates a key-value pair in the cache.
    -   If the key already exists, its value is updated, and its status is updated according to the eviction policy (e.g., marked as recently used for LRU, frequency incremented for LFU).
    -   If the key does not exist and the cache is full, an item is evicted based on the configured policy before the new item is inserted.

-   `std::optional<Value> get(const Key& key)`
    -   Retrieves the value associated with the given key.
    -   If the key is found, its status is updated according to the eviction policy, and an `std::optional` containing the value is returned.
    -   If the key is not found, `std::nullopt` is returned.

-   `bool erase(const Key& key)`
    -   Removes the key-value pair associated with the given key from the cache.
    -   Returns `true` if the key was found and removed, `false` otherwise.

-   `void clear()`
    -   Removes all items from the cache.

-   `size_t size() const`
    -   Returns the current number of items in the cache.

-   `bool empty() const`
    -   Returns `true` if the cache contains no items, `false` otherwise.

## Eviction Policies Enum

The `EvictionPolicy` enum is defined as:

```cpp
namespace cpp_collections {
    enum class EvictionPolicy {
        LRU,
        LFU,
        FIFO
    };
}
```

## Usage Example

```cpp
#include "thread_safe_cache.hpp"
#include <iostream>
#include <string>
#include <optional>

int main() {
    // Create an LRU cache with a capacity of 2
    cpp_collections::ThreadSafeCache<int, std::string> cache(2, cpp_collections::EvictionPolicy::LRU);

    cache.put(1, "apple");
    cache.put(2, "banana");
    std::cout << "Cache size: " << cache.size() << std::endl; // Output: 2

    if (auto val = cache.get(1)) {
        std::cout << "Got 1: " << val.value() << std::endl; // Output: apple
    }

    cache.put(3, "cherry"); // Cache is full, (2, "banana") should be evicted as it's LRU
                            // after accessing 1.
    std::cout << "Cache size after adding 3: " << cache.size() << std::endl; // Output: 2

    if (cache.get(2)) {
        std::cout << "Got 2: " << cache.get(2).value() << std::endl;
    } else {
        std::cout << "Key 2 not found (evicted)." << std::endl; // Output: Key 2 not found
    }

    if (auto val = cache.get(3)) {
        std::cout << "Got 3: " << val.value() << std::endl; // Output: cherry
    }

    // --- LFU Example ---
    cpp_collections::ThreadSafeCache<int, std::string> lfu_cache(2, cpp_collections::EvictionPolicy::LFU);
    lfu_cache.put(10, "item10"); // Freq(10)=1
    lfu_cache.put(20, "item20"); // Freq(20)=1

    lfu_cache.get(10); // Freq(10)=2
    lfu_cache.get(10); // Freq(10)=3
    lfu_cache.get(20); // Freq(20)=2
    // Freqs: 10:3, 20:2

    lfu_cache.put(30, "item30"); // Evicts item20 (LFU)
    if (!lfu_cache.get(20)) {
        std::cout << "Item 20 evicted by LFU policy." << std::endl;
    }
     if (auto val = lfu_cache.get(10)) {
        std::cout << "LFU Cache still has 10: " << val.value() << std::endl;
    }


    // --- FIFO Example ---
    cpp_collections::ThreadSafeCache<int, std::string> fifo_cache(2, cpp_collections::EvictionPolicy::FIFO);
    fifo_cache.put(100, "first_in");
    fifo_cache.put(200, "second_in");

    fifo_cache.get(100); // Accessing does not change FIFO order

    fifo_cache.put(300, "third_in"); // Evicts "first_in" (oldest)
    if (!fifo_cache.get(100)) {
        std::cout << "Item 100 evicted by FIFO policy." << std::endl;
    }
    if (auto val = fifo_cache.get(200)) {
        std::cout << "FIFO Cache still has 200: " << val.value() << std::endl;
    }

    return 0;
}
```

## Thread Safety Details

The `ThreadSafeCache` uses a single `std::mutex` to synchronize access to all its internal data structures. Every public method acquires this mutex upon entry (typically using `std::lock_guard`) and releases it upon exit. This ensures that operations are atomic and the cache's internal state remains consistent even when accessed by multiple threads simultaneously.

While this approach guarantees thread safety, it also means that operations are serialized. For highly contented caches, this could become a bottleneck. More advanced implementations might use finer-grained locking or lock-free techniques for higher concurrency, but these come with significantly increased complexity. For many common use cases, the current `std::mutex`-based approach provides a good balance of safety and performance.

## Implementation Notes

-   **LRU Policy**: Implemented using a `std::list` to keep track of the access order (MRU at the front, LRU at the back) and an `std::unordered_map` to store iterators to the list nodes for O(1) updates and removals.
-   **LFU Policy**: Implemented using a list of frequency nodes (`std::list<LfuFrequencyNode>`), where each node contains its frequency and a list of keys (`std::list<Key>`) that currently have that frequency. The list of keys within a frequency node is maintained in LRU order to break ties when multiple keys have the same lowest frequency. An `std::unordered_map` stores iterators to quickly locate a key's frequency node and its position within that node's key list.
-   **FIFO Policy**: Implemented using a `std::queue` to maintain the insertion order of keys.

The main data storage is an `std::unordered_map<Key, Value>`.Tool output for `create_file_with_block`:

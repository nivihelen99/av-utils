# LRU Cache Utilities (`lru_cache.h`)

## Overview

The `lru_cache.h` header provides a thread-safe Least Recently Used (LRU) cache implementation, `LRUCache<Key, Value>`, designed to store a fixed number of key-value pairs. When the cache reaches its maximum capacity and a new item is added, the least recently used item is evicted to make space.

Additionally, the header includes utilities for function caching:
-   `CachedFunction<Key, Value>`: A wrapper class to cache the results of a function.
-   `make_cached(...)`: A factory function to create `CachedFunction` instances.
-   `CACHED_FUNCTION(...)`: A macro for a more concise syntax to define cached functions.

## `LRUCache<Key, Value>`

### Features
-   **Fixed Size:** Stores up to a `max_size` number of items.
-   **LRU Eviction Policy:** When full, adding a new item evicts the least recently accessed item.
-   **Thread-Safe:** Access and modification methods are protected by a `std::shared_mutex`, allowing concurrent reads and serialized writes.
-   **Efficient Operations:**
    -   `get(key)`: O(1) average time. Marks the item as most recently used.
    -   `put(key, value)`: O(1) average time. Marks the item as most recently used.
    -   Eviction: O(1).
-   **Eviction Callback:** Optional callback function `EvictCallback(const Key&, const Value&)` is invoked when an item is evicted.
-   **Statistics:** Tracks hits, misses, and evictions. `get_stats()` returns a `Stats` struct with `hit_rate()`.
-   **Move Semantics:** `LRUCache` itself is move-constructible and move-assignable. Copy operations are deleted.
-   **Generic:** Templated on `Key` and `Value` types.

### Internal Implementation
-   Uses a `std::list<std::pair<Key, Value>>` to maintain the order of items by recency of use (Most Recently Used at the front, Least Recently Used at the back).
-   Uses an `std::unordered_map<Key, typename std::list<...>::iterator>` to map keys to their positions in the list for O(1) average time lookups.

### Public Interface Highlights
-   **Constructor**: `explicit LRUCache(size_t max_size, EvictCallback on_evict = nullptr)`
-   **`std::optional<Value> get(const Key& key)`**: Retrieves value; marks item as MRU. Returns `std::nullopt` if key not found.
-   **`void put(K&& key, V&& value)` / `void put(const Key& key, const Value& value)`**: Inserts or updates item; marks item as MRU. Evicts LRU item if cache is full.
-   **`bool contains(const Key& key) const`**: Checks if key is in cache.
-   **`bool erase(const Key& key)`**: Removes item by key.
-   **`void clear()`**: Clears all items.
-   **`size_t size() const` / `bool empty() const` / `size_t max_size() const`**.
-   **`Stats get_stats() const` / `void reset_stats()`**.

## Function Caching Utilities

### `CachedFunction<Key, Value>`
A wrapper that caches the results of a wrapped `std::function<Value(Key)>`. Uses an `LRUCache` internally.
-   **Constructor**: `CachedFunction(std::function<Value(Key)> func, size_t max_size = 128)`
-   **`Value operator()(const Key& key)`**: Executes the function or returns a cached result.

### `make_cached<Key, Value>(...)`
Factory function to simplify `CachedFunction` creation.
```cpp
auto my_cached_func = make_cached<ArgType, ReturnType>([](ArgType arg) { /* ... */ return result; }, cache_size);
```

### `CACHED_FUNCTION(...)` Macro
Provides syntactic sugar for defining cached functions.
```cpp
CACHED_FUNCTION(cached_func_name, ArgType, ReturnType, cache_size, {
    // Function body, 'arg' is the argument
    return result;
});
```

## Usage Examples

### Basic `LRUCache` Usage

```cpp
#include "lru_cache.h"
#include <iostream>
#include <string>
#include <vector>

void on_evict_logger(const int& key, const std::string& value) {
    std::cout << "[Callback] Evicted: Key=" << key << ", Value=" << value << std::endl;
}

int main() {
    LRUCache<int, std::string> cache(3, on_evict_logger); // Max size 3

    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");
    // Cache: (3,three) (2,two) (1,one) -- MRU to LRU

    if (auto val = cache.get(1)) { // Access 1, making it MRU
        std::cout << "Get 1: " << *val << std::endl;
    }
    // Cache: (1,one) (3,three) (2,two)

    cache.put(4, "four"); // Cache full, evicts 2 ("two")
    // Cache: (4,four) (1,one) (3,three)

    std::cout << "Contains 2? " << std::boolalpha << cache.contains(2) << std::endl; // false
    std::cout << "Cache size: " << cache.size() << std::endl; // 3

    auto stats = cache.get_stats();
    std::cout << "Hits: " << stats.hits << ", Misses: " << stats.misses
              << ", Evictions: " << stats.evictions << std::endl;
}
```

### Function Caching with `make_cached`

```cpp
#include "lru_cache.h"
#include <iostream>
#include <string>
#include <thread> // For std::this_thread::sleep_for
#include <chrono> // For std::chrono::milliseconds

// Simulate an expensive computation
auto expensive_calculation = make_cached<int, std::string>(
    [](int input) -> std::string {
        std::cout << "Expensive calculation for: " << input << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return "Result(" + std::to_string(input * input) + ")";
    },
    5 // Cache size
);

int main() {
    std::cout << expensive_calculation(5) << std::endl; // Computes
    std::cout << expensive_calculation(10) << std::endl; // Computes
    std::cout << expensive_calculation(5) << std::endl;  // Cached
    std::cout << expensive_calculation(10) << std::endl; // Cached

    auto stats = expensive_calculation.cache_stats();
    std::cout << "Function Cache Hits: " << stats.hits
              << ", Misses: " << stats.misses << std::endl;
}
```

## Dependencies
- `<list>`, `<unordered_map>`, `<functional>`, `<mutex>`, `<optional>`, `<utility>`, `<stdexcept>`, `<shared_mutex>`

This LRU cache implementation and its associated function caching utilities provide powerful tools for performance optimization by reducing redundant computations or data fetching.

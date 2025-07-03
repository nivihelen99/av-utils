# `PredicateCache<T>`: Cached Predicate Evaluation

## Overview

`PredicateCache<T>` is a C++17 header-only utility designed to optimize scenarios where multiple boolean conditions (predicates) are repeatedly evaluated for the same set of objects. It achieves this by caching the result of each predicate evaluation on a per-object, per-predicate basis. This is particularly useful in systems like rule engines, data processing pipelines, or UI components with complex filtering logic, where predicate evaluations can be computationally expensive or frequently repeated.

The cache ensures that a given predicate for a specific object is computed only once. Subsequent requests for that predicate's result on the same object will retrieve the cached value, avoiding redundant computation. The cache also allows for explicit invalidation of results (for a single object or globally) and manual priming of results.

## Features

*   **Lazy Evaluation & Caching**: Predicates are only evaluated if their result isn't already cached. The result is then stored for future use.
*   **Multiple Predicates**: Manages results for multiple, independent predicates per object.
*   **Stable Predicate IDs**: Predicates are registered and assigned a `PredicateId` (a `size_t`), which is used for all operations.
*   **Object-Specific Invalidation**: Cached results for a particular object can be cleared, forcing re-evaluation on the next access.
*   **Global Invalidation**: All cached results for all objects can be cleared.
*   **Object Removal**: Objects can be entirely removed from the cache.
*   **Manual Priming**: Cache entries can be set manually without invoking the predicate function.
*   **Cache Querying**: Check if a result is cached (`get_if`) and get the cache size (`size`).
*   **Header-Only**: Easy to integrate by just including `predicate_cache.h`.
*   **C++17**: Uses modern C++ features like `std::optional`.
*   **Thread-Compatible**: Safe to use in multi-threaded environments provided that access to a `PredicateCache` instance is externally synchronized (i.e., operations on the same instance from different threads must be protected by a mutex or similar mechanism). It is not internally thread-safe for concurrent modifications.

## API

```cpp
#include "predicate_cache.h" // Or full path from include directory
#include <functional>
#include <optional>

// Alias for predicate identifiers
using PredicateId = size_t;

template<typename T>
class PredicateCache {
public:
    // Type for predicate functions
    using Predicate = std::function<bool(const T&)>;

    // Registers a predicate and returns its ID.
    PredicateId register_predicate(Predicate p);

    // Evaluates predicate 'id' for 'obj'. Returns cached result or computes, caches, and returns.
    // Throws std::out_of_range if 'id' is invalid.
    bool evaluate(const T& obj, PredicateId id);

    // Returns cached result for 'obj' and 'id' if it exists, else std::nullopt.
    // Throws std::out_of_range if 'id' is invalid.
    std::optional<bool> get_if(const T& obj, PredicateId id) const;

    // Manually sets the cached result for 'obj' and 'id'.
    // Throws std::out_of_range if 'id' is invalid.
    void prime(const T& obj, PredicateId id, bool result);

    // Clears all cached predicate results for 'obj'.
    void invalidate(const T& obj);

    // Clears all cached results for all objects.
    void invalidate_all();

    // Removes 'obj' and all its cached results entirely from the cache.
    void remove(const T& obj);

    // Returns the number of objects currently being tracked in the cache.
    size_t size() const;
};
```

### Requirements for Type `T`

For `PredicateCache<T>` to function correctly, the type `T` must satisfy the following:
1.  Be **Hashable**: `std::hash<T>` must be specialized for `T`.
2.  Be **Equality Comparable**: `operator==` must be defined for `T`.
    These are requirements for using `T` as a key in `std::unordered_map`.

## Usage Example

```cpp
#include "predicate_cache.h" // Adjust path as needed
#include <iostream>
#include <string>
#include <vector>

struct MyData {
    int id;
    std::string name;
    int value;

    bool operator==(const MyData& other) const { return id == other.id; }
};

namespace std {
    template<> struct hash<MyData> {
        size_t operator()(const MyData& d) const { return std::hash<int>()(d.id); }
    };
}

int main() {
    PredicateCache<MyData> cache;

    // Register predicates
    PredicateId is_valuable_id = cache.register_predicate(
        [](const MyData& d) {
            std::cout << "  (Checking if valuable: ID " << d.id << ")" << std::endl;
            return d.value > 100;
        }
    );
    PredicateId has_short_name_id = cache.register_predicate(
        [](const MyData& d) {
            std::cout << "  (Checking name length: ID " << d.id << ")" << std::endl;
            return d.name.length() < 5;
        }
    );

    std::vector<MyData> items = {{1, "Pen", 5}, {2, "Book", 150}, {3, "Laptop", 1200}, {4, "Mug", 20}};

    std::cout << "Processing items:\n";
    for (const auto& item : items) {
        std::cout << "Item ID: " << item.id << ", Name: " << item.name << std::endl;
        bool valuable = cache.evaluate(item, is_valuable_id);
        bool short_name = cache.evaluate(item, has_short_name_id);
        std::cout << "  Is valuable? " << (valuable ? "Yes" : "No") << std::endl;
        std::cout << "  Has short name? " << (short_name ? "Yes" : "No") << std::endl;
    }

    std::cout << "\nReprocessing item ID 2 (Book, Value 150):\n";
    // Predicates for item 2 should now be cached
    bool valuable_item2 = cache.evaluate(items[1], is_valuable_id); // Should not print "(Checking...)"
    bool short_name_item2 = cache.evaluate(items[1], has_short_name_id); // Should not print "(Checking...)"
    std::cout << "  Is valuable? " << (valuable_item2 ? "Yes" : "No") << std::endl;
    std::cout << "  Has short name? " << (short_name_item2 ? "Yes" : "No") << std::endl;

    // Invalidate item 2
    std::cout << "\nInvalidating item ID 2 and reprocessing:\n";
    cache.invalidate(items[1]);
    valuable_item2 = cache.evaluate(items[1], is_valuable_id); // Will re-evaluate
    std::cout << "  Is valuable after invalidate? " << (valuable_item2 ? "Yes" : "No") << std::endl;

    return 0;
}
```

## Design Considerations & Tradeoffs

*   **Memory Usage**: The cache stores a `std::vector<std::optional<bool>>` for each unique object `T` encountered. The size of this vector is determined by the highest `PredicateId` evaluated or primed for that object. If there are many predicates and many objects, memory usage can grow. The `std::optional<bool>` uses more memory than a raw `bool` but is necessary to distinguish between a cached `false` and a "not yet cached" state.
*   **`PredicateId` Management**: `PredicateId`s are simply `size_t` indices into an internal vector of predicates. This is efficient but means IDs are only stable for the lifetime of the `PredicateCache` instance. They are not globally unique or persistent.
*   **`std::function` Overhead**: Using `std::function` provides flexibility but can have a slight performance overhead compared to raw function pointers or templates. For most use cases involving potentially expensive predicates, this overhead is negligible.
*   **No Eviction Policy**: This implementation does not include LRU (Least Recently Used) or time-based eviction policies. The cache grows indefinitely unless items are explicitly removed or invalidated. For very long-running processes or extremely large datasets, this might need to be considered.
*   **Thread Safety**: As mentioned, the class is thread-compatible. If an instance of `PredicateCache` is accessed by multiple threads, external synchronization (e.g., `std::mutex`) is required to prevent data races during write operations (`register_predicate`, `evaluate` (first call for an obj/id), `prime`, `invalidate`, `invalidate_all`, `remove`). Read operations (`get_if`, `size`, `evaluate` for already cached results) are `const` and generally safe if no concurrent writes are happening.

## Future Considerations (Not Implemented)

The current `PredicateCache` provides core functionality. Depending on specific needs, it could be extended with:
*   **Named Predicates**: Associating string names with `PredicateId`s for easier debugging or reflection.
*   **Eviction Policies**: LRU, LFU, or time-based eviction to manage cache size automatically.
*   **Concurrency**: Internal synchronization for thread-safe concurrent access (e.g., using `std::atomic` for individual cache entries or finer-grained locking). This would add complexity and potential performance overhead.
*   **Bitmap Optimizations**: For scenarios with a fixed, small number of very common predicates, a bitmap representation of results could be more memory-efficient for the inner cache structure.

This `PredicateCache` aims to provide a balance of simplicity, performance for its intended use case, and flexibility.

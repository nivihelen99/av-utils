# `FlatMap` - Sorted Vector-Based Map

## Overview

`FlatMap<Key, Value, Compare = std::less<Key>>` is a C++ container that provides a map interface with its underlying storage being a sorted `std::vector<std::pair<Key, Value>>`. This structure is designed for scenarios where read operations (lookups and iteration) are significantly more frequent than write operations (insertions and erasures).

Its primary advantages are cache-efficient lookups (due to data locality in the vector) and fast iteration, especially for small to medium-sized datasets.

## Motivation

-   **Performance in Read-Heavy Scenarios**: When a map is built once (or infrequently) and queried many times, `FlatMap` can outperform `std::map` (node-based) and `std::unordered_map` (hash table-based) for smaller datasets due to better cache performance and simpler lookup logic (binary search).
-   **Fast Iteration**: Iterating over a `FlatMap` is equivalent to iterating over a `std::vector`, which is highly efficient.
-   **Reduced Overhead**: Avoids the per-node memory overhead of `std::map` and the hash table overhead of `std::unordered_map`.

## When to Use

-   Static or rarely changing configuration maps.
-   Mapping keys to handlers/parsers where the mapping is defined at startup.
-   Small lookup tables where iteration speed is also a concern.
-   Cases where the number of elements is typically small (e.g., less than a few hundred), where the O(N) cost of insertion/erasure is acceptable compared to O(log N) lookup benefits.

## API Summary

The `FlatMap` class template requires `Key`, `Value`, and an optional `Compare` type (defaulting to `std::less<Key>`).

```cpp
template<typename Key, typename Value, typename Compare = std::less<Key>>
class FlatMap {
public:
    // Types
    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<Key, Value>;
    using key_compare = Compare;
    using reference = value_type&;
    using const_reference = const value_type&;
    using iterator = /* implementation-defined */;
    using const_iterator = /* implementation-defined */;
    using size_type = std::size_t;

    // Constructors
    FlatMap();
    explicit FlatMap(const Compare& comp);

    // Modifiers
    bool insert(const Key& key, const Value& value); // Returns true if new element inserted, false if value updated.
    Value& operator[](const Key& key);              // Inserts default Value if key absent.
    bool erase(const Key& key);                     // Returns true if element was removed.

    // Lookup
    Value* find(const Key& key);
    const Value* find(const Key& key) const;
    bool contains(const Key& key) const;
    Value& at(const Key& key);
    const Value& at(const Key& key) const;          // Throws std::out_of_range if key absent.

    // Capacity
    size_type size() const noexcept;
    bool empty() const noexcept;

    // Iterators (sorted by key)
    iterator begin() noexcept;
    const_iterator begin() const noexcept;
    iterator end() noexcept;
    const_iterator end() const noexcept;
    const_iterator cbegin() const noexcept;
    const_iterator cend() const noexcept;
};
```

## Key Operations and Complexity

| Operation        | Description                                           | Expected Complexity      |
| ---------------- | ----------------------------------------------------- | ------------------------ |
| `insert`         | Adds or replaces a key-value pair                     | O(log N) + O(N) shift    |
| `operator[]`     | Accesses or inserts (with default value)              | O(log N) + O(N) on insert|
| `erase`          | Removes key if present                                | O(log N) + O(N) shift    |
| `find`           | Returns pointer to value or `nullptr`                 | O(log N)                 |
| `contains`       | Returns true if key is present                        | O(log N)                 |
| `at`             | Returns reference to value, throws on missing key     | O(log N)                 |
| `begin`, `end`   | Iterators for sorted sequence                         | O(1)                     |
| Iteration        | Linear scan over elements                             | O(N) for full iteration  |
| `size`, `empty`  | Number of elements / check if empty                   | O(1)                     |

*(N is the number of elements in the map)*

## Usage Example

```cpp
#include "flat_map.h" // Or relevant include path
#include <iostream>
#include <string>

int main() {
    FlatMap<int, std::string> config;

    config.insert(101, "HostName");
    config.insert(202, "Port");
    config.insert(50, "RetryAttempts");

    // Access using operator[]
    config[202] = "8080"; // Updates Port
    config[300] = "Timeout"; // Inserts new

    std::cout << "Configuration for key 101: " << config.at(101) << std::endl;

    if (config.contains(500)) {
        std::cout << "Key 500 found." << std::endl;
    } else {
        std::cout << "Key 500 not found." << std::endl;
    }

    std::cout << "\nIterating through config (sorted by key):" << std::endl;
    for (const auto& entry : config) {
        std::cout << entry.first << ": " << entry.second << std::endl;
    }

    return 0;
}
```

## Building

Ensure `flat_map.h` is in your include path. If using the provided CMake setup:
1.  The `FlatMapLib` interface library is defined in the main `CMakeLists.txt`.
2.  Link against `FlatMapLib` in your target:
    ```cmake
    target_link_libraries(your_target PRIVATE FlatMapLib)
    ```

## Testing

Unit tests are provided in `tests/flat_map_test.cpp` and use the Google Test framework. They can be run via CTest if built with the provided CMake setup.

```bash
# From your build directory
ctest -R flat_map_test # Or appropriate test name if changed
```

This README provides a good starting point for users of the `FlatMap` component.
Further details on specific performance benchmarks or advanced use cases could be added as needed.

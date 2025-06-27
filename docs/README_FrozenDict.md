# FrozenDict

## Overview

`cpp_collections::FrozenDict` is a C++ template class that implements an **immutable, hashable dictionary (associative array)**. Once a `FrozenDict` is created, its contents cannot be changed. This makes it suitable for representing constant lookup tables, configurations, or for use as keys in other hash-based containers like `std::unordered_map` or `std::unordered_set`.

Internally, `FrozenDict` stores key-value pairs sorted by key, which provides efficient lookups and ordered iteration.

## Features

*   **Immutability:** After construction, a `FrozenDict` cannot be modified.
*   **Hashability:** `FrozenDict` instances can be used as keys in standard C++ unordered containers (e.g., `std::unordered_map`, `std::unordered_set`), provided that both its `Key` and `Value` types are hashable and `Key` is equality comparable.
*   **Ordered Iteration:** Iterators traverse elements in ascending order of keys.
*   **Efficient Lookups:** Key lookups are typically O(log N) due to the internal sorted vector storage.
*   **STL-like Interface:** Provides common dictionary operations like `at()`, `operator[]` (const access), `find()`, `contains()`, `size()`, `empty()`, and iterators (`begin()`, `end()`).
*   **Customizable:** Supports custom hash functions, key equality predicates, key comparison functions, and allocators via template parameters.

## Template Parameters

```cpp
template <
    typename Key,
    typename Value,
    typename HashFunc = std::hash<Key>,         // For hashing Keys (used by std::hash<FrozenDict>)
    typename KeyEqualFunc = std::equal_to<Key>, // For Key equality (used by std::hash<FrozenDict>)
    typename CompareFunc = std::less<Key>,      // For sorting Keys internally
    typename Allocator = std::allocator<std::pair<const Key, Value>>
>
class FrozenDict;
```

*   `Key`: The type of the keys. Must be comparable by `CompareFunc` and hashable by `HashFunc` if the `FrozenDict` itself is to be used as a key in an unordered container.
*   `Value`: The type of the values. Must be hashable by `std::hash<Value>` if the `FrozenDict` is to be used as a key.
*   `HashFunc`: Functor for hashing keys. Defaults to `std::hash<Key>`.
*   `KeyEqualFunc`: Functor for comparing keys for equality. Defaults to `std::equal_to<Key>`.
*   `CompareFunc`: Functor for comparing keys for ordering. Defaults to `std::less<Key>`. This determines the iteration order.
*   `Allocator`: Allocator for internal storage.

## Basic Usage

### Include Header

```cpp
#include "FrozenDict.h"
#include <string>
#include <iostream>
```

### Creating a FrozenDict

`FrozenDict` can be created from an initializer list:

```cpp
cpp_collections::FrozenDict<std::string, int> config = {
    {"timeout", 100},
    {"retries", 3},
    {"port", 8080}
};

// If the input initializer list contains duplicate keys, the "last one wins".
cpp_collections::FrozenDict<std::string, int> settings = {
    {"mode", 1},
    {"debug", 0},
    {"mode", 2} // "mode" will be 2
};
std::cout << "Mode: " << settings.at("mode") << std::endl; // Output: Mode: 2
```

Or from a range of pairs (e.g., from a `std::vector` or `std::map`):

```cpp
std::vector<std::pair<const std::string, double>> data_vec = {
    {"pi", 3.14159},
    {"e", 2.71828}
};
cpp_collections::FrozenDict<std::string, double> constants(data_vec.begin(), data_vec.end());
```

### Accessing Elements

```cpp
// Using at() (throws std::out_of_range if key not found)
std::cout << "Timeout: " << config.at("timeout") << std::endl;

// Using operator[] (const access, also throws if key not found)
std::cout << "Retries: " << config["retries"] << std::endl;

// Checking for existence
if (config.contains("port")) {
    std::cout << "Port: " << config.at("port") << std::endl;
}

// Using find()
auto it = config.find("host");
if (it != config.end()) {
    std::cout << "Host: " << it->second << std::endl;
} else {
    std::cout << "Host not found." << std::endl;
}

// Counting elements (0 or 1 for a specific key)
std::cout << "Count of 'port': " << config.count("port") << std::endl;
```

### Iteration

Elements are iterated in key-sorted order.

```cpp
std::cout << "Configuration items (sorted by key):" << std::endl;
for (const auto& pair : config) { // pair is std::pair<const Key, Value>
    std::cout << "  " << pair.first << " = " << pair.second << std::endl;
}
// Example output:
//   port = 8080
//   retries = 3
//   timeout = 100
```

### Size and Emptiness

```cpp
std::cout << "Config size: " << config.size() << std::endl;
if (!config.empty()) {
    std::cout << "Config is not empty." << std::endl;
}
```

## Using FrozenDict as a Key

To use `FrozenDict` as a key in `std::unordered_map` or `std::unordered_set`, ensure:
1.  The `Key` type of the `FrozenDict` has a `std::hash` specialization (or a custom `HashFunc` is provided to the `FrozenDict` and that hasher is accessible/usable by `std::hash<FrozenDict>`).
2.  The `Value` type of the `FrozenDict` has a `std::hash` specialization.
3.  The `Key` type of the `FrozenDict` has an appropriate equality comparison (e.g., `operator==` or `KeyEqualFunc`).

```cpp
#include <unordered_map>

// Assuming FrozenDict<std::string, int>
using MyFrozenDictKey = cpp_collections::FrozenDict<std::string, int>;

std::unordered_map<MyFrozenDictKey, std::string> metadata_map;

MyFrozenDictKey fd_key1 = {{"id", 1}, {"type", 100}};
MyFrozenDictKey fd_key2 = {{"id", 2}, {"type", 200}};
MyFrozenDictKey fd_key1_equivalent = {{"type", 100}, {"id", 1}}; // Same content

metadata_map[fd_key1] = "Metadata for item 1";
metadata_map[fd_key2] = "Metadata for item 2";

// fd_key1_equivalent should map to the same entry as fd_key1
std::cout << "Metadata for key1: " << metadata_map.at(fd_key1_equivalent) << std::endl;
// Output: Metadata for key1: Metadata for item 1

std::cout << "Map size: " << metadata_map.size() << std::endl; // Expected: 2
```

## Notes

*   The internal storage is a `std::vector` of `std::pair<const Key, Value>`, sorted by `Key`. This provides good cache locality for iteration and O(log N) lookups.
*   The choice of `CompareFunc` (defaulting to `std::less<Key>`) is crucial for the internal sorting and thus for the behavior of lookup functions and iteration order.
*   When constructing from a range or initializer list with duplicate keys, the implementation follows the "last one wins" policy: the value associated with the last occurrence of a key will be stored.
```cpp

[end of docs/README_FrozenDict.md]

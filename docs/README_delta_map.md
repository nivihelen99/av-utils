# DeltaMap

## Overview

`DeltaMap` is a C++ utility class designed to compute and represent the differences between two map-like containers (e.g., `std::map`, `std::unordered_map`). It identifies entries that have been added, removed, or changed between an "old" and a "new" version of a map. It also tracks entries that remain unchanged.

This utility is useful in scenarios where you need to understand the evolution of a key-value dataset, such as:
- Configuration management: identifying changes between two versions of a configuration.
- State synchronization: determining what updates to send or apply to reconcile two states.
- Auditing and logging: recording granular changes to data.

## Features

-   Works with `std::map` and `std::unordered_map` by default.
-   Supports custom map types that adhere to a similar interface.
-   Allows custom comparators for value types, enabling fine-grained control over what constitutes a "change".
-   Provides optimized comparison for ordered maps (`std::map`).
-   Offers methods to query added, removed, changed, and unchanged entries.
-   Includes helper methods to check the status of individual keys (`was_added`, `was_removed`, etc.).
-   Can apply a computed delta to a map to transform it to the "new" state.
-   Can invert a delta to represent the change from the "new" state back to the "old" state.
-   C++17 deduction guides for easier construction.

## Template Parameters

```cpp
template <typename K,                  // Key type
          typename V,                  // Value type
          typename MapType = std::map<K, V>, // Container type
          typename Equal = std::equal_to<V>> // Value equality comparator
class DeltaMap { ... };
```

-   `K`: The type of the keys in the map.
-   `V`: The type of the values in the map.
-   `MapType`: The underlying map container type. Defaults to `std::map<K, V>`. Can be `std::unordered_map<K, V>` or a compatible custom map type.
-   `Equal`: A binary predicate used to compare two values of type `V` for equality. Defaults to `std::equal_to<V>`, which uses `operator==` on the value type.

## Basic Usage

```cpp
#include "delta_map.h" // Or appropriate path
#include <iostream>
#include <string>
#include <map>

void print_map_entries(const std::string& title, const std::map<std::string, int>& m) {
    std::cout << title << ": ";
    for (const auto& [key, value] : m) {
        std::cout << "{\"" << key << "\": " << value << "} ";
    }
    std::cout << std::endl;
}

int main() {
    std::map<std::string, int> old_config = {
        {"timeout", 30},
        {"retries", 3},
        {"port", 8080}
    };

    std::map<std::string, int> new_config = {
        {"timeout", 60},      // Value changed
        {"retries", 3},       // Unchanged
        {"host", "127.0.0.1"},// Added
        // "port" was removed
    };

    // Compute the delta
    deltamap::DeltaMap delta(old_config, new_config);

    // Access the differences
    print_map_entries("Added", delta.added());
    print_map_entries("Removed", delta.removed());
    print_map_entries("Changed", delta.changed());
    print_map_entries("Unchanged", delta.unchanged());

    std::cout << "Total differences: " << delta.size() << std::endl;
    std::cout << "Is port removed? " << std::boolalpha << delta.was_removed("port") << std::endl;
    std::cout << "Is timeout changed? " << std::boolalpha << delta.was_changed("timeout") << std::endl;
    std::cout << "Is host added? " << std::boolalpha << delta.was_added("host") << std::endl;

    // Applying the delta
    std::map<std::string, int> reconstructed_new_config = delta.apply_to(old_config);
    // reconstructed_new_config will be equal to new_config

    return 0;
}
```

## Public Methods

-   `DeltaMap(const MapType& old_map, const MapType& new_map, Equal equal = Equal{})`: Constructor that computes the difference between `old_map` and `new_map`. An optional `equal` comparator can be provided for values.
-   `const MapType& added() const noexcept`: Returns a map containing entries present only in `new_map`.
-   `const MapType& removed() const noexcept`: Returns a map containing entries present only in `old_map`.
-   `const MapType& changed() const noexcept`: Returns a map containing entries present in both maps but with different values (according to `Equal`). Values are from `new_map`.
-   `const MapType& unchanged() const noexcept`: Returns a map containing entries present in both maps with identical values (according to `Equal`). Values are from `new_map`.
-   `bool empty() const noexcept`: Returns `true` if there are no added, removed, or changed entries (i.e., `old_map` and `new_map` are equivalent).
-   `std::size_t size() const noexcept`: Returns the total number of differences (sum of sizes of added, removed, and changed maps).
-   `bool was_added(const K& key) const`: Checks if a specific key was added.
-   `bool was_removed(const K& key) const`: Checks if a specific key was removed.
-   `bool was_changed(const K& key) const`: Checks if a specific key was changed.
-   `bool was_unchanged(const K& key) const`: Checks if a specific key was unchanged.
-   `DeltaMap invert(const MapType& old_map, const MapType& new_map) const`: Returns a new `DeltaMap` representing the inverse delta (i.e., `DeltaMap(new_map, old_map)`).
-   `MapType apply_to(MapType base_map) const`: Applies the computed delta (added, removed, changed entries) to `base_map` and returns the resulting map. This effectively transforms `base_map` (assumed to be in the "old" state) to the "new" state.

## Custom Comparators

You can provide a custom comparison function (or functor) as the `Equal` template parameter if `operator==` is not suitable for your value type, or if you want to define equality based on specific members of a struct/class.

Example:
```cpp
struct MyData {
    std::string id;
    int version;
    std::string metadata; // We want to ignore metadata for diffing
};

auto my_data_comparator = [](const MyData& a, const MyData& b) {
    return a.id == b.id && a.version == b.version;
};

std::map<std::string, MyData> old_data_map;
std::map<std::string, MyData> new_data_map;
// ... populate maps ...

deltamap::DeltaMap<std::string, MyData, std::map<std::string, MyData>, decltype(my_data_comparator)>
    data_delta(old_data_map, new_data_map, my_data_comparator);
```

## Supported Map Types

The `DeltaMap` is designed to work with standard map types:
-   `std::map<K, V>`: Uses an optimized path leveraging key order.
-   `std::unordered_map<K, V>`: Uses a general path based on key lookups.

It can also work with other map-like containers that provide:
-   `key_type`, `mapped_type`, `value_type` typedefs.
-   An iterator interface (`begin()`, `end()`).
-   `find()` method (for unordered types or if the optimized path is not used).
-   `emplace()` or `operator[]` for insertion.
-   `erase(key)` method.
```

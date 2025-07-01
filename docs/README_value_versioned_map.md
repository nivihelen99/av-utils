# ValueVersionedMap

## Overview

`ValueVersionedMap` is a C++ data structure that stores multiple versions of values associated with keys. Each value is timestamped or versioned using a user-defined version type (defaulting to `uint64_t`). This allows for querying the state of a key's value as it was at a specific version, or retrieving its most current value.

It's particularly useful for scenarios where you need to track changes to data over time, such as:
- Configuration management: Storing historical changes to configuration parameters.
- Auditing: Keeping a record of how data entities evolved.
- Temporal data analysis: Querying data based on specific points in time or version numbers.

The internal implementation uses `std::unordered_map` for keys and `std::map` for storing versions associated with each key. This ensures that versions are sorted, allowing for efficient retrieval of values based on version queries.

## Template Parameters

```cpp
template <
    typename Key,
    typename Value,
    typename VersionT = uint64_t, // Type for versioning (e.g., uint64_t, timestamp struct)
    typename Hash = std::hash<Key>, // Hasher for Key in unordered_map
    typename KeyEqual = std::equal_to<Key>, // Equality for Key in unordered_map
    typename CompareVersions = std::less<VersionT>, // Comparator for VersionT in std::map
    typename Allocator = std::allocator<std::pair<const Key, std::map<VersionT, Value, CompareVersions>>>
>
class ValueVersionedMap;
```

-   `Key`: The type of keys.
-   `Value`: The type of values.
-   `VersionT`: The type used for versioning. Must be comparable (default: `uint64_t`).
-   `Hash`: Hash function for `Key`.
-   `KeyEqual`: Equality comparison for `Key`.
-   `CompareVersions`: Comparison function for `VersionT`.
-   `Allocator`: Allocator for the underlying `std::unordered_map`.

## Core API

### Construction

```cpp
// Default constructor
ValueVersionedMap<Key, Value, VersionT> vvm;

// With custom comparators, hashers, allocator (example)
MyVersionComparator version_comp;
MyKeyHasher key_hasher;
MyKeyEqual key_eq_fn;
MyAllocator alloc;
ValueVersionedMap<Key, Value, VersionT, MyKeyHasher, MyKeyEqual, MyVersionComparator, MyAllocator> vvm_custom(version_comp, key_hasher, key_eq_fn, alloc);
```

### Modifiers

-   `void put(const Key& key, const Value& value, const VersionT& version)`
-   `void put(const Key& key, Value&& value, const VersionT& version)`
-   `void put(Key&& key, const Value& value, const VersionT& version)`
-   `void put(Key&& key, Value&& value, const VersionT& version)`
    Adds or updates a value for a key at a specific version. If the key-version pair already exists, its value is overwritten.

### Lookup

-   `std::optional<std::reference_wrapper<const Value>> get(const Key& key, const VersionT& version) const`
-   `std::optional<std::reference_wrapper<Value>> get(const Key& key, const VersionT& version)`
    Retrieves the value for `key` that was active at `version`. This means it finds the entry with the largest version number less than or equal to the specified `version`. Returns `std::nullopt` if no such version exists or the key is not found.

-   `std::optional<std::reference_wrapper<const Value>> get_exact(const Key& key, const VersionT& version) const`
-   `std::optional<std::reference_wrapper<Value>> get_exact(const Key& key, const VersionT& version)`
    Retrieves the value for `key` only if an entry exists for that exact `version`. Returns `std::nullopt` otherwise.

-   `std::optional<std::reference_wrapper<const Value>> get_latest(const Key& key) const`
-   `std::optional<std::reference_wrapper<Value>> get_latest(const Key& key)`
    Retrieves the most recent version of the value for `key`. Returns `std::nullopt` if the key is not found or has no versions.

-   `std::optional<std::reference_wrapper<const std::map<VersionT, Value, CompareVersions>>> get_all_versions(const Key& key) const`
-   `std::optional<std::reference_wrapper<std::map<VersionT, Value, CompareVersions>>> get_all_versions(const Key& key)`
    Returns a reference to the map containing all (version, value) pairs for the given `key`. Returns `std::nullopt` if the key is not found.

### Removal

-   `bool remove_key(const Key& key)`
    Removes `key` and all its associated versions from the map. Returns `true` if the key was found and removed, `false` otherwise.

-   `bool remove_version(const Key& key, const VersionT& version)`
    Removes a specific `version` of a value for `key`. If this is the last version for the key, the key itself is also removed from the map. Returns `true` if the version was found and removed, `false` otherwise.

### Capacity

-   `bool empty() const noexcept`
    Checks if the map contains any keys.
-   `size_type size() const noexcept`
    Returns the number of unique keys in the map.
-   `size_type total_versions() const noexcept`
    Returns the total number of (version, value) pairs across all keys.
-   `void clear() noexcept`
    Removes all keys and versions from the map.

### Existence Checks

-   `bool contains_key(const Key& key) const`
    Checks if the map contains any versions for the given `key`.
-   `bool contains_version(const Key& key, const VersionT& version) const`
    Checks if the map contains an entry for the exact `key` and `version`.

### Utility

-   `std::vector<Key> keys() const`
    Returns a `std::vector` containing all unique keys in the map. The order is not guaranteed.
-   `std::optional<std::vector<VersionT>> versions(const Key& key) const`
    Returns a `std::vector` of all version numbers for a given `key`, sorted by version. Returns `std::nullopt` if the key is not found.

### Iterators

The map provides standard iterators (`begin`, `end`, `cbegin`, `cend`) that iterate over pairs of `const Key` and `std::map<VersionT, Value, CompareVersions>`. This allows access to all versions for each key during iteration.

```cpp
cpp_collections::ValueVersionedMap<std::string, int> myMap;
// ... populate map ...

for (const auto& key_data_pair : myMap) {
    const std::string& current_key = key_data_pair.first;
    const auto& version_value_map_for_key = key_data_pair.second;

    std::cout << "Key: " << current_key << std::endl;
    for (const auto& version_value_pair : version_value_map_for_key) {
        std::cout << "  Version: " << version_value_pair.first
                  << ", Value: " << version_value_pair.second << std::endl;
    }
}
```

### Swap

-   `void swap(ValueVersionedMap& other) noexcept(...)`
    Member function to swap contents with another `ValueVersionedMap`.
-   `void swap(ValueVersionedMap& lhs, ValueVersionedMap& rhs) noexcept(...)`
    Non-member `std::swap` overload.

### Comparison

-   `bool operator==(const ValueVersionedMap& lhs, const ValueVersionedMap& rhs)`
-   `bool operator!=(const ValueVersionedMap& lhs, const ValueVersionedMap& rhs)`
    Compares two `ValueVersionedMap` instances. They are equal if they have the same set of keys, and for each key, the associated map of (version, value) pairs is identical.

## Example Usage

```cpp
#include "value_versioned_map.h"
#include <iostream>
#include <string>

int main() {
    cpp_collections::ValueVersionedMap<std::string, std::string> config;

    // Version 1: Initial database URL
    config.put("db_url", "postgres://server1/prod", 1);
    // Version 2: Update credentials
    config.put("db_url", "postgres://user:pass@server1/prod", 2);
    // Version 3: Failover to another server
    config.put("db_url", "postgres://user:pass@server2_backup/prod", 3);

    config.put("max_pool_size", "10", 1);
    config.put("max_pool_size", "20", 2); // Increased pool size at version 2

    // Get latest db_url
    if (auto latest_db = config.get_latest("db_url")) {
        std::cout << "Latest DB URL: " << latest_db.value().get() << std::endl; // postgres://user:pass@server2_backup/prod
    }

    // Get db_url as it was at version 2
    if (auto db_v2 = config.get("db_url", 2)) {
        std::cout << "DB URL at version 2: " << db_v2.value().get() << std::endl; // postgres://user:pass@server1/prod
    }

    // Get db_url as it was at version 1
    if (auto db_v1 = config.get("db_url", 1)) {
        std::cout << "DB URL at version 1: " << db_v1.value().get() << std::endl; // postgres://server1/prod
    }

    // Get max_pool_size as it was at version 1
    if (auto pool_v1 = config.get("max_pool_size", 1)) {
        std::cout << "Max Pool Size at version 1: " << pool_v1.value().get() << std::endl; // 10
    }

    // Get max_pool_size as it was at version 3 (should be latest available, which is from version 2)
    if (auto pool_v3 = config.get("max_pool_size", 3)) {
        std::cout << "Max Pool Size at version 3: " << pool_v3.value().get() << std::endl; // 20
    }

    // Get a non-existent key
    if (!config.get_latest("timeout").has_value()) {
        std::cout << "Timeout key not found." << std::endl;
    }

    return 0;
}
```

This documentation provides a guide to understanding and using the `ValueVersionedMap` data structure.
It is header-only and can be included via `#include "value_versioned_map.h"`.
Ensure your compiler supports C++17 or later.

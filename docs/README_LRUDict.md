# LRUDict - Least Recently Used Dictionary

## Overview

`LRUDict<Key, Value, Hash, KeyEqual, Allocator>` is a dictionary-like data structure that stores a fixed number of key-value pairs. When the dictionary reaches its capacity and a new item is inserted, the least recently used (LRU) item is automatically evicted to make space. This behavior makes it suitable for use as a cache.

Accessing an item (e.g., via `at()`, `get()`, or `operator[]`) marks it as the most recently used (MRU). Iteration occurs from the most recently used item to the least recently used item.

## Template Parameters

*   `Key`: The type of the keys.
*   `Value`: The type of the values.
*   `Hash`: A hash function for `Key` (defaults to `std::hash<Key>`).
*   `KeyEqual`: A function to compare keys for equality (defaults to `std::equal_to<Key>`).
*   `Allocator`: An allocator for the underlying storage (defaults to `std::allocator<std::pair<const Key, Value>>`).

## Key Features

*   **Fixed Capacity:** Stores up to a specified maximum number of items.
*   **LRU Eviction:** Automatically removes the least recently used item when capacity is exceeded.
*   **Efficient Lookups:** Average O(1) time complexity for insertions, deletions, and lookups, thanks to an underlying hash map.
*   **Access Updates Recency:** Operations like `at()`, `get()`, and `operator[]` update the accessed item to be the most recently used.
*   **Iteration Order:** Iterators traverse items from most recently used (MRU) to least recently used (LRU).

## Core API

### Constructors
*   `LRUDict(size_type capacity, ...)`: Constructs an LRU cache with the given capacity.

### Modifiers
*   `insert(const value_type& kv_pair)`: Inserts a key-value pair. If the key exists, its value is updated. Returns `std::pair<iterator, bool>`.
*   `insert_or_assign(const Key& k, M&& v)`: Inserts if key `k` doesn't exist, otherwise assigns `v` to it. Returns `std::pair<iterator, bool>`.
*   `try_emplace(const Key& k, Args&&... args)`: Inserts a new element constructed in-place if key `k` doesn't exist. Returns `std::pair<iterator, bool>`.
*   `operator[](const Key& key)`: Accesses or inserts an element. If key doesn't exist and capacity allows, default-constructs value.
*   `erase(const Key& key)`: Removes an element by key. Returns `true` if an element was removed.
*   `erase(iterator pos)`: Removes the element at the iterator position.
*   `clear()`: Removes all elements.

### Lookup (updates LRU order)
*   `at(const Key& key)`: Returns a reference to the value. Throws `std::out_of_range` if key not found.
*   `get(const Key& key)`: Returns `std::optional<std::reference_wrapper<Value>>`.

### Lookup (does NOT update LRU order)
*   `at(const Key& key) const`: Const version of `at`. Does not update LRU order.
*   `get(const Key& key) const`: Const version of `get`. Does not update LRU order.
*   `peek(const Key& key) const`: Returns `std::optional<std::reference_wrapper<const Value>>` without updating LRU order.
*   `contains(const Key& key) const`: Checks if a key exists.

### Capacity
*   `capacity() const`: Returns the maximum number of items the cache can hold.
*   `size() const`: Returns the current number of items in the cache.
*   `empty() const`: Checks if the cache is empty.
*   `is_full() const`: Checks if the cache is at full capacity.

### Iterators
*   `begin()`, `end()`: Provide iterators to traverse elements from MRU to LRU.
*   `cbegin()`, `cend()`: Const versions of iterators.

## Usage Example

```cpp
#include "LRUDict.h" // Adjust path as necessary
#include <iostream>
#include <string>

int main() {
    // Create an LRU cache with capacity 3
    cpp_collections::LRUDict<int, std::string> cache(3);

    cache.insert({1, "apple"});    // {1:apple}
    cache.insert({2, "banana"});   // {2:banana}, {1:apple}
    cache.insert({3, "cherry"});   // {3:cherry}, {2:banana}, {1:apple}

    std::cout << "Item with key 2: " << cache.at(2) << std::endl;
    // Accessing 2 makes it MRU. Order: {2:banana}, {3:cherry}, {1:apple}

    cache.insert({4, "date"});
    // Cache is full, inserting 4 evicts 1 (LRU).
    // Order: {4:date}, {2:banana}, {3:cherry}

    std::cout << "Cache contents (MRU to LRU):" << std::endl;
    for (const auto& pair : cache) {
        std::cout << "{" << pair.first << ": " << pair.second << "} ";
    }
    std::cout << std::endl;
    // Output: {4:date} {2:banana} {3:cherry}

    if (!cache.contains(1)) {
        std::cout << "Key 1 was successfully evicted." << std::endl;
    }

    return 0;
}
```

## Time Complexity

*   **Insertion (`insert`, `operator[]`, `try_emplace`, `insert_or_assign`):** Average O(1). Worst case O(N) if hash collisions occur or eviction happens (which is O(1) for list removal but map removal can be worst case O(N) for bad hash).
*   **Deletion (`erase`):** Average O(1). Worst case O(N).
*   **Lookup (`at`, `get`, `peek`, `contains`):** Average O(1). Worst case O(N).
*   **Capacity/Size/Empty:** O(1).

The underlying `std::list` operations (splice, push_front, pop_back) are O(1). The `std::unordered_map` operations are average O(1).

## Notes

*   A capacity of 0 means the `LRUDict` can never hold any items. Insertions will generally fail or have no effect, and `operator[]` might throw if it tries to insert.
*   The const versions of lookup methods (`at() const`, `get() const`) and `peek()` do **not** update the LRU status of the accessed element, preserving const-correctness. Non-const lookup methods do update the LRU status.
*   Iterators are bidirectional and iterate from the Most Recently Used (MRU) item to the Least Recently Used (LRU) item. Modifying the `LRUDict` (e.g., by insertion or erasure) may invalidate iterators, similar to `std::unordered_map`. Access operations that update LRU order (like non-const `at()` or `get()`) also modify the internal list order and can be considered modifying operations in terms of iterator stability for other iterators.
```

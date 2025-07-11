# `InsertionOrderedMap<Key, Value, Hasher, KeyEqual, Allocator>`

## Overview

`cpp_collections::InsertionOrderedMap` is a C++ associative container that combines features of `std::unordered_map` and `std::list`. It stores key-value pairs like a map, providing average O(1) time complexity for lookups, insertions, and erasures. Crucially, it also preserves the order in which elements are inserted, allowing iteration over elements in their original insertion sequence.

This data structure is useful in scenarios where both fast key-based access and maintenance of insertion order are required. Examples include:
- Implementing caches where eviction strategies might depend on insertion order (e.g., FIFO, or as a component in LRU/LFU).
- Processing configuration files or data formats where the order of items is significant.
- Building representations of data where iteration in the original order is necessary (e.g., preserving field order from a JSON object).

## Template Parameters

-   `Key`: The type of the keys. Must be Hashable by `Hasher` and comparable by `KeyEqual`.
-   `Value`: The type of the mapped values.
-   `Hasher`: A unary function object type that takes an object of type `Key` and returns a `std::size_t`. Defaults to `std::hash<Key>`.
-   `KeyEqual`: A binary predicate type that takes two arguments of type `Key` and returns a `bool`. Defaults to `std::equal_to<Key>`.
-   `Allocator`: The allocator type to be used for memory management of the underlying `std::list` and `std::unordered_map` nodes. Defaults to `std::allocator<std::pair<const Key, Value>>`.

## Core Features

-   **Insertion Order Preservation**: Iterators traverse elements in the order they were inserted.
-   **Fast Lookups**: Average O(1) time complexity for `find`, `operator[]`, `at`.
-   **Efficient Insertions/Erasures**: Average O(1) time complexity for `insert`, `emplace`, `erase(key)`. Erasing by iterator is also O(1).
-   **Standard Map Interface**: Provides common map operations like `insert`, `erase`, `at`, `operator[]`, `find`, `contains`, `clear`, `size`, `empty`.
-   **Bidirectional Iterators**: Supports forward and reverse iteration.
-   **Special Operations**:
    -   `to_front(key)`: Moves an existing element to the beginning of the insertion order.
    -   `to_back(key)`: Moves an existing element to the end of the insertion order.
    -   `pop_front()`: Removes and returns the first (oldest) inserted element.
    -   `pop_back()`: Removes and returns the last (newest) inserted element.

## Performance Characteristics

-   **Space Complexity**: O(N) where N is the number of elements. It stores each element in a list node and an entry in a hash map.
-   **Time Complexity**:
    -   `insert`, `emplace`, `operator[]` (insertion): Average O(1). Worst case O(N) due to hash collisions or list reallocation (though `std::list` node insertions are O(1)).
    -   `erase(key)`: Average O(1). Worst case O(N) for map lookup.
    -   `erase(iterator)`: Amortized O(1).
    -   `find`, `at`, `operator[]` (access), `contains`: Average O(1). Worst case O(N).
    -   `clear`: O(N).
    -   `size`, `empty`: O(1).
    -   Iteration: O(N) to iterate through all elements.
    -   `to_front`, `to_back`: O(1) after finding the element (average O(1) total).
    -   `pop_front`, `pop_back`: O(1).

## Basic Usage

```cpp
#include "InsertionOrderedMap.h" // Assuming it's in the include path
#include <iostream>
#include <string>

int main() {
    cpp_collections::InsertionOrderedMap<std::string, int> settings;

    // Insert elements
    settings.insert({"width", 1920});
    settings.insert({"height", 1080});
    settings["dpi"] = 96; // Using operator[]

    // Iterate in insertion order
    std::cout << "Settings (in order of insertion):\n";
    for (const auto& pair : settings) {
        std::cout << "  " << pair.first << ": " << pair.second << std::endl;
    }
    // Output:
    //   width: 1920
    //   height: 1080
    //   dpi: 96

    // Access and modify
    if (settings.contains("height")) {
        settings.at("height") = 1200;
    }
    std::cout << "Height updated to: " << settings["height"] << std::endl;

    // Move "width" to be the most recently accessed/inserted (end of order)
    settings.to_back("width");

    std::cout << "\nSettings after to_back(\"width\"):\n";
    for (const auto& [key, value] : settings) { // Structured binding
        std::cout << "  " << key << ": " << value << std::endl;
    }
    // Output (order changed):
    //   height: 1200
    //   dpi: 96
    //   width: 1920

    // Remove an element
    settings.erase("dpi");

    // Pop the oldest element
    auto oldest = settings.pop_front();
    if (oldest) {
        std::cout << "\nPopped oldest: {" << oldest->first << ": " << oldest->second << "}\n";
        // Output: Popped oldest: {height: 1200}
    }

    std::cout << "\nFinal settings:\n";
    for (const auto& [key, value] : settings) {
        std::cout << "  " << key << ": " << value << std::endl;
    }
    // Output:
    //   width: 1920

    return 0;
}

```

## Member Functions

A brief overview of key member functions. For detailed signatures, refer to the header file.

### Constructors
-   `InsertionOrderedMap()`
-   `explicit InsertionOrderedMap(const Allocator& alloc)`
-   `InsertionOrderedMap(std::initializer_list<value_type> ilist, ...)`
-   `template <typename InputIt> InsertionOrderedMap(InputIt first, InputIt last, ...)`
-   Copy/Move constructors and assignment operators.

### Iterators
-   `begin()`, `end()`: Returns iterators to the beginning and end for ordered traversal.
-   `cbegin()`, `cend()`: Const versions.
-   `rbegin()`, `rend()`: Returns reverse iterators.
-   `crbegin()`, `crend()`: Const versions.

### Capacity
-   `empty()`: Checks if the container is empty (O(1)).
-   `size()`: Returns the number of elements (O(1)).
-   `max_size()`: Returns the maximum possible number of elements.

### Element Access
-   `operator[](const Key& key)` / `operator[](Key&& key)`: Accesses or inserts an element.
-   `at(const Key& key)`: Accesses an element, throws `std::out_of_range` if key not found.

### Modifiers
-   `insert(const value_type& value)` / `insert(value_type&& value)`: Inserts an element if the key is unique. Returns `std::pair<iterator, bool>`.
-   `emplace(Args&&... args)`: Constructs an element in-place if the key is unique. Returns `std::pair<iterator, bool>`.
-   `insert_or_assign(const Key& k, M&& obj)` / `insert_or_assign(Key&& k, M&& obj)`: Inserts if key doesn't exist, otherwise assigns to existing key.
-   `erase(iterator pos)` / `erase(const_iterator pos)`: Erases the element at `pos`.
-   `erase(const Key& key)`: Erases the element with the given `key`.
-   `clear()`: Clears all elements.
-   `swap(InsertionOrderedMap& other)`: Swaps content with another map.
-   `to_front(const Key& key)`: Moves the element with `key` to the beginning of the insertion order.
-   `to_back(const Key& key)`: Moves the element with `key` to the end of the insertion order.
-   `pop_front()`: Removes and returns the first (oldest) element as `std::optional<value_type>`.
-   `pop_back()`: Removes and returns the last (newest) element as `std::optional<value_type>`.

### Lookup
-   `find(const Key& key)`: Returns an iterator to the element with `key`, or `end()` if not found.
-   `contains(const Key& key) const`: Checks if an element with `key` exists.

### Observers
-   `hash_function()`: Returns the hash function object.
-   `key_eq()`: Returns the key equality predicate.
-   `get_allocator()`: Returns the allocator.

## Notes
-   The iterators are bidirectional.
-   Iterator invalidation rules are similar to `std::list` for operations that modify the list structure (e.g., `erase`, `pop_front`, `pop_back`, `to_front`, `to_back`, `clear`). Iterators to erased elements are invalidated. References and pointers to elements are invalidated when the element is erased. Insertion does not invalidate iterators or references unless it's an insertion into an empty map, or if `std::list` reallocates (which it generally doesn't per-element). `std::unordered_map` iterator invalidation rules also apply for its internal state, but `InsertionOrderedMap` iterators primarily reflect the list.
-   If an operation modifies an element's position in the insertion order (e.g., `to_front`, `to_back`), iterators pointing to that element remain valid but will iterate from the new position.
-   The `Key` type must be hashable and equality-comparable.
-   The `Value` type must meet the requirements of the operations performed on it (e.g., copy/move constructibility/assignability).

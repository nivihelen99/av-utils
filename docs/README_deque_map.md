# DequeMap

## Overview

`std_ext::DequeMap` is a C++ data structure that combines the features of a double-ended queue (`std::deque`-like interface) with an associative map (`std::unordered_map`-like key access). It maintains elements in a specific order based on insertion (with options to add to front or back) while providing fast key-based lookups, insertions, and deletions.

It is implemented using a `std::list` to store the ordered key-value pairs and an `std::unordered_map` to map keys to their corresponding iterators in the list. This design ensures iterator stability for map operations and efficient additions/removals from both ends of the sequence.

## Features

*   **Ordered Key-Value Storage:** Elements are stored in an order determined by `push_front`, `push_back`, and `operator[]` (for new keys, which appends).
*   **Efficient Key-Based Access:** Average O(1) time complexity for accessing, inserting (if key is unique), and erasing elements by key.
*   **Efficient Push/Pop from Both Ends:** Amortized O(1) time complexity for adding elements to or removing elements from both the front and the back of the sequence.
*   **Standard Map-like Interface:** Provides common map operations like `operator[]`, `at()`, `find()`, `contains()`, `insert()`, `emplace()`, `erase()`.
*   **Standard Deque-like Interface:** Provides common deque operations like `push_front()`, `pop_front()`, `front()`, `push_back()`, `pop_back()`, `back()`.
*   **Bidirectional Iterators:** Supports forward and reverse iteration over the elements in their stored order.
*   **Allocator Support:** Customizable memory allocation.

## When to Use DequeMap

`DequeMap` is particularly useful in scenarios where you need:

*   **Ordered Caches:** Implementing LRU (Least Recently Used) or MRU (Most Recently Used) caches where quick key lookup, reordering (e.g., move to front/back on access), and efficient eviction from either end are required.
*   **Task Queues with Fast Lookup:** Managing a queue of tasks (e.g., by unique ID) where order is important, but you also need to quickly check for a task's existence or update its properties.
*   **History/Log Management:** Storing an ordered sequence of unique items (events, messages) that require fast retrieval by an identifier and efficient addition/pruning from ends.
*   **Maintaining Insertion Order with Deque Operations:** Similar to an `OrderedDict` but with a more explicit and potentially more intuitive API for deque-like operations (`push_front`, `pop_front`).

## API Summary

### Type Aliases
*   `key_type`
*   `mapped_type`
*   `value_type` (std::pair<const Key, Value>)
*   `iterator`
*   `const_iterator`
*   `reverse_iterator`
*   `const_reverse_iterator`
*   `size_type`, `allocator_type`, etc.

### Constructors
*   Default constructor.
*   Constructor with initial bucket count for the internal map.
*   Allocator-aware constructors.
*   Initializer list constructor: `DequeMap<K,V> dm = {{"key1", val1}, {"key2", val2}};`
*   Range constructor: `DequeMap<K,V> dm(first, last);`

### Deque-like Operations
*   `push_front(key, value)` / `emplace_front(key, args...)`: Adds an element to the front. Returns `std::pair<iterator, bool>`. Does not insert if key exists.
*   `push_back(key, value)` / `emplace_back(key, args...)`: Adds an element to the back. Returns `std::pair<iterator, bool>`. Does not insert if key exists.
*   `pop_front()`: Removes and returns the first element. Throws `std::out_of_range` if empty.
*   `pop_back()`: Removes and returns the last element. Throws `std::out_of_range` if empty.
*   `front()`: Returns a reference to the first element (const and non-const). Throws `std::out_of_range` if empty.
*   `back()`: Returns a reference to the last element (const and non-const). Throws `std::out_of_range` if empty.

### Map-like Operations
*   `operator[](key)`: Accesses element. If key doesn't exist, inserts a new element (default-constructed value) at the back.
*   `at(key)`: Accesses element (const and non-const). Throws `std::out_of_range` if key not found.
*   `insert(value)` / `insert(hint, value)`: Inserts element at the back if key is unique. Returns `std::pair<iterator, bool>`.
*   `emplace(args...)` / `try_emplace(key, args...)` / `try_emplace(hint, key, args...)`: Constructs element in-place at the back if key is unique.
*   `erase(key)`: Removes element by key. Returns `1` if erased, `0` otherwise.
*   `erase(iterator)` / `erase(const_iterator)`: Removes element at iterator position. Returns iterator to the next element.
*   `find(key)`: Returns iterator to element or `end()` if not found (const and non-const).
*   `contains(key)`: Checks if key exists.
*   `clear()`: Removes all elements.
*   `empty()`: Checks if the container is empty.
*   `size()`: Returns the number of elements.
*   `max_size()`: Returns the maximum possible number of elements.

### Iterators
*   `begin()`, `end()`
*   `cbegin()`, `cend()`
*   `rbegin()`, `rend()`
*   `crbegin()`, `crend()`

### Capacity
*   `empty()`, `size()`, `max_size()`

### Modifiers
*   `swap(other_dequemap)`

### Allocator Support
*   `get_allocator()`

## Time Complexity

*   **Access (`at`, `operator[]`, `find`, `contains`)**: Average O(1), Worst O(N) (due to `std::unordered_map`).
*   **Insertion (`push_front`, `push_back`, `emplace_front`, `emplace_back`, `insert`, `emplace` - if key is unique)**: Average O(1).
*   **Deletion (`pop_front`, `pop_back`, `erase(key)`, `erase(iterator)`)**: Average O(1).
*   **Iteration**: O(N) for full traversal. Incrementing/decrementing iterators is O(1).
*   **`size()`**: O(1).
*   **`clear()`**: O(N).

## Usage Example

```cpp
#include "deque_map.h"
#include <iostream>
#include <string>

int main() {
    std_ext::DequeMap<std::string, int> cache;

    // Add items
    cache.push_back("item1", 100); // item1 is at the back
    cache.push_front("item0", 50); // item0 is at the front
    cache["item2"] = 200;          // item2 added to the back (if new)

    // Order: item0, item1, item2
    std::cout << "Cache content:" << std::endl;
    for (const auto& entry : cache) {
        std::cout << "  " << entry.first << ": " << entry.second << std::endl;
    }

    // Access and modify
    if (cache.contains("item1")) {
        cache.at("item1") += 5; // Modify item1
    }
    std::cout << "Item1 new value: " << cache["item1"] << std::endl;


    // Simulate LRU: access "item0", move to back (as most recently used)
    if (cache.contains("item0")) {
        auto val = cache.at("item0");
        cache.erase("item0"); // Remove from its current position
        cache.push_back("item0", val); // Add to back
        std::cout << "Moved item0 to back (simulating LRU access)." << std::endl;
    }

    std::cout << "Cache after LRU simulation:" << std::endl;
    for (const auto& entry : cache) {
        std::cout << "  " << entry.first << ": " << entry.second << std::endl;
    }

    // Pop least recently used (front)
    if (!cache.empty()) {
        auto lru = cache.pop_front();
        std::cout << "Evicted LRU: " << lru.first << " -> " << lru.second << std::endl;
    }

    std::cout << "Final cache size: " << cache.size() << std::endl;

    return 0;
}

```

This `DequeMap` provides a flexible combination of ordered sequence operations and fast key-based lookups, suitable for a variety of application needs.

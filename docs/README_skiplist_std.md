# `SkipList<T, Compare>` (Standard Allocation Version)

## Overview

The `skiplist_std.h` header provides a C++ implementation of a Skip List data structure using standard memory allocation (`new` and `delete`). Skip lists are probabilistic data structures that offer logarithmic average time complexity (O(log N)) for search, insertion, and deletion operations. They serve as an alternative to balanced trees, often with simpler implementation logic.

This version of `SkipList` shares its core algorithms and public API with the potentially pooled-allocator version found in `skiplist.h` but exclusively uses standard C++ memory management.

Like other skip list implementations, it supports:
-   Storing elements in sorted order.
-   Efficient search, insertion, and deletion.
-   Concurrent-friendly design elements like `std::atomic` pointers and Compare-And-Swap (CAS) operations for node manipulation.
-   A "finger" pointer optimization to potentially speed up localized searches.

The `SkipList` can store elements of a generic type `T`. If `T` is a `std::pair<K, V>`, the skip list behaves like an ordered map, using the `K` part for comparison and ordering.

## Template Parameters

-   `T`: The type of elements to be stored.
    -   If `T` is `std::pair<K, V>`, comparisons are based on `K`.
-   `Compare`: A binary predicate defining the strict weak ordering for keys (or elements if `T` is not a pair).
    -   Defaults to `std::less<KeyType_t<T>>`, where `KeyType_t<T>` is `T::first_type` if `T` is a `std::pair`, or `T` itself otherwise. This results in an ascending order skip list.

## Features

-   **Logarithmic Performance:** Average O(log N) for search, insert, delete, and find operations.
-   **Ordered Storage:** Elements are maintained in sorted order.
-   **Standard Memory Allocation:** Uses `new` and `delete` for node management.
-   **Concurrent-Friendly Design Aspects:** Utilizes `std::atomic` pointers and CAS operations for linking nodes. (Full thread-safety depends on external synchronization if multiple threads modify the same `SkipList` instance or if element `T` modifications are not atomic.)
-   **Finger Pointer Optimization:** A hint to potentially accelerate searches.
-   **Generic Element Type `T`:** Supports simple types or `std::pair<Key, Value>` for map-like behavior.
-   **Iterators:** Forward iterators (`begin`, `end`, `cbegin`, `cend`) for ordered traversal.
-   **Rich API:** Includes `insert`, `remove`, `search`, `find`, `insert_or_assign`, `clear`, `size`, `empty`, `kthElement`, `rangeQuery`, and bulk operations.

## Core Components (Internal)

-   **`SkipListNode<T>`**: Node structure with `value`, an array of atomic `forward` pointers, and `node_level`.
-   **Helper templates (`KeyTypeHelper`, `get_comparable_value`, `value_to_log_string`)**: Used for generic handling of element types and keys, especially when `T` is `std::pair`.

## Public Interface Highlights

(The public interface is identical to the `SkipList` described for `skiplist.h`, but without the custom memory pool aspects.)

### Constructors & Destructor
-   **`explicit SkipList(int userMaxLevel = DEFAULT_MAX_LEVEL)`**: `DEFAULT_MAX_LEVEL` is typically 16.
-   **`~SkipList()`**: Deallocates all nodes using `delete`.

### Basic Operations
-   **`bool insert(T value)`**
-   **`std::pair<iterator, bool> insert_or_assign(const T& value)`**
-   **`bool remove(T value)`**
-   **`bool search(T value) const`**
-   **`iterator find(const KeyType& key_to_find)` / `const_iterator find(const KeyType& key_to_find) const`**
-   **`void clear()`**
-   **`bool empty() const`**
-   **`int size() const`** (O(N) traversal)

### Iterators (Forward Iteration)
-   **`iterator begin() / end()`**
-   **`const_iterator cbegin() / cend()`**

### Specialized Queries & Operations
-   **`T kthElement(int k) const`** (0-indexed, O(k) worst-case)
-   **`std::vector<T> rangeQuery(T minVal, T maxVal) const`**
-   **`void insert_bulk(const std::vector<T>& values)`**
-   **`size_t remove_bulk(const std::vector<T>& values)`**

### Utility & Debugging
-   **`std::vector<T> toVector() const`**
-   **`void display() const`** (Prints multi-level structure)
-   **`void printValues() const`** (Prints values in order)

## Usage Examples

(Based on `examples/use_skip.cpp`, which includes `skiplist_std.h`)

### Skip List with Integers

```cpp
#include "skiplist_std.h" // Assuming this header
#include <iostream>
#include <vector>

int main() {
    SkipList<int> sl;

    std::vector<int> initial_values = {3, 6, 7, 9, 12, 19, 17, 26, 21, 25};
    for (int val : initial_values) {
        sl.insert(val);
    }

    std::cout << "SkipList after insertions:" << std::endl;
    sl.printValues(); // Prints elements in sorted order

    std::cout << "\nSearch for 19: " << (sl.search(19) ? "Found" : "Not Found") << std::endl;
    std::cout << "Search for 100: " << (sl.search(100) ? "Found" : "Not Found") << std::endl;

    std::cout << "\n3rd element (0-indexed): " << sl.kthElement(2) << std::endl; // Expect 7 if sorted: 3,6,7...

    std::cout << "\nElements in range [10, 20]: ";
    std::vector<int> range_res = sl.rangeQuery(10, 20); // Inclusive of 10, exclusive of 20 in typical interpretation, but check impl.
                                                      // The skip list implementation seems inclusive for maxVal in rangeQuery.
                                                      // So, [10,20] means elements x such that minVal <= x <= maxVal.
    for (int val : range_res) {
        std::cout << val << " ";
    }
    std::cout << std::endl;

    sl.remove(12);
    std::cout << "\nAfter removing 12:" << std::endl;
    sl.printValues();
}
```

### Skip List as an Ordered Map (using `std::pair`)

```cpp
#include "skiplist_std.h"
#include <iostream>
#include <string>
#include <utility> // For std::pair

// Define a type for map entries
// Key is const for map-like behavior where key doesn't change
using MapEntry = std::pair<const int, std::string>;

int main() {
    SkipList<MapEntry> ordered_map;

    ordered_map.insert({20, "Twenty"});
    ordered_map.insert({10, "Ten"});
    ordered_map.insert_or_assign({20, "Vingt"}); // Update value for key 20

    std::cout << "\nOrdered Map (std::pair) contents:" << std::endl;
    for (const auto& entry : ordered_map) {
        std::cout << "  Key: " << entry.first << ", Value: " << entry.second << std::endl;
    }
    // Expected (sorted by key):
    // Key: 10, Value: Ten
    // Key: 20, Value: Vingt

    auto it = ordered_map.find(10); // Find by key (int)
    if (it != ordered_map.end()) {
        std::cout << "Found key 10, value: " << it->second << std::endl;
    }
}
```

## Dependencies
- Standard C++ libraries: `<iostream>`, `<vector>`, `<algorithm>`, `<atomic>`, `<mutex>`, `<random>`, `<iomanip>`, `<string>`, `<memory>`, `<iterator>`, `<cstddef>`, `<utility>`, `<type_traits>`, `<functional>`, `<new>`.

This version of `SkipList` provides the same rich functionality as its counterpart but relies on standard C++ memory management, which can be simpler for projects not requiring or wanting a custom memory pool.

# `SkipList<T, Compare>`

## Overview

The `skiplist.h` header provides a C++ implementation of a Skip List data structure. Skip lists are probabilistic data structures that offer logarithmic average time complexity (O(log N)) for search, insertion, and deletion operations, making them an alternative to balanced trees (like red-black trees or AVL trees). They are often considered simpler to implement than balanced trees, especially in concurrent contexts.

This particular implementation includes features suggesting concurrent-friendliness, such as the use of `std::atomic` for forward pointers and Compare-And-Swap (CAS) loops for updates. It also features an optional custom memory pool for `SkipListNode` allocations to potentially improve performance by reducing overhead from frequent small allocations and improving locality. A "finger" pointer optimization is used to speed up searches for elements near previously accessed locations.

The `SkipList` can store elements of a generic type `T`. If `T` is a `std::pair<K, V>`, the skip list behaves like an ordered map, using the `K` part for comparison and ordering.

## Template Parameters

-   `T`: The type of elements to be stored in the skip list.
    -   If `T` is `std::pair<K, V>`, comparisons are based on `K`.
-   `Compare`: A binary predicate that defines the strict weak ordering for the keys (or elements if `T` is not a pair).
    -   Defaults to `std::less<KeyType_t<T>>`, where `KeyType_t<T>` is `T::first_type` if `T` is a `std::pair`, or `T` itself otherwise. This results in an ascending order skip list.

## Features

-   **Logarithmic Performance:** Average O(log N) for search, insert, delete, and find operations.
-   **Ordered Storage:** Elements are maintained in sorted order according to the `Compare` functor.
-   **Concurrent-Friendly Design:** Uses `std::atomic` pointers and Compare-And-Swap (CAS) operations for node linking, aiming for better performance in concurrent scenarios (though full lock-freedom or thread-safety depends on the complete usage context and potential data races on `T` values themselves if modified).
-   **Custom Memory Pool (Optional):**
    -   If `SKIPLIST_USE_STD_ALLOC` is *not* defined (default), a custom memory pool is used for node allocations. This pool includes a global free list and a thread-local cache to optimize node reuse and reduce allocation overhead.
    -   If `SKIPLIST_USE_STD_ALLOC` *is* defined, standard `malloc`/`free` with placement new/destructor calls are used.
-   **Finger Pointer Optimization:** A `finger_` pointer is maintained as a hint to accelerate searches for elements near recently accessed ones.
-   **Generic Element Type `T`:** Can store simple types or `std::pair<Key, Value>` for map-like behavior.
-   **Iterators:** Provides forward iterators (`begin`, `end`, `cbegin`, `cend`) for traversing elements in order.
-   **Rich API:** Includes common set/map operations like `insert`, `remove`, `search` (boolean check), `find` (returns iterator), `insert_or_assign`, `clear`, `size`, `empty`, as well as specialized operations like `kthElement`, `rangeQuery`, and bulk operations.

## Core Components

-   **`SkipListNode<T>`**: Internal node structure containing the `value` (of type `T`), an array of atomic forward pointers (`forward`), and the `node_level`.
-   **`KeyTypeHelper<T>` / `KeyType_t<T>`**: Helper to determine the actual key type used for comparisons (either `T` or `T::first_type` for pairs).
-   **`get_comparable_value(const U& val)`**: Helper to extract the key for comparison from an element `val`.
-   **`value_to_log_string(const U& val)`**: Debugging helper to convert values to strings for logging (can be specialized).

## Public Interface Highlights

### Constructors & Destructor
-   **`explicit SkipList(int userMaxLevel = DEFAULT_MAX_LEVEL)`**: Constructor. `userMaxLevel` (default 16) sets the maximum possible height of any node's tower.
-   **`~SkipList()`**: Destructor, deallocates all nodes.

### Basic Operations
-   **`bool insert(T value)`**: Inserts `value`. Returns `true` if inserted, `false` if key already exists.
-   **`std::pair<iterator, bool> insert_or_assign(const T& value)`**: Inserts `value` or assigns to it if key exists. For `std::pair<const K, V>`, only `V` is assigned.
-   **`bool remove(T value)`**: Removes element whose key matches that of `value`. Returns `true` if removed.
-   **`bool search(T value) const`**: Checks if an element with the same key as `value` exists.
-   **`iterator find(const KeyType& key_to_find)` / `const_iterator find(const KeyType& key_to_find) const`**: Finds element by key, returns iterator or `end()`/`cend()`.
-   **`void clear()`**: Removes all elements.
-   **`bool empty() const`**: Checks if empty.
-   **`int size() const`**: Returns number of elements (O(N) traversal).

### Iterators (Forward Iteration)
-   **`iterator begin() / end()`**
-   **`const_iterator cbegin() / cend()`**

### Specialized Queries & Operations
-   **`T kthElement(int k) const`**: Returns the k-th smallest element (0-indexed). Throws if `k` is out of range. (O(k) worst-case).
-   **`std::vector<T> rangeQuery(T minVal, T maxVal) const`**: Returns elements in the range `[minVal, maxVal]` (inclusive, based on key comparison).
-   **`void insert_bulk(const std::vector<T>& values)`**: Inserts multiple values efficiently (sorts input first).
-   **`size_t remove_bulk(const std::vector<T>& values)`**: Removes multiple values.

### Utility & Debugging
-   **`std::vector<T> toVector() const`**: Returns a vector of all elements in order.
-   **`void display() const`**: Prints the multi-level structure of the skip list (for debugging).
-   **`void printValues() const`**: Prints all values in order (bottom level traversal).

## Usage Examples

**Note:** The provided example file `examples/use_skip.cpp` includes `skiplist_std.h`. The examples below are adapted for the API of `skiplist.h`.

### Basic Skip List with Integers

```cpp
#include "skiplist.h" // Assuming this is the path to the custom skiplist.h
#include <iostream>
#include <vector>

int main() {
    SkipList<int> sl;

    sl.insert(30);
    sl.insert(10);
    sl.insert(20);
    sl.insert(10); // Duplicate, insert should return false
    sl.insert(50);

    std::cout << "SkipList contents (using printValues): ";
    sl.printValues(); // Expected: 10 20 30 50

    std::cout << "Search for 20: " << (sl.search(20) ? "Found" : "Not Found") << std::endl; // Found
    std::cout << "Search for 25: " << (sl.search(25) ? "Found" : "Not Found") << std::endl; // Not Found

    sl.remove(20);
    std::cout << "After removing 20, search for 20: "
              << (sl.search(20) ? "Found" : "Not Found") << std::endl; // Not Found

    std::cout << "Iterating: ";
    for (int val : sl) { // Uses begin()/end()
        std::cout << val << " ";
    }
    std::cout << std::endl;
    // Expected: 10 30 50

    std::cout << "Size: " << sl.size() << std::endl;
}
```

### Skip List as an Ordered Map (using `std::pair`)

```cpp
#include "skiplist.h"
#include <iostream>
#include <string>
#include <utility> // For std::pair

// Define a type for map entries
using MapEntry = std::pair<const int, std::string>;

// Custom comparator for MapEntry (compares based on the int key)
struct CompareMapEntry {
    bool operator()(const MapEntry& a, const MapEntry& b) const {
        return a.first < b.first;
    }
    // Also needed for heterogeneous lookups if find takes KeyType
    bool operator()(const int& key, const MapEntry& entry) const {
        return key < entry.first;
    }
    bool operator()(const MapEntry& entry, const int& key) const {
        return entry.first < key;
    }
};


int main() {
    // Explicitly provide Compare that works on MapEntry's key.
    // Or, rely on default Compare with KeyType_t and get_comparable_value.
    // For map-like behavior, KeyType_t<MapEntry> will be `const int`.
    // Default std::less<const int> will work if get_comparable_value correctly extracts the key.
    SkipList<MapEntry> ordered_map; // Uses default Compare = std::less<KeyType_t<MapEntry>>
                                   // which is std::less<const int>

    ordered_map.insert({10, "Apple"});
    ordered_map.insert({5, "Banana"});
    ordered_map.insert_or_assign({10, "Apricot"}); // Updates value for key 10

    std::cout << "Ordered Map contents:" << std::endl;
    for (const auto& entry : ordered_map) {
        std::cout << "  Key: " << entry.first << ", Value: " << entry.second << std::endl;
    }
    // Expected (sorted by key):
    // Key: 5, Value: Banana
    // Key: 10, Value: Apricot

    auto it = ordered_map.find(5); // Find by key (int)
    if (it != ordered_map.end()) {
        std::cout << "Found key 5, value: " << it->second << std::endl;
    }

    bool removed = ordered_map.remove({5, ""}); // Remove by providing a pair (value part can be dummy for key match)
    std::cout << "Removed key 5? " << std::boolalpha << removed << std::endl;
}
```

## Dependencies
- Standard C++ libraries: `<vector>`, `<atomic>`, `<mutex>`, `<random>`, `<memory>`, `<iterator>`, `<algorithm>`, `<type_traits>`, `<functional>`, `<iostream>`, `<string>`, `<iomanip>`, `<cstddef>`, `<utility>`, `<cstdlib>` (if `SKIPLIST_USE_STD_ALLOC` is defined).

This Skip List implementation offers a feature-rich alternative to tree-based ordered associative containers, with considerations for concurrent access patterns and optional custom memory management.

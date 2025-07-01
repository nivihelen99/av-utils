# FrozenCounter

## Overview

`FrozenCounter` is an immutable data structure that stores counts of hashable elements, similar to Python's `collections.Counter` but with the immutability characteristics of `FrozenSet` or `FrozenDict`. Once created, a `FrozenCounter` cannot be modified. It's designed for scenarios where you need a fixed collection of counts that won't change, providing safe concurrent access without locks (as it's immutable) and predictable state.

Elements with non-positive counts (i.e., count <= 0) provided during construction are ignored and not stored in the `FrozenCounter`.

Internally, `FrozenCounter` stores elements and their counts in a sorted `std::vector` of pairs, which allows for efficient lookups (O(log N)) and ordered iteration.

## Template Parameters

```cpp
template <
    typename Key,
    typename Compare = std::less<Key>, // Comparator for sorting keys
    typename Allocator = std::allocator<std::pair<const Key, int>> // Allocator for internal storage
>
class FrozenCounter;
```

-   `Key`: The type of elements to be counted. Must be hashable (for `std::hash` specialization) and comparable (for sorting, using `Compare`).
-   `Compare`: A comparison function object type that defines the order of keys. Defaults to `std::less<Key>`.
-   `Allocator`: The allocator type to be used for managing the internal storage. Defaults to `std::allocator<std::pair<const Key, int>>`.

## Key Features

-   **Immutability**: Contents are fixed at construction.
-   **Sorted Order**: Elements are stored sorted by key, allowing ordered iteration.
-   **Efficient Lookups**: `count()`, `operator[]`, `contains()`, and `find()` operations are O(log N) due to binary search on the internal sorted vector.
-   **Summing Duplicates**: When constructed from a range or initializer list, if multiple entries for the same key exist, their counts are summed up.
-   **Non-Positive Count Filtering**: Elements with counts <= 0 in the input are automatically excluded.
-   **`most_common()`**: Provides a way to get elements sorted by their counts.
-   **Hashing**: Supports `std::hash`, allowing `FrozenCounter` objects to be used as keys in unordered associative containers like `std::unordered_map`.

## Member Functions

### Constructors

-   `FrozenCounter(const Allocator& alloc = Allocator())`
-   `FrozenCounter(const key_compare& comp, const Allocator& alloc = Allocator())`
    - Default and comparator-specified empty constructors.
-   `template <typename InputIt> FrozenCounter(InputIt first, InputIt last, const key_compare& comp = Compare(), const Allocator& alloc = Allocator())`
    - Constructs from an iterator range of `std::pair<Key, int>`. Counts are summed for duplicate keys.
-   `FrozenCounter(std::initializer_list<std::pair<Key, mapped_type>> ilist, const key_compare& comp = Compare(), const Allocator& alloc = Allocator())`
    - Constructs from an initializer list of `std::pair<Key, int>`.
-   `template <typename CounterHash, typename CounterKeyEqual> explicit FrozenCounter(const Counter<Key, CounterHash, CounterKeyEqual>& source_counter, const key_compare& comp = Compare(), const Allocator& alloc = Allocator())`
    - Constructs from an existing mutable `cpp_collections::Counter`.
-   Copy and Move constructors are also provided.

### Lookup and Access (All `const`)

-   `mapped_type count(const key_type& key) const`
    - Returns the count of `key`. If `key` is not found, returns 0.
-   `mapped_type operator[](const key_type& key) const`
    - Equivalent to `count(key)`.
-   `bool contains(const key_type& key) const`
    - Checks if `key` is present in the counter.
-   `const_iterator find(const key_type& key) const`
    - Returns a const iterator to the element if found, otherwise `end()`.

### Capacity and Size (All `const`)

-   `bool empty() const noexcept`
    - Returns `true` if the counter contains no elements.
-   `size_type size() const noexcept`
    - Returns the number of unique elements with positive counts.
-   `size_type total() const noexcept`
    - Returns the sum of all counts of all elements.
-   `size_type max_size() const noexcept`
    - Returns the maximum possible number of elements.

### Iterators (All `const`)

-   `iterator begin() const noexcept` / `const_iterator cbegin() const noexcept`
-   `iterator end() const noexcept` / `const_iterator cend() const noexcept`
    - Provide const iterators to traverse the elements (key-count pairs) in key-sorted order.

### Other Methods

-   `std::vector<std::pair<key_type, mapped_type>> most_common(size_type n = 0) const`
    - Returns a vector of the `n` most common elements and their counts.
    - If `n` is 0 (default) or greater than or equal to `size()`, all elements are returned.
    - Elements are sorted by count (descending). Ties are broken by key order (using `key_compare`).
-   `key_compare key_comp() const`
    - Returns the key comparison function object.
-   `allocator_type get_allocator() const noexcept`
    - Returns the allocator.

### Non-Member Functions

-   `operator==`, `operator!=`
    - Compare two `FrozenCounter` objects for equality. They are equal if they have the same elements with the same counts.
-   `std::swap`
    - Swaps the contents of two `FrozenCounter` objects.
-   `std::hash<cpp_collections::FrozenCounter<...>>`
    - Specialization of `std::hash` for `FrozenCounter`.

## Usage Example

```cpp
#include "FrozenCounter.h"
#include <iostream>
#include <string>
#include <vector>

int main() {
    // Construction from initializer_list
    cpp_collections::FrozenCounter<std::string> fc1 = {
        {"apple", 3}, {"banana", 2}, {"apple", 2}, {"orange", 1}, {"banana", 3}
    };
    // fc1 will contain: {"apple": 5, "banana": 5, "orange": 1}

    std::cout << "FrozenCounter fc1:\n";
    for (const auto& pair : fc1) {
        std::cout << "  " << pair.first << ": " << pair.second << "\n";
    }
    // Output (order by key: apple, banana, orange):
    //   apple: 5
    //   banana: 5
    //   orange: 1

    std::cout << "Count of 'apple': " << fc1.count("apple") << std::endl; // 5
    std::cout << "Count of 'grape': " << fc1["grape"] << std::endl;   // 0

    std::cout << "Total unique elements: " << fc1.size() << std::endl;  // 3
    std::cout << "Sum of all counts: " << fc1.total() << std::endl;    // 11

    std::cout << "Most common (top 2):\n";
    auto common_items = fc1.most_common(2);
    for (const auto& p : common_items) {
        std::cout << "  " << p.first << ": " << p.second << "\n";
    }
    // Output (sorted by count desc, then key asc for ties):
    //   apple: 5
    //   banana: 5

    // Construction from a cpp_collections::Counter
    cpp_collections::Counter<int> mutable_counter = {{10,3}, {20,5}, {10,2}};
    cpp_collections::FrozenCounter<int> fc2(mutable_counter);
    // fc2 will contain: {10: 5, 20: 5}
    std::cout << "fc2 (from mutable_counter) count of 10: " << fc2.count(10) << std::endl; // 5

    return 0;
}

```

## When to Use

-   When you need a frequency map (counter) that will not change after its creation.
-   For sharing count data across threads without synchronization, as its immutability guarantees thread safety for reads.
-   When you need a hashable counter to be used as a key in other data structures.
-   If you need elements to be iterated in a specific (sorted by key) order.
```

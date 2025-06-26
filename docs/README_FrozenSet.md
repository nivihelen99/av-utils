# FrozenSet

## Overview

`FrozenSet<Key, Compare, Allocator>` is an immutable container that stores a collection of unique, sorted elements of type `Key`. Once a `FrozenSet` is created, its contents cannot be modified. This immutability provides several benefits, including inherent thread safety for read operations and the ability for `FrozenSet` instances themselves to be hashable and thus usable as keys in hash-based associative containers like `std::unordered_map`.

The elements are sorted according to a comparator `Compare` (defaulting to `std::less<Key>`). Uniqueness is also determined based on this comparator: two elements `a` and `b` are considered equivalent if `!comp(a, b) && !comp(b, a)`.

Internally, `FrozenSet` typically uses a sorted `std::vector` to store its elements, providing cache-friendly iteration and efficient lookups (O(log N)).

## Template Parameters

*   `Key`: The type of elements stored in the `FrozenSet`.
*   `Compare`: A comparison function object type that defines the ordering of elements. Defaults to `std::less<Key>`. This comparator is used to sort elements and to determine uniqueness.
*   `Allocator`: The allocator type used for memory management of the elements. Defaults to `std::allocator<Key>`.

## Key Features

*   **Immutability**: Contents are fixed upon construction. No methods modify the set after it's created (e.g., no `insert`, `erase`).
*   **Uniqueness**: Stores only unique elements, as determined by the `Compare` functor.
*   **Sorted Order**: Elements are always stored in a sorted order defined by `Compare`.
*   **Hashable**: `std::hash<FrozenSet<...>>` is specialized, allowing `FrozenSet` instances to be used as keys in `std::unordered_map`, `std::unordered_set`, or the custom `cpp_collections::dict`.
*   **Efficient Lookups**: `find()`, `contains()`, and `count()` operations are O(log N) due to the sorted internal storage.
*   **Efficient Iteration**: Iteration is cache-friendly, similar to `std::vector`.

## API Reference

### Member Types

| Type              | Definition                                         |
|-------------------|----------------------------------------------------|
| `key_type`        | `Key`                                              |
| `value_type`      | `Key` (as sets store keys as values)               |
| `size_type`       | `std::size_t`                                      |
| `difference_type` | `std::ptrdiff_t`                                   |
| `key_compare`     | `Compare`                                          |
| `value_compare`   | `Compare`                                          |
| `allocator_type`  | `Allocator`                                        |
| `reference`       | `const Key&` (elements are always const)           |
| `const_reference` | `const Key&`                                       |
| `pointer`         | `std::allocator_traits<Allocator>::const_pointer`  |
| `const_pointer`   | `std::allocator_traits<Allocator>::const_pointer`  |
| `iterator`        | Constant iterator to `Key`                         |
| `const_iterator`  | Constant iterator to `Key`                         |

### Constructors

*   `FrozenSet(const Allocator& alloc = Allocator())`
    *   Default constructor. Creates an empty `FrozenSet`.
*   `FrozenSet(const Compare& comp, const Allocator& alloc = Allocator())`
    *   Creates an empty `FrozenSet` with a specific comparator.
*   `template <typename InputIt> FrozenSet(InputIt first, InputIt last, const Compare& comp = Compare(), const Allocator& alloc = Allocator())`
    *   Constructs a `FrozenSet` with elements from the range `[first, last)`. Elements are sorted and duplicates are removed.
*   `FrozenSet(std::initializer_list<Key> ilist, const Compare& comp = Compare(), const Allocator& alloc = Allocator())`
    *   Constructs a `FrozenSet` from an initializer list.
*   `FrozenSet(const FrozenSet& other)`
    *   Copy constructor.
*   `FrozenSet(FrozenSet&& other) noexcept`
    *   Move constructor.

### Iterators

*   `iterator begin() const noexcept`
*   `const_iterator cbegin() const noexcept`
    *   Returns an iterator to the beginning of the set.
*   `iterator end() const noexcept`
*   `const_iterator cend() const noexcept`
    *   Returns an iterator to the end of the set.

### Capacity

*   `bool empty() const noexcept`
    *   Returns `true` if the set is empty, `false` otherwise.
*   `size_type size() const noexcept`
    *   Returns the number of elements in the set.
*   `size_type max_size() const noexcept`
    *   Returns the maximum possible number of elements.

### Lookup

*   `size_type count(const Key& key) const`
    *   Returns `1` if an element equivalent to `key` exists, `0` otherwise. (O(log N))
*   `bool contains(const Key& key) const`
    *   Returns `true` if an element equivalent to `key` exists, `false` otherwise. (O(log N))
*   `iterator find(const Key& key) const`
    *   Returns an iterator to the element equivalent to `key` if found, otherwise returns `end()`. (O(log N))

### Observers

*   `key_compare key_comp() const`
    *   Returns the key comparison object.
*   `value_compare value_comp() const`
    *   Returns the value comparison object (same as `key_comp`).
*   `allocator_type get_allocator() const noexcept`
    *   Returns the allocator object.

### Comparison Operators

Standard comparison operators (`==`, `!=`, `<`, `<=`, `>`, `>=`) are provided for comparing two `FrozenSet` instances.
*   `operator==`: Two `FrozenSet`s are equal if they have the same size and all elements are element-wise equal according to their internal sorted order. This means their internal `data_` vectors must be identical. If custom comparators result in different canonical representations (e.g., "Apple" vs "apple" with a case-insensitive comparator where `std::unique` keeps the first seen), the `FrozenSet`s will not be equal.
*   `operator<`: Lexicographical comparison.

### Non-Member Functions

*   `template <typename K, typename C, typename A> void swap(FrozenSet<K,C,A>& lhs, FrozenSet<K,C,A>& rhs) noexcept(...)`
    *   Swaps the contents of `lhs` and `rhs`. (Note: This technically mutates the `FrozenSet` objects involved in the swap, but not their conceptual "frozen" content which is transferred.)

### Hashing

A specialization `std::hash<cpp_collections::FrozenSet<Key, Compare, Allocator>>` is provided, allowing `FrozenSet` objects to be used as keys in standard C++ unordered associative containers (e.g., `std::unordered_map`). The hash value is computed based on the sequence of elements in their sorted order.

## Time Complexity

*   **Construction**: O(M log M) where M is the number of elements in the input range (due to sorting), or O(M) if input is already sorted and unique.
*   **Lookup (`find`, `contains`, `count`)**: O(log N) where N is the size of the `FrozenSet`.
*   **Iteration**: O(N) to iterate through all elements.
*   **Comparison (`operator==`, `operator<`)**: O(N) in the worst case.

## Space Complexity

*   O(N) to store N elements.

## Example Usage

```cpp
#include "FrozenSet.h"
#include <iostream>
#include <string>
#include <unordered_map>

int main() {
    // Create a FrozenSet from an initializer list
    cpp_collections::FrozenSet<int> fs1 = {3, 1, 4, 1, 5, 9, 2, 6};

    std::cout << "fs1 size: " << fs1.size() << std::endl; // Output: 7
    std::cout << "fs1 elements: ";
    for (int x : fs1) {
        std::cout << x << " "; // Output: 1 2 3 4 5 6 9
    }
    std::cout << std::endl;

    if (fs1.contains(4)) {
        std::cout << "fs1 contains 4." << std::endl;
    }

    // FrozenSet as a key in std::unordered_map
    std::unordered_map<cpp_collections::FrozenSet<std::string>, int> word_set_ids;

    cpp_collections::FrozenSet<std::string> words1 = {"hello", "world"};
    cpp_collections::FrozenSet<std::string> words2 = {"world", "hello"}; // Equivalent to words1
    cpp_collections::FrozenSet<std::string> words3 = {"frozen", "set"};

    word_set_ids[words1] = 1;
    word_set_ids[words3] = 2;

    std::cout << "ID for {\"hello\", \"world\"}: " << word_set_ids[words1] << std::endl;
    std::cout << "ID for {\"world\", \"hello\"} (should be same): " << word_set_ids[words2] << std::endl;
    std::cout << "ID for {\"frozen\", \"set\"}: " << word_set_ids[words3] << std::endl;

    return 0;
}
```

## When to Use

*   When you need a set-like data structure whose contents will not change after creation.
*   When you need to use sets as keys in hash maps or elements in other sets.
*   In multi-threaded environments where immutable data can simplify concurrency (read operations are inherently safe).
*   To represent constant collections of unique items.
```

# UnorderedMultiset

## Overview

The `cpp_utils::UnorderedMultiset<T, Hash, KeyEqual>` is a C++ template class that implements an unordered multiset (also known as a bag). It is a container that stores a collection of elements where multiple elements can have the same value. Unlike `std::set` or `cpp_utils::OrderedSet`, the `UnorderedMultiset` does not maintain any specific order among its elements. Its primary advantage is fast average-time complexity for insertions, deletions, and lookups, similar to `std::unordered_set` or `std::unordered_map`.

This implementation uses `std::unordered_map<T, size_t>` as its underlying storage, where each key `T` is mapped to its count (`size_t`) within the multiset.

## Template Parameters

-   `T`: The type of elements to be stored in the multiset.
-   `Hash`: A hash function object that computes the hash value for elements of type `T`. Defaults to `std::hash<T>`.
-   `KeyEqual`: A binary predicate that checks if two elements of type `T` are equal. Defaults to `std::equal_to<T>`.

## Member Functions

### Constructors

-   `UnorderedMultiset()`
    -   Default constructor. Creates an empty multiset.
-   `explicit UnorderedMultiset(const Hash& hashfn, const KeyEqual& eq = KeyEqual())`
    -   Constructs an empty multiset with the specified hash function and equality predicate.

### Capacity

-   `bool empty() const noexcept`
    -   Returns `true` if the multiset is empty (i.e., `size()` is 0), `false` otherwise.
-   `size_type size() const noexcept`
    -   Returns the total number of elements in the multiset (sum of counts of all unique elements).

### Modifiers

-   `void insert(const T& value)`
    -   Inserts `value` into the multiset. If `value` already exists, its count is incremented.
-   `void insert(T&& value)`
    -   Inserts `value` (via move semantics) into the multiset. If `value` already exists, its count is incremented.
-   `size_type erase(const T& value)`
    -   Removes a single instance of `value` from the multiset.
    -   Returns `1` if an element was removed, `0` otherwise.
-   `size_type erase_all(const T& value)`
    -   Removes all instances of `value` from the multiset.
    -   Returns the number of elements removed.
-   `void clear() noexcept`
    -   Removes all elements from the multiset, making it empty.
-   `void swap(UnorderedMultiset& other) noexcept`
    -   Exchanges the contents of this multiset with the contents of `other`.

### Lookup

-   `size_type count(const T& value) const`
    -   Returns the number of times `value` appears in the multiset.
-   `bool contains(const T& value) const`
    -   Returns `true` if `value` is present in the multiset (i.e., its count is > 0), `false` otherwise.

### Iterators

The multiset provides `const_iterator` to iterate over its unique elements. The iterators are those of the underlying `std::unordered_map<T, size_t>`, where `iterator->first` is the unique element and `iterator->second` is its count.

-   `const_iterator begin() const noexcept`
-   `const_iterator cbegin() const noexcept`
    -   Returns a const iterator to the beginning of the (unique elements of the) multiset.
-   `const_iterator end() const noexcept`
-   `const_iterator cend() const noexcept`
    -   Returns a const iterator to the end of the (unique elements of the) multiset.

## Non-Member Functions

-   `template <typename T, typename Hash, typename KeyEqual> void swap(UnorderedMultiset<T, Hash, KeyEqual>& lhs, UnorderedMultiset<T, Hash, KeyEqual>& rhs) noexcept`
    -   Overload for `std::swap`. Exchanges the contents of `lhs` and `rhs`.

## Performance

-   **Insertion**: Average O(1), worst case O(N) (where N is `size()`).
-   **Deletion (single instance or all instances)**: Average O(1), worst case O(N).
-   **Lookup (`count`, `contains`)**: Average O(1), worst case O(N).
-   **`size()`, `empty()`**: O(1).

The performance characteristics are derived from the underlying `std::unordered_map`.

## Example Usage

```cpp
#include "unordered_multiset.h"
#include <iostream>
#include <string>

int main() {
    cpp_utils::UnorderedMultiset<std::string> fruit_basket;

    fruit_basket.insert("apple");
    fruit_basket.insert("banana");
    fruit_basket.insert("apple"); // Add another apple
    fruit_basket.insert("orange");
    fruit_basket.insert("apple"); // And another apple

    std::cout << "Fruit basket size: " << fruit_basket.size() << std::endl; // Output: 5

    std::cout << "Count of 'apple': " << fruit_basket.count("apple") << std::endl;   // Output: 3
    std::cout << "Count of 'banana': " << fruit_basket.count("banana") << std::endl; // Output: 1
    std::cout << "Count of 'grape': " << fruit_basket.count("grape") << std::endl;   // Output: 0

    if (fruit_basket.contains("orange")) {
        std::cout << "The basket contains oranges." << std::endl;
    }

    std::cout << "Unique fruits in the basket (with counts):" << std::endl;
    for (const auto& pair : fruit_basket) {
        std::cout << "- " << pair.first << ": " << pair.second << std::endl;
    }
    // Possible output (order may vary):
    // - orange: 1
    // - banana: 1
    // - apple: 3

    fruit_basket.erase("apple"); // Remove one apple
    std::cout << "After erasing one 'apple', count is: " << fruit_basket.count("apple") << std::endl; // Output: 2
    std::cout << "Basket size is now: " << fruit_basket.size() << std::endl; // Output: 4

    fruit_basket.erase_all("apple"); // Remove all remaining apples
    std::cout << "After erasing all 'apples', count is: " << fruit_basket.count("apple") << std::endl; // Output: 0
    std::cout << "Basket size is now: " << fruit_basket.size() << std::endl; // Output: 2 (banana, orange)

    fruit_basket.clear();
    std::cout << "After clearing, basket is empty: " << std::boolalpha << fruit_basket.empty() << std::endl; // Output: true

    return 0;
}

```

## When to Use

-   When you need to store a collection of items and allow duplicates.
-   When the order of elements does not matter.
-   When fast average-case insertion, deletion, and lookup are required.
-   For scenarios like frequency counting, tracking multiple instances of objects, etc.
```

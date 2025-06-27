# OrderedDict

`std_ext::OrderedDict` is a C++ associative container that remembers the order in which items were inserted. It behaves like a standard `std::unordered_map` for most operations but iterates over its elements in the order of their original insertion. This is similar to Python's `collections.OrderedDict` or modern Python dictionaries (version 3.7+).

## Features

*   **Insertion Order Preservation**: Iterators traverse elements in the order they were added.
*   **Map-like Interface**: Provides common map operations like `operator[]`, `at()`, `insert()`, `erase()`, `find()`, `contains()`, `clear()`, `size()`, `empty()`.
*   **Efficient Lookups**: Average O(1) time complexity for key-based lookups, insertions, and deletions (same as `std::unordered_map`).
*   **Bidirectional Iterators**: Supports forward and reverse iteration.
*   **Allocator Support**: Customizable allocators.
*   **Python-inspired `popitem()`**: Allows removing and retrieving the first or last inserted item.

## Template Parameters

```cpp
template<
    typename Key,
    typename Value,
    typename Hash = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>,
    typename Allocator = std::allocator<std::pair<const Key, Value>>
>
class OrderedDict;
```

*   `Key`: The type of the keys.
*   `Value`: The type of the mapped values.
*   `Hash`: A unary function object type that takes an object of type `Key` and returns a `std::size_t`. Defaults to `std::hash<Key>`.
*   `KeyEqual`: A binary predicate that takes two arguments of type `Key` and returns a `bool`. Defaults to `std::equal_to<Key>`.
*   `Allocator`: The allocator type used for all memory management. Defaults to `std::allocator<std::pair<const Key, Value>>`.

## Basic Usage

```cpp
#include "ordered_dict.h" // Assuming this path
#include <iostream>
#include <string>

int main() {
    std_ext::OrderedDict<std::string, int> scores;

    // Insert items - order will be preserved
    scores["Alice"] = 95;
    scores["Bob"] = 88;
    scores.insert({"Charlie", 92});

    std::cout << "Initial scores (insertion order):" << std::endl;
    for (const auto& pair : scores) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }
    // Output:
    // Alice: 95
    // Bob: 88
    // Charlie: 92

    scores["Alice"] = 96; // Update Alice's score - order remains the same
    scores.insert_or_assign("David", 85); // Add David

    std::cout << "\nUpdated scores:" << std::endl;
    for (const auto& pair : scores) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }
    // Output:
    // Alice: 96
    // Bob: 88
    // Charlie: 92
    // David: 85

    // Pop the last item
    if (!scores.empty()) {
        auto last_item = scores.popitem(); // Default is last=true
        std::cout << "\nPopped last item: " << last_item.first << ": " << last_item.second << std::endl;
    }

    std::cout << "\nScores after popitem:" << std::endl;
    for (const auto& pair : scores) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }
    // Output:
    // Alice: 96
    // Bob: 88
    // Charlie: 92

    return 0;
}
```

## Key Operations and Time Complexity

Most operations have time complexities similar to `std::unordered_map` for the map component and `std::list` for the list component.

| Operation             | Average Case Time Complexity | Worst Case Time Complexity | Notes                                                                                                |
|-----------------------|------------------------------|----------------------------|------------------------------------------------------------------------------------------------------|
| `operator[]` (access) | O(1)                         | O(N)                       | If key exists.                                                                                       |
| `operator[]` (insert) | O(1)                         | O(N)                       | If key doesn't exist; new element added to end.                                                      |
| `at()`                | O(1)                         | O(N)                       |                                                                                                      |
| `insert()`            | O(1)                         | O(N)                       | New element added to end.                                                                            |
| `insert_or_assign()`  | O(1)                         | O(N)                       | If new, added to end; if assign, order preserved.                                                    |
| `emplace()`           | O(1)                         | O(N)                       | New element added to end.                                                                            |
| `try_emplace()`       | O(1)                         | O(N)                       | New element added to end.                                                                            |
| `erase(key)`          | O(1)                         | O(N)                       |                                                                                                      |
| `erase(iterator)`     | O(1) (amortized)             | O(1) (amortized)           | Erasing from list is O(1), map erase is O(1) avg.                                                    |
| `find()`              | O(1)                         | O(N)                       |                                                                                                      |
| `count()`             | O(1)                         | O(N)                       |                                                                                                      |
| `contains()`          | O(1)                         | O(N)                       |                                                                                                      |
| `clear()`             | O(N)                         | O(N)                       | N is the number of elements.                                                                         |
| `size()`, `empty()`   | O(1)                         | O(1)                       |                                                                                                      |
| `popitem()`           | O(1) (amortized)             | O(1) (amortized)           | Erasing from list front/back is O(1), map erase is O(1) avg.                                         |
| Iteration             | O(N)                         | O(N)                       | Iterates N elements.                                                                                 |

*Worst case O(N) for map operations occurs due to hash collisions.*

## Member Functions

A brief overview of commonly used member functions:

### Constructors
*   Default constructor: `OrderedDict()`
*   Allocator-aware constructors.
*   Range constructor: `OrderedDict(InputIt first, InputIt last, ...)`
*   Initializer list: `OrderedDict(std::initializer_list<value_type> init, ...)`
*   Copy/Move constructors.

### Element Access
*   `operator[](const key_type& key)` / `operator[](key_type&& key)`: Accesses or inserts an element.
*   `at(const key_type& key)` (and `const` version): Accesses an element, throws `std::out_of_range` if key not found.

### Modifiers
*   `insert(const value_type& value)` / `insert(value_type&& value)`: Inserts element if key is unique. Returns `std::pair<iterator, bool>`.
*   `insert_or_assign(const key_type& k, M&& obj)` (and overloads): Inserts element or assigns to existing key.
*   `emplace(Args&&... args)`: Constructs element in-place if key is unique.
*   `try_emplace(const key_type& k, Args&&... args)` (and overloads): Constructs element in-place if key is unique, does nothing if key exists.
*   `erase(const key_type& key)`: Removes element by key. Returns number of elements removed (0 or 1).
*   `erase(iterator pos)` / `erase(const_iterator pos)`: Removes element at iterator position. Returns iterator to the next element.
*   `clear()`: Removes all elements.
*   `swap(OrderedDict& other)`: Swaps contents with another `OrderedDict`.

### Lookup
*   `find(const key_type& key)` (and `const` version): Returns iterator to element or `end()`/`cend()`.
*   `count(const key_type& key) const`: Returns 1 if key exists, 0 otherwise.
*   `contains(const key_type& key) const`: Returns `true` if key exists, `false` otherwise.

### Capacity
*   `empty() const`: Checks if the container is empty.
*   `size() const`: Returns the number of elements.
*   `max_size() const`: Returns the maximum possible number of elements.

### Iterators
*   `begin()`, `end()` (and `const` versions `cbegin()`, `cend()`): Return iterators for forward traversal.
*   `rbegin()`, `rend()` (and `const` versions `crbegin()`, `crend()`): Return iterators for reverse traversal.

### Special Python-like Methods
*   `popitem(bool last = true)`: Removes and returns the first (`last=false`) or last (`last=true`) inserted key-value pair as `std::pair<key_type, mapped_type>`. Throws `std::out_of_range` if empty.

## When to Use

`OrderedDict` is useful when:
*   The order of insertion of key-value pairs is significant and needs to be preserved during iteration.
*   You need the fast average-case lookup performance of a hash map (`std::unordered_map`).
*   Use cases include implementing LRU caches (where `popitem(false)` can remove the oldest item), parsing configuration files where section order matters, or processing data streams where event order is important.

It incurs a slightly higher memory overhead and constant factor for insertions/deletions compared to `std::unordered_map` due to the need to maintain an additional linked list.

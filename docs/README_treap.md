# Treap Data Structure (`treap.h`)

## Overview

The `treap.h` header provides a C++ implementation of a Treap, a randomized binary search tree. A Treap combines the properties of a binary search tree (BST) and a heap. Each node in the Treap has both a key (for BST ordering) and a randomly assigned priority (for heap ordering). This randomization helps in keeping the tree balanced on average, leading to logarithmic time complexity for common operations like insertion, deletion, and search, without the need for complex balancing algorithms like those in AVL or Red-Black trees.

This implementation is header-only, templated, and offers a map-like interface similar to `std::map`.

## Features

*   **Templated:** Can be used with various key and value types.
    *   `template <typename Key, typename Value, typename Compare = std::less<Key>> class Treap;`
*   **Map-like Interface:**
    *   Insertion: `insert()`, `operator[]`
    *   Deletion: `erase()`
    *   Search: `find()`, `contains()`
    *   Access: `operator[]` (for values)
*   **Iteration:** Supports forward iterators (e.g., for range-based for loops) that traverse elements in key-sorted order.
*   **Randomized Balancing:** Achieves average O(log N) performance for insertions, deletions, and lookups. Worst-case O(N) is possible but highly unlikely due to random priorities.
*   **Standard Operations:** `size()`, `empty()`, `clear()`.
*   **Move Semantics:** Supports move construction and move assignment for efficient transfers of resources.

## How it Works

*   **BST Property:** For any node `x`, all keys in its left subtree are less than `x.key`, and all keys in its right subtree are greater than `x.key` (according to the provided `Compare` functor).
*   **Heap Property:** For any node `x`, `x.priority` is greater than or equal to the priorities of its children. This means the node with the highest priority is the root (max-heap property on priorities).
*   **Random Priorities:** When a new node is inserted, it is assigned a random priority.
*   **Rotations:** To maintain the heap property after an insertion or deletion that violates it, tree rotations (left or right) are performed. These rotations also preserve the BST property.

## Usage

### Include Header

```cpp
#include "treap.h"
#include <string> // For example
#include <iostream> // For example
```

### Basic Operations

```cpp
// Create a Treap mapping integers to strings
Treap<int, std::string> myTreap;

// Insert elements
myTreap.insert(10, "Apple");
myTreap.insert(5, "Banana");
auto result = myTreap.insert(15, "Cherry"); // result is std::pair<iterator, bool>

if (result.second) {
    std::cout << "Inserted new element: " << result.first->first << " -> " << result.first->second << std::endl;
}

// Update an existing element (inserting a key that already exists updates its value)
myTreap.insert(10, "Apricot"); // Value for key 10 is now "Apricot"

// Using operator[]
myTreap[20] = "Fig"; // Inserts if key 20 doesn't exist
myTreap[5] = "Blueberry"; // Updates value for key 5

std::cout << "Value for key 20: " << myTreap[20] << std::endl;

// Find an element
int keyToFind = 5;
if (myTreap.contains(keyToFind)) {
    std::cout << "Found key " << keyToFind << ", value: " << *myTreap.find(keyToFind) << std::endl;
} else {
    std::cout << "Key " << keyToFind << " not found." << std::endl;
}

// Erase an element
myTreap.erase(10);

// Iterate over elements (sorted by key)
std::cout << "Treap contents:" << std::endl;
for (const auto& pair : myTreap) {
    std::cout << pair.first << ": " << pair.second << std::endl;
}
// Output will be sorted by key:
// 5: Blueberry
// 15: Cherry
// 20: Fig

// Size and empty check
std::cout << "Size: " << myTreap.size() << std::endl;
if (myTreap.empty()) {
    std::cout << "Treap is empty." << std::endl;
}

// Clear the treap
myTreap.clear();
std::cout << "Size after clear: " << myTreap.size() << std::endl;
```

### Custom Comparators

You can provide a custom comparison function object (functor) as the third template argument if the default `std::less<Key>` is not suitable (e.g., for descending order or custom key types).

```cpp
// Treap sorted in descending order of keys
Treap<int, std::string, std::greater<int>> descendingTreap;
descendingTreap.insert(10, "Ten");
descendingTreap.insert(20, "Twenty");
descendingTreap.insert(5, "Five");

// Iteration will yield: 20, 10, 5
for (const auto& pair : descendingTreap) {
    std::cout << pair.first << ": " << pair.second << std::endl;
}
```

## API Reference (Key Methods)

*   `Treap()`
    *   Default constructor.
*   `std::pair<iterator, bool> insert(const Key& key, const Value& value)`
    *   Inserts a key-value pair. If the key already exists, its value is updated.
    *   Returns a pair: the `iterator` points to the (newly inserted or existing) element, and `bool` is `true` if a new element was inserted, `false` otherwise.
*   `std::pair<iterator, bool> insert(Key&& key, Value&& value)`
    *   Move-semantic version of insert.
*   `Value& operator[](const Key& key)`
    *   Accesses the value associated with `key`. If `key` does not exist, it is inserted with a default-constructed `Value`, and a reference to this new value is returned.
*   `Value& operator[](Key&& key)`
    *   Move-semantic version of `operator[]` for the key.
*   `bool erase(const Key& key)`
    *   Removes the element with the given `key`. Returns `true` if an element was removed, `false` otherwise.
*   `Value* find(const Key& key)`
    *   Returns a pointer to the value associated with `key`, or `nullptr` if the key is not found.
*   `const Value* find(const Key& key) const`
    *   Const version of `find`.
*   `bool contains(const Key& key) const`
    *   Returns `true` if the treap contains an element with the given `key`, `false` otherwise.
*   `size_t size() const`
    *   Returns the number of elements in the treap.
*   `bool empty() const`
    *   Returns `true` if the treap is empty, `false` otherwise.
*   `void clear()`
    *   Removes all elements from the treap.
*   `iterator begin()` / `const_iterator begin() const` / `const_iterator cbegin() const`
    *   Returns an iterator to the first element (smallest key).
*   `iterator end()` / `const_iterator end() const` / `const_iterator cend() const`
    *   Returns an iterator to the element following the last element.

## Iterator

The `Treap::iterator` (and `const_iterator`) is a forward iterator. It allows iterating through the elements of the Treap in key-sorted order.

Dereferencing an iterator yields a `std::pair<const Key, Value>&`.

Example:
```cpp
for (Treap<int, std::string>::iterator it = myTreap.begin(); it != myTreap.end(); ++it) {
    std::cout << "Key: " << it->first << ", Value: " << it->second << std::endl;
}
```

## Complexity

*   **Insertion:** Average O(log N), Worst O(N)
*   **Deletion:** Average O(log N), Worst O(N)
*   **Search (`find`, `contains`, `operator[]` access):** Average O(log N), Worst O(N)
*   **Space:** O(N)

The worst-case scenarios are rare due to the use of random priorities.

## Notes

*   The implementation uses `std::unique_ptr` for managing node memory, ensuring automatic cleanup.
*   Copy construction and copy assignment are disabled to prevent accidental expensive copies of the tree structure. Move semantics should be used instead.
*   The random priorities are generated using `std::mt19937` and `std::uniform_int_distribution`.
*   The iterator implementation might have limitations or specific behaviors common to node-based containers with rotations, especially if external pointers to nodes were held (which is not typical for map-like iterators). The provided iterators are designed to be safe for standard iteration patterns.
```

# B-Tree Implementation

This document provides an overview of the B-Tree data structure implemented in `include/BTree.h`.

## Overview

The `BTree` class is a C++ template implementation of an in-memory B-Tree. B-Trees are self-balancing search trees designed to maintain sorted data and allow searches, sequential access, insertions, and deletions in logarithmic time. They are particularly well-suited for storage systems that read and write large blocks of data, but they can also be useful in-memory for their cache-friendly properties, especially with larger node sizes.

This implementation supports:
- Generic key and value types.
- Custom comparison functors.
- Configurable minimum degree (`t`), which dictates the size and branching factor of nodes.

## Template Parameters

The `BTree` class is templated as follows:

```cpp
template <typename Key, typename Value, typename Compare = std::less<Key>, int MinDegree = 2>
class BTree {
    // ...
};
```

-   `Key`: The type of the keys stored in the B-Tree.
-   `Value`: The type of the values associated with the keys.
-   `Compare`: A comparison functor (like `std::less<Key>`) that defines the ordering of keys. It must provide a strict weak ordering. Defaults to `std::less<Key>`.
-   `MinDegree`: An integer representing the minimum degree `t` of the B-Tree.
    -   `MinDegree` must be at least 2.
    -   Each node (except possibly the root) must have at least `MinDegree - 1` keys and at most `2 * MinDegree - 1` keys.
    -   Each internal node (except possibly the root) must have at least `MinDegree` children and at most `2 * MinDegree` children.
    -   The choice of `MinDegree` affects the height of the tree and the number of disk accesses (in a disk-based scenario) or cache misses (in an in-memory scenario). Larger `MinDegree` values lead to shorter, wider trees. Defaults to 2.

## Core Public API

-   **Constructor**: `BTree()`
    -   Creates an empty B-Tree.

-   **`void insert(const Key& k, const Value& v)`**:
    -   Inserts a key-value pair into the B-Tree.
    -   Time Complexity: O(t * log_t N), where `t` is `MinDegree` and `N` is the number of keys.

-   **`Value* search(const Key& k)`**:
    -   Searches for a key `k` in the B-Tree.
    -   Returns a pointer to the `Value` if the key is found, otherwise `nullptr`.
    -   A `const` version `const Value* search(const Key& k) const` is also available.
    -   Time Complexity: O(t * log_t N).

-   **`bool is_empty() const`**:
    -   Returns `true` if the B-Tree contains no elements, `false` otherwise.
    -   Time Complexity: O(1).

-   **`int get_min_degree() const`**:
    -   Returns the minimum degree `t` with which the B-Tree was configured.
    -   Time Complexity: O(1).

## Basic Usage Example

```cpp
#include "BTree.h" // Assuming BTree.h is in the include path
#include <iostream>
#include <string>

int main() {
    // Create a B-Tree with int keys, string values, and MinDegree = 3
    cpp_collections::BTree<int, std::string, std::less<int>, 3> tree;

    // Insert elements
    tree.insert(10, "Apple");
    tree.insert(20, "Banana");
    tree.insert(5, "Cherry");
    tree.insert(15, "Date");

    // Search for an element
    const std::string* value = tree.search(10);
    if (value) {
        std::cout << "Found key 10: " << *value << std::endl; // Output: Found key 10: Apple
    } else {
        std::cout << "Key 10 not found." << std::endl;
    }

    // Search for a non-existent element
    if (tree.search(100)) {
        std::cout << "Key 100 found (unexpected)." << std::endl;
    } else {
        std::cout << "Key 100 not found (as expected)." << std::endl;
    }

    std::cout << "Is tree empty? " << (tree.is_empty() ? "Yes" : "No") << std::endl; // Output: No
    std::cout << "Min Degree: " << tree.get_min_degree() << std::endl; // Output: 3

    return 0;
}
```

## Future Enhancements (Not Implemented)

-   **Deletion**: Removing keys from a B-Tree is a more complex operation involving potential merging or redistribution of keys between nodes.
-   **Iterators**: For traversing elements in sorted order.
-   **Update**: Modifying the value associated with an existing key.
-   **More comprehensive testing**: Including edge cases and performance benchmarks.

This implementation provides the fundamental insertion and search capabilities of a B-Tree.

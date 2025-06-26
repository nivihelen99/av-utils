# RedBlackTree

## Overview

The `RedBlackTree` is a self-balancing binary search tree. In most C++ standard libraries, `std::map` and `std::set` are implemented using red-black trees, but this class provides a standalone implementation, offering more direct control and an educational perspective on its workings.

Red-black trees maintain balance by adhering to a set of properties. These properties ensure that the longest path from the root to any leaf is no more than twice as long as the shortest path, which guarantees that operations like search, insert, and delete can be performed in O(log n) time complexity, where n is the number of nodes in the tree.

## Red-Black Properties

A red-black tree must satisfy the following properties:
1.  Every node is either red or black.
2.  The root is black.
3.  Every leaf (typically a sentinel NULL node) is black.
4.  If a node is red, then both its children are black.
5.  For each node, all simple paths from the node to descendant leaves contain the same number of black nodes (this is known as the black-height).

## Template Parameters

```cpp
template <typename Key, typename Value, typename Compare = std::less<Key>>
class RedBlackTree {
    // ...
};
```

-   `Key`: The type of the keys stored in the tree.
-   `Value`: The type of the values associated with the keys.
-   `Compare`: A comparison function object type, defaulting to `std::less<Key>`. This functor is used to order the keys in the tree.

## Public Interface

-   `RedBlackTree()`: Constructs an empty red-black tree.
-   `void insert(const Key& key, const Value& value)`: Inserts a key-value pair into the tree. If the key already exists, its value is updated. Maintains red-black properties.
    -   Time Complexity: O(log n)
-   `Value* find(const Key& key) const`: Searches for a key in the tree. Returns a pointer to the value if the key is found, otherwise returns `nullptr`.
    -   Time Complexity: O(log n)
-   `bool contains(const Key& key) const`: Checks if a key exists in the tree.
    -   Time Complexity: O(log n)
-   `void remove(const Key& key)`: Removes a key (and its associated value) from the tree. Maintains red-black properties. If the key is not found, the tree remains unchanged.
    -   Time Complexity: O(log n)
-   `bool isEmpty() const`: Returns `true` if the tree contains no elements, `false` otherwise.
    -   Time Complexity: O(1)
-   `void printTree() const`: (Primarily for debugging) Prints a visual representation of the tree structure to standard output.

## Internal Structure (Conceptual)

Each node in the `RedBlackTree` stores:
-   A key-value pair.
-   A color (Red or Black).
-   Pointers to its left child, right child, and parent.

The tree uses a special sentinel node (`TNULL`) to represent all logical null leaves, simplifying the implementation of the red-black algorithms. This `TNULL` is always considered black.

## Usage Example

```cpp
#include "RedBlackTree.h"
#include <iostream>
#include <string>

int main() {
    collections::RedBlackTree<int, std::string> rbt;

    rbt.insert(10, "apple");
    rbt.insert(20, "banana");
    rbt.insert(5, "cherry");
    rbt.insert(15, "date");
    rbt.insert(25, "elderberry");

    std::cout << "Tree structure after insertions:" << std::endl;
    rbt.printTree(); // For visual inspection

    std::cout << "\nSearching for key 15: ";
    std::string* value = rbt.find(15);
    if (value) {
        std::cout << "Found value: " << *value << std::endl;
    } else {
        std::cout << "Key not found." << std::endl;
    }

    std::cout << "Searching for key 99: ";
    value = rbt.find(99);
    if (value) {
        std::cout << "Found value: " << *value << std::endl;
    } else {
        std::cout << "Key not found." << std::endl;
    }

    std::cout << "\nTree contains key 20? " << (rbt.contains(20) ? "Yes" : "No") << std::endl;
    std::cout << "Tree contains key 99? " << (rbt.contains(99) ? "Yes" : "No") << std::endl;

    std::cout << "\nRemoving key 20 (banana)..." << std::endl;
    rbt.remove(20);

    std::cout << "Tree structure after deleting 20:" << std::endl;
    rbt.printTree();

    std::cout << "\nSearching for key 20 after deletion: ";
    value = rbt.find(20);
    if (value) {
        std::cout << "Found value: " << *value << std::endl;
    } else {
        std::cout << "Key not found." << std::endl;
    }

    rbt.insert(20, "new banana"); // Re-insert
    std::cout << "\nValue for key 20 after re-insert: " << *rbt.find(20) << std::endl;


    return 0;
}

```

### Expected Output of Example:

```
Tree structure after insertions:
R----10(BLACK)
     L----5(BLACK)
     R----20(BLACK)
          L----15(RED)
          R----25(RED)

Searching for key 15: Found value: date
Searching for key 99: Key not found.

Tree contains key 20? Yes
Tree contains key 99? No

Removing key 20 (banana)...
Tree structure after deleting 20:
R----10(BLACK)
     L----5(BLACK)
     R----25(BLACK)
          L----15(RED)

Searching for key 20 after deletion: Key not found.

Value for key 20 after re-insert: new banana
```
*(Note: The exact structure printed by `printTree` can vary based on insertion order and balancing operations, but the root will be black, and red-black properties will hold. The example output for `printTree` is illustrative.)*

## When to Use

-   When you need a balanced binary search tree that guarantees O(log n) performance for insertions, deletions, and lookups.
-   When you need an ordered associative container (like `std::map`) but want to study or have direct access to the underlying tree structure and operations.
-   As a foundational data structure for more complex algorithms or data structures.

Compared to a standard hash map (`std::unordered_map`), a `RedBlackTree` provides ordered traversal of elements and typically has a more consistent O(log n) performance, whereas hash maps can degrade to O(n) in worst-case scenarios (though average O(1) for many operations).

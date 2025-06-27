# Scapegoat Tree (`ScapegoatTree.h`)

## Overview

The `ScapegoatTree` is a C++ implementation of a self-balancing binary search tree. Unlike other balanced trees like Red-Black or AVL trees that store balance information (e.g., color, height) in each node, Scapegoat Trees achieve balance by rebuilding subtrees when an insertion or deletion makes them too "unbalanced" according to a weight-based criterion. The node responsible for the imbalance (the "scapegoat") is found, and the subtree rooted at the scapegoat is completely rebuilt into a perfectly balanced tree.

This implementation uses lazy deletion: when an element is erased, its node is marked as deleted but remains physically in the tree. These logically deleted nodes are physically removed during a rebuild operation.

## Template Parameters

```cpp
template <typename Key, typename Value, typename Compare = std::less<Key>>
class ScapegoatTree;
```

-   `Key`: The type of the keys stored in the tree.
-   `Value`: The type of the values associated with the keys.
-   `Compare`: A comparison function object type (defaults to `std::less<Key>`) that defines the ordering of keys.

## Balancing Mechanism

The tree uses an `alpha` parameter (typically between 0.5 and 1.0, e.g., 0.75) to determine balance. A node `x` is considered alpha-weight-balanced if the size of each of its children's subtrees is at most `alpha` times the size of `x`'s subtree. That is:
`size(left_child) <= alpha * size(x)` and `size(right_child) <= alpha * size(x)`.
(Note: The current implementation uses `subtree_size`, which includes all physical nodes, for this check.)

-   **Insertion**: After an insertion, the tree checks ancestors of the new node. If an unbalanced node is found, the highest such node on the path from the root (the scapegoat) has its subtree rebuilt into a perfectly balanced one.
-   **Deletion**: Deletion is lazy. Nodes are marked `is_deleted`.
    -   If the number of logically deleted nodes becomes too high relative to the total number of nodes (e.g., more than half the nodes are deleted), the entire tree is rebuilt, pruning all logically deleted nodes.

## Node Structure

Each node in the tree stores:
-   `key`: The key of the node.
-   `value`: The value associated with the key.
-   `left`, `right`: Unique pointers to the left and right children.
-   `subtree_size`: The total number of physical nodes in the subtree rooted at this node.
-   `active_nodes`: The number of active (not logically deleted) nodes in this subtree.
-   `is_deleted`: A boolean flag indicating if the node is logically deleted.

## Public API

### Constructors and Destructor

-   `explicit ScapegoatTree(double alpha = 0.75)`: Constructs an empty tree. `alpha` must be strictly between 0.5 and 1.0.
-   `~ScapegoatTree()`: Destructor.

### Basic Operations

-   `bool insert(const Key& key, const Value& value)`: Inserts a key-value pair. If the key exists, its value is updated. Returns `true` if a new node was physically inserted or an existing deleted node was reactivated, `false` if an existing active node's value was updated.
-   `bool insert(const Key& key, Value&& value)`: Rvalue version for the value.
-   `bool erase(const Key& key)`: Lazily deletes the node with the given key. Returns `true` if an active node was marked as deleted, `false` otherwise.
-   `const Value* find(const Key& key) const`: Returns a pointer to the value if the key is found and active, otherwise `nullptr`.
-   `bool contains(const Key& key) const`: Checks if an active key exists in the tree.
-   `size_t size() const noexcept`: Returns the number of active elements in the tree.
-   `bool empty() const noexcept`: Checks if the tree contains any active elements.
-   `void clear()`: Removes all elements from the tree.

### Iterators

The tree provides in-order iterators:
-   `iterator begin()`, `iterator end()`
-   `const_iterator begin() const`, `const_iterator end() const`
-   `const_iterator cbegin() const`, `const_iterator cend() const`

Iterators skip logically deleted nodes. The `value_type` of the iterator is effectively `std::pair<const Key&, Value&>` (or const Value& for const_iterator), accessed via a proxy object.

## Complexity

-   **`insert`**: Amortized O(log N) for insertion, potentially O(N) if a full rebuild is triggered (though this is amortized over many operations). The rebuild of a scapegoat's subtree of size `s` takes O(s log s) or O(s) depending on the flattening/rebuilding strategy.
-   **`erase` (lazy)**: O(log N) to find the node and mark it. Global rebuild if triggered is O(N).
-   **`find`/`contains`**: O(log N) in the worst case (height of the tree).
-   **`size`/`empty`/`clear`**: `size()` and `empty()` are O(1). `clear()` is O(N) due to node deallocation.
-   **Space**: O(N) for N elements.

## Usage Example

```cpp
#include "include/ScapegoatTree.h"
#include <iostream>

int main() {
    cpp_collections::ScapegoatTree<int, std::string> s_tree(0.7); // Alpha = 0.7

    s_tree.insert(10, "Apple");
    s_tree.insert(5, "Banana");
    s_tree.insert(15, "Cherry");
    s_tree.insert(3, "Date");

    if (s_tree.contains(5)) {
        std::cout << "Found 5: " << *s_tree.find(5) << std::endl;
    }

    s_tree.erase(10);
    std::cout << "Size after erasing 10: " << s_tree.size() << std::endl; // Will be 3

    s_tree.insert(10, "Apricot"); // Reactivates 10
    std::cout << "Size after re-inserting 10: " << s_tree.size() << std::endl; // Will be 4

    std::cout << "Tree elements (in-order):" << std::endl;
    for (const auto& pair : s_tree) {
        std::cout << pair.first << " -> " << pair.second << std::endl;
    }

    return 0;
}

```

## Notes

-   The balancing strategy aims to keep the tree reasonably balanced without the strict invariants of AVL or Red-Black trees, simplifying the implementation.
-   Lazy deletion means that memory for deleted nodes is only reclaimed during rebuild operations. This can lead to higher memory usage if many deletions occur without subsequent insertions that trigger local rebuilds or a global rebuild.
-   The choice of `alpha` affects the trade-off between balancing frequency and how skewed the tree can become. Values closer to 0.5 mean more frequent rebuilds but better balance. Values closer to 1.0 mean fewer rebuilds but potentially worse balance before a rebuild.
-   The current iterator implementation is for forward traversal. Bidirectional iterators would require more complex state management or parent pointers in nodes.
```

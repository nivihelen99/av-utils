# Disjoint Set Union (DSU) / Union-Find

## Overview

The `disjoint_set_union.h` header provides implementations of the Disjoint Set Union (DSU) data structure, also commonly known as the Union-Find algorithm. This data structure is designed to efficiently manage a collection of elements partitioned into a number of disjoint (non-overlapping) sets.

It primarily supports two operations:
-   **Find**: Determine which set a particular element belongs to by finding the representative (or root) of that set.
-   **Union**: Merge two disjoint sets into a single set.

The implementations in this header use path compression and union by rank/size heuristics to achieve nearly constant time complexity (amortized O(α(N)), where α is the very slowly growing inverse Ackermann function) for these operations.

Two main classes are provided:
1.  `DisjointSetUnion<T, Hash, KeyEqual>`: A generic DSU that can handle elements of any hashable type `T`.
2.  `FastDSU`: A specialized DSU optimized for contiguous integer elements (e.g., 0 to N-1), typically offering better performance for such cases due to vector-based storage.

The header also includes example applications of DSU within the `DSUApplications` namespace, such as Kruskal's algorithm, cycle detection, and counting connected components.

## `UnionStrategy` Enum
Determines the heuristic used during the `unionSets` operation:
-   `UnionStrategy::BY_RANK`: Attaches the root of the tree with a smaller rank (approximate depth) to the root of the tree with a larger rank.
-   `UnionStrategy::BY_SIZE`: Attaches the root of the tree with fewer elements to the root of the tree with more elements.

## `DisjointSetUnion<T, Hash, KeyEqual>` (Generic DSU)

### Template Parameters
-   `T`: The type of elements.
-   `Hash`: Hash functor for `T` (defaults to `std::hash<T>`).
-   `KeyEqual`: Equality comparison for `T` (defaults to `std::equal_to<T>`).

### Public Interface Highlights
-   **`DisjointSetUnion(UnionStrategy strategy = UnionStrategy::BY_RANK)`**: Constructor.
-   **`void makeSet(const T& x)`**: Adds `x` as a new set if it doesn't exist.
-   **`T find(const T& x)`**: Finds the representative of the set containing `x`. Auto-creates `x` if not present. Implements path compression.
-   **`bool unionSets(const T& x, const T& y)`**: Merges the sets containing `x` and `y`. Returns `true` if a merge occurred.
-   **`bool connected(const T& x, const T& y)`**: Checks if `x` and `y` are in the same set.
-   **`int size(const T& x)`**: Returns the size of the set containing `x`.
-   **`int countSets() const noexcept`**: Returns the number of disjoint sets.
-   **`int totalElements() const noexcept`**: Returns the total number of elements managed.
-   **`bool contains(const T& x) const`**: Checks if `x` is part of any set.
-   **`std::vector<T> getSetMembers(const T& x)`**: Gets all members of `x`'s set.
-   **`std::vector<std::vector<T>> getAllSets()`**: Gets all disjoint sets.
-   **`void reset()`**: Resets elements to individual sets (maintains existing elements).
-   **`void clear() noexcept`**: Removes all elements and sets.

## `FastDSU` (Integer-Optimized DSU)

Optimized for elements being integers in a range `0` to `N-1`. Uses `std::vector` internally.

### Public Interface Highlights
-   **`explicit FastDSU(int n, UnionStrategy strategy = UnionStrategy::BY_RANK)`**: Initializes for `n` elements (0 to `n-1`).
-   **`int find(int x)`**: Finds representative of `x`.
-   **`bool unionSets(int x, int y)`**: Merges sets containing `x` and `y`.
-   **`bool connected(int x, int y)`**: Checks if `x` and `y` are in the same set.
-   **`int size(int x)`**: Returns the size of the set containing `x`.
-   **`int countSets() const noexcept`**: Returns the number of disjoint sets.
-   **`bool contains(int x) const noexcept`**: Checks if `x` is within the valid range `[0, N-1)`.
-   **`void reset() noexcept`**: Resets elements to individual sets.
-   **`std::vector<std::vector<int>> getAllSets()`**.

## `DSUApplications` Namespace
Provides examples of algorithms using DSU:
-   `struct Edge { int u, v, weight; ... }`
-   `std::vector<Edge> kruskalMST(int n, std::vector<Edge>& edges)`
-   `bool hasCycle(int n, const std::vector<std::pair<int, int>>& edges)`
-   `int countConnectedComponents(int n, const std::vector<std::pair<int, int>>& edges)`

## Usage Examples

(Based on `examples/use_dsu.cpp`)

### Generic DSU with Strings

```cpp
#include "disjoint_set_union.h"
#include <iostream>
#include <string>
#include <vector>

int main() {
    DisjointSetUnion<std::string> groups;
    groups.makeSet("Alice");
    groups.makeSet("Bob");
    groups.makeSet("Charlie");

    groups.unionSets("Alice", "Bob");

    std::cout << "Alice and Bob connected? " << std::boolalpha << groups.connected("Alice", "Bob") << std::endl; // true
    std::cout << "Alice and Charlie connected? " << groups.connected("Alice", "Charlie") << std::endl; // false
    std::cout << "Number of groups: " << groups.countSets() << std::endl; // 2
    std::cout << "Size of Alice's group: " << groups.size("Alice") << std::endl; // 2
}
```

### `FastDSU` for Graph Problems (Cycle Detection)

```cpp
#include "disjoint_set_union.h" // Includes DSUApplications
#include <iostream>
#include <vector>
#include <utility> // For std::pair

int main() {
    int num_nodes = 5;
    std::vector<std::pair<int, int>> edges_with_cycle = {{0, 1}, {1, 2}, {2, 0}, {3, 4}};
    std::vector<std::pair<int, int>> edges_no_cycle = {{0, 1}, {1, 2}, {2, 3}, {3, 4}};

    bool cycle1 = DSUApplications::hasCycle(num_nodes, edges_with_cycle);
    std::cout << "Graph 1 has cycle: " << std::boolalpha << cycle1 << std::endl; // true

    bool cycle2 = DSUApplications::hasCycle(num_nodes, edges_no_cycle);
    std::cout << "Graph 2 has cycle: " << std::boolalpha << cycle2 << std::endl; // false

    std::cout << "Connected components in graph 2: "
              << DSUApplications::countConnectedComponents(num_nodes, edges_no_cycle) << std::endl; // 1
}
```

### Kruskal's Algorithm for MST using `FastDSU`

```cpp
#include "disjoint_set_union.h" // Includes DSUApplications
#include <iostream>
#include <vector>
#include <algorithm> // For std::sort in kruskalMST

int main() {
    int num_vertices = 9;
    std::vector<DSUApplications::Edge> graph_edges = {
        {0, 1, 4}, {0, 7, 8}, {1, 2, 8}, {1, 7, 11},
        {2, 3, 7}, {2, 8, 2}, {2, 5, 4}, {3, 4, 9},
        {3, 5, 14}, {4, 5, 10}, {5, 6, 2}, {6, 7, 1},
        {6, 8, 6}, {7, 8, 7}
    };

    std::vector<DSUApplications::Edge> mst_edges = DSUApplications::kruskalMST(num_vertices, graph_edges);

    std::cout << "Minimum Spanning Tree Edges:" << std::endl;
    int total_weight = 0;
    for (const auto& edge : mst_edges) {
        std::cout << edge.u << " -- " << edge.v << " (weight: " << edge.weight << ")" << std::endl;
        total_weight += edge.weight;
    }
    std::cout << "Total MST weight: " << total_weight << std::endl;
}
```

## Dependencies
- `<vector>`, `<unordered_map>`, `<unordered_set>` (used internally or by examples)
- `<algorithm>`, `<cassert>`, `<functional>`, `<iostream>`

DSU is a fundamental data structure with applications in graph algorithms (like Kruskal's, cycle detection, connected components), network connectivity problems, and more.

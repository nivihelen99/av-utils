# Graph Data Structure and Topological Sort

## Overview

The `Graph.h` header provides a generic, directed graph data structure (`cpp_utils::Graph`) and an implementation of the topological sort algorithm. This graph can be used with various node identifier types (e.g., `int`, `std::string`, custom structs/classes).

## Features

*   **Generic Node Types**: Templated on `NodeID`, allowing flexibility in how nodes are identified.
*   **Directed Graph**: Edges have a direction (from one node to another).
*   **Adjacency List Representation**: Uses `std::unordered_map<NodeID, std::unordered_set<NodeID>>` for efficient neighbor lookups and storage. `std::unordered_set` automatically handles duplicate edge additions.
*   **Basic Graph Operations**:
    *   Adding nodes and edges.
    *   Checking for node existence.
    *   Retrieving neighbors of a node.
    *   Getting all nodes in the graph.
    *   Querying the number of nodes and edges.
*   **Topological Sort**: Implements Kahn's algorithm to find a linear ordering of nodes such that for every directed edge `u -> v`, node `u` comes before node `v` in the ordering.
    *   Handles cycles: Returns `tl::expected<std::vector<NodeID>, std::string>` to indicate success (with the sorted list) or failure (with an error message if a cycle is detected).

## `cpp_utils::Graph<NodeID>` API

### Constructor
*   `Graph()`: Default constructor.

### Modifiers
*   `bool add_node(const NodeID& node_id)`: Adds a node to the graph. Returns `true` if added, `false` if it already existed.
    *   *Complexity*: Average O(1), worst case O(N) for `std::unordered_map` insertion.
*   `void add_edge(const NodeID& from_node_id, const NodeID& to_node_id)`: Adds a directed edge. If nodes don't exist, they are created.
    *   *Complexity*: Average O(1) for node lookups and `std::unordered_set` insertion. Worst case involves node creation costs.

### Accessors
*   `bool has_node(const NodeID& node_id) const`: Checks if a node exists.
    *   *Complexity*: Average O(1), worst case O(N) for `std::unordered_map` lookup.
*   `const std::unordered_set<NodeID>& get_neighbors(const NodeID& node_id) const`: Returns a const reference to the set of neighbors. Throws `std::out_of_range` if `node_id` doesn't exist.
    *   *Complexity*: Average O(1), worst case O(N) for `std::unordered_map` lookup.
*   `std::unordered_set<NodeID>& get_neighbors(const NodeID& node_id)`: Returns a non-const reference (for potential modification, though not typical for simple neighbor iteration). Throws `std::out_of_range` if `node_id` doesn't exist.
    *   *Complexity*: Average O(1), worst case O(N) for `std::unordered_map` lookup.
*   `std::vector<NodeID> get_all_nodes() const`: Returns a vector containing all node IDs. Order is not guaranteed.
    *   *Complexity*: O(V) where V is the number of nodes.
*   `size_t num_nodes() const`: Returns the number of nodes.
    *   *Complexity*: O(1) (after C++11, `size()` for `unordered_map` is O(1)).
*   `size_t num_edges() const`: Returns the number of edges.
    *   *Complexity*: O(V) in the current implementation as it iterates through the adjacency list. Could be O(1) if an edge counter is maintained.

### Topological Sort
*   `tl::expected<std::vector<NodeID>, std::string> topological_sort() const`: Performs topological sort.
    *   *Input*: The graph itself.
    *   *Output*: `tl::expected` containing either a `std::vector<NodeID>` with the sorted order or a `std::string` error message if a cycle is detected.
    *   *Algorithm*: Kahn's Algorithm.
    *   *Time Complexity*: O(V + E), where V is the number of vertices (nodes) and E is the number of edges. This is because calculating in-degrees is O(V+E), initializing the queue is O(V), and the main loop processes each node and edge once.
    *   *Space Complexity*: O(V + E) for storing in-degrees, the queue, and the result. The adjacency list itself also takes O(V+E).

## Usage Example

```cpp
#include "Graph.h"
#include <iostream>
#include <string>

int main() {
    cpp_utils::Graph<std::string> dependency_graph;

    // Adding task dependencies
    dependency_graph.add_edge("Task A", "Task C");
    dependency_graph.add_edge("Task B", "Task C");
    dependency_graph.add_edge("Task C", "Task D");
    dependency_graph.add_edge("Task B", "Task E");
    dependency_graph.add_node("Task F"); // An independent task

    std::cout << "Graph created with " << dependency_graph.num_nodes() << " nodes and "
              << dependency_graph.num_edges() << " edges." << std::endl;

    auto sorted_result = dependency_graph.topological_sort();

    if (sorted_result) {
        std::cout << "Topologically Sorted Order of Tasks:" << std::endl;
        for (const std::string& task : *sorted_result) {
            std::cout << "- " << task << std::endl;
        }
    } else {
        std::cout << "Error during topological sort: " << sorted_result.error() << std::endl;
    }

    // Example of a graph with a cycle
    cpp_utils::Graph<int> cyclic_graph;
    cyclic_graph.add_edge(1, 2);
    cyclic_graph.add_edge(2, 3);
    cyclic_graph.add_edge(3, 1); // Cycle!

    auto cyclic_result = cyclic_graph.topological_sort();
    if (!cyclic_result) {
        std::cout << "\nCyclic graph detected: " << cyclic_result.error() << std::endl;
    }

    return 0;
}
```

### Output of Example:
```
Graph created with 6 nodes and 4 edges.
Topologically Sorted Order of Tasks:
- Task A
- Task B
- Task F
- Task E
- Task C
- Task D
(Note: The exact order of 'Task A', 'Task B', 'Task F' might vary as they are initial independent tasks, similarly for 'Task E' and 'Task C' if 'Task B' comes before 'Task A')

Cyclic graph detected: Graph has a cycle, topological sort not possible.
```

## Notes

*   The `num_edges()` method currently iterates through the adjacency list to count edges. For frequent calls on large graphs where performance is critical, maintaining a separate edge counter updated during `add_edge` (and potentially `remove_edge` if implemented) would make this O(1).
*   The `NodeID` type should be hashable and comparable for use in `std::unordered_map` and `std::unordered_set`. Standard types like `int`, `std::string` work out of the box. For custom types, you might need to provide a `std::hash` specialization and an equality operator.
*   The `expected.h` utility (assumed to be `tl::expected` or similar) is used for error handling in `topological_sort`. Ensure this header is available and correctly included.
```

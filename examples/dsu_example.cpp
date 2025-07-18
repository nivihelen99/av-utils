#include "../include/disjoint_set_union.h" // Assuming this path is correct
#include <iostream>
#include <vector>
#include <string>
#include <chrono> // For performance test
#include <algorithm> // For std::sort in Kruskal's example (used by DSUApplications)

// The main function extracted from disjoint_set_union.h
int main() {
    std::cout << "=== Disjoint Set Union (Union-Find) Demo ===\n\n";

    // Test generic DSU with strings
    std::cout << "1. Generic DSU with strings:\n";
    DisjointSetUnion<std::string> dsu;

    // Create some sets
    std::vector<std::string> people = {"Alice", "Bob", "Charlie", "David", "Eve", "Frank"};
    for (const auto& person : people) {
        dsu.makeSet(person);
    }

    std::cout << "Initial sets: " << dsu.countSets() << '\n';

    // Form friendships (unions)
    (void)dsu.unionSets("Alice", "Bob");
    (void)dsu.unionSets("Charlie", "David");
    (void)dsu.unionSets("Alice", "Charlie"); // Now Alice, Bob, Charlie, David are connected

    std::cout << "After forming friendships: " << dsu.countSets() << " groups\n";
    std::cout << "Alice and David are " << (dsu.connected("Alice", "David") ? "connected" : "not connected") << '\n';
    std::cout << "Alice's group size: " << dsu.size("Alice") << '\n';

    // Print all friendship groups
    auto allSets = dsu.getAllSets();
    std::cout << "Friendship groups:\n";
    for (size_t i = 0; i < allSets.size(); i++) {
        std::cout << "Group " << i + 1 << ": ";
        for (const auto& person : allSets[i]) {
            std::cout << person << " ";
        }
        std::cout << '\n';
    }

    std::cout << "\n2. Fast DSU with integers (Graph algorithms):\n";

    // Test cycle detection
    std::vector<std::pair<int, int>> edges1 = {{0, 1}, {1, 2}, {2, 3}, {3, 4}}; // Requires <vector> and <utility> (for std::pair)
    // <utility> is often implicitly included but good to be aware. disjoint_set_union.h includes <vector>.
    std::cout << "Graph 1 has cycle: " << (DSUApplications::hasCycle(5, edges1) ? "Yes" : "No") << '\n';

    std::vector<std::pair<int, int>> edges2 = {{0, 1}, {1, 2}, {2, 0}, {3, 4}};
    std::cout << "Graph 2 has cycle: " << (DSUApplications::hasCycle(5, edges2) ? "Yes" : "No") << '\n';

    // Test connected components
    std::cout << "Connected components in graph 1: "
              << DSUApplications::countConnectedComponents(5, edges1) << '\n';
    std::cout << "Connected components in graph 2: "
              << DSUApplications::countConnectedComponents(5, edges2) << '\n';

    // Test Kruskal's MST
    std::cout << "\n3. Minimum Spanning Tree (Kruskal's Algorithm):\n";
    std::vector<DSUApplications::Edge> edges = { // DSUApplications::Edge is defined in disjoint_set_union.h
        {0, 1, 4}, {0, 7, 8}, {1, 2, 8}, {1, 7, 11},
        {2, 3, 7}, {2, 8, 2}, {2, 5, 4}, {3, 4, 9},
        {3, 5, 14}, {4, 5, 10}, {5, 6, 2}, {6, 7, 1},
        {6, 8, 6}, {7, 8, 7}
    };

    auto mst = DSUApplications::kruskalMST(9, edges); // DSUApplications::kruskalMST is in disjoint_set_union.h
                                                      // which includes <algorithm> for std::sort.
    std::cout << "MST edges:\n";
    int totalWeight = 0;
    for (const auto& edge : mst) {
        std::cout << edge.u << " -- " << edge.v << " (weight: " << edge.weight << ")\n";
        totalWeight += edge.weight;
    }
    std::cout << "Total MST weight: " << totalWeight << '\n';

    // Performance test
    std::cout << "\n4. Performance test:\n";
    FastDSU perfDSU(100000); // FastDSU is from disjoint_set_union.h

    auto start = std::chrono::high_resolution_clock::now(); // Requires <chrono>

    // Perform many union operations
    for (int i = 0; i < 50000; i++) {
        (void)perfDSU.unionSets(i, i + 50000);
    }

    // Perform many find operations
    for (int i = 0; i < 100000; i++) {
        (void)perfDSU.find(i);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "150,000 operations on 100,000 elements took: "
              << duration.count() << " microseconds\n";
    std::cout << "Final number of sets: " << perfDSU.countSets() << '\n';

    return 0;
}

#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cassert>
#include <functional>

/**
 * Disjoint Set Union (Union-Find) Data Structure
 * Supports path compression and union by rank optimizations
 * Template version supports any hashable type
 */
template<typename T = int>
class DisjointSetUnion {
private:
    std::unordered_map<T, T> parent;        // parent[x] = parent of x
    std::unordered_map<T, int> rank;        // rank[x] = approximate depth of tree rooted at x
    std::unordered_map<T, int> setSize;     // setSize[x] = size of set rooted at x
    int numSets;                            // total number of disjoint sets
    
public:
    DisjointSetUnion() : numSets(0) {}
    
    /**
     * Create a new set containing only element x
     * Time Complexity: O(1)
     */
    void makeSet(const T& x) {
        if (parent.find(x) != parent.end()) {
            return; // Element already exists
        }
        
        parent[x] = x;      // x is its own parent (root)
        rank[x] = 0;        // Initial rank is 0
        setSize[x] = 1;     // Set size is 1
        numSets++;
    }
    
    /**
     * Find the representative (root) of the set containing x
     * Applies path compression for optimization
     * Time Complexity: O(α(n)) - inverse Ackermann function
     */
    T find(const T& x) {
        if (parent.find(x) == parent.end()) {
            makeSet(x); // Auto-create if doesn't exist
        }
        
        // Path compression: make every node point directly to root
        if (parent[x] != x) {
            parent[x] = find(parent[x]);
        }
        
        return parent[x];
    }
    
    /**
     * Merge the sets containing x and y
     * Uses union by rank for optimization
     * Time Complexity: O(α(n))
     */
    bool unionSets(const T& x, const T& y) {
        T rootX = find(x);
        T rootY = find(y);
        
        if (rootX == rootY) {
            return false; // Already in same set
        }
        
        // Union by rank: attach smaller rank tree under root of higher rank tree
        if (rank[rootX] < rank[rootY]) {
            parent[rootX] = rootY;
            setSize[rootY] += setSize[rootX];
        } else if (rank[rootX] > rank[rootY]) {
            parent[rootY] = rootX;
            setSize[rootX] += setSize[rootY];
        } else {
            // Same rank: make rootY parent of rootX and increment rank
            parent[rootX] = rootY;
            setSize[rootY] += setSize[rootX];
            rank[rootY]++;
        }
        
        numSets--;
        return true;
    }
    
    /**
     * Check if x and y are in the same set
     * Time Complexity: O(α(n))
     */
    bool connected(const T& x, const T& y) {
        return find(x) == find(y);
    }
    
    /**
     * Get the size of the set containing x
     * Time Complexity: O(α(n))
     */
    int size(const T& x) {
        T root = find(x);
        return setSize[root];
    }
    
    /**
     * Get the total number of disjoint sets
     * Time Complexity: O(1)
     */
    int countSets() const {
        return numSets;
    }
    
    /**
     * Get total number of elements
     * Time Complexity: O(1)
     */
    int totalElements() const {
        return parent.size();
    }
    
    /**
     * Check if element exists in the structure
     * Time Complexity: O(1)
     */
    bool contains(const T& x) const {
        return parent.find(x) != parent.end();
    }
    
    /**
     * Get all elements in the same set as x
     * Time Complexity: O(n) where n is total elements
     */
    std::vector<T> getSetMembers(const T& x) {
        T root = find(x);
        std::vector<T> members;
        
        for (const auto& pair : parent) {
            if (find(pair.first) == root) {
                members.push_back(pair.first);
            }
        }
        
        return members;
    }
    
    /**
     * Get all disjoint sets as a vector of vectors
     * Time Complexity: O(n)
     */
    std::vector<std::vector<T>> getAllSets() {
        std::unordered_map<T, std::vector<T>> setMap;
        
        // Group elements by their root
        for (const auto& pair : parent) {
            T root = find(pair.first);
            setMap[root].push_back(pair.first);
        }
        
        std::vector<std::vector<T>> result;
        for (const auto& pair : setMap) {
            result.push_back(pair.second);
        }
        
        return result;
    }
    
    /**
     * Reset all elements to individual sets
     * Time Complexity: O(n)
     */
    void reset() {
        for (auto& pair : parent) {
            pair.second = pair.first;
            rank[pair.first] = 0;
            setSize[pair.first] = 1;
        }
        numSets = parent.size();
    }
    
    /**
     * Remove all elements
     * Time Complexity: O(1)
     */
    void clear() {
        parent.clear();
        rank.clear();
        setSize.clear();
        numSets = 0;
    }
    
    /**
     * Manually trigger path compression for all elements
     * Usually not needed as find() does this automatically
     * Time Complexity: O(n)
     */
    void compress() {
        for (const auto& pair : parent) {
            find(pair.first);
        }
    }
    
    /**
     * Debug: Print structure information
     */
    void printStructure() const {
        std::cout << "DSU Structure:\n";
        std::cout << "Total elements: " << totalElements() << "\n";
        std::cout << "Number of sets: " << countSets() << "\n";
        
        for (const auto& pair : parent) {
            std::cout << pair.first << " -> " << pair.second 
                     << " (rank: " << rank.at(pair.first) 
                     << ", size: " << setSize.at(pair.first) << ")\n";
        }
    }
};

/**
 * Specialized version for integers with better performance
 */
class FastDSU {
private:
    std::vector<int> parent;
    std::vector<int> rank;
    std::vector<int> setSize;
    int numSets;
    int maxElement;
    
public:
    explicit FastDSU(int n) : parent(n), rank(n, 0), setSize(n, 1), numSets(n), maxElement(n) {
        for (int i = 0; i < n; i++) {
            parent[i] = i;
        }
    }
    
    int find(int x) {
        assert(x >= 0 && x < maxElement);
        if (parent[x] != x) {
            parent[x] = find(parent[x]); // Path compression
        }
        return parent[x];
    }
    
    bool unionSets(int x, int y) {
        assert(x >= 0 && x < maxElement && y >= 0 && y < maxElement);
        
        int rootX = find(x);
        int rootY = find(y);
        
        if (rootX == rootY) return false;
        
        // Union by rank
        if (rank[rootX] < rank[rootY]) {
            parent[rootX] = rootY;
            setSize[rootY] += setSize[rootX];
        } else if (rank[rootX] > rank[rootY]) {
            parent[rootY] = rootX;
            setSize[rootX] += setSize[rootY];
        } else {
            parent[rootX] = rootY;
            setSize[rootY] += setSize[rootX];
            rank[rootY]++;
        }
        
        numSets--;
        return true;
    }
    
    bool connected(int x, int y) {
        return find(x) == find(y);
    }
    
    int size(int x) {
        return setSize[find(x)];
    }
    
    int countSets() const {
        return numSets;
    }
};

// Example applications and testing
namespace DSUApplications {
    
    /**
     * Kruskal's Algorithm for Minimum Spanning Tree
     */
    struct Edge {
        int u, v, weight;
        bool operator<(const Edge& other) const {
            return weight < other.weight;
        }
    };
    
    std::vector<Edge> kruskalMST(int n, std::vector<Edge>& edges) {
        std::sort(edges.begin(), edges.end());
        FastDSU dsu(n);
        std::vector<Edge> mst;
        
        for (const Edge& e : edges) {
            if (dsu.unionSets(e.u, e.v)) {
                mst.push_back(e);
                if (mst.size() == n - 1) break;
            }
        }
        
        return mst;
    }
    
    /**
     * Detect cycle in undirected graph
     */
    bool hasCycle(int n, const std::vector<std::pair<int, int>>& edges) {
        FastDSU dsu(n);
        
        for (const auto& edge : edges) {
            if (dsu.connected(edge.first, edge.second)) {
                return true; // Cycle detected
            }
            dsu.unionSets(edge.first, edge.second);
        }
        
        return false;
    }
    
    /**
     * Count connected components
     */
    int countConnectedComponents(int n, const std::vector<std::pair<int, int>>& edges) {
        FastDSU dsu(n);
        
        for (const auto& edge : edges) {
            dsu.unionSets(edge.first, edge.second);
        }
        
        return dsu.countSets();
    }
}

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
    
    std::cout << "Initial sets: " << dsu.countSets() << std::endl;
    
    // Form friendships (unions)
    dsu.unionSets("Alice", "Bob");
    dsu.unionSets("Charlie", "David");
    dsu.unionSets("Alice", "Charlie"); // Now Alice, Bob, Charlie, David are connected
    
    std::cout << "After forming friendships: " << dsu.countSets() << " groups\n";
    std::cout << "Alice and David are " << (dsu.connected("Alice", "David") ? "connected" : "not connected") << std::endl;
    std::cout << "Alice's group size: " << dsu.size("Alice") << std::endl;
    
    // Print all friendship groups
    auto allSets = dsu.getAllSets();
    std::cout << "Friendship groups:\n";
    for (size_t i = 0; i < allSets.size(); i++) {
        std::cout << "Group " << i + 1 << ": ";
        for (const auto& person : allSets[i]) {
            std::cout << person << " ";
        }
        std::cout << std::endl;
    }
    
    std::cout << "\n2. Fast DSU with integers (Graph algorithms):\n";
    
    // Test cycle detection
    std::vector<std::pair<int, int>> edges1 = {{0, 1}, {1, 2}, {2, 3}, {3, 4}};
    std::cout << "Graph 1 has cycle: " << (DSUApplications::hasCycle(5, edges1) ? "Yes" : "No") << std::endl;
    
    std::vector<std::pair<int, int>> edges2 = {{0, 1}, {1, 2}, {2, 0}, {3, 4}};
    std::cout << "Graph 2 has cycle: " << (DSUApplications::hasCycle(5, edges2) ? "Yes" : "No") << std::endl;
    
    // Test connected components
    std::cout << "Connected components in graph 1: " 
              << DSUApplications::countConnectedComponents(5, edges1) << std::endl;
    std::cout << "Connected components in graph 2: " 
              << DSUApplications::countConnectedComponents(5, edges2) << std::endl;
    
    // Test Kruskal's MST
    std::cout << "\n3. Minimum Spanning Tree (Kruskal's Algorithm):\n";
    std::vector<DSUApplications::Edge> edges = {
        {0, 1, 4}, {0, 7, 8}, {1, 2, 8}, {1, 7, 11},
        {2, 3, 7}, {2, 8, 2}, {2, 5, 4}, {3, 4, 9},
        {3, 5, 14}, {4, 5, 10}, {5, 6, 2}, {6, 7, 1},
        {6, 8, 6}, {7, 8, 7}
    };
    
    auto mst = DSUApplications::kruskalMST(9, edges);
    std::cout << "MST edges:\n";
    int totalWeight = 0;
    for (const auto& edge : mst) {
        std::cout << edge.u << " -- " << edge.v << " (weight: " << edge.weight << ")\n";
        totalWeight += edge.weight;
    }
    std::cout << "Total MST weight: " << totalWeight << std::endl;
    
    // Performance test
    std::cout << "\n4. Performance test:\n";
    FastDSU perfDSU(100000);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Perform many union operations
    for (int i = 0; i < 50000; i++) {
        perfDSU.unionSets(i, i + 50000);
    }
    
    // Perform many find operations
    for (int i = 0; i < 100000; i++) {
        perfDSU.find(i);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "150,000 operations on 100,000 elements took: " 
              << duration.count() << " microseconds\n";
    std::cout << "Final number of sets: " << perfDSU.countSets() << std::endl;
    
    return 0;
}

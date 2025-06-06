#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cassert>
#include <functional>

// Define UnionStrategy enum globally or within a suitable namespace
enum class UnionStrategy {
    BY_RANK,
    BY_SIZE
};

/**
 * @brief Disjoint Set Union (Union-Find) data structure.
 * 
 * Manages a collection of disjoint sets. It supports efficient union of sets
 * and finding the representative of a set.
 * This implementation uses path compression and union by rank for optimization.
 * 
 * @tparam T The type of elements.
 * @tparam Hash The hash functor for T. Defaults to std::hash<T>.
 * @tparam KeyEqual The equality comparison functor for T. Defaults to std::equal_to<T>.
 */
template<typename T, typename Hash = std::hash<T>, typename KeyEqual = std::equal_to<T>>
class DisjointSetUnion {
private:
    std::unordered_map<T, T, Hash, KeyEqual> parent;        // parent[x] = parent of x
    std::unordered_map<T, int, Hash, KeyEqual> rank;        // rank[x] = approximate depth of tree rooted at x
    std::unordered_map<T, int, Hash, KeyEqual> setSize;     // setSize[x] = size of set rooted at x (only valid for roots)
    int numSets;                            // total number of disjoint sets
    UnionStrategy strategy;                 // Strategy for union operations
    
public:
    /**
     * @brief Constructs an empty DisjointSetUnion structure.
     * Initializes the number of sets to 0 and sets the union strategy.
     * @param strategy The union strategy to use (default: BY_RANK).
     */
    DisjointSetUnion(UnionStrategy strategy = UnionStrategy::BY_RANK) : numSets(0), strategy(strategy) {}
    
    /**
     * @brief Creates a new set containing only the element x.
     * If x already exists in a set, this operation does nothing.
     * @param x The element to create a set for.
     * @note Time Complexity: O(α(N)) amortized, due to the initial `parent.find` check.
     *       If element does not exist, map operations are amortized O(1) on average.
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
     * @brief Finds the representative (root) of the set containing x.
     * Applies path compression: makes every node on the find path point directly to the root.
     * If x is not present, it is added as a new set (auto-creation).
     * @param x The element to find.
     * @return The representative of the set containing x.
     * @note Time Complexity: O(α(N)) amortized (N is total elements).
     */
    T find(const T& x) {
        if (parent.find(x) == parent.end()) {
            makeSet(x); // Auto-create if doesn't exist
        }
        
        // Path compression
        if (parent[x] != x) {
            parent[x] = find(parent[x]);
        }
        
        return parent[x];
    }
    
    /**
     * @brief Merges the sets containing elements x and y.
     * Uses union by rank: attaches the root of the smaller rank tree to the root of the larger rank tree.
     * If x and y are already in the same set, this operation does nothing.
     * If x or y are not present, they are added as new sets before union.
     * @param x An element in the first set.
     * @param y An element in the second set.
     * @return True if the sets were merged (were different), false otherwise.
     * @note Time Complexity: O(α(N)) amortized.
     */
    bool unionSets(const T& x, const T& y) {
        T rootX = find(x);
        T rootY = find(y);
        
        if (rootX == rootY) {
            return false; // Already in same set
        }
        
        if (strategy == UnionStrategy::BY_RANK) {
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
        } else { // UnionStrategy::BY_SIZE
            if (setSize[rootX] < setSize[rootY]) {
                parent[rootX] = rootY;
                setSize[rootY] += setSize[rootX];
            } else if (setSize[rootX] > setSize[rootY]) {
                parent[rootY] = rootX;
                setSize[rootX] += setSize[rootY];
            } else {
                // If sizes are equal, attach rootX to rootY.
                // Rank is only incremented if using BY_RANK strategy and ranks were equal.
                // For BY_SIZE, rank is not the primary factor.
                parent[rootX] = rootY;
                setSize[rootY] += setSize[rootX];
                // If ranks were equal and strategy was BY_RANK, rank[rootY] would have been incremented.
                // We don't strictly need to update rank here for BY_SIZE, but if we were to,
                // it would be: if (rank[rootX] == rank[rootY]) rank[rootY]++;
                // For now, we only update rank if strategy is BY_RANK.
            }
        }
        
        numSets--;
        return true;
    }
    
    /**
     * @brief Checks if elements x and y are in the same set.
     * @param x The first element.
     * @param y The second element.
     * @return True if x and y are in the same set, false otherwise.
     * @note Time Complexity: O(α(N)) amortized.
     */
    bool connected(const T& x, const T& y) {
        return find(x) == find(y);
    }
    
    /**
     * @brief Gets the size of the set containing element x.
     * If x is not present, it's added as a new set, and its size (1) is returned.
     * @param x The element.
     * @return The size of the set containing x.
     * @note Time Complexity: O(α(N)) amortized.
     */
    int size(const T& x) {
        T root = find(x);
        return setSize[root];
    }
    
    /**
     * @brief Gets the total number of disjoint sets.
     * @return The number of disjoint sets.
     * @note Time Complexity: O(1).
     */
    int countSets() const {
        return numSets;
    }

    /**
     * @brief Checks if the DSU structure is empty (contains no elements/sets).
     * @return True if empty, false otherwise.
     * @note Time Complexity: O(1).
     */
    bool isEmpty() const {
        return numSets == 0; 
    }
    
    /**
     * @brief Gets the total number of elements currently in the structure.
     * @return The total number of elements.
     * @note Time Complexity: O(1) (due to `std::unordered_map::size()`).
     */
    int totalElements() const {
        return parent.size();
    }
    
    /**
     * @brief Checks if an element exists in the structure.
     * @param x The element to check.
     * @return True if the element exists, false otherwise.
     * @note Time Complexity: O(1) on average for `std::unordered_map::find()`, O(N) in the worst case.
     */
    bool contains(const T& x) const {
        return parent.find(x) != parent.end();
    }
    
    /**
     * @brief Gets all elements belonging to the same set as x.
     * If x is not present, it's added as a new set, and its members (just x) are returned.
     * @param x An element in the set.
     * @return A vector containing all members of the set of x.
     * @note Time Complexity: O(N * α(N)) where N is the total number of elements in DSU,
     *       due to iterating all elements and calling `find` for each.
     */
    std::vector<T> getSetMembers(const T& x) {
        T root = find(x); // Ensures x is in the DSU
        std::vector<T> members;
        members.reserve(size(x)); // Optimization
        for (const auto& pair : parent) {
            if (find(pair.first) == root) {
                members.push_back(pair.first);
            }
        }
        return members;
    }
    
    /**
     * @brief Gets all disjoint sets currently in the structure.
     * Each set is represented as a vector of its members.
     * @return A vector of vectors, where each inner vector is a disjoint set.
     * @note Time Complexity: O(N * α(N)) where N is the total number of elements,
     *       due to iterating all elements and calling `find` for each.
     */
    std::vector<std::vector<T>> getAllSets() {
        std::unordered_map<T, std::vector<T>, Hash, KeyEqual> setMap;
        if (parent.empty()) return {};

        for (const auto& pair : parent) {
            setMap[find(pair.first)].push_back(pair.first);
        }
        
        std::vector<std::vector<T>> result;
        result.reserve(numSets); // Optimization
        for (const auto& pair : setMap) {
            result.push_back(pair.second);
        }
        return result;
    }
    
    /**
     * @brief Resets all elements to be in individual sets.
     * Maintains all existing elements. For each element x, parent[x]=x, rank[x]=0, setSize[x]=1.
     * The number of sets becomes equal to the total number of elements.
     * @note Time Complexity: O(N) where N is the total number of elements.
     */
    void reset() {
        if (parent.empty()) return;
        numSets = parent.size(); // All elements become individual sets
        for (auto& pair : parent) { // Iterate through all existing elements
            pair.second = pair.first; // parent[element] = element
            rank[pair.first] = 0;
            setSize[pair.first] = 1;
        }
    }
    
    /**
     * @brief Removes all elements and sets from the structure.
     * Resets the DSU to its initial empty state.
     * @note Time Complexity: O(N) where N is the number of elements, due to `std::unordered_map::clear()`.
     */
    void clear() {
        parent.clear();
        rank.clear();
        setSize.clear();
        numSets = 0;
    }
    
    /**
     * @brief Manually triggers path compression for all elements.
     * This is generally not needed as `find()` performs path compression automatically.
     * Could be useful if many `find` operations are anticipated after a series of unions
     * without intervening finds.
     * @note Time Complexity: O(N * α(N)) where N is the total number of elements.
     */
    void compress() {
        for (const auto& pair : parent) {
            find(pair.first);
        }
    }
    
    /**
     * @brief Debug utility to print the internal structure information.
     * Prints total elements, number of sets, and for each element: its parent, rank, and set size.
     * Note that rank and setSize are only truly meaningful for root elements.
     */
    void printStructure() const {
        std::cout << "DSU Structure (Generic):\n";
        std::cout << "Total elements: " << totalElements() << "\n";
        std::cout << "Number of sets: " << countSets() << "\n";
        
        // To make printing more informative, print root for each element
        // This is illustrative; direct parent map iteration is also fine.
        if (parent.empty()) return;
        for (const auto& pair_outer : parent) { // Iterate to get all known elements
            T element = pair_outer.first;
            // find is not const, so we need a const_cast here for a const method.
            // However, printStructure is typically a debug utility where some const-correctness
            // might be relaxed for ease of inspection if find() must be called.
            // For truly const behavior, one might need a separate const_find or avoid calling find.
            T root = const_cast<DisjointSetUnion<T, Hash, KeyEqual>*>(this)->find(element);
            std::cout << element << ": root=" << root 
                      << ", rank=" << rank.at(root) // Rank of the root
                      << ", set_size=" << setSize.at(root) // Size of the set identified by root
                      << " (direct parent in map: " << parent.at(element) << ")"
                      << std::endl;
        }
    }

    // FOR TESTING PURPOSES ONLY
    T getDirectParent_Test(const T& x) const {
        auto it = parent.find(x);
        if (it == parent.end()) {
             // Element not found in parent map.
             throw std::out_of_range("Element not found in parent map for getDirectParent_Test");
        }
        return it->second;
    }
};

/**
 * @brief A specialized Disjoint Set Union (DSU) optimized for contiguous integers (0 to N-1).
 * 
 * Manages a collection of disjoint sets for elements in a fixed integer range.
 * It is initialized with a maximum number of elements (N) and all elements from 0 to N-1
 * are initially in their own sets.
 * This implementation uses path compression and union by rank optimizations.
 * It typically offers better performance than the generic `DisjointSetUnion<T>` for
 * integer elements due to direct vector indexing instead of hash map lookups.
 */
class FastDSU {
private:
    std::vector<int> parent; // parent[i] = parent of element i
    std::vector<int> rank;   // rank[i] = rank of the tree rooted at i (if i is a root)
    std::vector<int> setSize;// setSize[i] = size of the set rooted at i (if i is a root)
    UnionStrategy strategy;  // Strategy for union operations
    int numSets;             // Total number of disjoint sets
    int maxElement;          // The maximum number of elements (N, elements are 0 to N-1)
    
public:
    /**
     * @brief Constructs a FastDSU structure for a fixed number of elements.
     * Initializes `n` elements (0 to n-1), each in its own set.
     * @param n The total number of elements. Must be non-negative.
     * @param strategy The union strategy to use (default: BY_RANK).
     * @note Time Complexity: O(N) where N is the number of elements.
     */
    explicit FastDSU(int n, UnionStrategy strategy = UnionStrategy::BY_RANK)
        : parent(n), rank(n, 0), setSize(n, 1), strategy(strategy), numSets(n), maxElement(n) {
        assert(n >= 0);
        for (int i = 0; i < n; i++) {
            parent[i] = i;
        }
    }
    
    /**
     * @brief Finds the representative (root) of the set containing element x.
     * Applies path compression.
     * @param x The element to find (must be between 0 and maxElement-1).
     * @return The representative of the set containing x.
     * @note Time Complexity: O(α(N)) amortized (N is maxElement).
     */
    int find(int x) {
        assert(x >= 0 && x < maxElement);
        if (parent[x] != x) {
            parent[x] = find(parent[x]); // Path compression
        }
        return parent[x];
    }
    
    /**
     * @brief Merges the sets containing elements x and y.
     * Uses union by rank.
     * @param x An element in the first set (must be between 0 and maxElement-1).
     * @param y An element in the second set (must be between 0 and maxElement-1).
     * @return True if the sets were merged, false if already in the same set.
     * @note Time Complexity: O(α(N)) amortized.
     */
    bool unionSets(int x, int y) {
        assert(x >= 0 && x < maxElement && y >= 0 && y < maxElement);
        
        int rootX = find(x);
        int rootY = find(y);
        
        if (rootX == rootY) return false;
        
        if (strategy == UnionStrategy::BY_RANK) {
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
        } else { // UnionStrategy::BY_SIZE
            if (setSize[rootX] < setSize[rootY]) {
                parent[rootX] = rootY;
                setSize[rootY] += setSize[rootX];
            } else if (setSize[rootX] > setSize[rootY]) {
                parent[rootY] = rootX;
                setSize[rootX] += setSize[rootY];
            } else {
                // If sizes are equal, attach rootX to rootY.
                // Rank is only incremented if using BY_RANK strategy and ranks were equal.
                parent[rootX] = rootY;
                setSize[rootY] += setSize[rootX];
                // if (strategy == UnionStrategy::BY_RANK && rank[rootX] == rank[rootY]) {
                // This condition is only for BY_RANK, handled above.
                // For BY_SIZE, rank update isn't the primary mechanism.
                // if (rank[rootX] == rank[rootY]) rank[rootY]++; // Only if also tie-breaking by rank for BY_SIZE
            }
        }
        
        numSets--;
        return true;
    }
    
    /**
     * @brief Checks if elements x and y are in the same set.
     * @param x The first element.
     * @param y The second element.
     * @return True if x and y are in the same set, false otherwise.
     * @note Time Complexity: O(α(N)) amortized.
     */
    bool connected(int x, int y) {
        assert(x >= 0 && x < maxElement && y >= 0 && y < maxElement);
        return find(x) == find(y);
    }
    
    /**
     * @brief Gets the size of the set containing element x.
     * @param x The element.
     * @return The size of the set containing x.
     * @note Time Complexity: O(α(N)) amortized.
     */
    int size(int x) {
        assert(x >= 0 && x < maxElement);
        return setSize[find(x)];
    }
    
    /**
     * @brief Gets the total number of disjoint sets.
     * @return The number of disjoint sets.
     * @note Time Complexity: O(1).
     */
    int countSets() const {
        return numSets;
    }

    /**
     * @brief Confirms an element is part of the DSU (i.e., it was initialized).
     * For FastDSU, this is a synonym for `contains(x)` as `makeSet` doesn't add new elements.
     * This primarily serves to ensure API consistency with the generic DSU and assert element validity.
     * @param x The element to check.
     * @note Time Complexity: O(1).
     */
    void makeSet(int x) {
        assert(x >= 0 && x < maxElement);
        // Elements are initialized in the constructor. This method is a no-op for structural changes.
    }

    /**
     * @brief Checks if an element index is within the valid range [0, maxElement-1).
     * @param x The element index to check.
     * @return True if the index is valid, false otherwise.
     * @note Time Complexity: O(1).
     */
    bool contains(int x) const {
        return x >= 0 && x < maxElement;
    }

    /**
     * @brief Checks if the DSU structure is empty.
     * For FastDSU, this means it was initialized with 0 elements.
     * @return True if empty (maxElement is 0), false otherwise.
     * @note Time Complexity: O(1).
     */
    bool isEmpty() const {
        return numSets == 0; // Also equivalent to maxElement == 0 for FastDSU
    }

    /**
     * @brief Resets all elements (0 to maxElement-1) to individual sets.
     * @note Time Complexity: O(N) where N is maxElement.
     */
    void reset() {
        if (maxElement == 0) return;
        numSets = maxElement;
        for (int i = 0; i < maxElement; ++i) {
            parent[i] = i;
            rank[i] = 0;
            setSize[i] = 1;
        }
    }

    /**
     * @brief Manually triggers path compression for all elements.
     * @note Time Complexity: O(N * α(N)) where N is maxElement.
     */
    void compress() {
        for (int i = 0; i < maxElement; ++i) {
            find(i); 
        }
    }

    /**
     * @brief Gets all disjoint sets. Each set is a vector of its integer members.
     * @return A vector of vectors, where each inner vector is a disjoint set.
     * @note Time Complexity: O(N * α(N)) where N is maxElement.
     */
    std::vector<std::vector<int>> getAllSets() {
        if (maxElement == 0) return {};
        std::unordered_map<int, std::vector<int>> setMap;
        for (int i = 0; i < maxElement; ++i) {
            setMap[find(i)].push_back(i);
        }
        
        std::vector<std::vector<int>> result;
        result.reserve(numSets); 
        for (const auto& pair : setMap) {
            result.push_back(pair.second);
        }
        return result;
    }

    /**
     * @brief Debug utility to print the internal structure information.
     */
    void printStructure() const {
        std::cout << "FastDSU Structure:\n";
        std::cout << "Max elements (0 to N-1): " << maxElement << "\n";
        std::cout << "Number of sets: " << countSets() << "\n";
        if (maxElement == 0) return;
        for (int i = 0; i < maxElement; ++i) {
            // To show root's rank/size, we need to call find.
            // Be careful if find() itself modifies state and this is meant to be const.
            // For FastDSU, find() can modify parent[] for path compression.
            // A truly const print would just show parent[i], rank[i], setSize[i].
            // This version provides more insight but technically find is not const.
            int root = const_cast<FastDSU*>(this)->find(i);
             std::cout << i << ": root=" << root
                      << ", rank=" << rank[root] 
                      << ", set_size=" << setSize[root]
                      << " (direct parent in vector: " << parent[i] << ")"
                      << std::endl;
        }
    }

    // FOR TESTING PURPOSES ONLY
    int getDirectParent_Test(int x) const {
        assert(x >= 0 && x < maxElement); // This assert is correct here
        return parent[x];
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
        FastDSU dsu(n); // Use FastDSU for Kruskal's as it's typically on integer nodes
        std::vector<Edge> mst;
        
        for (const Edge& e : edges) {
            if (dsu.unionSets(e.u, e.v)) {
                mst.push_back(e);
                if (mst.size() == static_cast<typename std::vector<Edge>::size_type>(n - 1)) break; // MST has V-1 edges
            }
        }
        return mst;
    }
    
    /**
     * Detect cycle in undirected graph using DSU
     */
    bool hasCycle(int n, const std::vector<std::pair<int, int>>& edges) {
        FastDSU dsu(n);
        for (const auto& edge : edges) {
            if (!dsu.unionSets(edge.first, edge.second)) {
                return true; // If unionSets returns false, they were already connected: cycle
            }
        }
        return false;
    }
    
    /**
     * Count connected components in a graph using DSU
     */
    int countConnectedComponents(int n, const std::vector<std::pair<int, int>>& edges) {
        FastDSU dsu(n);
        for (const auto& edge : edges) {
            dsu.unionSets(edge.first, edge.second);
        }
        return dsu.countSets();
    }
}
// main() function has been moved to examples/use_dsu.cpp

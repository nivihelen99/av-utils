#pragma once

#include <vector>
#include <iostream>
#include <cassert>

/**
 * @brief A class for Fenwick Tree (Binary Indexed Tree).
 * Supports point updates and range sum queries.
 * All operations (update, set, prefixSum, query, get) take O(log n) time.
 * Construction takes O(n log n) if an initial array is provided, or O(n) if initialized with a size.
 */
class FenwickTree {
private:
    std::vector<long long> tree;
    int n;
    
    // Get the least significant bit
    int lsb(int x) const {
        return x & (-x);
    }
    
public:
    /**
     * @brief Constructs a Fenwick Tree of a given size, initialized with zeros.
     * @param size The number of elements in the array.
     * @complexity O(n) for initialization.
     */
    FenwickTree(int size) : n(size) {
        tree.assign(n + 1, 0);  // 1-indexed
    }
    
    /**
     * @brief Constructs a Fenwick Tree from an existing array of values.
     * @param arr The input array of long long values.
     * @complexity O(n log n) as it calls update for each element.
     */
    FenwickTree(const std::vector<long long>& arr) : n(arr.size()) {
        tree.assign(n + 1, 0);
        for (int i = 0; i < n; i++) {
            update(i, arr[i]);
        }
    }
    
    /**
     * @brief Adds a delta to the element at a specific index.
     * @param i The 0-based index of the element to update.
     * @param delta The value to add to the element at index i.
     * @complexity O(log n).
     */
    void update(int i, long long delta) {
        assert(i >= 0 && i < n && "Index out of bounds in update");
        i++; // Convert to 1-indexed
        while (i <= n) {
            tree[i] += delta;
            i += lsb(i);
        }
    }
    
    /**
     * @brief Sets the element at a specific index to a new value.
     * This is achieved by calculating the difference and calling update.
     * @param i The 0-based index of the element to set.
     * @param value The new value for the element at index i.
     * @complexity O(log n) due to query and update calls.
     */
    void set(int i, long long value) {
        assert(i >= 0 && i < n && "Index out of bounds in set");
        long long current = query(i, i);
        update(i, value - current);
    }
    
    /**
     * @brief Calculates the prefix sum from index 0 up to index i (inclusive).
     * @param i The 0-based index up to which the sum is calculated.
     * @return The sum of elements from index 0 to i. Returns 0 if i is -1.
     * @complexity O(log n).
     */
    long long prefixSum(int i) const {
        assert(i >= -1 && i < n && "Index out of bounds in prefixSum");
        if (i < 0) return 0;
        i++; // Convert to 1-indexed
        long long sum = 0;
        while (i > 0) {
            sum += tree[i];
            i -= lsb(i);
        }
        return sum;
    }
    
    /**
     * @brief Calculates the sum of elements in a given range [l, r] (inclusive).
     * @param l The 0-based starting index of the range.
     * @param r The 0-based ending index of the range.
     * @return The sum of elements in the range [l, r].
     * @complexity O(log n) due to two calls to prefixSum.
     */
    long long query(int l, int r) const {
        assert(l <= r && "Left index cannot be greater than right index in query");
        assert(l >= 0 && r < n && "Indices out of bounds in query");
        return prefixSum(r) - prefixSum(l - 1);
    }
    
    /**
     * @brief Retrieves the value of the element at a specific index.
     * @param i The 0-based index of the element to retrieve.
     * @return The value of the element at index i.
     * @complexity O(log n) as it calls query(i, i).
     */
    long long get(int i) const {
        assert(i >= 0 && i < n && "Index out of bounds in get");
        return query(i, i);
    }
    
    /**
     * @brief Gets the size of the array represented by the Fenwick Tree.
     * @return The number of elements.
     * @complexity O(1).
     */
    int size() const {
        return n;
    }
    
    /**
     * @brief (Debug) Prints the internal tree structure.
     * Useful for debugging purposes.
     */
    void printTree() const {
        std::cout << "Tree array: ";
        for (int i = 1; i <= n; i++) {
            std::cout << tree[i] << " ";
        }
        std::cout << '\n';
    }
    
    /**
     * @brief (Debug) Prints the array values as perceived by the Fenwick Tree.
     * Useful for debugging purposes. It reconstructs values by calling get().
     * @complexity O(n log n) due to calling get() for each element.
     */
    void printArray() const {
        std::cout << "Array values: ";
        for (int i = 0; i < n; i++) {
            std::cout << get(i) << " ";
        }
        std::cout << '\n';
    }
};


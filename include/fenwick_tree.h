#pragma once

#include <vector>
#include <iostream>
#include <cassert>

class FenwickTree {
private:
    std::vector<long long> tree;
    int n;
    
    // Get the least significant bit
    int lsb(int x) {
        return x & (-x);
    }
    
public:
    // Constructor: initialize with size n, all elements are 0
    FenwickTree(int size) : n(size) {
        tree.assign(n + 1, 0);  // 1-indexed
    }
    
    // Constructor: initialize with given array
    FenwickTree(const std::vector<int>& arr) : n(arr.size()) {
        tree.assign(n + 1, 0);
        for (int i = 0; i < n; i++) {
            update(i, arr[i]);
        }
    }
    
    // Update element at index i by adding delta (0-indexed)
    void update(int i, long long delta) {
        assert(i >= 0 && i < n);
        i++; // Convert to 1-indexed
        while (i <= n) {
            tree[i] += delta;
            i += lsb(i);
        }
    }
    
    // Set element at index i to value (0-indexed)
    void set(int i, long long value) {
        long long current = query(i, i);
        update(i, value - current);
    }
    
    // Get prefix sum from index 0 to i (inclusive, 0-indexed)
    long long prefixSum(int i) {
        assert(i >= -1 && i < n);
        if (i < 0) return 0;
        i++; // Convert to 1-indexed
        long long sum = 0;
        while (i > 0) {
            sum += tree[i];
            i -= lsb(i);
        }
        return sum;
    }
    
    // Get range sum from l to r (inclusive, 0-indexed)
    long long query(int l, int r) {
        assert(l >= 0 && r < n && l <= r);
        return prefixSum(r) - prefixSum(l - 1);
    }
    
    // Get single element value at index i (0-indexed)
    long long get(int i) {
        return query(i, i);
    }
    
    // Get the size of the array
    int size() const {
        return n;
    }
    
    // Debug: print the tree structure
    void printTree() {
        std::cout << "Tree array: ";
        for (int i = 1; i <= n; i++) {
            std::cout << tree[i] << " ";
        }
        std::cout << std::endl;
    }
    
    // Debug: print the actual array values
    void printArray() {
        std::cout << "Array values: ";
        for (int i = 0; i < n; i++) {
            std::cout << get(i) << " ";
        }
        std::cout << std::endl;
    }
};


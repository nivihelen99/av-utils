#pragma once

#include <vector>
#include <functional>
#include <stdexcept> // For std::out_of_range
#include <numeric>   // For std::iota potentially, or concepts if C++20
#include <algorithm> // For std::min, std::max

namespace cpp_utils {

template <typename T, typename CombineOp = std::plus<T>>
class SegmentTree {
public:
    // Constructor from a vector of initial values
    SegmentTree(const std::vector<T>& initial_values, CombineOp op = CombineOp(), T identity_val = T{})
        : operation_(op), identity_(identity_val), n_(initial_values.size()) {
        if (n_ == 0) {
            // Handle empty input gracefully, or throw. For now, allow empty tree.
            tree_.clear();
            return;
        }
        // The size of the segment tree array is typically 2 * 2^ceil(log2(n)) or roughly 4n
        // A simpler way for full binary tree is 2*n for the data part if 1-indexed, or if 0-indexed and leaves are at [n, 2n-1].
        // Let's use 1-based indexing for the tree array for simpler parent/child calculations.
        // Tree size will be 2*n. Nodes 1 to n-1 are internal, n to 2n-1 are leaves (if n is power of 2).
        // Or more generally, if using 0-indexing for tree array, size is ~2*next_power_of_2(n).
        // For simplicity with 1-based indexing for tree nodes and direct mapping:
        // tree_ will store values. tree_[1] is the root.
        // Leaves will correspond to original array elements.
        // Let's use a common segment tree representation: array of size ~2N (if N is power of 2) or ~4N.
        // If original array has size `n_`, tree will have size `2 * n_` (for leaves) + `2 * n_` (for internal nodes) roughly.
        // A common size is 4*n_ to be safe for recursive implementations.
        // Or, if we map array elements to the second half of the tree array:
        // tree size of 2*n_ where leaves are at tree_[n_...2*n_-1]. tree_[0] unused or stores size.
        // Let's use the latter 0-indexed style for the tree array.
        // Leaves at [n_, 2n_-1], root at tree_[1] if we build it like a heap.
        // This is a common way: tree of size 2*n. tree[i] = tree[2*i] + tree[2*i+1].
        // Leaves are from n to 2n-1.
        // tree[i+n] = initial_values[i]

        original_size_ = n_; // Store original size if needed for query validation
        tree_.resize(2 * n_);

        // Initialize leaves
        for (size_t i = 0; i < n_; ++i) {
            tree_[n_ + i] = initial_values[i];
        }
        // Build the tree by calculating parent nodes
        for (size_t i = n_ - 1; i > 0; --i) {
            tree_[i] = operation_(tree_[2 * i], tree_[2 * i + 1]);
        }
    }

    // Constructor with size and default value
    SegmentTree(size_t count, const T& default_value, CombineOp op = CombineOp(), T identity_val = T{})
        : operation_(op), identity_(identity_val), n_(count) {
        if (n_ == 0) {
            tree_.clear();
            return;
        }
        original_size_ = n_;
        tree_.resize(2 * n_);
        for (size_t i = 0; i < n_; ++i) {
            tree_[n_ + i] = default_value;
        }
        for (size_t i = n_ - 1; i > 0; --i) {
            tree_[i] = operation_(tree_[2 * i], tree_[2 * i + 1]);
        }
    }


    // Updates the element at original_index to new_value
    void update(size_t original_index, const T& new_value) {
        if (original_index >= original_size_) {
            throw std::out_of_range("SegmentTree::update index out of bounds");
        }
        if (n_ == 0) return; // Nothing to update in an empty tree

        size_t tree_idx = n_ + original_index;
        tree_[tree_idx] = new_value;

        // Propagate changes upwards
        while (tree_idx > 1) {
            tree_idx /= 2; // Move to parent
            tree_[tree_idx] = operation_(tree_[2 * tree_idx], tree_[2 * tree_idx + 1]);
        }
    }

    // Queries the range [range_left, range_right)
    // range_left is inclusive, range_right is exclusive
    T query(size_t range_left, size_t range_right) const {
        if (range_left >= range_right || range_right > original_size_) {
             // Consider how to handle invalid ranges. Throw, or return identity?
             // If range_left == range_right, it's an empty range, should return identity.
             // If range_left > range_right, invalid.
             // If range_right > original_size, invalid.
            if (range_left == range_right && range_left <= original_size_) { // Empty range is valid
                return identity_;
            }
            throw std::out_of_range("SegmentTree::query range invalid");
        }
        if (n_ == 0) return identity_;


        T result = identity_;
        size_t l = n_ + range_left;
        size_t r = n_ + range_right; // exclusive, so points one past the last element in tree terms

        while (l < r) {
            if (l % 2 == 1) { // If l is odd, it's a right child, include its value
                result = operation_(result, tree_[l]);
                l++; // Move to next element, which will be start of a new parent's range
            }
            if (r % 2 == 1) { // If r is odd, it's a right child.
                              // The actual element is r-1. Include tree_[r-1]
                r--; // r is exclusive, so tree_[r-1] is the last element in this block
                result = operation_(result, tree_[r]);
            }
            // Move to parents
            l /= 2;
            r /= 2;
        }
        return result;
    }

    size_t size() const {
        return original_size_;
    }

    bool empty() const {
        return original_size_ == 0;
    }

private:
    CombineOp operation_;
    T identity_; // Identity element for the operation (e.g., 0 for sum, infinity for min)
    std::vector<T> tree_;
    size_t n_; // Size of the original input array, also used for tree indexing logic
    size_t original_size_; // To store the actual number of elements user thinks they have
};

// Helper for common operations like min/max where T might not have a default identity
// For std::plus<int>, T{} (which is 0) is fine.
// For std::minimum<int>, T{} (0) might not be appropriate if array contains negative numbers.
// User should provide a correct identity value.

// Example typical CombineOp for min
template <typename T>
struct MinOp {
    T operator()(const T& a, const T& b) const {
        return std::min(a, b);
    }
};

// Example typical CombineOp for max
template <typename T>
struct MaxOp {
    T operator()(const T& a, const T& b) const {
        return std::max(a, b);
    }
};

} // namespace cpp_utils

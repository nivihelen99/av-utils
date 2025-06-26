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
        : operation_(op), identity_(identity_val), n_(initial_values.size()), original_size_(initial_values.size()) {
        if (n_ == 0) {
            // For an empty initial_values, tree_ remains empty, n_ and original_size_ are 0.
            // query(0,0) should still work and return identity_.
            return;
        }
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
        : operation_(op), identity_(identity_val), n_(count), original_size_(count) {
        if (n_ == 0) {
            return;
        }
        tree_.resize(2 * n_);
        for (size_t i = 0; i < n_; ++i) {
            tree_[n_ + i] = default_value;
        }
        if (n_ > 0) { // Avoid building if n_ is 0, though loop condition i > 0 would handle it.
            for (size_t i = n_ - 1; i > 0; --i) {
                tree_[i] = operation_(tree_[2 * i], tree_[2 * i + 1]);
            }
        }
    }


    // Updates the element at original_index to new_value
    void update(size_t original_index, const T& new_value) {
        if (original_index >= original_size_) {
            throw std::out_of_range("SegmentTree::update index out of bounds");
        }
        // If original_size_ is 0, the above check throws, so n_ must be > 0 here.
        // Thus, tree_ is not empty.

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
        if (range_left > original_size_ || range_right > original_size_ || range_left > range_right) {
            throw std::out_of_range("SegmentTree::query range invalid");
        }
        if (range_left == range_right) { // Empty range is valid and should return identity
            return identity_;
        }
        // At this point, range_left < range_right and both are within [0, original_size_]
        // And original_size_ (and thus n_) must be > 0 for a non-empty range.

        T result = identity_;
        size_t l = n_ + range_left;
        size_t r = n_ + range_right; // exclusive, so points one past the last element in tree terms

        while (l < r) {
            if (l % 2 == 1) { // If l is odd, it's a right child, include its value
                result = operation_(result, tree_[l]);
                l++;
            }
            if (r % 2 == 1) {
                r--;
                result = operation_(result, tree_[r]);
            }
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
    T identity_;
    std::vector<T> tree_;
    size_t n_; // Stores original_size for tree calculation convenience, could be same as original_size_
    size_t original_size_;
};

template <typename T>
struct MinOp {
    T operator()(const T& a, const T& b) const {
        return std::min(a, b);
    }
};

template <typename T>
struct MaxOp {
    T operator()(const T& a, const T& b) const {
        return std::max(a, b);
    }
};

} // namespace cpp_utils

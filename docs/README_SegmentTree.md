# Segment Tree (`segment_tree.h`)

## Overview

The `SegmentTree` class template in `segment_tree.h` provides a versatile data structure for efficient range queries and point updates on an array. It can support various associative operations like sum, minimum, maximum, product, GCD, etc., over specified ranges.

A Segment Tree is a binary tree where each leaf node represents an element of the input array. Each internal node represents the combined result of its children's ranges. This structure allows for both queries and updates to be performed in O(log N) time, where N is the number of elements in the array.

## Template Parameters

```cpp
template <typename T, typename CombineOp = std::plus<T>>
class SegmentTree {
    // ...
};
```

*   `T`: The type of data stored in the segment tree (e.g., `int`, `double`, custom struct).
*   `CombineOp`: A binary function object (or lambda, function pointer) that takes two arguments of type `T` and returns their combined value. This operation **must be associative**. Defaults to `std::plus<T>`.

## Public API

### Constructors

1.  **From an initial set of values:**
    ```cpp
    SegmentTree(const std::vector<T>& initial_values,
                CombineOp op = CombineOp(),
                T identity_val = T{});
    ```
    *   `initial_values`: A `std::vector<T>` containing the initial data.
    *   `op`: The combine operation to be used (e.g., `std::plus<int>()`, `MinOp<int>()`).
    *   `identity_val`: The identity element for the `CombineOp`. This is the value that, when combined with any element `x`, results in `x`. For example:
        *   `0` for addition.
        *   `1` for multiplication.
        *   `std::numeric_limits<T>::max()` for minimum operations on numeric types.
        *   `std::numeric_limits<T>::min()` (or lowest possible value) for maximum operations.
        *   A default-constructed `T` might be suitable if `T{}` is the identity (like for `std::plus<int>`).
        *   For an empty `initial_values` vector, the tree is initialized as empty. `query(0,0)` on such a tree will return `identity_val`.

2.  **With a specific size and default value:**
    ```cpp
    SegmentTree(size_t count,
                const T& default_value,
                CombineOp op = CombineOp(),
                T identity_val = T{});
    ```
    *   `count`: The number of elements in the conceptual array.
    *   `default_value`: The value to initialize all elements with.
    *   `op` and `identity_val`: Same as above.
    *   If `count` is 0, the tree is initialized as empty. `query(0,0)` on such a tree will return `identity_val`.

### Methods

*   **`void update(size_t original_index, const T& new_value)`**
    Updates the element at `original_index` in the underlying data to `new_value`.
    Propagates this change up the tree.
    *   Complexity: O(log N)
    *   Throws `std::out_of_range` if `original_index` is out of bounds.

*   **`T query(size_t range_left, size_t range_right) const`**
    Queries the combined value of elements in the range `[range_left, range_right)`.
    The range is inclusive of `range_left` and exclusive of `range_right`.
    *   Complexity: O(log N)
    *   Throws `std::out_of_range` if the range is invalid (e.g., `range_left > range_right`, or `range_left` or `range_right` exceeds the size).
    *   If `range_left == range_right` (an empty range that is within bounds, e.g., `query(k,k)` where `0 <= k <= size()`), it returns the `identity_val`.

*   **`size_t size() const`**
    Returns the number of elements in the segment tree (i.e., the size of the original array).
    *   Complexity: O(1)

*   **`bool empty() const`**
    Returns `true` if the segment tree is empty (size is 0), `false` otherwise.
    *   Complexity: O(1)

## Custom Operations

The `CombineOp` can be any callable that is associative. Common examples include:

*   `std::plus<T>()` for range sum.
*   `std::multiplies<T>()` for range product.
*   Custom functors for range minimum/maximum (example helpers `MinOp<T>` and `MaxOp<T>` are provided in `segment_tree.h`):
    ```cpp
    // Provided in segment_tree.h
    template <typename T>
    struct MinOp {
        T operator()(const T& a, const T& b) const { return std::min(a, b); }
    };

    template <typename T>
    struct MaxOp {
        T operator()(const T& a, const T& b) const { return std::max(a, b); }
    };
    ```
    When using `MinOp`, the identity value should typically be the largest possible value of `T`. For `MaxOp`, it should be the smallest.

## Example Usage

```cpp
#include "segment_tree.h"
#include <iostream>
#include <vector>
#include <limits> // For std::numeric_limits

// For MinOp (can be included from segment_tree.h or defined locally)
// using cpp_utils::MinOp;

int main() {
    // Example 1: Range Sum
    std::vector<int> data = {1, 2, 3, 4, 5};
    cpp_utils::SegmentTree<int> sum_st(data, std::plus<int>(), 0);

    std::cout << "Sum [0, 5): " << sum_st.query(0, 5) << std::endl; // Output: 15
    std::cout << "Sum [1, 3): " << sum_st.query(1, 3) << std::endl; // Output: 2 + 3 = 5

    sum_st.update(2, 10); // Data becomes {1, 2, 10, 4, 5}
    std::cout << "After update, Sum [0, 5): " << sum_st.query(0, 5) << std::endl; // Output: 1+2+10+4+5 = 22

    // Example 2: Range Minimum
    std::vector<int> rmq_data = {5, 2, 8, 1, 9};
    // Using the MinOp from cpp_utils namespace (defined in segment_tree.h)
    cpp_utils::SegmentTree<int, cpp_utils::MinOp<int>> min_st(
        rmq_data, cpp_utils::MinOp<int>(), std::numeric_limits<int>::max()
    );

    std::cout << "Min [0, 5): " << min_st.query(0, 5) << std::endl; // Output: 1
    std::cout << "Min [0, 2): " << min_st.query(0, 2) << std::endl; // Output: min(5, 2) = 2

    min_st.update(3, 0); // rmq_data becomes {5, 2, 8, 0, 9}
    std::cout << "After update, Min [0, 5): " << min_st.query(0, 5) << std::endl; // Output: 0

    // Example 3: Empty tree and query(0,0)
    std::vector<int> empty_vec;
    cpp_utils::SegmentTree<int> empty_st(empty_vec, std::plus<int>(), 0);
    std::cout << "Query [0,0) on empty tree: " << empty_st.query(0,0) << std::endl; // Output: 0 (identity)
}
```

## Time and Space Complexity

*   **Space Complexity:** O(N) - The tree uses an internal vector of size 2N.
*   **Build Time:** O(N) - When constructing from initial values.
*   **Query Time:** O(log N)
*   **Update Time:** O(log N)

## Notes

*   The `CombineOp` **must** be associative: `op(op(a, b), c)` must be equal to `op(a, op(b, c))`.
*   The `identity_val` provided to the constructor must be a true identity element for the `CombineOp`.
*   The implementation uses a 0-indexed `tree_` vector of size `2*n_` (where `n_` is the original array size). Leaf `i` (0-indexed from original array) is at `tree_[n_ + i]`. The parent of `tree_[k]` is `tree_[k/2]`. Children of `tree_[k]` are `tree_[2*k]` and `tree_[2*k+1]`. The root is `tree_[1]` (if `n_ > 0`). `tree_[0]` is unused in this particular style if `n_ > 0`.

```

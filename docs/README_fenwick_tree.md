# `FenwickTree` (Binary Indexed Tree)

## Overview

The `FenwickTree` class (`fenwick_tree.h`) implements a Fenwick Tree, also known as a Binary Indexed Tree (BIT). This data structure is designed for efficient processing of two main operations on an array of numbers:
1.  **Point Update:** Modifying the value of an element at a specific index.
2.  **Prefix Sum Query:** Calculating the sum of elements from the beginning of the array up to a specific index.

Using prefix sums, range sum queries (sum of elements in a range `[l, r]`) can also be performed efficiently. Both update and query operations have a time complexity of O(log N), where N is the size of the array.

This implementation stores `long long` values and uses 0-based indexing for its public API, converting to 1-based indexing internally, which is common for Fenwick Tree algorithms.

## Features

-   **Efficient Operations:** O(log N) for point updates, point set, prefix sum queries, and range sum queries.
-   **Space Complexity:** O(N).
-   **Initialization:** Can be initialized with a specific size (all elements to zero) or from an existing `std::vector<long long>`.
-   **User-Friendly Indexing:** Public API uses 0-based indexing.
-   **Core Functionality:**
    -   Add a delta to an element (`update`).
    -   Set an element to a new value (`set`).
    -   Get the value of an element (`get`).
    -   Calculate sum up to an index (`prefixSum`).
    -   Calculate sum over a range (`query`).
-   **Debugging Utilities:** Includes methods to print the internal tree structure and the conceptual array values.

## Public Interface

### Constructors
-   **`FenwickTree(int size)`**:
    -   Constructs a tree for `size` elements, all initialized to 0.
    -   Complexity: O(N).
-   **`FenwickTree(const std::vector<long long>& arr)`**:
    -   Constructs a tree from the values in `arr`.
    -   Complexity: O(N log N).

### Operations
-   **`void update(int i, long long delta)`**:
    -   Adds `delta` to the element at 0-based index `i`.
    -   Complexity: O(log N).
-   **`void set(int i, long long value)`**:
    -   Sets the element at 0-based index `i` to `value`.
    -   Complexity: O(log N).
-   **`long long prefixSum(int i)`**:
    -   Calculates sum of elements from index 0 to `i` (inclusive). Returns 0 if `i < 0`.
    -   Complexity: O(log N).
-   **`long long query(int l, int r)`**:
    -   Calculates sum of elements in the 0-based range `[l, r]` (inclusive).
    -   Complexity: O(log N).
-   **`long long get(int i)`**:
    -   Retrieves the current value of the element at 0-based index `i`.
    -   Complexity: O(log N).
-   **`int size() const`**:
    -   Returns the number of elements the tree manages.
    -   Complexity: O(1).

### Debugging
-   **`void printTree()`**: Prints the internal representation of the Fenwick tree.
-   **`void printArray()`**: Reconstructs and prints the conceptual array values (O(N log N)).

## Usage Examples

### Basic Initialization and Updates

```cpp
#include "fenwick_tree.h" // Assuming fenwick_tree.h is in the include path
#include <iostream>
#include <vector>

int main() {
    // Initialize with a size (all zeros)
    FenwickTree ft1(5); // Manages 5 elements: indices 0, 1, 2, 3, 4

    ft1.update(0, 10); // arr[0] += 10  => arr = [10, 0, 0, 0, 0]
    ft1.update(2, 5);  // arr[2] += 5   => arr = [10, 0, 5, 0, 0]
    ft1.update(4, 3);  // arr[4] += 3   => arr = [10, 0, 5, 0, 3]

    std::cout << "Fenwick Tree 1 (after updates):" << std::endl;
    ft1.printArray(); // Expected: Array values: 10 0 5 0 3

    // Initialize from a vector
    std::vector<long long> initial_values = {1, 2, 3, 4, 5};
    FenwickTree ft2(initial_values);

    std::cout << "\nFenwick Tree 2 (from vector {1,2,3,4,5}):" << std::endl;
    ft2.printArray(); // Expected: Array values: 1 2 3 4 5
}
```

### Prefix Sum and Range Queries

```cpp
#include "fenwick_tree.h"
#include <iostream>
#include <vector>

int main() {
    std::vector<long long> data = {2, 1, 1, 3, 2, 3, 4, 5, 6, 7, 8, 9};
    FenwickTree ft(data);

    std::cout << "Initial array:" << std::endl;
    ft.printArray(); // 2 1 1 3 2 3 4 5 6 7 8 9

    // Prefix sums
    std::cout << "Prefix sum up to index 0 (arr[0]): " << ft.prefixSum(0) << std::endl;             // 2
    std::cout << "Prefix sum up to index 3 (arr[0]..arr[3]): " << ft.prefixSum(3) << std::endl;     // 2+1+1+3 = 7
    std::cout << "Prefix sum up to index 5 (arr[0]..arr[5]): " << ft.prefixSum(5) << std::endl;     // 2+1+1+3+2+3 = 12

    // Range queries
    // Sum of arr[2..5] = arr[2]+arr[3]+arr[4]+arr[5]
    std::cout << "Sum of range [2, 5]: " << ft.query(2, 5) << std::endl; // 1+3+2+3 = 9
    // Sum of arr[0..0]
    std::cout << "Sum of range [0, 0] (get value at index 0): " << ft.query(0, 0) << std::endl; // 2
    std::cout << "Value at index 0 (using get): " << ft.get(0) << std::endl; // 2

    // Update a value and see query changes
    ft.update(3, 2); // arr[3] was 3, now 3+2=5. Array: 2 1 1 5 2 3 4 5 6 7 8 9
    std::cout << "\nAfter updating index 3 by +2:" << std::endl;
    ft.printArray();
    std::cout << "New prefix sum up to index 3: " << ft.prefixSum(3) << std::endl; // 2+1+1+5 = 9
    std::cout << "New sum of range [2, 5]: " << ft.query(2, 5) << std::endl;       // 1+5+2+3 = 11
}
```

### Setting Values

```cpp
#include "fenwick_tree.h"
#include <iostream>

int main() {
    FenwickTree ft(5); // [0, 0, 0, 0, 0]

    ft.set(1, 100); // Sets arr[1] = 100
    ft.set(3, 50);  // Sets arr[3] = 50
                    // Array becomes: [0, 100, 0, 50, 0]

    std::cout << "Array after set operations:" << std::endl;
    ft.printArray();

    std::cout << "Value at index 1: " << ft.get(1) << std::endl; // 100
    std::cout << "Prefix sum up to index 2: " << ft.prefixSum(2) << std::endl; // 0+100+0 = 100
    std::cout << "Sum of range [1, 3]: " << ft.query(1, 3) << std::endl; // 100+0+50 = 150
}
```

## Dependencies
- `<vector>`
- `<iostream>` (for print utilities)
- `<cassert>` (for assertions)

The Fenwick Tree is a powerful tool for problems involving frequent point updates and range sum queries on a list of numbers.

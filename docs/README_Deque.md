# Deque

## Overview

The `ankerl::Deque<T>` is a sequence container that provides functionality similar to `std::deque`. It allows for efficient insertion and deletion of elements at both its beginning (front) and its end (back). Unlike `std::vector`, which only offers efficient appends/removes at the end, `Deque` provides O(1) amortized time complexity for front operations as well.

This implementation uses a circular buffer (dynamic array) as its underlying storage, which allows for efficient use of memory and cache-friendly iteration when elements are contiguous.

## Features

-   **Template-based**: Can store elements of any type `T`.
-   **Dynamic Sizing**: Automatically grows as elements are added.
-   **Efficient Insertions/Deletions**:
    -   Amortized O(1) for `push_front`, `pop_front`, `push_back`, `pop_back`.
-   **Element Access**:
    -   O(1) for `front()`, `back()`.
    -   O(1) for `operator[]` (no bounds checking).
    -   O(1) for `at()` (with bounds checking, throws `std::out_of_range`).
-   **Iterators**:
    -   Provides `std::random_access_iterator` compatible iterators (`begin`, `end`, `cbegin`, `cend`).
-   **Standard Interface**:
    -   `empty()`: Checks if the deque is empty.
    -   `size()`: Returns the number of elements.
    -   `clear()`: Removes all elements.
-   **Constructors**:
    -   Default constructor.
    -   Constructor with count and value.
    -   Constructor from `std::initializer_list`.
    -   Copy and move constructors.
-   **Assignment**:
    -   Copy and move assignment operators.

## Usage

### Include Header

```cpp
#include "deque.h" // Adjust path as necessary
```

### Creating a Deque

```cpp
// Default constructor
ankerl::Deque<int> d1;

// Constructor with initial size and value
ankerl::Deque<std::string> d2(5, "hello"); // Creates a deque with 5 copies of "hello"

// Constructor with initializer list
ankerl::Deque<double> d3 = {1.0, 2.5, 3.14, 0.99};
```

### Adding Elements

```cpp
ankerl::Deque<int> d;
d.push_back(10);    // d: [10]
d.push_back(20);    // d: [10, 20]
d.push_front(5);    // d: [5, 10, 20]
d.push_front(0);    // d: [0, 5, 10, 20]
```

### Accessing Elements

```cpp
ankerl::Deque<int> d = {0, 5, 10, 20};

int first = d.front(); // first = 0
int last = d.back();   // last = 20

int val_at_idx1 = d[1];       // val_at_idx1 = 5 (no bounds check)
int val_at_idx2 = d.at(2);    // val_at_idx2 = 10 (bounds check)

// Modifying an element
d[1] = 7; // d: [0, 7, 10, 20]
```

### Removing Elements

```cpp
ankerl::Deque<int> d = {0, 5, 10, 20};

d.pop_front(); // d: [5, 10, 20]
d.pop_back();  // d: [5, 10]
```

### Iterating

```cpp
ankerl::Deque<int> d = {1, 2, 3, 4, 5};

// Using a range-based for loop (C++11 and later)
for (int val : d) {
    std::cout << val << " ";
}
// Output: 1 2 3 4 5

// Using iterators explicitly
for (ankerl::Deque<int>::iterator it = d.begin(); it != d.end(); ++it) {
    std::cout << *it << " ";
}
// Output: 1 2 3 4 5
```

### Other Operations

```cpp
ankerl::Deque<int> d = {10, 20};
std::cout << "Size: " << d.size() << std::endl; // Output: Size: 2
std::cout << "Empty: " << (d.empty() ? "true" : "false") << std::endl; // Output: Empty: false

d.clear();
std::cout << "Size after clear: " << d.size() << std::endl; // Output: Size after clear: 0
```

## When to Use

-   When you need efficient additions and removals from both ends of a sequence.
-   As a queue (FIFO) or a stack (LIFO).
-   For algorithms like Breadth-First Search (BFS) where elements are added to one end and removed from the other.
-   Sliding window algorithms.

## Comparison with `std::deque`

-   **API Similarity**: This `ankerl::Deque` aims to provide a similar interface to `std::deque`.
-   **Underlying Implementation**: `std::deque` is often implemented as a collection of individually allocated fixed-size arrays (a block map), which can provide more stable iterator/reference validity upon insertions/deletions in the middle (though `ankerl::Deque` does not currently support O(1) middle insertions/deletions). This `ankerl::Deque` uses a single contiguous circular buffer that resizes, which might offer better cache performance for sequential access if elements remain relatively localized in memory.
-   **Performance**: Performance characteristics can vary. `ankerl::Deque`'s single array approach might be faster for pure front/back operations if reallocations are infrequent, while `std::deque` might handle many small allocations more gracefully. Benchmarking specific use cases is recommended.

## Future Considerations
*   Allocator support.
*   `emplace` methods.
*   `shrink_to_fit` method.
*   More extensive iterator invalidation guarantees documentation.

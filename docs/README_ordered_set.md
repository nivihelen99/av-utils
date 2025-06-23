# `OrderedSet<T, Hash, KeyEqual>`

## Overview

The `OrderedSet<T, ...>` class (`ordered_set.h`) implements a set data structure that maintains two key properties:
1.  **Uniqueness:** Like `std::set` or `std::unordered_set`, it stores only unique elements.
2.  **Insertion Order Preservation:** Unlike standard C++ sets, `OrderedSet` remembers and preserves the order in which elements were originally inserted. Iteration over an `OrderedSet` yields elements in this insertion order.

It achieves this by internally using a `std::list` to maintain the insertion sequence and an `std::unordered_map` to ensure uniqueness and provide fast lookups (O(1) average) for operations like `contains`, `insert`, and `erase`.

## Template Parameters

-   `T`: The type of elements to be stored. This type must be:
    -   Hashable (compatible with `std::hash<T>` or a custom `Hash` functor).
    -   EqualityComparable (compatible with `std::equal_to<T>` or a custom `KeyEqual` functor).
-   `Hash`: The hash function type for elements of type `T`. Defaults to `std::hash<T>`.
-   `KeyEqual`: The equality comparison function type for elements of type `T`. Defaults to `std::equal_to<T>`.

## Features

-   **Uniqueness of Elements:** Automatically handles duplicate insertions.
-   **Insertion Order Maintained:** Iterators traverse elements in the order they were inserted.
-   **Fast Lookups:** `contains()` operation is O(1) on average.
-   **Efficient Insertions/Erasures (on average):** `insert()` and `erase()` are O(1) on average, thanks to the `std::unordered_map` backing.
-   **STL-like Interface:** Provides common methods like `insert()`, `erase()`, `contains()`, `clear()`, `size()`, `empty()`, and iterators (`begin`, `end`, `rbegin`, `rend`, and their const versions).
-   **Safe Element Access:** `front()` and `back()` methods return `const_reference` to prevent accidental modification that could break internal map invariants.
-   **Copy and Move Semantics:** Supports proper copy and move construction/assignment.
-   **Utility Methods:** `as_vector()` to get a `std::vector` copy, `merge()` to combine with another `OrderedSet`.
-   **Comparison:** `operator==` and `operator!=` compare both content and order.

**Important Note on Modifying Elements via Iterators:**
While non-const iterators (`begin()`, `end()`) are provided and allow modification of elements if `T` itself is mutable, users must be extremely cautious. If an element's value is changed in a way that alters its hash code or its equality comparison result relative to other elements, the internal `std::unordered_map`'s invariants will be violated, leading to undefined behavior. It is generally safer to treat elements as immutable once inserted or to erase and re-insert if such modifications are needed.

## Public Interface Highlights

### Constructors & Assignment
-   **`OrderedSet()`**: Default constructor.
-   **`OrderedSet(std::initializer_list<value_type> ilist)`**: Constructs from an initializer list.
-   Copy/Move constructors and assignment operators.

### Core Operations
-   **`std::pair<iterator, bool> insert(const value_type& value)` / `insert(value_type&& value)`**:
    -   Inserts `value` if not already present. Preserves insertion order.
    -   Returns a pair: an iterator to the (possibly newly inserted) element and a boolean indicating if insertion took place (`true` if new, `false` if duplicate).
-   **`size_type erase(const key_type& key)`**:
    -   Removes `key` from the set. Returns 1 if an element was removed, 0 otherwise.
-   **`bool contains(const key_type& key) const`**:
    -   Checks if `key` is present in the set.
-   **`void clear() noexcept`**: Removes all elements.
-   **`size_type size() const noexcept`**: Returns the number of elements.
-   **`bool empty() const noexcept`**: Checks if the set is empty.

### Iterators (Preserve Insertion Order)
-   **`iterator begin() / end() noexcept`**
-   **`const_iterator cbegin() / cend() const noexcept`**
-   **`reverse_iterator rbegin() / rend() noexcept`**
-   **`const_reverse_iterator crbegin() / crend() const noexcept`**

### Element Access
-   **`const_reference front() const`**: Returns a const reference to the first (oldest) inserted element. Throws `std::out_of_range` if empty.
    *(A non-const version `reference front()` is also provided but returns `const value_type&` for safety.)*
-   **`const_reference back() const`**: Returns a const reference to the last (newest) inserted element. Throws `std::out_of_range` if empty.
    *(A non-const version `reference back()` is also provided but returns `const value_type&` for safety.)*

### Utilities & Comparison
-   **`std::vector<value_type> as_vector() const`**: Returns a `std::vector` of the elements in insertion order.
-   **`void merge(const OrderedSet& other)` / `void merge(OrderedSet&& other)`**: Merges elements from `other` set.
-   **`bool operator==(const OrderedSet& other) const` / `bool operator!=(const OrderedSet& other) const`**: Compares for equality (both content and order must match).

## Usage Examples

(Based on `tests/test_ordered_set.cpp`)

### Basic Operations

```cpp
#include "ordered_set.h" // Adjust path as needed
#include <iostream>
#include <string>
#include <vector>

int main() {
    OrderedSet<std::string> visited_urls;

    visited_urls.insert("http://example.com/page1");
    visited_urls.insert("http://example.com/page2");
    auto result = visited_urls.insert("http://example.com/page1"); // Duplicate

    if (!result.second) {
        std::cout << "'" << *result.first << "' was already present." << std::endl;
    }

    std::cout << "Set size: " << visited_urls.size() << std::endl; // Expected: 2

    if (visited_urls.contains("http://example.com/page2")) {
        std::cout << "Contains page2." << std::endl;
    }

    std::cout << "Visited URLs in order:" << std::endl;
    for (const std::string& url : visited_urls) {
        std::cout << "  " << url << std::endl;
    }
    // Expected order: page1, then page2

    if (!visited_urls.empty()) {
        std::cout << "First visited: " << visited_urls.front() << std::endl;
        std::cout << "Last visited: " << visited_urls.back() << std::endl;
    }

    visited_urls.erase("http://example.com/page1");
    std::cout << "After erasing page1, first visited: " << visited_urls.front() << std::endl;
}
```

### Using Custom Hashable Types

```cpp
#include "ordered_set.h"
#include <iostream>
#include <string>
#include <vector>

struct Point {
    int x, y;
    bool operator==(const Point& other) const { return x == other.x && y == other.y; }
};

// Required for std::unordered_map when Point is a key
struct PointHash {
    std::size_t operator()(const Point& p) const {
        auto h1 = std::hash<int>{}(p.x);
        auto h2 = std::hash<int>{}(p.y);
        return h1 ^ (h2 << 1); // Simple hash combination
    }
};

std::ostream& operator<<(std::ostream& os, const Point& p) { // For printing
    return os << "(" << p.x << "," << p.y << ")";
}

int main() {
    OrderedSet<Point, PointHash> unique_points;
    unique_points.insert({1, 1});
    unique_points.insert({2, 2});
    unique_points.insert({1, 1}); // Duplicate, ignored

    std::cout << "Unique points in order of insertion:" << std::endl;
    for (const Point& p : unique_points) {
        std::cout << "  " << p << std::endl;
    }
    // Expected: (1,1), then (2,2)
}
```

## Dependencies
- `<list>`, `<unordered_map>`, `<vector>`, `<initializer_list>`, `<iterator>`, `<stdexcept>`, `<algorithm>`, `<utility>`

`OrderedSet` is a useful container when both element uniqueness and preservation of insertion order are required, offering a blend of `std::set` and `std::list` characteristics with efficient average-case performance for its main operations.

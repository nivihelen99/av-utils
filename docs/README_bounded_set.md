# `BoundedSet<T>`

## Overview

The `BoundedSet<T>` class (`bounded_set.h`) implements a specialized set data structure with the following key characteristics:
-   **Fixed Capacity:** The set has a maximum number of elements it can hold, defined at construction or modified later.
-   **Insertion Order Maintained:** Elements are stored and can be iterated in the order they were inserted.
-   **Uniqueness:** Like a standard set, duplicate elements are not allowed.
-   **FIFO Eviction:** When the set is at capacity and a new, unique element is inserted, the oldest element (the one that was inserted first among the current elements) is automatically evicted to make space.

This data structure is particularly useful for managing collections of recent items, such as caches, history lists, or tracking recent events where only the latest N unique items are relevant.

## Template Parameter
-   `T`: The type of elements to be stored. `T` must be hashable (for use with `std::unordered_map`) and equality-comparable.

## Features
-   Efficient average time complexity for common operations:
    -   `insert()`: O(1) on average.
    -   `contains()`: O(1) on average.
    -   `erase()`: O(1) on average.
    -   Eviction of the oldest element: O(1).
-   Provides iterators to access elements in insertion order (oldest to newest).
-   Allows access to the oldest (`front()`) and newest (`back()`) elements.
-   Capacity can be changed dynamically using `reserve()`.

## Internal Implementation
-   A `std::list<T>` (`order_`) is used to maintain the insertion order of elements and allow efficient O(1) removal from the front (oldest) and addition to the back (newest).
-   An `std::unordered_map<T, typename std::list<T>::iterator>` (`index_`) is used for fast O(1) average time lookups (to check for existence and to get an iterator for removal from the `std::list`).

## Public Interface

### Constructor
-   **`explicit BoundedSet(size_type capacity)`**
    -   Constructs a `BoundedSet` with the specified maximum `capacity`.
    -   Throws `std::invalid_argument` if `capacity` is 0.

### Modifiers
-   **`bool insert(const T& value)`**
    -   Inserts `value` into the set.
    -   If `value` already exists, it's a no-op and returns `false`.
    -   If `value` is new:
        -   It's added as the newest element.
        -   If the set was at capacity, the oldest element is evicted.
    -   Returns `true` if `value` was newly inserted.
-   **`bool erase(const T& value)`**
    -   Removes `value` from the set. Returns `true` if removed, `false` if not found.
-   **`void clear()`**
    -   Removes all elements.
-   **`void reserve(size_type new_capacity)`**
    -   Changes the maximum capacity. If `new_capacity` is smaller than the current size, oldest elements are evicted. Throws `std::invalid_argument` if `new_capacity` is 0.
-   **`void shrink_to_fit()`**
    -   Explicitly evicts oldest elements if the current size exceeds the maximum capacity.

### Capacity
-   **`size_type size() const`**: Returns the current number of elements.
-   **`size_type capacity() const`**: Returns the maximum capacity.
-   **`bool empty() const`**: Checks if the set is empty.

### Element Access
-   **`bool contains(const T& value) const`**: Checks if `value` is in the set.
-   **`const T& front() const`**: Returns a const reference to the oldest element. Throws `std::runtime_error` if empty.
-   **`const T& back() const`**: Returns a const reference to the newest element. Throws `std::runtime_error` if empty.

### Iterators
-   **`const_iterator begin() const`**: Returns a const iterator to the oldest element.
-   **`const_iterator end() const`**: Returns a const iterator to one past the newest element.
    (Allows iteration from oldest to newest).

### Utility
-   **`std::vector<T> as_vector() const`**: Returns a `std::vector` containing all elements in their insertion order (oldest to newest).

## Usage Examples

(Based on `examples/bounded_set_example.cpp`)

### Basic Operations and Eviction

```cpp
#include "bounded_set.h"
#include <iostream>
#include <string>

void print_set(const BoundedSet<int>& s, const std::string& name) {
    std::cout << name << " (size=" << s.size() << ", cap=" << s.capacity() << "): ";
    for (int val : s) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
}

int main() {
    BoundedSet<int> recent_items(3); // Capacity of 3
    print_set(recent_items, "Initial");

    recent_items.insert(10); print_set(recent_items, "Inserted 10"); // [10]
    recent_items.insert(20); print_set(recent_items, "Inserted 20"); // [10, 20]
    recent_items.insert(30); print_set(recent_items, "Inserted 30"); // [10, 20, 30]

    bool inserted_duplicate = recent_items.insert(20); // Try to insert duplicate
    std::cout << "Attempted to insert 20 again, inserted: " << std::boolalpha << inserted_duplicate << std::endl;
    print_set(recent_items, "After duplicate 20"); // [10, 20, 30] - no change

    recent_items.insert(40); // Set is full, 10 (oldest) will be evicted
    print_set(recent_items, "Inserted 40 (evicted 10)"); // [20, 30, 40]

    std::cout << "Contains 10? " << std::boolalpha << recent_items.contains(10) << std::endl; // false
    std::cout << "Contains 20? " << std::boolalpha << recent_items.contains(20) << std::endl; // true

    std::cout << "Oldest item (front): " << recent_items.front() << std::endl; // 20
    std::cout << "Newest item (back): " << recent_items.back() << std::endl;   // 40
}
```

### Erasing and Changing Capacity

```cpp
#include "bounded_set.h"
#include <iostream>
#include <vector>

// (print_set function as above)

int main() {
    BoundedSet<int> my_set(4);
    my_set.insert(1); my_set.insert(2); my_set.insert(3); my_set.insert(4);
    print_set(my_set, "Set: {1,2,3,4}"); // [1 2 3 4]

    my_set.erase(2);
    print_set(my_set, "After erasing 2"); // [1 3 4]

    // Change capacity
    my_set.reserve(2); // New capacity is 2. Oldest elements (1) will be evicted.
    print_set(my_set, "After reserve(2)"); // [3 4]

    my_set.insert(5);
    print_set(my_set, "Inserted 5 (evicted 3)"); // [4 5]

    my_set.clear();
    print_set(my_set, "After clear"); // []
    std::cout << "Is empty? " << std::boolalpha << my_set.empty() << std::endl; // true
}
```

## Use Cases
-   **Caching Recent Items:** Storing the N most recently accessed unique items (e.g., recently viewed products, recently opened files).
-   **History Lists:** Maintaining a short history of unique user actions or commands.
-   **Duplicate Detection for Recent Events:** Keeping track of recently processed event IDs to avoid reprocessing duplicates within a certain window.
-   **Limited Size Unique Logs:** Storing the last N unique log messages.

## Dependencies
- `<list>`
- `<unordered_map>`
- `<vector>` (for `as_vector()`)
- `<stdexcept>` (for exceptions)

This `BoundedSet` provides a good balance of features for managing a size-constrained, ordered collection of unique items.

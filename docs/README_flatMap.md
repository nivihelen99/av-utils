# `FlatMap<Key, Value>`

## Overview

The `FlatMap<Key, Value>` class (`flatMap.h`) implements a map-like associative container that stores its elements (key-value pairs) in a sorted `std::vector`. This approach, often called a "flat map" or "sorted vector map," offers different performance trade-offs compared to node-based maps like `std::map` (which uses balanced binary trees) or `std::unordered_map` (which uses hash tables).

## Performance Characteristics

-   **Lookup (`find`, `contains`, `at`, `operator[]` for access):** O(log N) due to binary search (`std::lower_bound`) on the sorted vector.
-   **Insertion (`insert`, `operator[]` for insertion):** O(N) in the worst case, as inserting into a sorted vector might require shifting subsequent elements to maintain order.
-   **Erasure (`erase`):** O(N) in the worst case, due to element shifting after removal.
-   **Iteration:** Very cache-friendly due to the contiguous memory layout of `std::vector`, potentially leading to faster iteration than node-based maps for large datasets.
-   **Memory Usage:** Generally lower memory overhead per element compared to `std::map` or `std::unordered_map` because it avoids storing tree pointers or hash table overhead.

`FlatMap` is most suitable for scenarios where:
-   The map is built once (or infrequently modified) and then queried many times.
-   The number of elements is relatively small, making O(N) insertions/erasures acceptable.
-   Cache performance during iteration is critical.

## Template Parameters

-   `Key`: The type of the keys. Must be comparable (support `operator<`).
-   `Value`: The type of the mapped values.

## Internal Storage

-   `std::vector<std::pair<Key, Value>> data_`: A vector that stores `std::pair<Key, Value>` objects, kept sorted by `Key`.

## Public Interface Highlights

### Constructor
-   **`FlatMap()`**: Default constructor, creates an empty map.

### Modifiers
-   **`void insert(const Key& key, const Value& value)`**:
    -   Inserts the key-value pair. If `key` already exists, its value is updated.
    -   Maintains the sorted order of elements.
-   **`bool erase(const Key& key)`**:
    -   Removes the element with the specified `key`. Returns `true` if an element was removed, `false` otherwise.
-   **`Value& operator[](const Key& key)`**:
    -   If `key` exists, returns a reference to its value.
    -   If `key` does not exist, it inserts `key` with a default-constructed `Value` (if `Value` is default-constructible), maintains sort order, and returns a reference to the newly inserted value.

### Lookup & Access
-   **`Value* find(const Key& key)` / `const Value* find(const Key& key) const`**:
    -   Finds an element by `key`. Returns a pointer to its value if found, otherwise `nullptr`.
-   **`const Value& at(const Key& key) const`**:
    -   Returns a const reference to the value associated with `key`. Throws `std::out_of_range` if `key` is not found.
-   **`bool contains(const Key& key) const`**:
    -   Checks if an element with `key` exists in the map.

### Capacity
-   **`size_t size() const`**: Returns the number of elements in the map.
-   **`bool empty() const`**: Checks if the map is empty.

### Iterators
-   **`auto begin() / auto end()`** (const and non-const versions):
    -   Return iterators to the beginning and end of the underlying sorted vector.
    -   Allows iteration over key-value pairs in key-sorted order.

## Usage Examples

(Based on `examples/flatMap_example.cpp`)

### Basic Operations

```cpp
#include "flatMap.h" // Assuming flatMap.h is in your include path
#include <iostream>
#include <string>

int main() {
    FlatMap<int, std::string> my_map;

    // Insert elements (order of insertion doesn't matter for final sorted state)
    my_map.insert(30, "Thirty");
    my_map.insert(10, "Ten");
    my_map.insert(20, "Twenty");
    my_map.insert(10, "TEN_UPDATED"); // Updates existing key 10

    std::cout << "Map contents (sorted by key):" << std::endl;
    for (const auto& pair : my_map) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
    }
    // Expected output (sorted):
    // Key: 10, Value: TEN_UPDATED
    // Key: 20, Value: Twenty
    // Key: 30, Value: Thirty

    // Using operator[]
    my_map[40] = "Forty"; // Inserts if not present
    my_map[20] = "TWENTY_MODIFIED"; // Modifies if present

    std::cout << "\nMap after operator[] usage:" << std::endl;
    for (const auto& pair : my_map) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
    }

    // Find and contains
    if (my_map.contains(30)) {
        std::cout << "\nKey 30 maps to: " << *my_map.find(30) << std::endl;
    }

    // Erase
    if (my_map.erase(20)) {
        std::cout << "Key 20 was erased." << std::endl;
    }
    std::cout << "Map size after erase: " << my_map.size() << std::endl;
}
```

### Using `at()` for Checked Access

```cpp
#include "flatMap.h"
#include <iostream>
#include <stdexcept> // For std::out_of_range

int main() {
    FlatMap<std::string, int> ages;
    ages.insert("Alice", 30);
    ages.insert("Bob", 25);

    try {
        std::cout << "Alice's age: " << ages.at("Alice") << std::endl;
        // The following line would throw std::out_of_range
        // std::cout << "Charlie's age: " << ages.at("Charlie") << std::endl;
    } catch (const std::out_of_range& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
```

## Dependencies
- `<vector>`
- `<utility>` (for `std::pair`)
- `<algorithm>` (for `std::lower_bound`, `std::sort` - though sort is not directly used by FlatMap itself as it maintains order on insert)
- `<stdexcept>` (for `std::out_of_range`)

`FlatMap` offers a memory-efficient and cache-friendly alternative for map-like structures when the performance profile matches its strengths (frequent lookups/iteration, infrequent modifications, or small sizes).

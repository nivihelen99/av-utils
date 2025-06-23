# `pydict::dict<Key, Value, ...>`

## Overview

The `pydict::dict` class (`dict.h`) is a C++ associative container designed to emulate the behavior of Python's dictionaries, particularly their maintenance of **insertion order** (a feature standard in Python 3.7+). It provides a map-like interface where elements (key-value pairs) are stored and can be retrieved, but iteration over the dictionary occurs in the order that items were originally inserted.

Internally, it uses an `std::unordered_map` for efficient O(1) average time complexity for lookups, insertions, and deletions by key, and a `std::vector` to preserve the insertion sequence of keys.

## Template Parameters

-   `Key`: The type of the keys.
-   `Value`: The type of the mapped values.
-   `Hash`: The hash function for keys (defaults to `std::hash<Key>`).
-   `KeyEqual`: The equality comparison function for keys (defaults to `std::equal_to<Key>`).
-   `Allocator`: The allocator for memory management (defaults to `std::allocator<std::pair<const Key, Value>>`).

## Key Features

-   **Insertion Order Preservation:** Iterating through the `dict` (e.g., using range-based for loops or its `keys()`, `values()`, `items()` methods) yields elements in the order they were first inserted.
-   **Python-like API:** Offers many methods familiar to Python dictionary users, such as `get()`, `pop()`, `popitem()`, `setdefault()`, `update()`, `keys()`, `values()`, `items()`.
-   **Efficient Lookups:** Leverages `std::unordered_map` for average O(1) key lookups.
-   **Custom Iterators:** Provides custom iterators that respect insertion order. These iterators dereference to a proxy object providing `first()` (key) and `second()` (value) methods.
-   **STL Compatibility:** Generally compatible with STL map concepts and some algorithms, though custom iterators require using proxy access methods (e.g., `it->first()`, `it->second()`).

## Public Interface Highlights

### Constructors & Assignment
-   Default constructor.
-   Constructors from `std::initializer_list`, iterator ranges, and allocators.
-   Copy/move constructors and assignment operators.

### Element Access
-   **`Value& operator[](const Key& key)` / `Value& operator[](Key&& key)`**:
    -   If `key` exists, returns a reference to its value.
    -   If `key` does not exist, inserts `key` with a default-constructed `Value` (if `Value` is default-constructible), adds `key` to the insertion order, and returns a reference to the new value.
-   **`Value& at(const Key& key)` / `const Value& at(const Key& key) const`**:
    -   Accesses element by `key`; throws `std::out_of_range` if `key` is not found.
-   **`Value get(const Key& key, const Value& default_value = Value{}) const`**:
    -   Returns the value for `key` if it exists, otherwise returns `default_value`. `Value` must be default-constructible if `default_value` is not provided.
-   **`std::optional<Value> get_optional(const Key& key) const`**:
    -   Returns `std::optional` containing the value if key exists, else `std::nullopt`.

### Iteration (Preserves Insertion Order)
-   **`iterator begin() / end()`**
-   **`const_iterator cbegin() / cend()`**
    *(Note: Iterators dereference to a proxy object; use `it->first()` and `it->second()` to access key/value).*

### Python-like Methods
-   **`Value pop(const Key& key)`**: Removes `key` and returns its value; throws if `key` not found.
-   **`Value pop(const Key& key, const Value& default_value)`**: Removes `key` and returns value; returns `default_value` if `key` not found.
-   **`std::pair<Key, Value> popitem()`**: Removes and returns the *last inserted* (key, value) pair; throws if empty.
-   **`Value& setdefault(const Key& key, const Value& default_value = Value{})`**: Returns value for `key` if it exists; otherwise inserts `key` with `default_value` and returns `default_value`.
-   **`void update(const dict& other)` / `void update(std::initializer_list<...>)`**: Updates dict with items from another source, overwriting existing keys.
-   **`std::vector<Key> keys() const`**: Returns keys in insertion order.
-   **`std::vector<Value> values() const`**: Returns values in insertion order.
-   **`std::vector<std::pair<Key, Value>> items() const`**: Returns (key, value) pairs in insertion order.

### Standard Map-like Methods
-   `empty()`, `size()`, `clear()`, `insert()`, `emplace()`, `erase()`, `count()`, `find()`, `contains()`.
-   Hash policy methods: `load_factor()`, `rehash()`, `reserve()`, etc.

### Comparison & Swap
-   **`bool operator==(const dict& other) const` / `bool operator!=(const dict& other) const`**: Compares content *and* insertion order.
-   `swap()`: Member and non-member versions.

### Stream Output
-   **`std::ostream& operator<<(std::ostream& os, const dict& d)`**: Prints in Python dict format, e.g., `{key1: value1, key2: value2}`.

## Usage Examples

(Based on `examples/use_dict.cpp`)

### Basic Initialization and Iteration

```cpp
#include "dict.h" // Assuming pydict namespace
#include <iostream>
#include <string>

int main() {
    pydict::dict<std::string, int> scores = {
        {"Alice", 95},
        {"Bob", 88},
        {"Charlie", 92}
    };

    // Iteration preserves insertion order
    std::cout << "Scores: " << scores << std::endl;
    // Output: {Alice: 95, Bob: 88, Charlie: 92}

    scores["David"] = 75; // Add David
    scores["Alice"] = 97; // Update Alice's score

    std::cout << "Updated Scores: " << scores << std::endl;
    // Output: {Alice: 97, Bob: 88, Charlie: 92, David: 75}
    // (Alice updated, David added at the end of original insertion sequence)

    std::cout << "Bob's score: " << scores["Bob"] << std::endl; // 88
    std::cout << "Eve's score (default): " << scores["Eve"] << std::endl; // 0 (Eve added with default int value)
    std::cout << "Final Scores: " << scores << std::endl;
    // Output: {Alice: 97, Bob: 88, Charlie: 92, David: 75, Eve: 0}
}
```

### Python-like Methods

```cpp
#include "dict.h"
#include <iostream>
#include <string>
#include <vector>

int main() {
    pydict::dict<std::string, int> config = {{"timeout", 30}, {"retries", 3}, {"port", 8080}};

    std::cout << "Keys: ";
    for (const auto& k : config.keys()) {
        std::cout << k << " ";
    }
    std::cout << std::endl; // Output: timeout retries port

    std::cout << "Port (get with default): " << config.get("port", 0) << std::endl;       // 8080
    std::cout << "Host (get with default): " << config.get("host", 80) << std::endl; // 80 (host not in dict)

    if (config.contains("retries")) {
        std::cout << "Config contains 'retries'." << std::endl;
    }

    int port_val = config.pop("port");
    std::cout << "Popped port: " << port_val << ". Dict now: " << config << std::endl;
    // Output: Popped port: 8080. Dict now: {timeout: 30, retries: 3}

    auto last_item = config.popitem(); // Pops last inserted: {retries: 3}
    std::cout << "Popped item: " << last_item.first << ":" << last_item.second << std::endl;
    std::cout << "Dict after popitem: " << config << std::endl;
    // Output: {timeout: 30}

    config.setdefault("max_users", 100);
    std::cout << "Dict after setdefault('max_users', 100): " << config << std::endl;
    // Output: {timeout: 30, max_users: 100}
}
```

## Dependencies
- `<unordered_map>`, `<vector>`, `<utility>`, `<stdexcept>`, `<functional>`, `<memory>`, `<type_traits>`, `<initializer_list>`, `<iostream>`, `<optional>`

The `pydict::dict` provides a useful C++ container for developers who appreciate Python's dictionary semantics, especially the ordered nature and rich API, while retaining C++ performance characteristics for lookups.

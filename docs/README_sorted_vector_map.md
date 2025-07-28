# sorted_vector_map

## Overview

`sorted_vector_map` is a C++ data structure that implements an associative container with a `std::map`-like interface. It stores key-value pairs in a `std::vector`, sorted by key.

This data structure is particularly useful in scenarios where the number of elements is relatively small, or when the map is built once and then accessed many times. In these cases, `sorted_vector_map` can be more efficient than `std::map` due to better cache locality.

## Performance Characteristics

-   **Lookup (`find`, `at`, `operator[]`):** O(log n) - performed using binary search.
-   **Insertion (`insert`):** O(n) - requires shifting elements to maintain sorted order.
-   **Deletion (`erase`):** O(n) - requires shifting elements to fill the gap.

## Usage

To use `sorted_vector_map`, include the header file `sorted_vector_map.h`:

```cpp
#include "sorted_vector_map.h"
```

### Creating a sorted_vector_map

You can create an empty `sorted_vector_map` like this:

```cpp
sorted_vector_map<int, std::string> map;
```

### Inserting elements

```cpp
map.insert({5, "apple"});
map.insert({2, "banana"});
```

### Accessing elements

```cpp
std::cout << map[2] << std::endl; // prints "banana"

try {
    std::cout << map.at(10) << std::endl;
} catch (const std::out_of_range& e) {
    std::cerr << "Key not found" << std::endl;
}
```

### Iterating over elements

The elements are iterated in sorted order of keys.

```cpp
for (const auto& pair : map) {
    std::cout << pair.first << ": " << pair.second << std::endl;
}
```

## When to use sorted_vector_map

-   When you have a small number of elements.
-   When the map is built once and then mostly read from.
-   When you need to iterate over the elements in sorted order and cache performance is important.

## When not to use sorted_vector_map

-   When you have a large number of elements and frequent insertions or deletions. In this case, `std::map` or `std::unordered_map` would be more suitable.

```markdown
# `flat_map`

## Overview

`cpp_collections::flat_map` is a map-like container that stores its elements in a sorted `std::vector`. This design offers a trade-off between fast lookups and slower insertions/erasures compared to `std::map` (which is typically implemented as a red-black tree).

`flat_map` is particularly well-suited for scenarios where the map is built once and then queried many times, as lookups are performed using binary search on the sorted vector, which is very cache-friendly.

## Features

-   **`std::map`-like Interface:** Provides a familiar interface, including `insert`, `erase`, `find`, `at`, `operator[]`, and iterators.
-   **Efficient Lookups:** Lookups are O(log N) due to the use of binary search on a sorted `std::vector`.
-   **Cache-Friendly:** The contiguous storage of elements in a `std::vector` can lead to better performance than node-based containers like `std::map` due to improved cache locality.
-   **Slower Insertions/Erasures:** Insertions and erasures are O(N) because they may require shifting elements in the vector to maintain the sorted order.

## Usage

To use `flat_map`, include the `flat_map.h` header file:

```cpp
#include "flat_map.h"
```

### Creation

You can create a `flat_map` in several ways:

```cpp
// Create an empty flat_map
cpp_collections::flat_map<std::string, int> word_counts;

// Create from a range of elements
std::vector<std::pair<int, std::string>> data = {{3, "three"}, {1, "one"}, {2, "two"}};
cpp_collections::flat_map<int, std::string> numbers(data.begin(), data.end());
```

### Accessing Elements

```cpp
// Use operator[] to insert or access elements
word_counts["apple"] += 1;
word_counts["date"] = 3;

// Use find to look up an element
auto it = word_counts.find("banana");
if (it != word_counts.end()) {
    // ...
}

// Use at for safe access (throws std::out_of_range if key not found)
int count = word_counts.at("apple");
```

### Iteration

`flat_map` provides standard iterators that allow you to traverse the elements in sorted order by key:

```cpp
for (const auto& pair : word_counts) {
    std::cout << pair.first << ": " << pair.second << std::endl;
}
```

## Performance

-   **Lookup (`find`, `at`, `operator[]`):** O(log N)
-   **Insertion (`insert`):** O(N)
-   **Erasure (`erase`):** O(N)
-   **Iteration:** O(N)
```

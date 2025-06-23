# `LazySortedMerger<Iterator, Value, Compare>`

## Overview

The `LazySortedMerger` class (`lazy_sorted_merger.h`) implements a K-way merge algorithm. It takes multiple sorted input sequences (provided as pairs of iterators) and allows iterating over their elements in a combined sorted order. The merging is "lazy," meaning it doesn't combine all sequences into one large sorted container upfront. Instead, it uses a min-priority queue (by default) to efficiently determine the next smallest element across all input sequences on demand. This makes it memory-efficient, especially when dealing with a large number of sequences or very long sequences.

## Template Parameters

-   `Iterator`: The type of iterators defining the input sorted sequences (e.g., `std::vector<int>::const_iterator`).
-   `Value`: The type of elements contained in the sequences. This is typically deduced from `std::iterator_traits<Iterator>::value_type`.
-   `Compare`: A binary predicate that defines the strict weak ordering for the elements.
    -   Defaults to `std::less<Value>`, which results in the merger producing elements in ascending order (smallest element first).
    -   To merge in descending order, `std::greater<Value>` can be used.

## Features

-   **K-Way Merge:** Merges multiple (K) sorted input sequences.
-   **Lazy Evaluation:** Computes the next element in the merged sequence only when requested via `next()`.
-   **Memory Efficient:** Only stores at most K elements in its internal priority queue (one from each active input sequence).
-   **Customizable Order:** Supports custom comparison logic via the `Compare` template parameter.
-   **Iterator-Based:** Works with any input iterators that model at least input iterator requirements and whose value types are comparable.
-   **Optional-Based `next()`:** The `next()` method returns `std::optional<Value>`, clearly indicating when the merged sequence is exhausted.

## Internal Structure (Conceptual)

-   It maintains a list of iterator pairs, one for each input sequence, tracking the current position and end of that sequence.
-   A min-priority queue (using `std::priority_queue` with a custom comparator) stores the current "head" element from each non-exhausted input sequence. The priority queue is ordered based on the `Compare` functor, ensuring that the globally smallest (or largest, depending on `Compare`) element is always at the top.

## Public Interface

### Constructor
-   **`LazySortedMerger(const std::vector<std::pair<Iterator, Iterator>>& sources, Compare comp = Compare())`**:
    -   `sources`: A vector where each element is a pair of iterators `(begin, end)` defining one sorted input sequence.
    -   `comp`: The comparison functor.

### Methods
-   **`bool hasNext() const`**:
    -   Returns `true` if there are more elements to be yielded from the merged sequence, `false` otherwise.
-   **`std::optional<Value> next()`**:
    -   Retrieves and removes the next element from the merged sequence according to the sort order.
    -   Returns `std::optional<Value>` containing the element.
    -   Returns `std::nullopt` if no more elements are available.

### Factory Function
-   **`template <...> lazy_merge(const std::vector<std::pair<Iterator, Iterator>>& sources, Compare comp = Compare())`**:
    -   A convenience function to create `LazySortedMerger` instances with template argument deduction.

## Usage Examples

(Based on `examples/lazy_sorted_merger_example.cpp`)

### Merging Sorted Vectors of Integers (Ascending Order)

```cpp
#include "lazy_sorted_merger.h" // Adjust path as necessary
#include <iostream>
#include <vector>
#include <utility> // For std::pair

int main() {
    std::vector<int> vec1 = {1, 5, 10, 15};
    std::vector<int> vec2 = {2, 6, 11, 16};
    std::vector<int> vec3 = {3, 7, 12, 17};

    std::vector<std::pair<std::vector<int>::const_iterator, std::vector<int>::const_iterator>> sources;
    sources.push_back({vec1.begin(), vec1.end()});
    sources.push_back({vec2.begin(), vec2.end()});
    sources.push_back({vec3.begin(), vec3.end()});

    // Default Compare is std::less<int> for ascending merge
    auto merger = lazy_merge(sources);

    std::cout << "Merged integers (ascending): ";
    while (merger.hasNext()) {
        if (auto val_opt = merger.next()) {
            std::cout << *val_opt << " ";
        }
    }
    std::cout << std::endl;
    // Output: Merged integers (ascending): 1 2 3 5 6 7 10 11 12 15 16 17
}
```

### Merging with Custom Comparator (Descending Order)

```cpp
#include "lazy_sorted_merger.h"
#include <iostream>
#include <vector>
#include <functional> // For std::greater

int main() {
    std::vector<int> vec_a = {15, 10, 5, 1}; // Input sorted descending
    std::vector<int> vec_b = {16, 11, 6, 2}; // Input sorted descending

    std::vector<std::pair<std::vector<int>::const_iterator, std::vector<int>::const_iterator>> sources;
    sources.push_back({vec_a.begin(), vec_a.end()});
    sources.push_back({vec_b.begin(), vec_b.end()});

    // Use std::greater<int> to merge in descending order
    auto merger = lazy_merge(sources, std::greater<int>());

    std::cout << "Merged integers (descending): ";
    while (merger.hasNext()) {
        if (auto val_opt = merger.next()) {
            std::cout << *val_opt << " ";
        }
    }
    std::cout << std::endl;
    // Output: Merged integers (descending): 16 15 11 10 6 5 2 1
}
```

### Merging Custom Structs

```cpp
#include "lazy_sorted_merger.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm> // For std::sort

struct Record {
    int id;
    std::string data;
};

struct CompareRecordById {
    bool operator()(const Record& a, const Record& b) const {
        return a.id < b.id; // Sort by ID, ascending
    }
};

int main() {
    std::vector<Record> list1 = {{3, "Charlie"}, {7, "Grace"}};
    std::vector<Record> list2 = {{1, "Alice"}, {8, "Henry"}};
    std::vector<Record> list3 = {{2, "Bob"}, {5, "Eve"}};

    // Ensure input lists are sorted according to the comparator
    std::sort(list1.begin(), list1.end(), CompareRecordById());
    std::sort(list2.begin(), list2.end(), CompareRecordById());
    std::sort(list3.begin(), list3.end(), CompareRecordById());

    std::vector<std::pair<std::vector<Record>::const_iterator, std::vector<Record>::const_iterator>> sources;
    sources.push_back({list1.begin(), list1.end()});
    sources.push_back({list2.begin(), list2.end()});
    sources.push_back({list3.begin(), list3.end()});

    auto merger = lazy_merge(sources, CompareRecordById());

    std::cout << "Merged Records (by ID):" << std::endl;
    while (merger.hasNext()) {
        if (auto record_opt = merger.next()) {
            std::cout << "  ID: " << record_opt->id << ", Data: " << record_opt->data << std::endl;
        }
    }
    // Expected Output (sorted by ID):
    //   ID: 1, Data: Alice
    //   ID: 2, Data: Bob
    //   ID: 3, Data: Charlie
    //   ID: 5, Data: Eve
    //   ID: 7, Data: Grace
    //   ID: 8, Data: Henry
}
```

## Dependencies
- `<vector>`
- `<utility>` (for `std::pair`)
- `<queue>` (for `std::priority_queue`)
- `<functional>` (for `std::function`, `std::less`, `std::greater`)
- `<optional>` (for `std::optional`)
- `<iterator>` (for `std::iterator_traits`)

The `LazySortedMerger` is a valuable tool for efficiently combining multiple pre-sorted data streams without incurring the cost of loading all data into memory at once.

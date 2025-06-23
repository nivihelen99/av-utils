# `IntervalMap<T>`

## Overview

The `interval_map.h` header provides implementations for an interval map data structure. An interval map associates values with intervals (defined by a start and end integer point). It allows querying for all values whose associated intervals overlap a given point or a given query interval.

The header defines three distinct implementations:
1.  `IntervalMapVector<T>`: Uses a `std::vector` of interval structures.
2.  `IntervalMapSorted<T>`: Uses a `std::map` keyed by interval start points, where each value is a vector of (end_point, value) pairs.
3.  `IntervalMapSegment<T>`: Also uses a `std::vector` of interval structures. Its name suggests an intent for a segment tree-like structure, but the current implementation is a simpler vector-based approach similar to `IntervalMapVector`.

By default, `IntervalMap<T>` is an alias for `IntervalMapSorted<T>`.

**Note on Performance:** The provided implementations, including the default `IntervalMapSorted`, generally offer O(N) complexity for query operations in the worst case (where N is the total number of intervals). For applications requiring highly optimized interval queries (e.g., O(log N + k) where k is the number of reported intervals), more specialized data structures like augmented interval trees or segment trees would typically be used. The current implementations are simpler and may be suitable for smaller datasets or scenarios where O(N) query time is acceptable.

## Common Interface (Provided by all implementations)

-   **`void insert(int start, int end, const T& value)`**:
    -   Inserts an interval `[start, end)` associated with `value`.
    -   Throws `std::invalid_argument` if `start >= end` or if an exact duplicate interval (same start and end) already exists.
-   **`bool remove(int start, int end)`**:
    -   Removes the interval defined by `start` and `end`. Returns `true` if found and removed.
-   **`std::vector<T> query(int point) const`**:
    -   Returns a vector of values whose intervals contain the given `point`. An interval `[s, e)` contains `point` if `s <= point < e`.
-   **`std::vector<T> query_range(int qstart, int qend) const`**:
    -   Returns a vector of values whose intervals overlap with the query range `[qstart, qend)`. Overlap occurs if `interval.start < qend` and `qstart < interval.end`.
-   **`std::vector<std::tuple<int, int, T>> all_intervals() const`**:
    -   Returns all stored intervals as tuples of (start, end, value).
-   **`bool empty() const`**: Checks if the map is empty.
-   **`size_t size() const`**: Returns the number of stored intervals.

## `IntervalMap<T>` (Defaults to `IntervalMapSorted<T>`)

This is the default type alias. It uses `std::map<int, std::vector<std::pair<int, T>>>` where the map key is the interval's start point.

## Usage Examples

(Based on example/test code embedded in `interval_map.h`)

### Basic Operations

```cpp
#include "interval_map.h" // Or your actual include path
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept> // For std::invalid_argument

int main() {
    // IntervalMap<std::string> defaults to IntervalMapSorted<std::string>
    IntervalMap<std::string> rules_map;

    // Insert intervals
    try {
        rules_map.insert(10, 20, "RuleA: Applies to 10-19");
        rules_map.insert(15, 25, "RuleB: Applies to 15-24");
        rules_map.insert(30, 40, "RuleC: Applies to 30-39");
        // rules_map.insert(5, 5, "InvalidRule"); // Would throw std::invalid_argument
        // rules_map.insert(10, 20, "DuplicateRule"); // Would throw std::invalid_argument
    } catch (const std::invalid_argument& e) {
        std::cerr << "Error during insertion: " << e.what() << std::endl;
    }

    std::cout << "Map size: " << rules_map.size() << std::endl;

    // Point queries
    std::cout << "\nQuerying point 12:" << std::endl;
    std::vector<std::string> results_at_12 = rules_map.query(12);
    for (const auto& val : results_at_12) {
        std::cout << "  - " << val << std::endl; // Expect RuleA
    }

    std::cout << "\nQuerying point 18:" << std::endl;
    std::vector<std::string> results_at_18 = rules_map.query(18);
    for (const auto& val : results_at_18) {
        std::cout << "  - " << val << std::endl; // Expect RuleA, RuleB
    }

    std::cout << "\nQuerying point 28 (no overlap):" << std::endl;
    std::vector<std::string> results_at_28 = rules_map.query(28);
    if (results_at_28.empty()) {
        std::cout << "  No rules apply at point 28." << std::endl;
    }
}
```

### Range Queries and Removal

```cpp
#include "interval_map.h"
#include <iostream>
#include <vector>

int main() {
    IntervalMap<int> data_map;
    data_map.insert(0, 10, 100);    // [0, 10) -> 100
    data_map.insert(5, 15, 200);    // [5, 15) -> 200
    data_map.insert(20, 30, 300);   // [20, 30) -> 300

    // Range query: [8, 22)
    // Overlaps with [0,10) -> 100 (because 0 < 22 and 8 < 10)
    // Overlaps with [5,15) -> 200 (because 5 < 22 and 8 < 15)
    // Overlaps with [20,30) -> 300 (because 20 < 22 and 8 < 30) -- wait, 8 < 30 is true, but 20 < 22 is true. So this overlaps.
    // Query range [qstart, qend) overlaps with interval [start, end) if:
    // start < qend AND qstart < end

    std::cout << "Querying range [8, 22):" << std::endl;
    std::vector<int> range_results = data_map.query_range(8, 22);
    for (int val : range_results) {
        std::cout << "  - Value: " << val << std::endl;
        // Expected: 100, 200, 300 (based on overlap logic for [8,22))
    }
    // Interval [0,10) overlaps [8,22) because 0 < 22 and 8 < 10. Correct.
    // Interval [5,15) overlaps [8,22) because 5 < 22 and 8 < 15. Correct.
    // Interval [20,30) overlaps [8,22) because 20 < 22 and 8 < 30. Correct.


    // Remove an interval
    bool removed = data_map.remove(5, 15);
    std::cout << "\nRemoved interval [5, 15): " << std::boolalpha << removed << std::endl; // true
    std::cout << "Map size after removal: " << data_map.size() << std::endl;

    std::cout << "\nQuerying point 12 after removal:" << std::endl;
    std::vector<int> results_at_12_after_remove = data_map.query(12); // Should only find value from [0,10) if 12 is not in it.
                                                                   // Point 12 is not in [0,10).
                                                                   // Before removal, [5,15) contained 12. Now it's gone.
                                                                   // So, query(12) should be empty.
    if(results_at_12_after_remove.empty()){
        std::cout << "  No values at point 12 after removing [5,15)." << std::endl;
    } else {
        for (const auto& val : results_at_12_after_remove) {
            std::cout << "  - " << val << std::endl;
        }
    }


    std::cout << "\nAll remaining intervals:" << std::endl;
    for (const auto& tuple_interval : data_map.all_intervals()) {
        std::cout << "  [" << std::get<0>(tuple_interval) << ", " << std::get<1>(tuple_interval)
                  << ") -> " << std::get<2>(tuple_interval) << std::endl;
    }
}
```

## Dependencies
- `<vector>`, `<tuple>`, `<algorithm>`, `<stdexcept>`, `<map>`, `<set>`

The `IntervalMap` implementations in this header provide basic interval mapping capabilities. For applications requiring very high performance on large numbers of intervals and complex queries, dedicated interval tree or segment tree libraries might be more appropriate.

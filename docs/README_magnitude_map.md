# MagnitudeMap

## Overview

`MagnitudeMap` is a C++ header-only data structure that provides a map-like interface for storing key-value pairs where keys are of an arithmetic type. Its distinctive feature is the ability to efficiently query for all elements whose keys fall within a specified "magnitude" or distance from a given query key.

This is particularly useful for scenarios involving numerical data where proximity is important, such as:
- Time-series data: Finding all events or readings that occurred within a certain time window around a specific timestamp.
- Sensor data: Locating sensor readings whose measurement points (e.g., IDs, coordinates, values) are close to a query point.
- Numerical datasets: Identifying entries that are "nearby" a specific numerical value.

The `MagnitudeMap` is implemented using `std::map` as its underlying storage, ensuring that keys are sorted and providing logarithmic time complexity for basic operations.

## Template Parameters

```cpp
template <typename KeyType, typename ValueType>
class MagnitudeMap;
```

-   `KeyType`: The type of the keys. Must be an arithmetic type (e.g., `int`, `double`, `unsigned long`). This is enforced by a `static_assert(std::is_arithmetic_v<KeyType>, ...)`.
-   `ValueType`: The type of the values associated with the keys.

## Public API

### Constructors
-   `MagnitudeMap()`: Default constructor. Creates an empty map.

### Modifiers
-   `void insert(const KeyType& key, const ValueType& value)`
-   `void insert(const KeyType& key, ValueType&& value)`
-   `void insert(KeyType&& key, const ValueType& value)`
-   `void insert(KeyType&& key, ValueType&& value)`
    Inserts a key-value pair. If the key already exists, its value is updated.
-   `bool remove(const KeyType& key)`
    Removes the element with the specified key. Returns `true` if an element was removed, `false` otherwise.

### Accessors
-   `ValueType* get(const KeyType& key)`
-   `const ValueType* get(const KeyType& key) const`
    Returns a pointer to the value associated with the key, or `nullptr` if the key is not found.
-   `bool contains(const KeyType& key) const`
    Returns `true` if the map contains an element with the specified key, `false` otherwise.
-   `size_t size() const`
    Returns the number of elements in the map.
-   `bool empty() const`
    Returns `true` if the map is empty, `false` otherwise.

### Core Query Function
-   `std::vector<std::pair<KeyType, ValueType>> find_within_magnitude(const KeyType& query_key, const KeyType& magnitude) const`
    Returns a `std::vector` of key-value pairs where each key `k` in the map satisfies the condition `abs(k - query_key) <= magnitude`.
    -   The `magnitude` parameter should ideally be non-negative. If a negative magnitude is provided for signed or floating-point `KeyType`, it is treated as `0`. For unsigned `KeyType`, magnitude is used as is (it's always non-negative).
    -   The returned vector contains pairs sorted by key.

## Time Complexity

-   `insert`, `remove`, `get`, `contains`: O(log N), where N is the number of elements in the map (due to `std::map` operations).
-   `size`, `empty`: O(1).
-   `find_within_magnitude`: O(log N + M), where N is the total number of elements in the map, and M is the number of elements that fall within the calculated initial search range `[query_key - magnitude, query_key + magnitude]`. The final filtering step iterates over these M elements. In the worst case, M could be N.

## Usage Example

```cpp
#include "magnitude_map.h" // Assuming it's in the include path
#include <iostream>
#include <string>
#include <vector>

int main() {
    cpp_utils::MagnitudeMap<double, std::string> event_log;

    event_log.insert(10.5, "Event A");
    event_log.insert(12.3, "Event B");
    event_log.insert(12.8, "Event C");
    event_log.insert(15.0, "Event D");

    std::cout << "Log size: " << event_log.size() << std::endl;

    double query_time = 12.5;
    double time_magnitude = 0.5; // Search for events between 12.0 and 13.0

    std::vector<std::pair<double, std::string>> nearby_events =
        event_log.find_within_magnitude(query_time, time_magnitude);

    std::cout << "Events near " << query_time << " (magnitude " << time_magnitude << "):" << std::endl;
    if (nearby_events.empty()) {
        std::cout << "  None found." << std::endl;
    } else {
        for (const auto& event_pair : nearby_events) {
            std::cout << "  Time: " << event_pair.first << ", Data: \"" << event_pair.second << "\"" << std::endl;
        }
    }
    // Expected output:
    // Events near 12.5 (magnitude 0.5):
    //   Time: 12.3, Data: "Event B"
    //   Time: 12.8, Data: "Event C"

    // Get a specific event
    if (event_log.contains(15.0)) {
        std::cout << "Event at time 15.0: " << *event_log.get(15.0) << std::endl;
    }

    event_log.remove(10.5);
    std::cout << "Log size after removing 10.5: " << event_log.size() << std::endl;

    return 0;
}

```

## Implementation Details

The `MagnitudeMap` uses `std::map<KeyType, ValueType>` as its internal data store.
The `find_within_magnitude` method calculates a search range `[query_key - magnitude, query_key + magnitude]`, carefully handling potential arithmetic overflows/underflows for integral key types and saturation at `std::numeric_limits` boundaries. It then uses `std::map::lower_bound` and `std::map::upper_bound` to efficiently find an initial range of elements. Each element in this initial range is then precisely checked against the `abs(key - query_key) <= magnitude` condition. This two-step process (broad phase query + precise filtering) ensures correctness, especially for floating-point keys and edge cases with numeric limits.
The results are collected into a `std::vector` and returned.
The header includes necessary standard library headers like `<map>`, `<vector>`, `<type_traits>`, `<cmath>`, and `<limits>`.
It is placed within the `cpp_utils` namespace.
```

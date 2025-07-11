# `group_by_consecutive` Utility (`cpp_collections` namespace)

## Overview

The `group_by_consecutive.h` header provides utility functions within the `cpp_collections` namespace to group consecutive elements in a range. This component is inspired by Python's `itertools.groupby` but is designed for C++ iterators and returns fully materialized groups.

It's important to distinguish this from `std::group_by` (from `<algorithm>`), which requires the input range to be pre-sorted according to the grouping criterion. `cpp_collections::group_by_consecutive` and `cpp_collections::group_by_consecutive_pred` operate on the range as-is, grouping elements that are already consecutive and share a common characteristic.

Two primary function templates are provided:
1.  `cpp_collections::group_by_consecutive(first, last, getKey)`: Groups elements based on a key returned by a user-provided `getKey` function.
2.  `cpp_collections::group_by_consecutive_pred(first, last, are_in_same_group)`: Groups elements based on a binary predicate that determines if two adjacent elements belong to the same group.

## Features

-   **Header-only**: Include `group_by_consecutive.h`.
-   **Namespace**: All components are within the `cpp_collections` namespace.
-   **Iterator-based**: Works with input iterators satisfying ForwardIterator requirements.
-   **C++20 Standard**: Leverages C++20 features like `std::invoke_result_t`.
-   **Two Grouping Strategies**:
    -   **Key Extraction**: Supply a function that extracts a key from each element. Consecutive elements yielding the same key are grouped.
    -   **Predicate-based**: Supply a binary predicate `(previous_element, current_element)` which returns `true` if `current_element` should be grouped with `previous_element`.
-   **Structured Group Representation**: Returns a `std::vector` of `cpp_collections::Group` objects.

## `cpp_collections::Group<KeyType, ValueType>` Struct

The result of the grouping operations is a vector of `Group` objects. This struct is defined as:

```cpp
namespace cpp_collections {
    template <typename Key, typename Value>
    struct Group {
        Key key;
        std::vector<Value> items;

        // Default constructor
        Group() = default;

        // Constructor
        Group(Key k, std::vector<Value> i);

        // Equality operator for easy testing
        bool operator==(const Group<Key, Value>& other) const;
    };
}
```
-   `key`: For the key-extraction version, this is the common key for the group. For the predicate version, this is a copy of the first element of the group.
-   `items`: A `std::vector<ValueType>` containing copies of all elements belonging to that consecutive group.

## API Details

### 1. `group_by_consecutive` (Key Function Version)

```cpp
template <typename InputIt, typename GetKey>
auto group_by_consecutive(InputIt first, InputIt last, GetKey getKey)
    -> std::vector<Group<
           std::invoke_result_t<GetKey, typename std::iterator_traits<InputIt>::value_type>,
           typename std::iterator_traits<InputIt>::value_type>>;
```
-   **Template Parameters**:
    -   `InputIt`: An input iterator type (must be at least a ForwardIterator).
    -   `GetKey`: A callable type for the key extraction function.
-   **Parameters**:
    -   `first`, `last`: Iterators defining the input range `[first, last)`.
    -   `getKey`: A unary function object (lambda, functor, etc.) that takes an element from the input range (of `std::iterator_traits<InputIt>::value_type`) and returns its key. The key type must be comparable using `operator==`.
-   **Returns**: A `std::vector<cpp_collections::Group<KeyType, ValueType>>`.
    -   `KeyType` is deduced from the return type of `getKey`.
    -   `ValueType` is deduced from the input iterator.

### 2. `group_by_consecutive_pred` (Predicate Version)

```cpp
template <typename InputIt, typename AreInSameGroup>
auto group_by_consecutive_pred(InputIt first, InputIt last, AreInSameGroup are_in_same_group)
    -> std::vector<Group<
           typename std::iterator_traits<InputIt>::value_type,
           typename std::iterator_traits<InputIt>::value_type>>;
```
-   **Template Parameters**:
    -   `InputIt`: An input iterator type (must be at least a ForwardIterator).
    -   `AreInSameGroup`: A callable type for the binary predicate.
-   **Parameters**:
    -   `first`, `last`: Iterators defining the input range `[first, last)`.
    -   `are_in_same_group`: A binary predicate that accepts two arguments: `(previous_element, current_element)` from the range. It must return `true` if `current_element` belongs to the same group as `previous_element`, and `false` otherwise.
-   **Returns**: A `std::vector<cpp_collections::Group<ValueType, ValueType>>`.
    -   The `key` for each group in this version is a copy of the first element of that group.
    -   `ValueType` is deduced from the input iterator.

## Usage Example

```cpp
#include "group_by_consecutive.h" // Assumed to be in include path
#include <iostream>
#include <vector>
#include <string>
#include <iomanip> // For std::quoted
#include <cmath>   // For std::abs
#include <type_traits> // For std::is_same_v (used in example's print helper)

// Example helper to print groups (from group_by_consecutive_example.cpp)
template <typename Key, typename Value>
void print_demo_groups(const std::string& title,
                       const std::vector<cpp_collections::Group<Key, Value>>& groups) {
    std::cout << title << ":" << std::endl;
    if (groups.empty()) {
        std::cout << "  (empty)" << std::endl;
        return;
    }
    for (const auto& group : groups) {
        std::cout << "  Key: ";
        if constexpr (std::is_same_v<Key, std::string>) {
            std::cout << std::quoted(group.key);
        } else if constexpr (std::is_same_v<Key, char>) {
            std::cout << "'" << group.key << "'";
        } else {
            std::cout << group.key; // Assumes Key has operator<<
        }
        std::cout << ", Items: [";
        bool first_item = true;
        for (const auto& item : group.items) {
            if (!first_item) std::cout << ", ";
            if constexpr (std::is_same_v<Value, std::string>) {
                std::cout << std::quoted(item);
            } else {
                std::cout << item; // Assumes Value has operator<<
            }
            first_item = false;
        }
        std::cout << "]" << std::endl;
    }
    std::cout << std::endl;
}

int main() {
    // Example 1: Grouping integers by their value (Key Function)
    std::vector<int> numbers = {1, 1, 1, 2, 2, 1, 3, 3, 2};
    auto int_groups_key = cpp_collections::group_by_consecutive(
        numbers.begin(), numbers.end(),
        [](int x) { return x; } // Key is the number itself
    );
    print_demo_groups("Integers grouped by value (key function)", int_groups_key);
    // Output:
    // Integers grouped by value (key function):
    //   Key: 1, Items: [1, 1, 1]
    //   Key: 2, Items: [2, 2]
    //   Key: 1, Items: [1]
    //   Key: 3, Items: [3, 3]
    //   Key: 2, Items: [2]

    // Example 2: Grouping strings by their first character (Key Function)
    std::vector<std::string> words = {"apple", "apricot", "banana", "blueberry", "cherry"};
    auto string_groups_key = cpp_collections::group_by_consecutive(
        words.begin(), words.end(),
        [](const std::string& s) { return s.empty() ? ' ' : s[0]; }
    );
    print_demo_groups("Strings grouped by first char (key function)", string_groups_key);
    // Output:
    // Strings grouped by first char (key function):
    //   Key: 'a', Items: ["apple", "apricot"]
    //   Key: 'b', Items: ["banana", "blueberry"]
    //   Key: 'c', Items: ["cherry"]

    // Example 3: Grouping integers if their difference is <= 1 (Predicate)
    std::vector<int> sequence = {1, 2, 3, 5, 6, 8, 9, 10, 12};
    auto int_groups_pred = cpp_collections::group_by_consecutive_pred(
        sequence.begin(), sequence.end(),
        [](int prev, int curr) { return std::abs(curr - prev) <= 1; }
    );
    print_demo_groups("Integers grouped if diff <= 1 (predicate)", int_groups_pred);
    // Output:
    // Integers grouped if diff <= 1 (predicate):
    //   Key: 1, Items: [1, 2, 3]  (Key is the first item of the group)
    //   Key: 5, Items: [5, 6]
    //   Key: 8, Items: [8, 9, 10]
    //   Key: 12, Items: [12]

    return 0;
}
```

## When to Use

-   When you need to process or analyze consecutive runs of identical items (or items deemed equivalent by a predicate) within a sequence.
-   If the original order of elements outside these consecutive groups must be preserved, making a full sort-by-key (as required by `std::group_by`) undesirable.
-   Useful for parsing data streams or logs where entries are naturally batched or chunked by some property, and you need to operate on these batches.

This utility offers a convenient and direct method for such grouping tasks, abstracting away manual loop management for identifying group boundaries.

## Dependencies
- `<vector>`
- `<functional>`
- `<iterator>` (for `std::iterator_traits`, `std::next`)
- `<type_traits>` (for `std::invoke_result_t`)

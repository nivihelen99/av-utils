# `utils::group_by_consecutive`

## Overview

The `group_by_consecutive.h` header provides a utility function, `utils::group_by_consecutive`, that groups consecutive elements in an input sequence (specified by iterators or a container) based on a key. The key for each element is determined by a user-provided key function.

This function is similar in concept to Python's `itertools.groupby`, but instead of returning iterators to groups, it collects all grouped items into a `std::vector` of pairs, where each pair consists of the common key and a `std::vector` of the elements belonging to that consecutive group.

## Features

-   **Groups Consecutive Elements:** Identifies contiguous blocks of elements that share the same key.
-   **Custom Key Function:** Allows flexible grouping logic by accepting a user-defined function to extract the key from each element.
-   **Iterator and Container Support:** Provides overloads for both iterator ranges and whole containers.
-   **Type Deduction:** Uses modern C++ features for automatic type deduction of keys and values.
-   **Move Semantics:** Utilizes `std::move` for potentially better performance when constructing the result.

## Functions

### Iterator-Based Overload
```cpp
template<typename Iterator, typename KeyFunc>
auto group_by_consecutive(Iterator begin, Iterator end, KeyFunc key_func)
    -> std::vector<std::pair<
           typename std::decay_t<decltype(key_func(*begin))>, // KeyType
           std::vector<typename std::iterator_traits<Iterator>::value_type> // GroupType
       >>;
```
-   `begin`, `end`: Iterators defining the input sequence.
-   `key_func`: A callable (lambda, function pointer, functor) that accepts an element from the sequence and returns a key. Elements are grouped as long as this key remains the same for consecutive elements.
-   **Returns**: A `std::vector` of `std::pair`. Each pair contains:
    -   `first`: The common key for the group.
    -   `second`: A `std::vector` containing all original elements that formed that consecutive group.

### Container-Based Overload (Wrapper)
```cpp
template<typename Container, typename KeyFunc>
auto group_by_consecutive(const Container& container, KeyFunc key_func)
    -> decltype(group_by_consecutive(container.begin(), container.end(), key_func));
```
-   `container`: A container (e.g., `std::vector`, `std::list`) passed by const reference.
-   `key_func`: Same as in the iterator-based version.
-   **Returns**: Same as the iterator-based version.

## Usage Examples

(Based on `examples/group_by_consecutive_example.cpp`)

### Basic Usage with `std::pair`

```cpp
#include "group_by_consecutive.h"
#include <vector>
#include <string>
#include <iostream>
#include <utility> // For std::pair

int main() {
    std::vector<std::pair<char, int>> data = {
        {'a', 1}, {'a', 2}, {'b', 3}, {'b', 4}, {'a', 5}
    };

    // Group by the first element of the pair (the char)
    auto key_function = [](const std::pair<char, int>& p) {
        return p.first;
    };

    auto groups = utils::group_by_consecutive(data, key_function);

    for (const auto& group_pair : groups) {
        std::cout << "Key: " << group_pair.first << ", Values: [ ";
        for (const auto& item : group_pair.second) {
            std::cout << "{'" << item.first << "', " << item.second << "} ";
        }
        std::cout << "]" << std::endl;
    }
    // Expected Output:
    // Key: a, Values: [ {'a', 1} {'a', 2} ]
    // Key: b, Values: [ {'b', 3} {'b', 4} ]
    // Key: a, Values: [ {'a', 5} ]
}
```

### Grouping Integers by Their Value

```cpp
#include "group_by_consecutive.h"
#include <vector>
#include <iostream>

int main() {
    std::vector<int> numbers = {1, 1, 1, 2, 2, 3, 1, 1, 4, 4, 4, 4};

    // Group by the number itself
    auto groups = utils::group_by_consecutive(numbers.begin(), numbers.end(), [](int val) {
        return val;
    });

    for (const auto& group_pair : groups) {
        std::cout << "Key: " << group_pair.first << ", Values: [ ";
        for (int item : group_pair.second) {
            std::cout << item << " ";
        }
        std::cout << "]" << std::endl;
    }
    // Expected Output:
    // Key: 1, Values: [ 1 1 1 ]
    // Key: 2, Values: [ 2 2 ]
    // Key: 3, Values: [ 3 ]
    // Key: 1, Values: [ 1 1 ]
    // Key: 4, Values: [ 4 4 4 4 ]
}
```

### Grouping Custom Structs

```cpp
#include "group_by_consecutive.h"
#include <vector>
#include <string>
#include <iostream>
#include <iomanip> // For std::quoted

struct Item {
    int id;
    std::string category;
};

int main() {
    std::vector<Item> items = {
        {1, "fruit"}, {2, "fruit"}, {3, "vegetable"},
        {4, "fruit"}, {5, "fruit"}, {6, "dairy"}
    };

    auto get_item_category = [](const Item& item) -> const std::string& {
        return item.category;
    };

    auto grouped_items = utils::group_by_consecutive(items, get_item_category);

    for (const auto& group : grouped_items) {
        std::cout << "Category: " << std::quoted(group.first) << std::endl;
        for (const auto& item : group.second) {
            std::cout << "  ID: " << item.id << ", Cat: " << std::quoted(item.category) << std::endl;
        }
    }
    // Expected Output:
    // Category: "fruit"
    //   ID: 1, Cat: "fruit"
    //   ID: 2, Cat: "fruit"
    // Category: "vegetable"
    //   ID: 3, Cat: "vegetable"
    // Category: "fruit"
    //   ID: 4, Cat: "fruit"
    //   ID: 5, Cat: "fruit"
    // Category: "dairy"
    //   ID: 6, Cat: "dairy"
}
```

## Dependencies
- `<vector>`
- `<utility>` (for `std::pair`)
- `<type_traits>` (for `std::decay_t`)
- `<iterator>` (for `std::iterator_traits`, `std::next`)

This utility is helpful for processing sequences where handling runs of identical (by key) consecutive elements is required.

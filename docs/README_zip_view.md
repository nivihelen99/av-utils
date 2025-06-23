# `zip_utils::zip` and `zip_utils::enumerate` Views

## Overview

The `zip_view.h` header provides C++17 utilities for creating "views" that allow simultaneous iteration over multiple containers (zipping) or iteration over a single container with an accompanying index (enumeration). These are inspired by Python's built-in `zip()` and `enumerate()` functions.

-   **`zip_utils::zip(container1, container2, ...)`**: Takes two or more containers and returns a view. Iterating over this view yields tuples where each tuple contains the i-th element from each of the input containers. Iteration stops when the shortest input container is exhausted.
-   **`zip_utils::enumerate(container)`**: Takes a single container and returns a view. Iterating over this view yields pairs of `(index, element_reference)`.

These utilities are implemented using custom iterator and view classes (`ZippedIterator`, `ZippedView`, `EnumerateView::iterator`, `EnumerateView`) and are designed to work with range-based for loops.

## Namespace
All utilities are within the `zip_utils` namespace.

## `zip_utils::zip`

### Features
-   **Multi-Container Iteration:** Iterate over corresponding elements from several containers at once.
-   **Heterogeneous Containers:** Works with containers of different types (e.g., `std::vector<int>` and `std::list<std::string>`).
-   **Shortest Sequence Termination:** Iteration automatically stops when the shortest input container is exhausted.
-   **Reference Semantics:** The zipped tuple elements are typically references to the original container elements, allowing modification if the original containers are non-const.
-   **Lvalue/Rvalue Handling:** Correctly handles both lvalue and rvalue container arguments using forwarding references. Rvalue containers will be moved into the `ZippedView`.

### How it Works
-   The `zip(...)` factory function returns a `ZippedView<...ViewTypes>`.
-   `ZippedView` stores the input containers (or references to them) in a `std::tuple`.
-   It provides `begin()` and `end()` methods that return `ZippedIterator` (or `ZippedConstIterator`).
-   `ZippedIterator` internally holds a tuple of iterators, one for each zipped container.
-   Dereferencing (`operator*`) a `ZippedIterator` returns a `std::tuple` of references to the current elements from each container (e.g., `std::tuple<int&, std::string&>`).

### Usage Example

```cpp
#include "zip_view.h" // Adjust path as needed
#include <iostream>
#include <vector>
#include <list>
#include <string>

int main() {
    std::vector<int> numbers = {1, 2, 3, 4};
    std::list<std::string> words = {"one", "two", "three"};
    std::vector<char> chars = {'a', 'b', 'c', 'd', 'e'};

    std::cout << "Zipping numbers, words, and chars (stops at shortest - words):" << std::endl;
    // Iteration will stop after 3 elements due to 'words' being the shortest
    for (auto&& [num, word, ch] : zip_utils::zip(numbers, words, chars)) {
        std::cout << "Number: " << num << ", Word: \"" << word << "\", Char: '" << ch << "'" << std::endl;
        // num = num * 10; // Can modify if 'numbers' is non-const
    }
    // Example Output:
    // Number: 1, Word: "one", Char: 'a'
    // Number: 2, Word: "two", Char: 'b'
    // Number: 3, Word: "three", Char: 'c'

    std::cout << "\nModifying elements through zip:" << std::endl;
    std::vector<int> v1 = {10, 20};
    std::vector<int> v2 = {30, 40};
    for (auto&& [x, y] : zip_utils::zip(v1, v2)) {
        x += 1;
        y *= 2;
    }
    // v1 is now {11, 21}
    // v2 is now {60, 80}
    std::cout << "v1: "; for(int i : v1) std::cout << i << " "; std::cout << std::endl;
    std::cout << "v2: "; for(int i : v2) std::cout << i << " "; std::cout << std::endl;
}
```

## `zip_utils::enumerate`

### Features
-   **Indexed Iteration:** Iterate over a container while getting both the index and the element.
-   **Reference Semantics:** Provides a reference to the original element, allowing modification if the container is non-const.
-   **Lvalue/Rvalue Handling:** Works with both lvalue and rvalue containers.

### How it Works
-   The `enumerate(...)` factory function returns an `EnumerateView<ViewType>`.
-   `EnumerateView` stores the input container (or a reference to it).
-   It provides `begin()` and `end()` methods that return `EnumerateView::iterator`.
-   `EnumerateView::iterator` internally holds the current index and an iterator to the underlying container's element.
-   Dereferencing (`operator*`) an `EnumerateView::iterator` returns a `std::pair<std::size_t, ElementReferenceType>` (e.g., `std::pair<size_t, int&>`).

### Usage Example

```cpp
#include "zip_view.h" // Adjust path as needed
#include <iostream>
#include <vector>
#include <string>

int main() {
    std::vector<std::string> items = {"apple", "banana", "cherry"};

    std::cout << "Enumerating items:" << std::endl;
    for (auto&& [index, item_ref] : zip_utils::enumerate(items)) {
        std::cout << "Index: " << index << ", Item: \"" << item_ref << "\"" << std::endl;
        if (index == 1) {
            item_ref = "blueberry"; // Modify original vector
        }
    }
    // Example Output:
    // Index: 0, Item: "apple"
    // Index: 1, Item: "banana"
    // Index: 2, Item: "cherry"

    std::cout << "\nItems after modification:" << std::endl;
    for (const auto& item : items) {
        std::cout << item << " "; // apple blueberry cherry
    }
    std::cout << std::endl;

    // Enumerating a const container
    const std::vector<int> const_numbers = {10, 20, 30};
    std::cout << "\nEnumerating const container:" << std::endl;
    for (auto&& [idx, num_ref] : zip_utils::enumerate(const_numbers)) {
        std::cout << "Idx: " << idx << ", Num: " << num_ref << std::endl;
        // num_ref = 5; // This would be a compile error, as num_ref is const int&
    }
}
```

## Dependencies
- `<tuple>`, `<iterator>`, `<type_traits>`, `<utility>`
- Standard library containers like `<vector>`, `<list>`, `<string>`, `<deque>` are used in examples.

These utilities provide convenient, Python-inspired iteration patterns for C++17, enhancing code readability and expressiveness when dealing with multiple sequences or indexed iteration.

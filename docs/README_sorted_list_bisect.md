# `SortedList<T, Compare>`

## Overview

The `SortedList<T, Compare>` class (`sorted_list_bisect.h`) implements a dynamic container that automatically maintains its elements in sorted order. It is built upon `std::vector` as its underlying storage. Whenever elements are inserted, they are placed in their correct sorted position. Duplicates are allowed.

This container is suitable when:
-   You need a collection that is always sorted.
-   Frequent sorted iteration or fast lookups (O(log N)) are required.
-   Insertions and deletions are not extremely frequent or the collection size is moderate, as these operations can take O(N) time due to element shifting in the underlying vector.

The "bisect" in the filename likely refers to its reliance on binary search algorithms (like `std::lower_bound`) for finding insertion points and for search operations.

## Template Parameters

-   `T`: The type of elements to be stored in the list.
-   `Compare`: A binary predicate that defines the strict weak ordering for the elements.
    -   Defaults to `std::less<T>`, resulting in ascending sort order.
    -   Can be customized (e.g., `std::greater<T>` for descending order, or a custom functor for complex types).

## Features

-   **Automatic Sorting:** Elements are always kept in sorted order according to the `Compare` functor.
-   **Duplicate Elements Allowed:** Unlike `std::set`, `SortedList` permits duplicate values.
-   **Logarithmic Search:** Operations like `find()`, `contains()`, `lower_bound()`, `upper_bound()`, `count()`, `index_of()` have O(log N) time complexity.
-   **Linear Time Insertion/Deletion:** `insert()`, `emplace()`, and `erase()` operations are typically O(N) due to the need to shift elements in the underlying vector. `pop_back()` is O(1).
-   **Read-Only Element Access:** All direct element access methods (`at()`, `operator[]`, `front()`, `back()`) and iterators provide `const` access to the elements. This is a safety measure to prevent modifications that could break the sorted order.
-   **STL-like Interface:** Offers many methods familiar from `std::vector` and sorted associative containers.
-   **Custom Comparators:** Supports user-defined comparison logic.

## Public Interface Highlights

### Constructors
-   **`SortedList()`**: Default constructor.
-   **`explicit SortedList(const Compare& comp)`**: Constructor with a custom comparator.
-   **`SortedList(std::initializer_list<T> init, const Compare& comp = Compare())`**: Constructs and sorts elements from an initializer list.

### Capacity
-   **`size_type size() const noexcept`**
-   **`bool empty() const noexcept`**
-   **`void clear() noexcept`**
-   **`void reserve(size_type capacity)`**
-   **`size_type capacity() const noexcept`**
-   **`void shrink_to_fit()`**

### Element Access (Read-Only)
-   **`const_reference at(size_type index) const`**: Access element by index with bounds checking.
-   **`const_reference operator[](size_type index) const noexcept`**: Access element by index (no bounds check).
-   **`const_reference front() const`**: Access the first (smallest or largest, depending on `Compare`) element.
-   **`const_reference back() const`**: Access the last (largest or smallest) element.

### Modifiers
-   **`void insert(const T& value)` / `void insert(T&& value)`**: Inserts `value`, maintaining sort order. (O(N))
-   **`template <typename... Args> iterator emplace(Args&&... args)`**: Constructs element in-place, maintaining sort order. Returns `const_iterator`. (O(N))
-   **`bool erase(const T& value)`**: Removes the first occurrence of `value`. (O(N))
-   **`void erase_at(size_type index)`**: Removes element at `index`. (O(N))
-   **`iterator erase(const_iterator pos)` / `iterator erase(const_iterator first, const_iterator last)`**: Removes element(s) at iterator position(s). Returns `const_iterator`. (O(N))
-   **`void pop_front()`**: Removes the first element. (O(N))
-   **`void pop_back()`**: Removes the last element. (O(1))

### Search Operations (Logarithmic Time)
-   **`size_type index_of(const T& value) const`**: Returns index of first occurrence; throws `std::runtime_error` if not found.
-   **`size_type lower_bound(const T& value) const`**: Index of first element not less than `value`.
-   **`size_type upper_bound(const T& value) const`**: Index of first element greater than `value`.
-   **`const_iterator find(const T& value) const`**: Iterator to first `value`, or `end()` if not found.
-   **`bool contains(const T& value) const`**: True if `value` is present.
-   **`size_type count(const T& value) const`**: Number of occurrences of `value`.

### Range Operations
-   **`std::vector<T> range(const T& low, const T& high) const`**: Returns a vector of elements `x` such that `!comp(x, low)` and `comp(x, high)`. (Effectively `[low, high)` for `std::less`).
-   **`std::pair<size_type, size_type> range_indices(const T& low, const T& high) const`**: Returns start and end indices for the range.

### Iterators (Provide `const` access to elements)
-   **`const_iterator begin() / end() const noexcept`**
-   **`const_iterator cbegin() / cend() const noexcept`**
-   **`const_reverse_iterator rbegin() / rend() const noexcept`**
-   **`const_reverse_iterator crbegin() / crend() const noexcept`**

### Comparison Operators
-   `operator==`, `operator!=`, `operator<`, `operator<=`, `operator>`, `operator>=` (lexicographical comparison).

## Usage Examples

(Based on `examples/use_sorted_list.cpp`)

### Basic Integer List (Ascending Order)

```cpp
#include "sorted_list_bisect.h" // Adjust path as needed
#include <iostream>
#include <vector>

int main() {
    SortedList<int> sorted_numbers; // Default: std::less<int>

    sorted_numbers.insert(30);
    sorted_numbers.insert(10);
    sorted_numbers.insert(20);
    sorted_numbers.insert(10); // Duplicates are allowed

    std::cout << "Sorted numbers: ";
    for (int num : sorted_numbers) { // Iteration is ordered
        std::cout << num << " ";
    }
    std::cout << std::endl; // Output: 10 10 20 30

    std::cout << "Contains 20? " << std::boolalpha << sorted_numbers.contains(20) << std::endl; // true
    std::cout << "Count of 10: " << sorted_numbers.count(10) << std::endl; // 2

    if (!sorted_numbers.empty()) {
        std::cout << "First element: " << sorted_numbers.front() << std::endl; // 10
        std::cout << "Last element: " << sorted_numbers.back() << std::endl;   // 30
    }

    sorted_numbers.erase(10); // Removes one instance of 10
    std::cout << "After erasing one 10: ";
    for (int num : sorted_numbers) std::cout << num << " ";
    std::cout << std::endl; // Output: 10 20 30
}
```

### List with Custom Comparator (Strings, Descending)

```cpp
#include "sorted_list_bisect.h"
#include <iostream>
#include <string>
#include <functional> // For std::greater

int main() {
    SortedList<std::string, std::greater<std::string>> sorted_strings_desc;

    sorted_strings_desc.insert("banana");
    sorted_strings_desc.insert("apple");
    sorted_strings_desc.insert("cherry");

    std::cout << "Strings sorted descending:" << std::endl;
    for (const std::string& s : sorted_strings_desc) {
        std::cout << "  " << s << std::endl;
    }
    // Expected Output:
    //   cherry
    //   banana
    //   apple
}
```

## Dependencies
- `<vector>`, `<algorithm>`, `<functional>`, `<stdexcept>`, `<iterator>`, `<initializer_list>`

`SortedList` is a convenient container when you need a list that automatically maintains sort order and allows for efficient binary search-based lookups, while understanding the O(N) cost of modifications.

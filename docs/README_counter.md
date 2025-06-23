# `Counter<T, Hash, KeyEqual>`

## Overview

The `Counter<T>` class (`counter.h`) is a C++ template class that provides a specialized dictionary (map) for counting the frequency of hashable objects. It is similar in functionality to Python's `collections.Counter`.

A `Counter` stores elements of type `T` as keys and their respective counts (frequencies) as integer values. It offers convenient methods for adding/subtracting counts, retrieving frequencies, finding the most common elements, and performing arithmetic and set-like operations between counters.

## Template Parameters

-   `T`: The type of elements to be counted. This type must be hashable and equality-comparable.
-   `Hash`: The hash function to use for elements of type `T`. Defaults to `std::hash<T>`.
-   `KeyEqual`: The equality comparison function for elements of type `T`. Defaults to `std::equal_to<T>`.

## Features

-   **Frequency Counting:** Easily count occurrences of items from various sources (initializer lists, iterators).
-   **STL-like Interface:** Provides familiar methods like `size()`, `empty()`, `begin()`, `end()`, `contains()`, `erase()`, `clear()`.
-   **Convenient Access:**
    -   `count(const T& value)`: Get count of an element (0 if not present).
    -   `operator[](const T& value)`: Read-only access returns count (0 if not present). Read-write access allows direct modification of counts (inserts element with count 0 if not present).
-   **Arithmetic Operations:** Supports `+`, `-`, `+=`, `-=` for combining or differencing counters.
-   **Set Operations:** `intersection()` (minimum counts) and `union_with()` (maximum counts).
-   **`most_common(size_type n = 0)`:** Returns a sorted list of the `n` most common elements and their counts.
-   **Filtering:** Allows creating new counters based on predicates (`filter()`, `positive()`, `negative()`).
-   **Custom Type Support:** Can be used with user-defined types if appropriate hash and equality functions are provided.
-   **Underlying `std::unordered_map`:** Leverages `std::unordered_map` for efficient average-case performance.

## Public Interface Highlights

### Constructors
```cpp
Counter();
Counter(std::initializer_list<T> init); // Counts elements in list
Counter(InputIt first, InputIt last);    // Counts elements in range
Counter(std::initializer_list<std::pair<T, int>> init); // Initializes with key-count pairs
```

### Core Counting Operations
-   **`void add(const T& value, int count = 1)`** (and rvalue overload): Increases the count of `value`.
-   **`void subtract(const T& value, int count = 1)`**: Decreases the count of `value`. If count becomes <= 0, the element is removed.
-   **`int count(const T& value) const noexcept`**: Returns the count of `value` (0 if absent).
-   **`int operator[](const T& value) const noexcept`**: Read-only access to count.
-   **`int& operator[](const T& value)`**: Read-write access to count (inserts with count 0 if new).
-   **`bool contains(const T& value) const noexcept`**: Checks if `value` is in the counter.
-   **`size_type erase(const T& value)`**: Removes `value` from the counter.

### Iteration & Size
-   **`iterator begin() / end()`**, **`const_iterator cbegin() / cend()`**: Iterate over `std::pair<const T, int>`.
-   **`size_type size() const noexcept`**: Number of unique elements with counts > 0.
-   **`bool empty() const noexcept`**: Checks if empty.
-   **`int total() const noexcept`**: Sum of all counts.

### Advanced Operations
-   **`std::vector<std::pair<T, int>> most_common(size_type n = 0) const`**: Returns `n` most common items.
-   **`Counter operator+(const Counter& other) const`** (and `-`, `+=`, `-=`).
-   **`Counter intersection(const Counter& other) const`**.
-   **`Counter union_with(const Counter& other) const`**.
-   **`template<typename Predicate> Counter filter(Predicate pred) const`**.
-   **`Counter positive() const` / `Counter negative() const`**.

### Underlying Map Access
-   **`const std::unordered_map<T, int, Hash, KeyEqual>& data() const noexcept`**: Access the raw underlying map.
-   Methods like `reserve()`, `bucket_count()`, `load_factor()`, `rehash()`.

## Usage Examples

(Based on `examples/use_counter.cpp`)

### Basic Counting

```cpp
#include "counter.h"
#include <iostream>
#include <string>
#include <vector>

int main() {
    // Create from a list of items
    Counter<std::string> word_counts = {"apple", "banana", "apple", "orange", "apple"};

    std::cout << "Count of 'apple': " << word_counts.count("apple") << std::endl; // 3
    std::cout << "Count of 'banana': " << word_counts["banana"] << std::endl;   // 1
    std::cout << "Count of 'grape': " << word_counts["grape"] << std::endl;    // 0 (read-only access)

    // Add more items
    word_counts.add("banana");
    word_counts.add("orange", 2);

    std::cout << "\nAfter additions:" << std::endl;
    for (const auto& pair : word_counts) { // or `const auto& [word, count]` in C++17
        std::cout << "  " << pair.first << ": " << pair.second << std::endl;
    }
    // Example output:
    //   orange: 3
    //   banana: 2
    //   apple: 3

    word_counts.subtract("apple", 2);
    std::cout << "\nAfter subtracting 2 'apple's: " << word_counts["apple"] << std::endl; // 1
}
```

### `most_common()`

```cpp
#include "counter.h"
#include <iostream>
#include <vector>

int main() {
    Counter<char> char_counts = {'a', 'b', 'a', 'c', 'b', 'a', 'd', 'a', 'b'};
    // Counts: a:4, b:3, c:1, d:1

    std::cout << "All items sorted by frequency:" << std::endl;
    for (const auto& item : char_counts.most_common()) {
        std::cout << "  '" << item.first << "': " << item.second << std::endl;
    }

    std::cout << "\nTop 2 most common characters:" << std::endl;
    for (const auto& item : char_counts.most_common(2)) {
        std::cout << "  '" << item.first << "': " << item.second << std::endl;
    }
    // Output:
    //   'a': 4
    //   'b': 3
}
```

### Arithmetic and Set Operations

```cpp
#include "counter.h"
#include <iostream>

int main() {
    Counter<std::string> c1 = {{"a", 3}, {"b", 2}, {"c", 1}};
    Counter<std::string> c2 = {{"b", 1}, {"c", 4}, {"d", 5}};

    Counter<std::string> c_sum = c1 + c2;
    // c_sum will be: {a:3, b:3, c:5, d:5}
    std::cout << "c1 + c2:" << std::endl;
    for(const auto& p : c_sum) std::cout << "  " << p.first << ": " << p.second << std::endl;


    Counter<std::string> c_intersect = c1.intersection(c2);
    // c_intersect will be: {b:1, c:1} (min counts for common keys)
    std::cout << "\nc1.intersection(c2):" << std::endl;
    for(const auto& p : c_intersect) std::cout << "  " << p.first << ": " << p.second << std::endl;

    Counter<std::string> c_union = c1.union_with(c2);
    // c_union will be: {a:3, b:2, c:4, d:5} (max counts for all keys)
     std::cout << "\nc1.union_with(c2):" << std::endl;
    for(const auto& p : c_union) std::cout << "  " << p.first << ": " << p.second << std::endl;
}
```

## Dependencies
- `<unordered_map>`
- `<vector>`
- `<algorithm>`
- `<utility>`
- `<iterator>`
- `<initializer_list>`
- `<functional>`
- `<type_traits>`

The `Counter` class is a versatile tool for frequency analysis and related tasks, providing a Python-like convenience in C++.

# FrozenList

## Overview

`cpp_collections::FrozenList<T, Allocator>` is a C++ template class that provides an immutable, ordered sequence of elements, similar to `std::vector<T>`, but with the guarantee that its contents cannot be modified after construction. This makes `FrozenList` inherently thread-safe for concurrent reads without needing explicit locking mechanisms, provided its elements `T` are themselves thread-safe for reading.

The `FrozenList` stores its elements in contiguous memory, offering efficient element access and cache performance comparable to `std::vector`. Once a `FrozenList` is created, its size and elements are fixed.

## Purpose and Benefits

-   **Immutability**: Guarantees that the list's content remains constant after creation. This helps in designing more predictable and robust systems, especially in multi-threaded environments or when passing collections between components where accidental modification is undesirable.
-   **Thread Safety (for reads)**: Due to its immutable nature, a `FrozenList` can be safely read by multiple threads simultaneously without external synchronization, assuming the element type `T` itself is safe for concurrent reads.
-   **Performance**: Offers `O(1)` random access to elements. Contiguous storage can lead to good cache locality.
-   **API Familiarity**: Provides a subset of `std::vector`'s interface for element access and iteration, making it easy to use for those familiar with standard C++ containers.

## Construction

A `FrozenList` can be constructed in several ways:

1.  **Default Construction**: Creates an empty `FrozenList`.
    ```cpp
    cpp_collections::FrozenList<int> fl_empty;
    ```

2.  **From an Initializer List**:
    ```cpp
    cpp_collections::FrozenList<std::string> fl_strings = {"apple", "banana", "cherry"};
    ```

3.  **From a Range of Iterators**: (e.g., from another container like `std::vector` or `std::list`)
    ```cpp
    std::vector<double> data_source = {1.0, 2.1, 3.14};
    cpp_collections::FrozenList<double> fl_doubles(data_source.begin(), data_source.end());
    ```

4.  **With a Count and a Value**: Creates a list with `n` copies of a specific value.
    ```cpp
    cpp_collections::FrozenList<char> fl_chars(5, 'z'); // Contains five 'z' characters
    ```

5.  **Copy Construction**:
    ```cpp
    cpp_collections::FrozenList<int> original = {1, 2, 3};
    cpp_collections::FrozenList<int> copy = original;
    ```

6.  **Move Construction**:
    ```cpp
    cpp_collections::FrozenList<int> source = {4, 5, 6};
    cpp_collections::FrozenList<int> moved_list = std::move(source);
    // source is now in a valid but unspecified state (likely empty)
    ```

All constructors ensure that the internal storage is minimized (equivalent to calling `shrink_to_fit()` on a `std::vector`) because the size is fixed upon construction.

## Main Operations

`FrozenList` supports a const-only interface mirroring `std::vector` where appropriate.

### Element Access

-   `operator[](size_type pos) const`: Access element at specific position (no bounds checking).
-   `at(size_type pos) const`: Access element at specific position (with bounds checking, throws `std::out_of_range` if `pos >= size()`).
-   `front() const`: Access the first element (undefined behavior if empty).
-   `back() const`: Access the last element (undefined behavior if empty).
-   `data() const noexcept`: Returns a const pointer to the underlying contiguous storage.

### Iterators (All `const_iterator` types)

-   `begin()`, `cbegin()`: Iterator to the beginning.
-   `end()`, `cend()`: Iterator to the end.
-   `rbegin()`, `crbegin()`: Reverse iterator to the beginning.
-   `rend()`, `crend()`: Reverse iterator to the end.

### Capacity

-   `empty() const noexcept`: Checks if the list is empty.
-   `size() const noexcept`: Returns the number of elements.
-   `max_size() const noexcept`: Returns the maximum possible number of elements.

### Comparison

Standard comparison operators (`==`, `!=`, `<`, `<=`, `>`, `>=`) are provided, performing lexicographical comparisons.

### Hashing

A specialization `std::hash<cpp_collections::FrozenList<T, Allocator>>` is provided, allowing `FrozenList` instances to be used as keys in unordered associative containers (e.g., `std::unordered_map`, `std::unordered_set`), provided `std::hash<T>` is available.

### Swap

Both member `swap` and a non-member `std::swap` overload are available.

### Assignment

`FrozenList` supports copy, move, and initializer list assignment. Assignment creates a new internal state for the list.

```cpp
cpp_collections::FrozenList<int> list;
list = {1, 2, 3}; // Initializer list assignment

cpp_collections::FrozenList<int> another_list = {4, 5, 6};
list = another_list; // Copy assignment

list = std::move(another_list); // Move assignment
```

## Usage Example

```cpp
#include "FrozenList.h"
#include <iostream>
#include <string>
#include <vector>

int main() {
    // Create a FrozenList from an initializer list
    cpp_collections::FrozenList<std::string> words = {"Frozen", "List", "is", "immutable"};

    // Print its size
    std::cout << "Size: " << words.size() << std::endl; // Output: Size: 4

    // Iterate and print elements
    std::cout << "Elements: ";
    for (const auto& word : words) {
        std::cout << word << " ";
    }
    std::cout << std::endl; // Output: Elements: Frozen List is immutable

    // Access elements by index
    if (words.size() > 1) {
        std::cout << "First word: " << words.front() << std::endl; // Output: First word: Frozen
        std::cout << "Second word: " << words[1] << std::endl;   // Output: Second word: List
    }

    // Attempting to modify would result in a compile error:
    // words.push_back("error"); // Compile error: no member named 'push_back'
    // words[0] = "Melted";      // Compile error: operator[] returns const_reference

    // Hash the list
    std::hash<cpp_collections::FrozenList<std::string>> hasher;
    std::cout << "Hash value: " << hasher(words) << std::endl;

    return 0;
}

```

## Template Parameters

-   `T`: The type of elements stored in the list.
-   `Allocator`: The allocator type to be used for memory management, defaults to `std::allocator<T>`.

This structure provides a robust, immutable list that can be a valuable addition to C++ projects requiring fixed collections with thread-safe read access.

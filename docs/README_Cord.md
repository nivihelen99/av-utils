# Cord Data Structure

## Overview

A `Cord` is a data structure used to represent and efficiently manipulate large sequences of characters, typically strings. It is designed to make operations like concatenation, substring, and prepending/appending faster than with traditional flat string representations (like `std::string`) when dealing with very long strings, by avoiding large data copies.

Cords achieve this by representing the string as a tree (often a binary tree, sometimes a B-tree like structure) of smaller string fragments. Leaf nodes in this tree hold the actual character data, while internal nodes represent the concatenation of their children.

## Features of this Implementation

*   **Rope-like Structure**: Internally, this `Cord` uses a tree of nodes. Leaf nodes store `std::string` fragments, and internal nodes represent concatenation.
*   **Efficient Concatenation**: Concatenating two `Cord` objects typically involves creating a new internal node that points to the existing Cords, rather than copying all character data. This can be significantly faster for large strings.
*   **Efficient Substrings (Conceptual)**: While the current `substr` implementation might not be fully optimized for copy-avoidance in all tree rebalancing scenarios, the design allows for future enhancements where substrings can often be represented by new `Cord` objects that share parts of the original tree structure.
*   **Shared Data**: `std::shared_ptr` is used for managing tree nodes, allowing for structural sharing of common underlying data between different Cord instances (e.g., after copy or certain operations).
*   **Basic String API**: Provides common string operations:
    *   Construction from `const char*` and `std::string`.
    *   `length()`, `empty()`, `clear()`.
    *   `operator+` for concatenation.
    *   `at()`, `operator[]` for character access.
    *   `substr()` for extracting substrings.
    *   `ToString()` for converting the Cord's content to a flat `std::string`.

## Internal Structure

The `Cord` class primarily manages a pointer to its root node, which is represented by `cord_detail::NodeVariant`. This variant can be either:

1.  `cord_detail::LeafNode`: Contains a `std::string` fragment.
2.  `std::shared_ptr<cord_detail::InternalNode>`: Points to an internal node.

An `cord_detail::InternalNode` has two children, `left` and `right`, which are `std::shared_ptr<const Cord>`. This means an internal node represents the concatenation of two (potentially shared and immutable) child Cords. The `InternalNode` also stores `length_left`, the length of its left child, to speed up operations like character access and substring.

## Usage

```cpp
#include "cord.h"
#include <iostream>

int main() {
    // Creation
    Cord c1("Hello, ");
    Cord c2("World!");
    Cord c3(" This is a long string segment.");

    // Concatenation
    Cord greeting = c1 + c2;
    std::cout << "Greeting: " << greeting.ToString() << std::endl; // "Hello, World!"
    std::cout << "Length: " << greeting.length() << std::endl;   // 12

    Cord full_text = greeting + c3;
    std::cout << "Full text: " << full_text.ToString() << std::endl;
    std::cout << "Full text length: " << full_text.length() << std::endl;

    // Character access
    if (full_text.length() > 7) {
        std::cout << "Character at index 7: " << full_text.at(7) << std::endl; // 'W'
    }

    // Substring
    Cord sub = full_text.substr(7, 5); // "World"
    std::cout << "Substring (7, 5): " << sub.ToString() << std::endl;

    return 0;
}
```

## API Reference

### Constructors
*   `Cord()`: Default constructor, creates an empty Cord.
*   `Cord(const char* s)`: Creates a Cord from a C-style string.
*   `Cord(const std::string& s)`: Creates a Cord from an `std::string`.
*   `Cord(std::string&& s)`: Creates a Cord by moving from an `std::string`.
*   `Cord(const Cord& other)`: Copy constructor (efficient, shares underlying data).
*   `Cord(Cord&& other) noexcept`: Move constructor.

### Assignment Operators
*   `operator=(const Cord& other)`
*   `operator=(Cord&& other) noexcept`
*   `operator=(const std::string& s)`
*   `operator=(std::string&& s)`
*   `operator=(const char* s)`

### Basic Operations
*   `size_t length() const`: Returns the total length of the string.
*   `bool empty() const`: Checks if the Cord is empty.
*   `void clear()`: Clears the Cord, making it empty.

### Concatenation
*   `Cord operator+(const Cord& other) const`
*   `Cord operator+(const std::string& s_other) const`
*   `Cord operator+(const char* c_other) const`
*   `friend Cord operator+(const std::string& s_lhs, const Cord& rhs)`
*   `friend Cord operator+(const char* c_lhs, const Cord& rhs)`

### Character Access
*   `char at(size_t index) const`: Returns character at `index`. Throws `std::out_of_range` if `index` is invalid.
*   `char operator[](size_t index) const`: Returns character at `index`. Behavior is undefined if `index` is out of bounds (like `std::string`).

### Substring
*   `Cord substr(size_t pos = 0, size_t count = std::string::npos) const`: Returns a new Cord representing the substring. Throws `std::out_of_range` if `pos` is invalid.

### Conversion
*   `std::string ToString() const`: Converts the Cord to a flat `std::string`. This operation can be expensive for very large Cords as it involves copying all character data.

## Future Considerations
*   **Balancing**: The current tree structure is not explicitly balanced. For very deep trees resulting from many small concatenations, performance might degrade. A balancing strategy (e.g., like ropes or B-trees) could be implemented.
*   **Small String Optimization (SSO)**: Leaf nodes currently use `std::string`. For very short fragments, SSO within the leaf or storing small strings directly in the Cord structure could reduce overhead.
*   **Copy-on-Write (CoW)**: While `std::shared_ptr` provides structural sharing, true CoW for modifications (if mutable operations were added) would be more complex. The current design leans towards immutability for shared structures.
*   **More Advanced Operations**: Iterators, find/replace, streaming input/output.
*   **Memory Management**: Finer-grained control over memory allocation for nodes if `std::string`'s allocator isn't sufficient.

## Performance Characteristics (General Idea)
*   **Concatenation**: O(1) or O(log N) for balanced trees, not O(N) like `std::string`.
*   **Length**: O(1) as it's stored.
*   **Character Access (`at`, `[]`)**: O(log N) in a balanced tree, potentially O(N) in a degenerate tree (current simple binary tree could be O(depth)).
*   **Substring**: Can be O(log N) to create new Cord sharing structure, not O(N) data copy. Actual character copy is deferred until `ToString()` or character iteration.
*   `ToString()`: O(N) as it must copy all characters.
The current simple binary tree implementation might exhibit O(depth) for access/substring, where depth can be O(N) in worst-case concatenation patterns.
A balanced tree (e.g., an AVL or Red-Black tree managing Cord nodes, or a B-tree structure) would guarantee O(log N) for these operations.
This initial version prioritizes the structural idea of Cords over complex balancing algorithms.

# `BiMap<Left, Right>`

## Overview

The `BiMap<Left, Right>` class (`bimap.h`) implements a bidirectional map, also known as a bijective map. It maintains a strict one-to-one relationship between elements of type `Left` and elements of type `Right`. This means that each `Left` value maps to a unique `Right` value, and conversely, each `Right` value maps to a unique `Left` value.

You can efficiently look up:
- The `Right` value associated with a `Left` key.
- The `Left` value associated with a `Right` key.

The implementation uses two internal `std::unordered_map`s to provide average O(1) time complexity for insertions, deletions, and lookups.

## Features

-   **One-to-One Mapping:** Enforces that both `Left` and `Right` values are unique across all stored pairs.
-   **Bidirectional Lookup:** Efficiently find values using either `Left` or `Right` keys (`at_left`, `at_right`, `find_left`, `find_right`).
-   **View-Based Iteration:** Provides `left()` and `right()` views, each with its own iterators, allowing iteration over `(Left, Right)` pairs or `(Right, Left)` pairs respectively.
-   **STL Compatibility:**
    -   Iterators are compatible with STL algorithms.
    -   Provides common type aliases like `size_type`.
    -   Supports range-based for loops.
-   **Comprehensive API:** Includes methods for insertion (`insert`, `insert_or_assign`, `emplace`, `try_emplace_left`/`right`), erasure, clearing, size, emptiness checks, and swapping.
-   **Move Semantics:** Supports move semantics for efficient insertion and assignment of rvalue objects.
-   **Rule of Five:** Properly implemented copy/move constructors and assignment operators.
-   **Comparison Operators:** `operator==` and `operator!=` are provided.

## Core Components

### Internal Structure
-   `std::unordered_map<Left, Right> left_to_right_`
-   `std::unordered_map<Right, Left> right_to_left_`

### `left_view` and `right_view`
Nested classes that provide access to the bimap from one perspective:
-   `bimap.left()`: Iterates over `std::pair<const Left, Right>`.
-   `bimap.right()`: Iterates over `std::pair<const Right, Left>`.
Each view has its own `begin()`, `end()`, `at()`, `find()`, `contains()`, `size()`, and `empty()` methods.

### Iterators
-   Custom `bimap_iterator` that adapts to iterate over either the left or right view.
-   Default iteration over the `BiMap` object itself iterates the `left_view`.

## Public Interface Highlights

### Construction & Assignment
```cpp
BiMap<std::string, int> users; // Default constructor
BiMap<std::string, int> users_copy = users; // Copy constructor
users_copy = users; // Copy assignment
BiMap<std::string, int> users_moved = std::move(users); // Move constructor
users_moved = std::move(users_copy); // Move assignment
```

### Insertion
-   **`bool insert(const Left& left, const Right& right)`** (and rvalue overloads)
    -   Inserts if neither `left` nor `right` key already exists. Returns `true` on success.
-   **`void insert_or_assign(const Left& left, const Right& right)`** (and rvalue overloads)
    -   Inserts the pair. If `left` exists, its mapping is updated. If `right` exists, its mapping is updated. Any old conflicting associations are removed to maintain one-to-one mapping.
-   **`std::pair<left_iterator, bool> emplace(Args&&... args)`**
    -   Constructs `std::pair<const Left, Right>` in-place. Rolls back if reverse mapping fails.
-   **`std::pair<left_iterator, bool> try_emplace_left(const/&& Left& k, Args&&... args)`**
-   **`std::pair<right_iterator, bool> try_emplace_right(const/&& Right& k, Args&&... args)`**
    -   Conditionally emplaces if both the new key and the constructed value do not violate uniqueness.

### Lookup & Access
-   **`const Right& at_left(const Left& key) const`** (throws `std::out_of_range` if not found)
-   **`const Left& at_right(const Right& key) const`** (throws `std::out_of_range` if not found)
-   **`const Right* find_left(const Left& key) const noexcept`** (returns `nullptr` if not found)
-   **`const Left* find_right(const Right& key) const noexcept`** (returns `nullptr` if not found)
-   **`bool contains_left(const Left& key) const noexcept`**
-   **`bool contains_right(const Right& key) const noexcept`**

### Erasure
-   **`bool erase_left(const Left& key)`**
-   **`bool erase_right(const Right& key)`**
-   **`left_iterator erase_left(left_const_iterator pos)`**
-   **`right_iterator erase_right(right_const_iterator pos)`**

### Other Operations
-   **`void clear() noexcept`**
-   **`size_type size() const noexcept`**
-   **`bool empty() const noexcept`**
-   **`void swap(BiMap& other) noexcept`** (member and non-member `std::swap` via ADL)
-   **`bool operator==(const BiMap& other) const`**
-   **`bool operator!=(const BiMap& other) const`**

### View Access
-   **`left_view left() const noexcept`**
-   **`right_view right() const noexcept`**
-   `BiMap` itself is iterable (defaults to `left_view`).

## Usage Examples

(Based on `examples/use_bimap.cpp`)

### Basic Operations

```cpp
#include "bimap.h"
#include <iostream>
#include <string>

int main() {
    BiMap<std::string, int> name_to_id;

    // Insertion
    name_to_id.insert("Alice", 101);
    name_to_id.insert("Bob", 102);
    bool success = name_to_id.insert("Alice", 103); // Fails, "Alice" already exists
    std::cout << "Insertion of (Alice, 103) " << (success ? "succeeded" : "failed") << std::endl;

    success = name_to_id.insert("Charlie", 101); // Fails, ID 101 already exists
    std::cout << "Insertion of (Charlie, 101) " << (success ? "succeeded" : "failed") << std::endl;

    // Lookup
    try {
        std::cout << "Alice's ID: " << name_to_id.at_left("Alice") << std::endl; // 101
        std::cout << "Name for ID 102: " << name_to_id.at_right(102) << std::endl; // Bob
    } catch (const std::out_of_range& e) {
        std::cerr << "Lookup error: " << e.what() << std::endl;
    }

    if (name_to_id.contains_left("David")) {
        std::cout << "David found." << std::endl;
    } else {
        std::cout << "David not found." << std::endl;
    }

    // Erasure
    name_to_id.erase_left("Alice");
    std::cout << "After erasing Alice, size is: " << name_to_id.size() << std::endl; // 1
    std::cout << "Contains Alice? " << std::boolalpha << name_to_id.contains_left("Alice") << std::endl; // false
    std::cout << "Contains ID 101? " << std::boolalpha << name_to_id.contains_right(101) << std::endl; // false
}
```

### Iterating with Views and STL Algorithms

```cpp
#include "bimap.h"
#include <iostream>
#include <string>
#include <algorithm> // For std::for_each

int main() {
    BiMap<std::string, int> country_codes;
    country_codes.insert("USA", 1);
    country_codes.insert("Canada", 1); // This will fail due to duplicate right value '1'
    country_codes.insert("Canada", 2); // Assume this is the intended unique mapping
    country_codes.insert("Mexico", 52);
    country_codes.insert_or_assign("USA", 44); // Updates USA to 44, removes old mapping of 1.

    std::cout << "Iterating left view (Country to Code):" << std::endl;
    for (const auto& pair : country_codes.left()) {
        std::cout << "  " << pair.first << " -> " << pair.second << std::endl;
    }
    // Or using structured binding (C++17):
    // for (const auto& [country, code] : country_codes.left()) {
    //     std::cout << "  " << country << " -> " << code << std::endl;
    // }


    std::cout << "\nIterating right view (Code to Country):" << std::endl;
    std::for_each(country_codes.right().begin(), country_codes.right().end(),
        [](const std::pair<const int, std::string>& pair) {
            std::cout << "  " << pair.first << " -> " << pair.second << std::endl;
        }
    );
}

```

### `insert_or_assign`

```cpp
#include "bimap.h"
#include <iostream>
#include <string>

int main() {
    BiMap<std::string, int> user_pins;
    user_pins.insert("userA", 1234);
    user_pins.insert("userB", 5678);

    std::cout << "userA PIN: " << user_pins.at_left("userA") << std::endl; // 1234
    std::cout << "PIN 5678 belongs to: " << user_pins.at_right(5678) << std::endl; // userB

    // Case 1: Update userA's PIN. Old PIN 1234 becomes free.
    user_pins.insert_or_assign("userA", 9999);
    std::cout << "userA new PIN: " << user_pins.at_left("userA") << std::endl; // 9999
    std::cout << "Is PIN 1234 still mapped? " << std::boolalpha << user_pins.contains_right(1234) << std::endl; // false

    // Case 2: Assign PIN 5678 (currently userB's) to userC. userB's mapping is removed.
    user_pins.insert_or_assign("userC", 5678);
    std::cout << "userC PIN: " << user_pins.at_left("userC") << std::endl; // 5678
    std::cout << "Is userB still mapped? " << std::boolalpha << user_pins.contains_left("userB") << std::endl; // false
    std::cout << "PIN 5678 now belongs to: " << user_pins.at_right(5678) << std::endl; // userC
}
```

## Dependencies
- `<unordered_map>`
- `<stdexcept>` (for `std::out_of_range`)
- `<utility>` (for `std::pair`, `std::move`, etc.)
- `<iostream>` (used in example, and potentially for debugging in BiMap)
- `<algorithm>`, `<vector>` (used in example)

The `BiMap` class offers a powerful and flexible way to manage one-to-one relationships between two distinct types of data.

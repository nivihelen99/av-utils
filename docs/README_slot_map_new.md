# `utils::SlotMapNew<T>`

## Overview

The `utils::SlotMapNew<T>` class (`slot_map_new.h`) implements a "slot map" data structure. A slot map is a container that stores objects in a way that provides stable identifiers (keys) even when elements are added or removed. This is achieved by using a generation counter associated with each slot. When a slot is reused, its generation is incremented, invalidating old keys that pointed to the previous occupant of that slot.

This implementation aims for efficient element access (O(1) once a key is validated) and memory reuse by maintaining a free list of available slots. It is particularly useful for managing collections of objects like entities in a game engine or simulation, where stable references are needed despite frequent creation and deletion.

`SlotMapNew<T>` is designed to be move-only.

## Key Features

-   **Stable Keys:** Elements are accessed via a `Key` struct (`{uint32_t index, uint32_t generation}`). Keys remain valid until the specific element they refer to (matching both index and generation) is erased.
-   **Generation Tracking:** Solves the "ABA problem" or stale key issue. When a slot is erased and reused, its generation is incremented, invalidating old keys.
-   **Efficient Access:** `get()`, `at()`, `operator[]` provide O(1) access after key validation.
-   **Memory Reuse:** Freed slots are recycled for new insertions via a free list.
-   **In-Place Construction:** `emplace()` allows constructing objects directly in their allocated slots.
-   **Explicit Lifetime Management:** Correctly calls destructors for `T` upon erasure or clearing, and uses placement new for constructing in reused slots.
-   **Iterators:** Provides custom forward iterators that correctly skip over inactive (freed) slots, allowing range-based for loops over active elements.
-   **Move-Only Semantics:** The `SlotMapNew` itself is move-only (copy operations are deleted).

## `Key` Structure
```cpp
struct Key {
    uint32_t index;
    uint32_t generation;

    constexpr bool operator==(const Key& other) const noexcept;
    constexpr bool operator!=(const Key& other) const noexcept;
    constexpr bool operator<(const Key& other) const noexcept; // For use in ordered containers
    constexpr bool is_valid() const noexcept; // Checks against INVALID_KEY
};
static constexpr Key INVALID_KEY = { /* ... */ };
```

## Public Interface Highlights

### Constructors & Lifetime
-   **`SlotMapNew()`**: Default constructor.
-   **`explicit SlotMapNew(size_type reserve_size)`**: Reserves initial capacity.
-   Move constructor and move assignment are defaulted. Copy operations are deleted.

### Element Management
-   **`template<typename... Args> Key emplace(Args&&... args)`**: Constructs an element in-place and returns its `Key`.
-   **`Key insert(T value)`**: Inserts `value` (by moving) and returns its `Key`.
-   **`bool erase(Key key) noexcept`**: Erases the element identified by `key`. Returns `true` if successful. Calls destructor of `T`.
-   **`void clear() noexcept`**: Erases all elements, calling their destructors and updating generations.

### Element Access
-   **`T* get(Key key) noexcept` / `const T* get(Key key) const noexcept`**: Returns a pointer to the element if `key` is valid and active, else `nullptr`.
-   **`T& at(Key key)` / `const T& at(Key key) const`**: Returns a reference to the element. Throws `std::out_of_range` if `key` is invalid or inactive.
-   **`T& operator[](Key key) noexcept` / `const T& operator[](Key key) const noexcept`**: Unchecked access (asserts key validity in debug builds).
-   **`bool contains(Key key) const noexcept`**: Checks if `key` refers to a valid, active element.

### Capacity & Size
-   **`void reserve(size_type capacity)` / `void shrink_to_fit()`**.
-   **`size_type size() const noexcept`**: Number of active elements.
-   **`size_type capacity() const noexcept`**: Current storage capacity.
-   **`bool empty() const noexcept`**: Checks if there are no active elements.
-   **`size_type max_size() const noexcept`**.

### Iterators
Custom forward iterators (`iterator`, `const_iterator`) are provided that skip inactive slots.
-   **`iterator begin() / end()`**
-   **`const_iterator cbegin() / cend()`**
-   Dereferencing an iterator yields a `std::pair<Key, T&>` (or `std::pair<Key, const T&>`).

## Usage Examples

(Based on `examples/use_slot_map_new.cpp`)

### Basic Operations

```cpp
#include "slot_map_new.h" // Adjust path as needed
#include <iostream>
#include <string>

int main() {
    utils::SlotMapNew<std::string> names;

    // Insert elements
    auto key_alice = names.emplace("Alice");
    auto key_bob = names.insert("Bob");
    utils::SlotMapNew<std::string>::Key key_charlie = names.emplace("Charlie");

    std::cout << "Size: " << names.size() << std::endl; // 3

    // Access elements
    if (std::string* p_alice = names.get(key_alice)) {
        std::cout << "Alice's name: " << *p_alice << std::endl;
        *p_alice = "Alicia"; // Modify through pointer
    }
    std::cout << "Bob's name (operator[]): " << names[key_bob] << std::endl;
    std::cout << "Charlie's name (at): " << names.at(key_charlie) << std::endl;
    std::cout << "Alicia's new name: " << names[key_alice] << std::endl;


    // Erase an element
    bool bob_erased = names.erase(key_bob);
    std::cout << "Bob erased? " << std::boolalpha << bob_erased << std::endl; // true
    std::cout << "Size after erase: " << names.size() << std::endl; // 2
    std::cout << "Contains Bob? " << names.contains(key_bob) << std::endl; // false

    // Add a new element, potentially reusing Bob's slot
    auto key_dave = names.emplace("Dave");
    std::cout << "Added Dave. Size: " << names.size() << std::endl; // 3
    std::cout << "Dave's key - Index: " << key_dave.index
              << ", Generation: " << key_dave.generation << std::endl;
    // If Bob's slot was reused, key_dave.index might be same as key_bob.index,
    // but key_dave.generation will be different from key_bob.generation.

    // Try accessing with Bob's old key (should fail due to generation mismatch)
    if (!names.get(key_bob)) {
        std::cout << "Correctly cannot access data with Bob's old key." << std::endl;
    }
}
```

### Iteration

```cpp
#include "slot_map_new.h"
#include <iostream>
#include <string>

struct GameObject { std::string name; int id; };

int main() {
    utils::SlotMapNew<GameObject> game_objects;
    auto k1 = game_objects.emplace("Player", 1);
    auto k2 = game_objects.emplace("EnemyA", 2);
    game_objects.emplace("Item", 3);

    game_objects.erase(k2); // Remove EnemyA
    game_objects.emplace("EnemyB", 4); // Might reuse EnemyA's slot

    std::cout << "\nActive Game Objects:" << std::endl;
    for (auto pair_ref : game_objects) { // Iterates over active elements
        // pair_ref is effectively std::pair<Key, GameObject&>
        utils::SlotMapNew<GameObject>::Key current_key = pair_ref.first;
        GameObject& obj = pair_ref.second;
        std::cout << "  Key(idx:" << current_key.index << ",gen:" << current_key.generation << ") "
                  << "-> Name: " << obj.name << ", ID: " << obj.id << std::endl;
    }
}
```

## Dependencies
- `<vector>`, `<cstdint>`, `<limits>`, `<cassert>`, `<type_traits>`, `<utility>`, `<algorithm>`, `<stdexcept>`

`SlotMapNew` provides a robust and modern implementation of the slot map pattern, suitable for managing dynamic collections of objects where stable identifiers and efficient reuse of memory are important.

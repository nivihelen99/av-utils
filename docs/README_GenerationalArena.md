# GenerationalArena

**Header:** `GenerationalArena.h`

## Overview

`GenerationalArena<T>` is a C++ data structure that provides efficient allocation and deallocation of objects of a fixed type `T`. It's particularly useful in scenarios where many objects of the same type are created and destroyed frequently, such as in game development (e.g., managing entities, particles), simulations, or complex algorithms that require temporary object pools.

The "generational" aspect refers to the use of generation counts associated with each slot in the arena. When an object is allocated, it's given a handle that includes the slot's index and its current generation. When the object is deallocated, the slot's generation is incremented. This mechanism helps prevent the "ABA problem" by ensuring that a handle for a previously deallocated object becomes invalid, even if the same slot is later reused for a new object.

## Features

*   **Fast Allocation/Deallocation:** Objects are allocated from pre-reserved or newly added slots, typically faster than general-purpose allocators like `new`. Deallocation is also efficient.
*   **Stable Handles:** Objects are accessed via `ArenaHandle`s. These handles remain valid even if other objects are allocated or deallocated, as long as the specific object they point to is still alive. Pointers to objects obtained via `get()` are only valid as long as the object is not deallocated and the arena itself is not modified in a way that invalidates pointers (e.g., reallocation of the underlying slot vector, though `GenerationalArena` uses `std::vector` which can reallocate).
*   **Reduced Fragmentation:** By allocating objects of the same type contiguously (or in chunks if the arena grows), it can help reduce memory fragmentation compared to many individual allocations.
*   **ABA Problem Mitigation:** Generational handles help detect stale handles, improving safety when objects are frequently created and destroyed.
*   **Cache-Friendly (Potentially):** Storing objects of the same type together can improve cache locality, though the sparse nature (due to deallocations) means iteration might not always be perfectly cache-efficient without compaction (which this arena does not do automatically).
*   **Header-Only:** Easy to integrate by including `GenerationalArena.h`.

## Template Parameters

*   `typename T`: The type of objects to be stored in the arena.

## Public Interface

### `ArenaHandle`

A public struct used to refer to objects within the `GenerationalArena`.

```cpp
struct ArenaHandle {
    uint32_t index;      // Index of the slot in the arena
    uint32_t generation; // Generation count of the slot

    bool operator==(const ArenaHandle& other) const;
    bool operator!=(const ArenaHandle& other) const;
    bool is_null() const; // Checks if the handle is a default-constructed "null" handle
    static ArenaHandle null(); // Returns a "null" handle
};

// std::hash<cpp_collections::ArenaHandle> is specialized.
```

### `GenerationalArena<T>` Member Functions

*   **Constructors & Destructor:**
    *   `GenerationalArena()`: Default constructor.
    *   `explicit GenerationalArena(size_t initial_capacity)`: Constructor that reserves space for `initial_capacity` slots.
    *   `~GenerationalArena()`: Destructor; ensures all active objects are properly destructed.
*   **Move Semantics:**
    *   `GenerationalArena(GenerationalArena&& other) noexcept`: Move constructor.
    *   `GenerationalArena& operator=(GenerationalArena&& other) noexcept`: Move assignment operator.
    *   Copy constructor and copy assignment operator are deleted.
*   **Allocation & Deallocation:**
    *   `template <typename... Args> handle_type allocate(Args&&... args)`: Allocates and constructs an object of type `T` using the provided arguments. Returns a handle to the new object.
    *   `void deallocate(handle_type handle)`: Deallocates the object associated with the given handle. If the handle is invalid, the operation is a no-op.
*   **Access & Validity:**
    *   `T* get(handle_type handle)`: Returns a pointer to the object if the handle is valid; otherwise, returns `nullptr`.
    *   `const T* get(handle_type handle) const`: Const version of `get`.
    *   `bool is_valid(handle_type handle) const`: Checks if the given handle currently refers to a live object in the arena.
*   **Capacity & Size:**
    *   `size_t size() const noexcept`: Returns the number of active objects currently in the arena.
    *   `bool empty() const noexcept`: Returns `true` if the arena contains no active objects, `false` otherwise.
    *   `size_t capacity() const noexcept`: Returns the total number of slots (active or free) currently allocated by the arena.
    *   `void reserve(size_t new_capacity)`: Increases the capacity of the arena to at least `new_capacity` slots.
    *   `void clear()`: Deallocates all objects in the arena and resets its state. All existing handles become invalid.
*   **Iterators:**
    *   `iterator begin()` / `iterator end()`: Returns iterators to the beginning and end of the sequence of active objects.
    *   `const_iterator begin() const` / `const_iterator end() const`: Const versions.
    *   `const_iterator cbegin() const` / `const_iterator cend() const`: Const versions.
    *   The iterators are forward iterators and skip over inactive slots. The order of iteration is by slot index, which may not correspond to allocation order if slots are reused.

## Usage Example

```cpp
#include "GenerationalArena.h"
#include <iostream>
#include <string>

struct GameObject {
    int id;
    std::string type;

    GameObject(int i, std::string t) : id(i), type(std::move(t)) {}

    void print() const {
        std::cout << "GameObject { id: " << id << ", type: \"" << type << "\" }" << std::endl;
    }
};

int main() {
    cpp_collections::GenerationalArena<GameObject> entity_arena;

    // Allocate entities
    cpp_collections::ArenaHandle player_handle = entity_arena.allocate(1, "Player");
    cpp_collections::ArenaHandle enemy_handle = entity_arena.allocate(2, "EnemyNPC");

    std::cout << "Arena size: " << entity_arena.size() << std::endl; // Output: 2

    // Access entities
    if (GameObject* player = entity_arena.get(player_handle)) {
        player->print(); // Output: GameObject { id: 1, type: "Player" }
    }

    // Deallocate an entity
    entity_arena.deallocate(enemy_handle);
    std::cout << "Is enemy_handle valid? " << (entity_arena.is_valid(enemy_handle) ? "Yes" : "No") << std::endl; // Output: No

    // Allocate a new entity - might reuse enemy_handle's slot
    cpp_collections::ArenaHandle item_handle = entity_arena.allocate(101, "HealthPotion");
    if (GameObject* item = entity_arena.get(item_handle)) {
        item->print();
    }

    // Old enemy_handle is still invalid, even if its slot was reused
    if (entity_arena.get(enemy_handle) == nullptr) {
        std::cout << "Access to old enemy_handle correctly returns nullptr." << std::endl;
    }

    std::cout << "\nIterating through arena:" << std::endl;
    for (const auto& entity : entity_arena) {
        entity.print();
    }

    entity_arena.clear();
    std::cout << "\nArena size after clear: " << entity_arena.size() << std::endl; // Output: 0

    return 0;
}
```

## Notes

*   **Thread Safety:** `GenerationalArena` is not thread-safe. If used in a multi-threaded context, external synchronization is required.
*   **Exception Safety:** Provides basic exception safety. If `T`'s constructor throws during `allocate`, the arena remains in a valid state.
*   **Pointer Invalidation:** Pointers obtained via `get()` are invalidated if the object is deallocated or `clear()` is called. Iterators are invalidated by any operation that modifies the arena's structure (allocate, deallocate, clear, reserve if it causes reallocation). Handles remain valid across non-destructive operations on *other* objects.
*   **Object Type `T`:** `T` must be movable for move construction/assignment of the arena itself to be efficient (though the current arena implementation relies on `std::vector`'s move semantics for its internal `slots_` which store `Slot` objects, not `T` directly). `T` must be destructible.

This data structure provides a good balance of performance for object management with the added safety of generational handles.

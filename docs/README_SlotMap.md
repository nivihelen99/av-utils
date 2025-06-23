# `SlotMap<T>`

## Overview

The `SlotMap<T>` (found in `utils/SlotMap.h`, possibly an older version given the include guard `UTILS_SLOTMAP_OLD_H`) is a container that stores elements of type `T` and provides stable access to them through a `Key` object. It is designed for scenarios where you need persistent identifiers for objects that might be added or removed frequently, while also preventing issues caused by "stale keys" (keys that refer to a slot that has been reused by a different object).

This is achieved by using a generation counter for each slot. A `Key` is only considered valid if its generation matches the current generation of the slot it points to. This ensures that if an object is removed and its slot is later reused for a new object, old keys pointing to that slot become invalid.

SlotMaps typically offer O(1) average time complexity for insertion, deletion, and access.

## Key Features

-   **Stable Keys:** Elements are accessed via a `Key` struct, which remains valid even if other elements are inserted or removed (unless the specific element pointed to by the key is itself removed).
-   **Generation Tracking:** Each slot has a generation counter. Keys also store a generation. Access is only granted if the key's generation matches the slot's current generation, preventing accidental access to reused slots with old keys.
-   **Efficient Operations:** Insertion, deletion, and lookup are generally O(1) on average.
-   **Memory Reuse:** Freed slots are added to a free list and are reused for subsequent insertions, which helps in reducing memory fragmentation and keeping the underlying storage compact.

## `Key` Structure

```c++
struct Key {
    uint32_t index;      // Index into the internal data store
    uint32_t generation; // Generation counter for the slot

    bool operator==(const Key& other) const = default;
    bool operator!=(const Key& other) const = default;
};
```

## Public Interface

(Within `namespace utils`)

-   **`Key insert(T value)`**
    -   Inserts `value` into the `SlotMap`.
    -   Reuses a free slot if available, otherwise appends to the internal storage.
    -   Returns a `Key` that can be used to access the inserted element.

-   **`bool erase(Key key)`**
    -   Removes the element associated with `key`.
    -   The slot is added to a free list for reuse.
    -   The generation of the slot is incremented, invalidating `key` and any other copies of it.
    -   Returns `true` if erasure was successful (key was valid), `false` otherwise.

-   **`T* get(Key key)`**
    -   Returns a pointer to the element identified by `key`.
    -   Returns `nullptr` if the `key` is invalid (e.g., wrong index, mismatched generation, or slot is free).

-   **`const T* get(Key key) const`**
    -   Const-qualified version of `get(Key key)`.

-   **`bool contains(Key key) const`**
    -   Checks if `key` currently refers to a valid, live element in the `SlotMap`.
    -   Returns `true` if valid, `false` otherwise.

-   **`size_t size() const`**
    -   Returns the number of active elements currently stored in the `SlotMap`.

-   **`bool empty() const`**
    -   Returns `true` if the `SlotMap` contains no active elements, `false` otherwise.

## Usage Examples

(Based on `examples/slotmap_example.cpp`)

### Basic Operations

```cpp
#include "SlotMap.h" // Assuming 'utils' namespace is used within SlotMap.h
#include <iostream>
#include <string>

// Helper to print key details
void print_sm_key(const utils::SlotMap<std::string>::Key& key) {
    std::cout << "Key(idx: " << key.index << ", gen: " << key.generation << ")";
}

int main() {
    utils::SlotMap<std::string> string_map;

    std::cout << "Initial size: " << string_map.size() << ", empty: " << string_map.empty() << std::endl;

    // Insert elements
    utils::SlotMap<std::string>::Key key_hello = string_map.insert("Hello");
    std::cout << "Inserted \"Hello\", got "; print_sm_key(key_hello); std::cout << std::endl;

    utils::SlotMap<std::string>::Key key_world = string_map.insert("World");
    std::cout << "Inserted \"World\", got "; print_sm_key(key_world); std::cout << std::endl;

    std::cout << "Size after inserts: " << string_map.size() << std::endl;

    // Retrieve elements
    if (const std::string* val = string_map.get(key_hello)) {
        std::cout << "Value for key_hello: " << *val << std::endl;
    } else {
        std::cout << "Value for key_hello: not found or invalid key" << std::endl;
    }

    // Check contains
    std::cout << "Map contains key_world? " << std::boolalpha << string_map.contains(key_world) << std::endl;

    // Erase an element
    std::cout << "Erasing key_world..." << std::endl;
    bool erased = string_map.erase(key_world);
    std::cout << "Erase successful: " << erased << std::endl;
    std::cout << "Size after erase: " << string_map.size() << std::endl;
    std::cout << "Map contains key_world after erase? " << std::boolalpha << string_map.contains(key_world) << std::endl; // false

    // Attempt to retrieve erased element (using the old key_world)
    if (const std::string* val = string_map.get(key_world)) {
        std::cout << "Value for key_world (post-erase): " << *val << " <- Should not happen!" << std::endl;
    } else {
        std::cout << "Value for key_world (post-erase): nullptr (Correct, key is stale)" << std::endl;
    }
}
```

### Stale Key Invalidation and Slot Reuse

```cpp
#include "SlotMap.h"
#include <iostream>
#include <string>

// (print_sm_key helper function as above)

int main() {
    utils::SlotMap<std::string> my_map;

    auto key_a = my_map.insert("Apple");
    auto key_b = my_map.insert("Banana");
    std::cout << "Inserted Apple with "; print_sm_key(key_a); std::cout << std::endl;
    std::cout << "Inserted Banana with "; print_sm_key(key_b); std::cout << std::endl;

    // Erase "Apple"
    my_map.erase(key_a);
    std::cout << "Erased Apple (key_a)." << std::endl;
    std::cout << "my_map.contains(key_a): " << std::boolalpha << my_map.contains(key_a) << std::endl; // false

    // Insert "Cherry", which might reuse Apple's slot
    auto key_c = my_map.insert("Cherry");
    std::cout << "Inserted Cherry with "; print_sm_key(key_c); std::cout << std::endl;

    // key_a is now stale. Its index might be the same as key_c's index,
    // but its generation will be different.
    std::cout << "Attempting to get value with stale key_a: ";
    if (const std::string* val = my_map.get(key_a)) {
        std::cout << *val << " <- Error, stale key access!" << std::endl;
    } else {
        std::cout << "nullptr (Correct, key_a is stale)" << std::endl;
    }

    std::cout << "Attempting to get value with new key_c: ";
    if (const std::string* val = my_map.get(key_c)) {
        std::cout << *val << std::endl; // Should print "Cherry"
    } else {
        std::cout << "nullptr <- Error, key_c should be valid!" << std::endl;
    }

    std::cout << "key_a.index: " << key_a.index << ", key_a.generation: " << key_a.generation << std::endl;
    std::cout << "key_c.index: " << key_c.index << ", key_c.generation: " << key_c.generation << std::endl;
    // Observe if key_a.index == key_c.index and key_a.generation != key_c.generation
}
```

## Internal Structure

-   `std::vector<T> data_`: Stores the actual element data.
-   `std::vector<uint32_t> generations_`: Stores the generation count for each corresponding slot in `data_`.
-   `std::vector<size_t> free_list_`: A list of indices pointing to slots in `data_` that are currently unused and available for new insertions.

This implementation provides a robust way to manage collections of objects where stable, safe identifiers are required throughout the objects' lifetimes.
The name of the header file (`SlotMap.h`) and the include guard (`UTILS_SLOTMAP_OLD_H`) might indicate this is an older version. A `slot_map_new.h` also exists in the include directory, which might be a newer or different implementation.

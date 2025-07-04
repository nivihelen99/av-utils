# PackedSlotMap

## Overview

`PackedSlotMap<T>` is a data structure that stores a collection of objects of type `T` and provides stable keys for accessing them. Unlike a standard `std::vector`, keys (which are a combination of an index and a generation count) remain valid even when other elements are erased from the map. A key feature of `PackedSlotMap` is that it stores all active elements contiguously in memory. This provides excellent cache performance for iterating over all elements.

It's an alternative to a traditional `SlotMap` where the underlying data storage might become sparse after many erasures. `PackedSlotMap` ensures data locality for active elements by moving the last element in the contiguous storage into the slot of an erased element.

## Key Features

-   **Stable Keys**: `PackedSlotMap::Key` (composed of an index and a generation) provides stable access to elements. A key remains valid until its specific element is erased.
-   **Contiguous Storage**: Active elements are stored contiguously in an underlying `std::vector`, leading to efficient iteration and good cache locality.
-   **Efficient Operations**:
    -   Insertion: Amortized O(1) (due to potential vector reallocation).
    -   Erasure: O(1) (involves moving data from the end of the storage).
    -   Lookup: O(1).
-   **Generation Tracking**: Keys include a generation count. If a slot is reused after an element is erased, old keys with a previous generation become invalid, preventing access to unrelated new data.

## API

### Key Structure

```cpp
struct PackedSlotMap<T>::Key {
    uint32_t slot_idx;
    uint32_t generation;
    // bool operator==(const Key& other) const;
    // bool operator!=(const Key& other) const;
};
```

### Main Methods

-   `Key insert(const T& value)`: Inserts a copy of `value`.
-   `Key insert(T&& value)`: Moves `value` into the map.
-   `template <typename... Args> Key emplace(Args&&... args)`: Constructs an element in-place.
-   `bool erase(Key key)`: Erases the element associated with `key`. Returns `true` if successful.
-   `T* get(Key key)`: Returns a pointer to the element if `key` is valid, else `nullptr`.
-   `const T* get(Key key) const`: Const version of `get`.
-   `bool contains(Key key) const`: Checks if `key` is valid and points to an active element.
-   `size_type size() const`: Returns the number of active elements.
-   `bool empty() const`: Checks if the map is empty.
-   `void clear()`: Removes all elements.
-   `void reserve(size_type new_cap)`: Reserves capacity for `data_` storage.
-   `iterator begin(), iterator end()`: Iterators for accessing elements.
-   `const_iterator begin() const, const_iterator end() const`: Const iterators.
-   `const_iterator cbegin() const, const_iterator cend() const`: Const iterators.

## Usage Example

```cpp
#include "PackedSlotMap.h" // Or appropriate path
#include <iostream>
#include <string>

struct MyData { std::string info; int id; };

int main() {
    PackedSlotMap<MyData> psm;

    PackedSlotMap<MyData>::Key key_a = psm.insert({"Data A", 1});
    PackedSlotMap<MyData>::Key key_b = psm.emplace("Data B", 2);

    if (MyData* data_a = psm.get(key_a)) {
        std::cout << "Found A: " << data_a->info << ", ID: " << data_a->id << std::endl;
    }

    psm.erase(key_a); // Erase Data A

    if (!psm.contains(key_a)) {
        std::cout << "Key A is no longer valid." << std::endl;
    }

    // Iterate over remaining elements (guaranteed contiguous)
    for (const MyData& data : psm) {
        std::cout << "Iterated: " << data.info << ", ID: " << data.id << std::endl;
    }

    return 0;
}
```

## Performance Characteristics

-   **Iteration**: Very fast due to contiguous memory layout (cache-friendly). Equivalent to iterating a `std::vector`.
-   **Insertion (`insert`/`emplace`)**: Typically O(1) amortized. If the underlying `std::vector` for data or slots needs to reallocate, it can be O(N).
-   **Erasure (`erase`)**: O(1). Involves:
    1.  Validating the key.
    2.  Marking the slot as inactive and incrementing its generation.
    3.  Moving the last element from the data vector into the position of the element being erased.
    4.  Updating the slot information for the moved element.
    5.  Popping the last element from the data vector.
-   **Lookup (`get`/`contains`)**: O(1). Involves:
    1.  Accessing the slot entry.
    2.  Validating generation and active status.
    3.  Accessing the data entry using the index from the slot.

## Comparison with `SlotMap`

The repository might also contain a standard `SlotMap`. Here's how `PackedSlotMap` differs:

| Feature             | `PackedSlotMap`                                   | `SlotMap` (Typical)                               |
| ------------------- | ------------------------------------------------- | ------------------------------------------------- |
| **Data Storage**    | Active elements are contiguous.                   | Active elements may become sparse (non-contiguous).|
| **Iteration**       | Cache-friendly, like `std::vector`.               | Potentially less cache-friendly if sparse.        |
| **Erasure Effect**  | Moves last element; may change order of elements. | Leaves a hole (managed by free list); order preserved. |
| **Pointer Stability**| Pointers to other elements are stable. Pointers to the *moved* element during an erase are invalidated (but its key remains valid and points to its new location). | Pointers to other elements are stable.            |
| **Use Cases**       | Prioritizes fast iteration and data locality. Good when element order is not critical. | Good when pointer/reference stability to *all* elements is paramount and iteration performance over sparse data is acceptable. |

Choose `PackedSlotMap` when you need stable identifiers (keys) and frequent, fast iteration over all active elements is more critical than preserving the relative order of elements or the absolute stability of pointers to elements that might be moved during an erase operation.

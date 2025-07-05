# SparseSet

## Overview

`SparseSet<T, IndexT, AllocatorT, AllocatorIndexT>` is a C++ data structure that stores a set of items (typically non-negative integers like entity IDs or resource handles) from a potentially large universal set `[0, MaxValue)`. It is designed to provide O(1) average time complexity for add, remove, and contains operations, along with cache-friendly iteration over the elements currently present in the set.

This structure is particularly useful in scenarios requiring efficient management of dynamic sets of IDs where the range of possible IDs is large, but the number of active IDs at any given time is relatively small. A common use case is in Entity-Component-System (ECS) architectures in game development or simulations.

## Template Parameters

-   `T`: The type of elements stored. This type must be usable as an array index (convertible to `IndexT` or `size_t`) and should typically represent non-negative integer values. Defaults to `uint32_t`.
-   `IndexT`: The type used for indices within the internal dense array and for storing the count of elements. Defaults to `uint32_t`.
-   `AllocatorT`: The allocator type for storing elements of type `T` in the dense array. Defaults to `std::allocator<T>`.
-   `AllocatorIndexT`: The allocator type for storing indices of type `IndexT` in the sparse array. Defaults to `std::allocator_traits<AllocatorT>::template rebind_alloc<IndexT>`.

## How it Works

The `SparseSet` uses two internal arrays (typically `std::vector`):
1.  `dense_`: A tightly packed array storing the actual elements that are currently in the set. Iteration happens over this array.
2.  `sparse_`: An array indexed by the element values themselves. For an element `e`, `sparse_[e]` stores the index of `e` within the `dense_` array.

A `count_` member variable tracks the number of valid elements currently stored in the `dense_` array.

-   **`contains(value)`**: Checks `sparse_[value]`. If the index stored there is less than `count_`, and `dense_[sparse_[value]] == value`, then the element is present. This two-part check ensures validity even if `sparse_` contains stale data.
-   **`insert(value)`**: If not already present and within `max_value_capacity`, `value` is added to the end of `dense_` (at `dense_[count_]`), and `sparse_[value]` is set to `count_`. `count_` is then incremented.
-   **`erase(value)`**: If present, the element `value` at `dense_[idx]` (where `idx = sparse_[value]`) is removed by:
    1.  Taking the last element from `dense_` (at `dense_[count_ - 1]`).
    2.  Placing this last element into `dense_[idx]`.
    3.  Updating the sparse array for this moved element: `sparse_[last_element] = idx`.
    4.  Decrementing `count_`.
    This swap-and-pop technique maintains the dense packing and O(1) complexity.

## API

### Construction
-   `SparseSet(size_type max_value, size_type initial_dense_capacity = 0, const AllocatorT& alloc_t = AllocatorT(), const AllocatorIndexT& alloc_idx_t = AllocatorIndexT())`:
    Constructs a `SparseSet`. `max_value` defines the exclusive upper limit for element values (i.e., elements `0` to `max_value - 1` can be stored). `sparse_` array is sized to `max_value`. `initial_dense_capacity` can be used to pre-reserve memory for the `dense_` array.

### Modifiers
-   `std::pair<iterator, bool> insert(const_reference value)`: Inserts `value`. Returns `true` and iterator to element if inserted, `false` and iterator if already present or out of range. O(1) average.
-   `bool erase(const_reference value)`: Erases `value`. Returns `true` if erased, `false` otherwise. O(1).
-   `void clear() noexcept`: Removes all elements. O(N) to clear dense vector, or O(1) if only count is reset (current implementation clears dense vector).
-   `void swap(SparseSet& other) noexcept`: Swaps contents with another `SparseSet`.

### Lookup
-   `bool contains(const_reference value) const noexcept`: Checks if `value` is in the set. O(1).
-   `iterator find(const_reference value)`: Returns iterator to `value` if found, else `end()`. O(1).
-   `const_iterator find(const_reference value) const`: Const version of `find`. O(1).

### Capacity
-   `bool empty() const noexcept`: Returns `true` if size is 0. O(1).
-   `size_type size() const noexcept`: Returns the number of elements. O(1).
-   `size_type max_value_capacity() const noexcept`: Returns `max_value` set at construction (size of `sparse_` array).
-   `size_type dense_capacity() const noexcept`: Returns current capacity of the `dense_` array.
-   `void reserve_dense(size_type new_cap)`: Reserves memory for `dense_` array.

### Iterators
-   `iterator begin() noexcept`, `iterator end() noexcept`
-   `const_iterator begin() const noexcept`, `const_iterator end() const noexcept`
-   `const_iterator cbegin() const noexcept`, `const_iterator cend() const noexcept`
    Iterators are random access and iterate over the elements in the `dense_` array. The order of elements is generally the insertion order mixed by erasures; it is *not* sorted by value.

### Comparison
-   `bool operator==(const SparseSet& other) const`: Checks for set equality (same elements, regardless of `max_value_capacity` or internal order).
-   `bool operator!=(const SparseSet& other) const`: Inverse of `operator==`.

## Time and Space Complexity

-   **Time Complexity**:
    -   `insert`, `erase`, `contains`, `find`: O(1) average.
    -   `clear`: O(1) (resets count, `dense_.clear()` is O(N) where N is current size but often fast).
    -   Iteration: O(N) where N is the number of elements currently in the set.
-   **Space Complexity**: O(MaxValue + N), where `MaxValue` is the `max_value_capacity` given at construction (for the `sparse_` array) and N is the number of elements currently stored (for the `dense_` array).

## Usage Example

```cpp
#include "sparse_set.h" // Adjust path as necessary
#include <iostream>

int main() {
    // Create a SparseSet for entity IDs up to 999.
    cpp_collections::SparseSet<uint32_t> active_entities(1000);

    // Add some entities
    active_entities.insert(10);
    active_entities.insert(55);
    active_entities.insert(120);

    std::cout << "Number of active entities: " << active_entities.size() << std::endl;

    // Check if an entity is active
    if (active_entities.contains(55)) {
        std::cout << "Entity 55 is active." << std::endl;
    }

    if (!active_entities.contains(100)) {
        std::cout << "Entity 100 is not active." << std::endl;
    }

    // Iterate over active entities (order is not guaranteed to be sorted by ID)
    std::cout << "Active entity IDs: ";
    for (uint32_t id : active_entities) {
        std::cout << id << " ";
    }
    std::cout << std::endl;

    // Remove an entity
    active_entities.erase(55);
    std::cout << "Entity 55 removed. New size: " << active_entities.size() << std::endl;

    // Clear all entities
    active_entities.clear();
    std::cout << "All entities cleared. Size: " << active_entities.size() << std::endl;

    return 0;
}

```

This documentation provides an overview of the `SparseSet`, its API, complexity, and a basic usage example.

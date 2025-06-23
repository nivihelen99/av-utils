# `IDAllocator<IDType>`

## Overview

The `IDAllocator<IDType>` is a C++ template class designed to manage a pool of unique IDs within a specified numerical range. It supports allocation of individual IDs, freeing of IDs for reuse, explicit reservation of IDs, and allocation/release of contiguous ranges of IDs. The allocator prioritizes reusing freed IDs before allocating new ones sequentially.

This component is useful in scenarios where resources need to be identified by unique numerical handles, such as managing network ports, entity IDs in a game or simulation, or any system requiring dynamic ID assignment from a limited set.

## Template Parameter

-   `IDType`: The underlying integral type for the IDs (e.g., `int`, `short`, `unsigned long`). It should support comparison, increment, and subtraction.

## Public Interface

### Constructor

-   **`IDAllocator(IDType min_id, IDType max_id)`**
    -   Constructs an `IDAllocator` to manage IDs in the inclusive range `[min_id, max_id]`.
    -   Throws `std::invalid_argument` if `min_id > max_id`.

### Single ID Operations

-   **`std::optional<IDType> allocate()`**
    -   Allocates a single available ID.
    -   It first attempts to reuse an ID from the pool of previously freed IDs (smallest available freed ID is chosen).
    -   If no freed IDs are available, it allocates the next available sequential ID starting from `min_id` (or the ID after the last sequentially allocated one), up to `max_id`.
    -   Skips any IDs that are already allocated or reserved.
    -   Returns an `std::optional<IDType>` containing the allocated ID, or `std::nullopt` if no IDs are available.

-   **`bool free(IDType id)`**
    -   Releases a previously allocated or reserved `id` back to the pool, making it available for future allocations.
    -   Returns `true` if the `id` was successfully freed (i.e., it was in the valid range and marked as used).
    -   Returns `false` if the `id` was out of range or not currently marked as used.

-   **`bool reserve(IDType id)`**
    -   Marks a specific `id` as used, preventing it from being returned by `allocate()`.
    -   This is useful if certain IDs need to be set aside for special purposes.
    -   Returns `true` if the `id` was successfully reserved (i.e., it was in the valid range and not already used).
    -   Returns `false` if the `id` was out of range or already marked as used.

### Range ID Operations

-   **`std::optional<IDType> allocate_range(size_t n)`**
    -   Attempts to allocate a contiguous block of `n` IDs.
    -   This method currently only searches for a suitable block starting from the `next_available_id_` (the ID that would be returned by a sequential allocation if there were no freed IDs). It does not currently scan the `freed_ids_` pool or gaps between allocated segments for ranges.
    -   If a suitable block is found, all IDs in that block are marked as allocated, and the starting ID of the block is returned.
    -   Returns `std::nullopt` if `n` is 0, or if a contiguous block of `n` IDs cannot be found from the `next_available_id_` onwards within the `[min_id, max_id]` bounds.

-   **`bool release_range(IDType start_id, size_t n)`**
    -   Releases a contiguous block of `n` IDs, starting from `start_id`.
    -   All IDs within the specified range `[start_id, start_id + n - 1]` must currently be allocated for the operation to succeed.
    -   If `n` is 0, the function returns `true` without performing any action.
    -   Released IDs are added to the `freed_ids_` pool for potential reuse by `allocate()`.
    -   Returns `true` if the range was successfully released or `n` was 0.
    -   Returns `false` if the range is invalid (e.g., out of `[min_id, max_id]` bounds, `start_id + n - 1` overflows, or any ID within the range is not currently allocated).

### Status and Utility

-   **`bool is_allocated(IDType id) const`**
    -   Checks if a given `id` is currently marked as used (either allocated or reserved).
    -   Returns `true` if used, `false` otherwise.

-   **`size_t used() const`**
    -   Returns the total number of IDs currently marked as used (allocated or reserved).

-   **`size_t capacity() const`**
    -   Returns the total number of IDs that the allocator can manage (`max_id - min_id + 1`).

-   **`size_t available() const`**
    -   Returns the number of IDs that are currently available for allocation (`capacity() - used()`).

-   **`void reset()`**
    -   Resets the allocator to its initial state. All previously allocated and reserved IDs are cleared. The pool of freed IDs is emptied, and the next sequential ID to be allocated is reset to `min_id`.

## Usage Examples

### Basic Allocation and Freeing

```cpp
#include "IDAllocator.h"
#include <iostream>
#include <optional>

void print_status(const IDAllocator<int>& alloc) {
    std::cout << "Used: " << alloc.used() << ", Available: " << alloc.available() << std::endl;
}

int main() {
    IDAllocator<int> allocator(1, 5); // Manages IDs from 1 to 5
    print_status(allocator); // Used: 0, Available: 5

    std::optional<int> id1 = allocator.allocate(); // id1 = 1
    if(id1) std::cout << "Allocated: " << *id1 << std::endl;
    print_status(allocator); // Used: 1, Available: 4

    std::optional<int> id2 = allocator.allocate(); // id2 = 2
    if(id2) std::cout << "Allocated: " << *id2 << std::endl;
    print_status(allocator); // Used: 2, Available: 3

    if(id1) allocator.free(*id1); // Free ID 1
    std::cout << "Freed ID 1" << std::endl;
    print_status(allocator); // Used: 1, Available: 4

    std::optional<int> id3 = allocator.allocate(); // id3 = 1 (reuses freed ID)
    if(id3) std::cout << "Allocated: " << *id3 << std::endl;
    print_status(allocator); // Used: 2, Available: 3
}
```

### Reserving an ID

```cpp
#include "IDAllocator.h"
#include <iostream>
#include <optional>

int main() {
    IDAllocator<int> allocator(10, 20);

    int reserved_id = 15;
    if (allocator.reserve(reserved_id)) {
        std::cout << "ID " << reserved_id << " reserved." << std::endl; // Success
    }
    std::cout << "Is ID 15 allocated? " << std::boolalpha << allocator.is_allocated(15) << std::endl; // true

    // Try to allocate IDs; 15 should be skipped if encountered sequentially
    for (int i = 0; i < 5; ++i) {
        std::optional<int> id = allocator.allocate();
        if (id) {
            std::cout << "Allocated: " << *id << std::endl; // Will allocate 10, 11, 12, 13, 14
            if (*id == reserved_id) {
                std::cout << "Error: Reserved ID was allocated!" << std::endl;
            }
        }
    }
    // Next call to allocate() would give 16 if 10-14 are taken.
}
```

### Allocating and Releasing a Range

```cpp
#include "IDAllocator.h"
#include <iostream>
#include <optional>

void print_status(const IDAllocator<int>& alloc) {
    std::cout << "Used: " << alloc.used() << ", Available: " << alloc.available() << std::endl;
}

int main() {
    IDAllocator<int> allocator(100, 120);
    print_status(allocator); // Used: 0, Available: 21

    size_t range_size = 5;
    std::optional<int> start_id_opt = allocator.allocate_range(range_size);
    if (start_id_opt) {
        int start_id = *start_id_opt;
        std::cout << "Allocated range: " << start_id << " to " << (start_id + range_size - 1) << std::endl;
        // Expected: 100 to 104
    }
    print_status(allocator); // Used: 5, Available: 16

    // Allocate another range
    std::optional<int> start_id2_opt = allocator.allocate_range(3);
     if (start_id2_opt) {
        int start_id2 = *start_id2_opt;
        std::cout << "Allocated range: " << start_id2 << " to " << (start_id2 + 3 - 1) << std::endl;
        // Expected: 105 to 107
    }
    print_status(allocator); // Used: 8, Available: 13

    if (start_id_opt) {
        if (allocator.release_range(*start_id_opt, range_size)) {
            std::cout << "Released range starting at " << *start_id_opt << std::endl;
        }
    }
    print_status(allocator); // Used: 3, Available: 18 (IDs 100-104 are now free)

    // Individual IDs from the released range can now be re-allocated
    std::optional<int> reused_id = allocator.allocate();
    if(reused_id) {
        std::cout << "Re-allocated ID from freed range: " << *reused_id << std::endl; // Expected: 100
    }
    print_status(allocator); // Used: 4, Available: 17
}

```

## Internal Logic

-   **Freed IDs:** Managed by a min-priority queue (`std::priority_queue<IDType, std::vector<IDType>, std::greater<IDType>>`), ensuring that the smallest freed ID is reused first.
-   **Used IDs:** Tracked in a `std::set<IDType>` for efficient lookup (`is_allocated`, and checking for conflicts during allocation/reservation).
-   **Sequential Allocation:** A `next_available_id_` marker is used. When allocating sequentially, the allocator iterates from this marker, skipping any IDs already in `used_ids_`.
-   **Range Allocation:** `allocate_range` currently checks for contiguous free blocks only from `next_available_id_`. It updates `next_available_id_` past the allocated range.
-   **Range Release:** `release_range` adds all IDs in the specified valid range to the `freed_ids_` priority queue.

This structure provides a balance between reusing IDs to keep them compact and efficiently allocating new IDs.

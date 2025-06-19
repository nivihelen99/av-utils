#include "IDAllocator.h"
#include <iostream>
#include <vector> // Required for std::vector
#include <numeric> // Required for std::iota
#include <string> // Required for std::to_string with short

// Template this print_status function to work with any IDAllocator type
template <typename IDType>
void print_status(const IDAllocator<IDType>& allocator) {
    std::cout << "Capacity: " << allocator.capacity()
              << ", Used: " << allocator.used()
              << ", Available: " << allocator.available() << std::endl;
}

int main() {
    std::cout << "Creating IDAllocator<int> for IDs 1 to 10." << std::endl;
    IDAllocator<int> allocator(1, 10);

    std::cout << "Initial state (int):" << std::endl;
    print_status(allocator);
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "Allocating 3 IDs:" << std::endl;
    std::optional<int> id1_int = allocator.allocate();
    std::optional<int> id2_int = allocator.allocate();
    std::optional<int> id3_int = allocator.allocate();

    if (id1_int) std::cout << "Allocated ID: " << *id1_int << std::endl;
    if (id2_int) std::cout << "Allocated ID: " << *id2_int << std::endl;
    if (id3_int) std::cout << "Allocated ID: " << *id3_int << std::endl;
    print_status(allocator);
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "Attempting to allocate all remaining int IDs (7 more):" << std::endl;
    std::vector<std::optional<int>> allocated_ids_int;
    for (int i = 0; i < 7; ++i) {
        allocated_ids_int.push_back(allocator.allocate());
    }
    for (const auto& id_opt : allocated_ids_int) {
        if (id_opt) std::cout << "Allocated ID: " << *id_opt << std::endl;
        else std::cout << "Allocation failed (std::nullopt)" << std::endl;
    }
    print_status(allocator);
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "Attempting to allocate one more int ID (should fail):" << std::endl;
    std::optional<int> id_overflow_int = allocator.allocate();
    if (!id_overflow_int) {
        std::cout << "Allocation failed as expected (std::nullopt)" << std::endl;
    } else {
        std::cout << "Allocated ID: " << *id_overflow_int << " (unexpected)" << std::endl;
    }
    print_status(allocator);
    std::cout << "----------------------------------------" << std::endl;

    if (id2_int) {
        std::cout << "Freeing ID: " << *id2_int << std::endl;
        if (allocator.free(*id2_int)) {
            std::cout << "ID " << *id2_int << " freed successfully." << std::endl;
        } else {
            std::cout << "Failed to free ID " << *id2_int << "." << std::endl;
        }
        print_status(allocator);
    }
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "Allocating again (should reuse the freed int ID " << (id2_int ? std::to_string(*id2_int) : "N/A") << "):" << std::endl;
    std::optional<int> id_reused_int = allocator.allocate();
    if (id_reused_int) {
        std::cout << "Allocated ID: " << *id_reused_int << std::endl;
    } else {
        std::cout << "Allocation failed (std::nullopt)" << std::endl;
    }
    print_status(allocator);
    std::cout << "----------------------------------------" << std::endl;

    int reserve_id_int = 5;
    std::cout << "Reserving int ID: " << reserve_id_int << std::endl;
    if (allocator.is_allocated(reserve_id_int)) {
         std::cout << "ID " << reserve_id_int << " is already allocated. Freeing it first for demonstration." << std::endl;
         allocator.free(reserve_id_int); // Free if it was allocated by the reuse above or earlier
    }
    if (allocator.reserve(reserve_id_int)) {
        std::cout << "ID " << reserve_id_int << " reserved successfully." << std::endl;
    } else {
        std::cout << "Failed to reserve ID " << reserve_id_int << " (maybe out of range or already used and not freed)." << std::endl;
    }
    print_status(allocator);
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "Attempting to allocate int ID " << reserve_id_int << " (should not be allocated if reserved):" << std::endl;
    // To demonstrate this, we need to ensure other IDs are allocated first if reserve_id_int is next in line.
    // Or, simply try to allocate until all are taken and see if 5 is skipped.
    // For simplicity, let's assume the allocator prioritizes non-reserved IDs.
    // The current allocate() logic will skip it if it's encountered via next_available_id_.
    // If it was in freed_ids_, it would be allocated. But reserve() doesn't add to freed_ids_.

    // Let's try to fill up the allocator to see if 5 is skipped.
    // First, free up space if allocator is full
    if (allocator.available() == 0 && id1_int && *id1_int != reserve_id_int) { // Ensure id1_int is not the reserve_id_int
        std::cout << "Freeing ID " << *id1_int << " to make space for testing reservation." << std::endl;
        allocator.free(*id1_int);
    }


    std::cout << "Attempting to allocate an int ID. If " << reserve_id_int << " is the next available via counter, it should be skipped." << std::endl;
    std::optional<int> id_after_reserve_int = allocator.allocate();
    if (id_after_reserve_int) {
        std::cout << "Allocated ID: " << *id_after_reserve_int << std::endl;
        if (*id_after_reserve_int == reserve_id_int) {
            std::cout << "Error: Reserved ID " << reserve_id_int << " was allocated!" << std::endl;
        }
    } else {
        std::cout << "Allocation failed, no IDs available or only reserved ID " << reserve_id_int << " was left." << std::endl;
    }
    print_status(allocator);
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "Filling all available int IDs:" << std::endl;
    int allocated_count_int = 0;
    while(true) {
        std::optional<int> id_int = allocator.allocate();
        if (id_int) {
            std::cout << "Allocated ID: " << *id_int << std::endl;
            allocated_count_int++;
        } else {
            std::cout << "Allocation failed (no more int IDs or only reserved ones left)." << std::endl;
            break;
        }
    }
    print_status(allocator);
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "Checking 'is_allocated' status for int IDs:" << std::endl;
    std::cout << "Is ID 1 allocated? " << (allocator.is_allocated(1) ? "Yes" : "No") << std::endl;
    std::cout << "Is ID " << reserve_id_int << " (reserved) allocated? " << (allocator.is_allocated(reserve_id_int) ? "Yes" : "No") << std::endl;
    int free_check_id_int = 0; // Find a free ID if possible
    bool found_free_for_check_int = false;
    // Assuming capacity gives total number of IDs, min_id is 1.
    for(int i = static_cast<int>(allocator.capacity()); i >= 1; --i) {
        if (!allocator.is_allocated(i)) {
            free_check_id_int = i;
            found_free_for_check_int = true;
            break;
        }
    }
    if(found_free_for_check_int){
        std::cout << "Is ID " << free_check_id_int << " (expected free) allocated? " << (allocator.is_allocated(free_check_id_int) ? "Yes" : "No") << std::endl;
    } else {
        std::cout << "Could not find a free int ID to check (all might be used or reserved)." << std::endl;
    }
    print_status(allocator);
    std::cout << "----------------------------------------" << std::endl;


    std::cout << "Resetting int allocator." << std::endl;
    allocator.reset();
    std::cout << "State after reset (int):" << std::endl;
    print_status(allocator);
    std::cout << "Is ID 1 allocated after reset? " << (allocator.is_allocated(1) ? "Yes" : "No") << std::endl;
    std::cout << "Is ID " << reserve_id_int << " allocated after reset? " << (allocator.is_allocated(reserve_id_int) ? "Yes" : "No") << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "Demonstrating constructor error with max_id < min_id (int):" << std::endl;
    try {
        IDAllocator<int> invalid_allocator_int(10, 1);
    } catch (const std::invalid_argument& e) {
        std::cout << "Caught expected exception: " << e.what() << std::endl;
    }
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "Demonstrating reserving an out-of-range int ID:" << std::endl;
    if (!allocator.reserve(100)) {
        std::cout << "Failed to reserve out-of-range int ID 100, as expected." << std::endl;
    }
    print_status(allocator);
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "Demonstrating freeing an int ID that is not allocated:" << std::endl;
    if (!allocator.free(8)) { // Assuming 8 is not allocated after reset
        std::cout << "Failed to free non-allocated int ID 8, as expected." << std::endl;
    }
    print_status(allocator);
    std::cout << "----------------------------------------" << std::endl;

    // --- Demonstrating with IDAllocator<short> ---
    std::cout << "\n--- Demonstrating with IDAllocator<short> ---" << std::endl;
    IDAllocator<short> short_allocator(100, 105);
    std::cout << "Initial state (short):" << std::endl;
    print_status(short_allocator);

    std::optional<short> short_id1 = short_allocator.allocate();
    if (short_id1) std::cout << "Allocated short ID: " << *short_id1 << std::endl;

    std::optional<short> short_id2 = short_allocator.allocate();
    if (short_id2) std::cout << "Allocated short ID: " << *short_id2 << std::endl;
    print_status(short_allocator);

    if (short_id1) {
        std::cout << "Freeing short ID: " << *short_id1 << std::endl;
        short_allocator.free(*short_id1);
    }
    print_status(short_allocator);

    std::optional<short> short_id3 = short_allocator.allocate(); // Should reuse *short_id1
    if (short_id3) std::cout << "Allocated short ID (reused): " << *short_id3 << std::endl;
    print_status(short_allocator);

    short reserve_id_short = 104;
    std::cout << "Reserving short ID: " << reserve_id_short << std::endl;
    short_allocator.reserve(reserve_id_short);
    print_status(short_allocator);

    std::cout << "Allocating remaining short IDs:" << std::endl;
    for(int i=0; i<4; ++i) { // Try to allocate a few more
        std::optional<short> s_id = short_allocator.allocate();
        if(s_id) std::cout << "Allocated short ID: " << *s_id << std::endl;
        else std::cout << "Allocation failed for short ID." << std::endl;
    }
    print_status(short_allocator);
    std::cout << "Is short ID " << reserve_id_short << " allocated? " << (short_allocator.is_allocated(reserve_id_short) ? "Yes" : "No") << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // --- Demonstrating Range Operations with IDAllocator<int> ---
    std::cout << "\n--- Demonstrating Range Operations with IDAllocator<int> ---" << std::endl;
    IDAllocator<int> range_allocator(1, 20); // New allocator for range tests
    std::cout << "Initial state (range_allocator 1-20):" << std::endl;
    print_status(range_allocator);
    std::cout << "----------------------------------------" << std::endl;

    // Demonstrate allocate_range
    std::cout << "Attempting to allocate a range of 5 IDs..." << std::endl;
    std::optional<int> range1_start = range_allocator.allocate_range(5);
    if (range1_start) {
        std::cout << "Allocated range starting at: " << range1_start.value() << " (IDs: " << range1_start.value() << "-" << range1_start.value() + 4 << ")" << std::endl;
    } else {
        std::cout << "Failed to allocate range of 5 IDs." << std::endl;
    }
    print_status(range_allocator);
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "Attempting to allocate another range of 3 IDs..." << std::endl;
    std::optional<int> range2_start = range_allocator.allocate_range(3);
    if (range2_start) {
        std::cout << "Allocated range starting at: " << range2_start.value() << " (IDs: " << range2_start.value() << "-" << range2_start.value() + 2 << ")" << std::endl;
    } else {
        std::cout << "Failed to allocate range of 3 IDs." << std::endl;
    }
    print_status(range_allocator);
    std::cout << "----------------------------------------" << std::endl;

    // IDs 1-5 allocated (next_available_id_ should be 6)
    // IDs 6-8 allocated (next_available_id_ should be 9)
    // Available for sequential: 9-20 (12 IDs)
    std::cout << "Attempting to allocate a range of 10 IDs (should succeed, next_available_id_ is 9, max is 20)..." << std::endl;
    std::optional<int> range3_start = range_allocator.allocate_range(10); // Needs 9 to 18
    if (range3_start) {
        std::cout << "Allocated range starting at: " << range3_start.value() << " (IDs: " << range3_start.value() << "-" << range3_start.value() + 9 << ")" << std::endl;
    } else {
        std::cout << "Failed to allocate range of 10 IDs." << std::endl;
    }
    print_status(range_allocator); // next_available_id_ should be 19
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "Attempting to allocate a range of 5 IDs (should fail, only 19-20 available sequentially)..." << std::endl;
    std::optional<int> range4_start = range_allocator.allocate_range(5);
    if (range4_start) {
        std::cout << "Allocated range starting at: " << range4_start.value() << " (IDs: " << range4_start.value() << "-" << range4_start.value() + 4 << ")" << std::endl;
    } else {
        std::cout << "Failed to allocate range of 5 IDs as expected." << std::endl;
    }
    print_status(range_allocator);
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "Demonstrating allocate_range(1)..." << std::endl;
    std::optional<int> single_alloc_from_range = range_allocator.allocate_range(1); // Should allocate 19
    if (single_alloc_from_range) {
        std::cout << "allocate_range(1) allocated ID: " << single_alloc_from_range.value() << std::endl;
    } else {
        std::cout << "allocate_range(1) failed." << std::endl;
    }
    print_status(range_allocator);
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "Demonstrating allocate_range(0)..." << std::endl;
    std::optional<int> zero_alloc_from_range = range_allocator.allocate_range(0);
    if (!zero_alloc_from_range) {
        std::cout << "allocate_range(0) returned std::nullopt as expected." << std::endl;
    } else {
        std::cout << "allocate_range(0) allocated ID: " << zero_alloc_from_range.value() << " (unexpected)" << std::endl;
    }
    print_status(range_allocator);
    std::cout << "----------------------------------------" << std::endl;

    // Demonstrate release_range
    if (range1_start) {
        std::cout << "Attempting to release range " << range1_start.value() << "-" << range1_start.value() + 4 << " (5 IDs)..." << std::endl;
        bool released_range1 = range_allocator.release_range(range1_start.value(), 5);
        std::cout << "Release range " << range1_start.value() << "-" << range1_start.value() + 4 << ": " << (released_range1 ? "Success" : "Failure") << std::endl;
        print_status(range_allocator);
        std::cout << "----------------------------------------" << std::endl;

        std::cout << "Attempting to allocate ID " << range1_start.value() << " individually (should succeed from freed pool)..." << std::endl;
        std::optional<int> realloc_freed = range_allocator.allocate();
        if (realloc_freed) {
            std::cout << "Allocated ID: " << realloc_freed.value() << std::endl;
        } else {
            std::cout << "Failed to reallocate from freed pool." << std::endl;
        }
        print_status(range_allocator);
        std::cout << "----------------------------------------" << std::endl;
    }

    std::cout << "Attempting to release a partially unallocated range (e.g., 100-104, but allocator is 1-20)..." << std::endl;
    bool released_invalid_range = range_allocator.release_range(100, 5);
    std::cout << "Release invalid range (100-104): " << (released_invalid_range ? "Success (unexpected)" : "Failure (expected)") << std::endl;
    print_status(range_allocator);
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "Attempting to release a range not fully allocated (e.g., ID 2 was re-allocated, try releasing 1-5)..." << std::endl;
    // Assuming ID 1 was re-allocated from the freed pool, 2-5 were part of range1_start and are now free
    // Let's try to release 2-4 (which are free)
    bool released_not_allocated = range_allocator.release_range(2, 3);
    std::cout << "Release range 2-4 (should be free): " << (released_not_allocated ? "Success (unexpected)" : "Failure (expected)") << std::endl;
    print_status(range_allocator);
    std::cout << "----------------------------------------" << std::endl;

    if (range1_start) { // If range1 was 1-5 and ID 1 was reallocated, 1 is used.
        std::cout << "Attempting to release already released range (partially, e.g., " << range1_start.value() + 1 << "-" << range1_start.value() + 4 << ")..." << std::endl;
        bool released_already_freed = range_allocator.release_range(range1_start.value() + 1, 4);
        std::cout << "Release range " << range1_start.value()+1 << "-" << range1_start.value()+4 << ": " << (released_already_freed ? "Success (unexpected)" : "Failure (expected)") << std::endl;
        print_status(range_allocator);
        std::cout << "----------------------------------------" << std::endl;
    }

    std::cout << "Demonstrating release_range with n=0..." << std::endl;
    bool released_zero_range = range_allocator.release_range(1, 0);
    std::cout << "Release range with n=0: " << (released_zero_range ? "Success (expected)" : "Failure (unexpected)") << std::endl;
    print_status(range_allocator);
    std::cout << "----------------------------------------" << std::endl;

    return 0;
}

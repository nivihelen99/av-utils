#include "IDAllocator.h"
#include <iostream>
#include <vector> // Required for std::vector
#include <numeric> // Required for std::iota

void print_status(const IDAllocator& allocator) {
    std::cout << "Capacity: " << allocator.capacity()
              << ", Used: " << allocator.used()
              << ", Available: " << allocator.available() << std::endl;
}

int main() {
    std::cout << "Creating IDAllocator for IDs 1 to 10." << std::endl;
    IDAllocator allocator(1, 10);

    std::cout << "Initial state:" << std::endl;
    print_status(allocator);
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "Allocating 3 IDs:" << std::endl;
    std::optional<int> id1 = allocator.allocate();
    std::optional<int> id2 = allocator.allocate();
    std::optional<int> id3 = allocator.allocate();

    if (id1) std::cout << "Allocated ID: " << *id1 << std::endl;
    if (id2) std::cout << "Allocated ID: " << *id2 << std::endl;
    if (id3) std::cout << "Allocated ID: " << *id3 << std::endl;
    print_status(allocator);
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "Attempting to allocate all remaining IDs (7 more):" << std::endl;
    std::vector<std::optional<int>> allocated_ids;
    for (int i = 0; i < 7; ++i) {
        allocated_ids.push_back(allocator.allocate());
    }
    for (const auto& id_opt : allocated_ids) {
        if (id_opt) std::cout << "Allocated ID: " << *id_opt << std::endl;
        else std::cout << "Allocation failed (std::nullopt)" << std::endl;
    }
    print_status(allocator);
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "Attempting to allocate one more ID (should fail):" << std::endl;
    std::optional<int> id_overflow = allocator.allocate();
    if (!id_overflow) {
        std::cout << "Allocation failed as expected (std::nullopt)" << std::endl;
    } else {
        std::cout << "Allocated ID: " << *id_overflow << " (unexpected)" << std::endl;
    }
    print_status(allocator);
    std::cout << "----------------------------------------" << std::endl;

    if (id2) {
        std::cout << "Freeing ID: " << *id2 << std::endl;
        if (allocator.free(*id2)) {
            std::cout << "ID " << *id2 << " freed successfully." << std::endl;
        } else {
            std::cout << "Failed to free ID " << *id2 << "." << std::endl;
        }
        print_status(allocator);
    }
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "Allocating again (should reuse the freed ID " << (id2 ? std::to_string(*id2) : "N/A") << "):" << std::endl;
    std::optional<int> id_reused = allocator.allocate();
    if (id_reused) {
        std::cout << "Allocated ID: " << *id_reused << std::endl;
    } else {
        std::cout << "Allocation failed (std::nullopt)" << std::endl;
    }
    print_status(allocator);
    std::cout << "----------------------------------------" << std::endl;

    int reserve_id = 5;
    std::cout << "Reserving ID: " << reserve_id << std::endl;
    if (allocator.is_allocated(reserve_id)) {
         std::cout << "ID " << reserve_id << " is already allocated. Freeing it first for demonstration." << std::endl;
         allocator.free(reserve_id); // Free if it was allocated by the reuse above or earlier
    }
    if (allocator.reserve(reserve_id)) {
        std::cout << "ID " << reserve_id << " reserved successfully." << std::endl;
    } else {
        std::cout << "Failed to reserve ID " << reserve_id << " (maybe out of range or already used and not freed)." << std::endl;
    }
    print_status(allocator);
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "Attempting to allocate ID " << reserve_id << " (should not be allocated if reserved):" << std::endl;
    // To demonstrate this, we need to ensure other IDs are allocated first if reserve_id is next in line.
    // Or, simply try to allocate until all are taken and see if 5 is skipped.
    // For simplicity, let's assume the allocator prioritizes non-reserved IDs.
    // The current allocate() logic will skip it if it's encountered via next_available_id_.
    // If it was in freed_ids_, it would be allocated. But reserve() doesn't add to freed_ids_.

    // Let's try to fill up the allocator to see if 5 is skipped.
    // First, free up space if allocator is full
    if (allocator.available() == 0 && id1 && *id1 != reserve_id) { // Ensure id1 is not the reserved_id
        std::cout << "Freeing ID " << *id1 << " to make space for testing reservation." << std::endl;
        allocator.free(*id1);
    }


    std::cout << "Attempting to allocate an ID. If " << reserve_id << " is the next available via counter, it should be skipped." << std::endl;
    std::optional<int> id_after_reserve = allocator.allocate();
    if (id_after_reserve) {
        std::cout << "Allocated ID: " << *id_after_reserve << std::endl;
        if (*id_after_reserve == reserve_id) {
            std::cout << "Error: Reserved ID " << reserve_id << " was allocated!" << std::endl;
        }
    } else {
        std::cout << "Allocation failed, no IDs available or only reserved ID " << reserve_id << " was left." << std::endl;
    }
    print_status(allocator);
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "Filling all available IDs:" << std::endl;
    int allocated_count = 0;
    while(true) {
        std::optional<int> id = allocator.allocate();
        if (id) {
            std::cout << "Allocated ID: " << *id << std::endl;
            allocated_count++;
        } else {
            std::cout << "Allocation failed (no more IDs or only reserved ones left)." << std::endl;
            break;
        }
    }
    print_status(allocator);
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "Checking 'is_allocated' status:" << std::endl;
    std::cout << "Is ID 1 allocated? " << (allocator.is_allocated(1) ? "Yes" : "No") << std::endl;
    std::cout << "Is ID " << reserve_id << " (reserved) allocated? " << (allocator.is_allocated(reserve_id) ? "Yes" : "No") << std::endl;
    int free_check_id = 0; // Find a free ID if possible
    bool found_free_for_check = false;
    for(int i = allocator.capacity(); i>=1; --i) { // Iterate from max potential ID downwards
        if (!allocator.is_allocated(i)) {
            free_check_id = i;
            found_free_for_check = true;
            break;
        }
    }
    if(found_free_for_check){
        std::cout << "Is ID " << free_check_id << " (expected free) allocated? " << (allocator.is_allocated(free_check_id) ? "Yes" : "No") << std::endl;
    } else {
        std::cout << "Could not find a free ID to check (all might be used or reserved)." << std::endl;
    }
    print_status(allocator);
    std::cout << "----------------------------------------" << std::endl;


    std::cout << "Resetting allocator." << std::endl;
    allocator.reset();
    std::cout << "State after reset:" << std::endl;
    print_status(allocator);
    std::cout << "Is ID 1 allocated after reset? " << (allocator.is_allocated(1) ? "Yes" : "No") << std::endl;
    std::cout << "Is ID " << reserve_id << " allocated after reset? " << (allocator.is_allocated(reserve_id) ? "Yes" : "No") << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "Demonstrating constructor error with max_id < min_id:" << std::endl;
    try {
        IDAllocator invalid_allocator(10, 1);
    } catch (const std::invalid_argument& e) {
        std::cout << "Caught expected exception: " << e.what() << std::endl;
    }
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "Demonstrating reserving an out-of-range ID:" << std::endl;
    if (!allocator.reserve(100)) {
        std::cout << "Failed to reserve out-of-range ID 100, as expected." << std::endl;
    }
    print_status(allocator);
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "Demonstrating freeing an ID that is not allocated:" << std::endl;
    if (!allocator.free(8)) { // Assuming 8 is not allocated after reset
        std::cout << "Failed to free non-allocated ID 8, as expected." << std::endl;
    }
    print_status(allocator);
    std::cout << "----------------------------------------" << std::endl;


    return 0;
}

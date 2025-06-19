#include "IDAllocator.h"
#include "gtest/gtest.h"
#include <vector>
#include <algorithm> // For std::sort if needed
#include <set>
// #include <iostream> // Logging removed

// Test fixture for IDAllocator tests
class IDAllocatorTest : public ::testing::Test {
protected:
    // Optional: SetUp() and TearDown() can be used for common setup/cleanup
};

TEST_F(IDAllocatorTest, ConstructorTest) {
    // std::cerr << "[TEST ConstructorTest] Start" << std::endl; // Logging removed
    IDAllocator allocator(1, 10);
    EXPECT_EQ(allocator.capacity(), 10);
    EXPECT_EQ(allocator.used(), 0);
    EXPECT_EQ(allocator.available(), 10);

    IDAllocator single_id_allocator(5, 5);
    EXPECT_EQ(single_id_allocator.capacity(), 1);
    EXPECT_EQ(single_id_allocator.used(), 0);
    EXPECT_EQ(single_id_allocator.available(), 1);

    EXPECT_THROW(IDAllocator(10, 1), std::invalid_argument);
}

TEST_F(IDAllocatorTest, AllocationTest) {
    IDAllocator allocator(1, 3);
    EXPECT_EQ(allocator.available(), 3);

    std::optional<int> id1 = allocator.allocate();
    ASSERT_TRUE(id1.has_value());
    EXPECT_EQ(*id1, 1);
    EXPECT_TRUE(allocator.is_allocated(1));
    EXPECT_EQ(allocator.used(), 1);
    EXPECT_EQ(allocator.available(), 2);

    std::optional<int> id2 = allocator.allocate();
    ASSERT_TRUE(id2.has_value());
    EXPECT_EQ(*id2, 2);
    EXPECT_TRUE(allocator.is_allocated(2));
    EXPECT_EQ(allocator.used(), 2);
    EXPECT_EQ(allocator.available(), 1);

    std::optional<int> id3 = allocator.allocate();
    ASSERT_TRUE(id3.has_value());
    EXPECT_EQ(*id3, 3);
    EXPECT_TRUE(allocator.is_allocated(3));
    EXPECT_EQ(allocator.used(), 3);
    EXPECT_EQ(allocator.available(), 0);

    std::optional<int> id_overflow = allocator.allocate();
    EXPECT_FALSE(id_overflow.has_value());

    EXPECT_FALSE(allocator.is_allocated(4)); // Test for an ID not allocated
}

TEST_F(IDAllocatorTest, FreeTest) {
    IDAllocator allocator(1, 5);

    allocator.allocate(); // id 1
    std::optional<int> id2_opt = allocator.allocate(); // id 2
    allocator.allocate(); // id 3
    ASSERT_TRUE(id2_opt.has_value());
    int id2 = *id2_opt;

    EXPECT_EQ(allocator.used(), 3);
    EXPECT_TRUE(allocator.is_allocated(id2));

    EXPECT_TRUE(allocator.free(id2));
    EXPECT_EQ(allocator.used(), 2);
    EXPECT_EQ(allocator.available(), 3);
    EXPECT_FALSE(allocator.is_allocated(id2));

    std::optional<int> reused_id = allocator.allocate();
    ASSERT_TRUE(reused_id.has_value());
    EXPECT_EQ(*reused_id, id2); // Smallest freed ID (2) should be reused
    EXPECT_TRUE(allocator.is_allocated(id2));
    EXPECT_EQ(allocator.used(), 3);

    EXPECT_FALSE(allocator.free(100)); // Out of range
    EXPECT_FALSE(allocator.free(4));   // Not allocated (was never allocated)

    // Test freeing a reserved ID (which is allowed by current free() impl)
    allocator.reserve(5);
    EXPECT_TRUE(allocator.is_allocated(5));
    EXPECT_EQ(allocator.used(), 4); // 1,2,3,5
    EXPECT_TRUE(allocator.free(5));
    EXPECT_FALSE(allocator.is_allocated(5));
    EXPECT_EQ(allocator.used(), 3);
}

TEST_F(IDAllocatorTest, DoubleFreeTest) {
    IDAllocator allocator(1, 3);
    std::optional<int> id1 = allocator.allocate();
    ASSERT_TRUE(id1.has_value());

    EXPECT_TRUE(allocator.free(*id1));
    EXPECT_FALSE(allocator.free(*id1)); // Try to free again
}

TEST_F(IDAllocatorTest, ReserveTest) {
    IDAllocator allocator(1, 5);

    EXPECT_TRUE(allocator.reserve(3));
    EXPECT_EQ(allocator.used(), 1);
    EXPECT_EQ(allocator.available(), 4);
    EXPECT_TRUE(allocator.is_allocated(3));

    std::optional<int> id1 = allocator.allocate(); // Should be 1
    std::optional<int> id2 = allocator.allocate(); // Should be 2
    ASSERT_TRUE(id1.has_value());
    ASSERT_TRUE(id2.has_value());
    EXPECT_EQ(*id1, 1);
    EXPECT_EQ(*id2, 2);

    std::optional<int> id_next = allocator.allocate(); // Should be 4 (skipping reserved 3)
    ASSERT_TRUE(id_next.has_value());
    EXPECT_EQ(*id_next, 4);
    EXPECT_NE(*id_next, 3);

    EXPECT_FALSE(allocator.reserve(1));   // Already allocated by allocate()
    EXPECT_FALSE(allocator.reserve(3));   // Already reserved
    EXPECT_FALSE(allocator.reserve(100)); // Out of range

    // Free a reserved ID and then allocate it
    EXPECT_TRUE(allocator.free(3));
    EXPECT_FALSE(allocator.is_allocated(3));
    EXPECT_EQ(allocator.used(), 3); // 1, 2, 4 are used

    std::optional<int> allocated_reserved = allocator.allocate(); // Should be 3 (freed pool)
    ASSERT_TRUE(allocated_reserved.has_value());
    EXPECT_EQ(*allocated_reserved, 3);
    EXPECT_TRUE(allocator.is_allocated(3));
}

TEST_F(IDAllocatorTest, ResetTest) {
    IDAllocator allocator(1, 5);
    allocator.allocate(); // 1
    allocator.reserve(3); // 3
    allocator.allocate(); // 2

    EXPECT_EQ(allocator.used(), 3);

    allocator.reset();
    EXPECT_EQ(allocator.used(), 0);
    EXPECT_EQ(allocator.available(), 5);
    EXPECT_EQ(allocator.capacity(), 5);

    EXPECT_FALSE(allocator.is_allocated(1));
    EXPECT_FALSE(allocator.is_allocated(2));
    EXPECT_FALSE(allocator.is_allocated(3));

    std::optional<int> id_after_reset = allocator.allocate();
    ASSERT_TRUE(id_after_reset.has_value());
    EXPECT_EQ(*id_after_reset, 1); // Allocation starts from min_id
}

TEST_F(IDAllocatorTest, EdgeCaseTest) {
    IDAllocator single_allocator(5, 5);
    EXPECT_EQ(single_allocator.capacity(), 1);

    // std::cerr << "[TEST EdgeCaseTest] Creating IDAllocator single_allocator(5, 5);" << std::endl; // Logging removed
    IDAllocator single_allocator_obj(5, 5); // Renamed to avoid conflict with single_allocator in other tests if any confusion.
    // std::cerr << "[TEST EdgeCaseTest] Initial state: capacity=" << single_allocator_obj.capacity() // Logging removed
    //           << ", used=" << single_allocator_obj.used()
    //           << ", available=" << single_allocator_obj.available() << std::endl;

    // std::cerr << "[TEST EdgeCaseTest] Calling allocate() for the first time..." << std::endl; // Logging removed
    std::optional<int> id = single_allocator_obj.allocate();
    ASSERT_TRUE(id.has_value());
    ASSERT_EQ(id.value(), 5);
    // std::cerr << "[TEST EdgeCaseTest] First allocate() returned: " << (id.has_value() ? std::to_string(id.value()) : "nullopt") << std::endl; // Logging removed
    // std::cerr << "[TEST EdgeCaseTest] State after first allocate: capacity=" << single_allocator_obj.capacity() // Logging removed
    //           << ", used=" << single_allocator_obj.used()
    //           << ", available=" << single_allocator_obj.available() << std::endl;
    EXPECT_TRUE(single_allocator_obj.is_allocated(5));
    EXPECT_EQ(single_allocator_obj.used(), 1);
    EXPECT_EQ(single_allocator_obj.available(), 0);

    // std::cerr << "[TEST EdgeCaseTest] Calling allocate() for the second time..." << std::endl; // Logging removed
    std::optional<int> id_overflow = single_allocator_obj.allocate();
    // std::cerr << "[TEST EdgeCaseTest] Second allocate() returned: " << (id_overflow.has_value() ? std::to_string(id_overflow.value()) : "nullopt") << std::endl; // Logging removed
    // std::cerr << "[TEST EdgeCaseTest] State after second allocate: capacity=" << single_allocator_obj.capacity() // Logging removed
    //           << ", used=" << single_allocator_obj.used()
    //           << ", available=" << single_allocator_obj.available() << std::endl;

    // This is the failing line - simplified version kept from previous step
    bool has_val = id_overflow.has_value();
    // std::cerr << "[TEST EdgeCaseTest] Directly before EXPECT_FALSE, id_overflow.has_value() is: " << (has_val ? "true" : "false") << std::endl; // Logging removed
    EXPECT_FALSE(has_val);

    EXPECT_TRUE(single_allocator_obj.free(5));
    EXPECT_FALSE(single_allocator_obj.is_allocated(5));
    EXPECT_EQ(single_allocator_obj.used(), 0);

    // std::cerr << "[TEST EdgeCaseTest] Reserving ID 5..." << std::endl; // Logging removed
    EXPECT_TRUE(single_allocator_obj.reserve(5));
    EXPECT_TRUE(single_allocator_obj.is_allocated(5));
    EXPECT_EQ(single_allocator_obj.used(), 1);
    // std::cerr << "[TEST EdgeCaseTest] Calling allocate() after reserving 5..." << std::endl; // Logging removed
    id_overflow = single_allocator_obj.allocate(); // Should fail as 5 is reserved
    // std::cerr << "[TEST EdgeCaseTest] Allocate after reserve returned: " << (id_overflow.has_value() ? std::to_string(id_overflow.value()) : "nullopt") << std::endl; // Logging removed
    EXPECT_FALSE(id_overflow.has_value());
    single_allocator_obj.reset();
    // std::cerr << "[TEST EdgeCaseTest] single_allocator_obj reset." << std::endl; // Logging removed

    // Test with min_id and max_id
    // std::cerr << "[TEST EdgeCaseTest] Testing allocator(1,3)..." << std::endl; // Logging removed
    IDAllocator allocator(1, 3);
    allocator.allocate(); // 1
    allocator.allocate(); // 2
    allocator.allocate(); // 3
    EXPECT_EQ(allocator.used(), 3);

    EXPECT_TRUE(allocator.free(1));
    EXPECT_TRUE(allocator.free(3));
    EXPECT_EQ(allocator.used(), 1); // 2 is still used

    std::optional<int> re_id1 = allocator.allocate(); // Should be 1 (from freed pool)
    std::optional<int> re_id3 = allocator.allocate(); // Should be 3 (from freed pool)
    ASSERT_TRUE(re_id1.has_value());
    ASSERT_TRUE(re_id3.has_value());
    EXPECT_EQ(*re_id1, 1);
    EXPECT_EQ(*re_id3, 3);
    EXPECT_TRUE(allocator.is_allocated(1));
    EXPECT_TRUE(allocator.is_allocated(3));
}

TEST_F(IDAllocatorTest, MixedOperationsTest) {
    // std::cerr << "[TEST MixedOperationsTest] Start" << std::endl; // Logging removed
    IDAllocator allocator(1, 5); // Capacity 5

    // allocate 1, 2
    // std::cerr << "[TEST MixedOperationsTest] Allocating 1, 2" << std::endl; // Logging removed
    ASSERT_TRUE(allocator.allocate().has_value()); // 1
    ASSERT_TRUE(allocator.allocate().has_value()); // 2
    EXPECT_EQ(allocator.used(), 2);
    EXPECT_TRUE(allocator.is_allocated(1));
    EXPECT_TRUE(allocator.is_allocated(2));

    // std::cerr << "[TEST MixedOperationsTest] Reserving 4" << std::endl; // Logging removed
    ASSERT_TRUE(allocator.reserve(4));
    EXPECT_EQ(allocator.used(), 3); // 1, 2, 4(reserved)
    EXPECT_TRUE(allocator.is_allocated(4));

    // std::cerr << "[TEST MixedOperationsTest] Freeing 1" << std::endl; // Logging removed
    ASSERT_TRUE(allocator.free(1));
    EXPECT_EQ(allocator.used(), 2); // 2, 4(reserved)
    EXPECT_FALSE(allocator.is_allocated(1));

    // std::cerr << "[TEST MixedOperationsTest] Allocating (expect 1)" << std::endl; // Logging removed
    std::optional<int> id_reused_1 = allocator.allocate();
    ASSERT_TRUE(id_reused_1.has_value());
    EXPECT_EQ(*id_reused_1, 1);
    EXPECT_EQ(allocator.used(), 3); // 1, 2, 4(reserved)

    // std::cerr << "[TEST MixedOperationsTest] Allocating (expect 3)" << std::endl; // Logging removed
    std::optional<int> id_next_3 = allocator.allocate();
    ASSERT_TRUE(id_next_3.has_value());
    EXPECT_EQ(*id_next_3, 3);
    EXPECT_EQ(allocator.used(), 4); // 1, 2, 3, 4(reserved)

    // std::cerr << "[TEST MixedOperationsTest] Freeing 4 (reserved)" << std::endl; // Logging removed
    ASSERT_TRUE(allocator.free(4));
    EXPECT_EQ(allocator.used(), 3); // 1, 2, 3
    EXPECT_FALSE(allocator.is_allocated(4));

    // std::cerr << "[TEST MixedOperationsTest] Allocating (expect 4)" << std::endl; // Logging removed
    std::optional<int> id_reused_4 = allocator.allocate();
    ASSERT_TRUE(id_reused_4.has_value());
    EXPECT_EQ(*id_reused_4, 4);
    EXPECT_EQ(allocator.used(), 4); // 1, 2, 3, 4

    // std::cerr << "[TEST MixedOperationsTest] Allocating (expect 5)" << std::endl; // Logging removed
    std::optional<int> id_next_5 = allocator.allocate();
    ASSERT_TRUE(id_next_5.has_value());
    EXPECT_EQ(*id_next_5, 5);
    EXPECT_EQ(allocator.used(), 5); // 1, 2, 3, 4, 5

    // std::cerr << "[TEST MixedOperationsTest] Allocating (expect nullopt)" << std::endl; // Logging removed
    ASSERT_FALSE(allocator.allocate().has_value());
    // std::cerr << "[TEST MixedOperationsTest] End" << std::endl; // Logging removed
}

TEST_F(IDAllocatorTest, AllocateSkipsReserved) {
    // std::cerr << "[TEST AllocateSkipsReserved] Start" << std::endl; // Logging removed
    IDAllocator allocator(1, 5);
    EXPECT_TRUE(allocator.reserve(1));
    EXPECT_TRUE(allocator.reserve(3));
    EXPECT_TRUE(allocator.reserve(5));
    // Used: 1,3,5. Available: 2,4

    std::optional<int> id_val = allocator.allocate();
    ASSERT_TRUE(id_val.has_value());
    EXPECT_EQ(*id_val, 2); // Skips 1

    id_val = allocator.allocate();
    ASSERT_TRUE(id_val.has_value());
    EXPECT_EQ(*id_val, 4); // Skips 3

    id_val = allocator.allocate(); // No more non-reserved IDs
    EXPECT_FALSE(id_val.has_value());

    EXPECT_TRUE(allocator.is_allocated(1));
    EXPECT_TRUE(allocator.is_allocated(2));
    EXPECT_TRUE(allocator.is_allocated(3));
    EXPECT_TRUE(allocator.is_allocated(4));
    EXPECT_TRUE(allocator.is_allocated(5));
    EXPECT_EQ(allocator.used(), 5);
    // std::cerr << "[TEST AllocateSkipsReserved] End" << std::endl; // Logging removed
}

TEST_F(IDAllocatorTest, FreeingReservedIdMakesItAvailableViaFreedQueue) {
    // std::cerr << "[TEST FreeingReservedIdMakesItAvailableViaFreedQueue] Start" << std::endl; // Logging removed
    IDAllocator allocator(1,3);
    allocator.reserve(2); // 2 is reserved
    allocator.allocate(); // 1 is allocated
    EXPECT_TRUE(allocator.is_allocated(1));
    EXPECT_TRUE(allocator.is_allocated(2));
    EXPECT_EQ(allocator.used(), 2);

    EXPECT_TRUE(allocator.free(2)); // Free the reserved ID
    EXPECT_FALSE(allocator.is_allocated(2)); // No longer "used" in the sense of being taken
    EXPECT_EQ(allocator.used(), 1); // Only 1 is used

    std::optional<int> next_id = allocator.allocate(); // Should give 2 from freed_ids_
    ASSERT_TRUE(next_id.has_value());
    EXPECT_EQ(*next_id, 2);
    EXPECT_EQ(allocator.used(), 2);
    // std::cerr << "[TEST FreeingReservedIdMakesItAvailableViaFreedQueue] End" << std::endl; // Logging removed
}

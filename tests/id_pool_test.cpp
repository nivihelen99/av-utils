#include "gtest/gtest.h"
#include "id_pool.h"
#include <set> // To check uniqueness of IDs

// Test fixture for IdPool tests
class IdPoolTest : public ::testing::Test {
protected:
    IdPool pool;
};

// Test basic allocation of a single ID
TEST_F(IdPoolTest, BasicAllocation) {
    Id id1 = pool.allocate();
    EXPECT_EQ(pool.size(), 1);
    EXPECT_TRUE(pool.is_valid(id1));
    EXPECT_EQ(id1.index, 0);
    EXPECT_EQ(id1.generation, 0);
}

// Test allocation of multiple IDs
TEST_F(IdPoolTest, MultipleAllocations) {
    Id id1 = pool.allocate(); // index 0, gen 0
    Id id2 = pool.allocate(); // index 1, gen 0

    EXPECT_EQ(pool.size(), 2);
    EXPECT_TRUE(pool.is_valid(id1));
    EXPECT_TRUE(pool.is_valid(id2));

    EXPECT_NE(id1.index, id2.index);
    EXPECT_EQ(id1.generation, 0);
    EXPECT_EQ(id2.generation, 0);

    Id id3 = pool.allocate(); // index 2, gen 0
    EXPECT_EQ(pool.size(), 3);
    EXPECT_TRUE(pool.is_valid(id3));
    EXPECT_EQ(id3.index, 2);
    EXPECT_EQ(id3.generation, 0);
}

// Test releasing an ID and then re-allocating it
TEST_F(IdPoolTest, ReleaseAndReallocate) {
    Id id1 = pool.allocate(); // index 0, gen 0
    EXPECT_TRUE(pool.is_valid(id1));
    EXPECT_EQ(pool.size(), 1);

    pool.release(id1);
    EXPECT_FALSE(pool.is_valid(id1)); // Old id1 should now be invalid
    EXPECT_EQ(pool.size(), 0);

    Id id2 = pool.allocate(); // Should reuse index 0, but with incremented generation
    EXPECT_TRUE(pool.is_valid(id2));
    EXPECT_EQ(pool.size(), 1);
    EXPECT_EQ(id2.index, id1.index); // Reused index
    EXPECT_EQ(id2.generation, id1.generation + 1); // Incremented generation
}

// Test generation tracking for stale IDs
TEST_F(IdPoolTest, StaleIdDetection) {
    Id id_original = pool.allocate(); // index 0, gen 0
    ASSERT_TRUE(pool.is_valid(id_original));
    ASSERT_EQ(pool.size(), 1);

    pool.release(id_original);
    ASSERT_FALSE(pool.is_valid(id_original)); // id_original is now stale
    ASSERT_EQ(pool.size(), 0);

    Id id_reused = pool.allocate(); // index 0, gen 1
    ASSERT_TRUE(pool.is_valid(id_reused));
    ASSERT_EQ(id_reused.index, id_original.index);
    ASSERT_EQ(id_reused.generation, id_original.generation + 1);

    // Crucially, the original id_original handle should still be invalid
    EXPECT_FALSE(pool.is_valid(id_original)) << "Original ID should remain invalid after its slot is reused.";

    pool.release(id_reused); // Release the reused ID
    ASSERT_FALSE(pool.is_valid(id_reused)); // id_reused is now stale

    Id id_reused_again = pool.allocate(); // index 0, gen 2
    ASSERT_TRUE(pool.is_valid(id_reused_again));
    ASSERT_EQ(id_reused_again.index, id_original.index);
    ASSERT_EQ(id_reused_again.generation, id_original.generation + 2);

    EXPECT_FALSE(pool.is_valid(id_original)) << "Original ID should still be invalid.";
    EXPECT_FALSE(pool.is_valid(id_reused)) << "First reused ID should also be invalid now.";
}

// Test size tracking
TEST_F(IdPoolTest, SizeTracking) {
    EXPECT_EQ(pool.size(), 0);

    Id id1 = pool.allocate();
    EXPECT_EQ(pool.size(), 1);

    Id id2 = pool.allocate();
    EXPECT_EQ(pool.size(), 2);

    pool.release(id1);
    EXPECT_EQ(pool.size(), 1);

    pool.release(id2);
    EXPECT_EQ(pool.size(), 0);

    Id id3 = pool.allocate(); // Reuses one slot
    EXPECT_EQ(pool.size(), 1);
}

// Test uniqueness of many allocated IDs
TEST_F(IdPoolTest, UniquenessManyIds) {
    const int num_ids = 1000;
    std::set<Id> allocated_ids; // Using std::set with Id::operator< to check uniqueness

    for (int i = 0; i < num_ids; ++i) {
        Id new_id = pool.allocate();
        EXPECT_TRUE(pool.is_valid(new_id));
        auto result = allocated_ids.insert(new_id);
        EXPECT_TRUE(result.second) << "Failed to insert ID: index=" << new_id.index << ", gen=" << new_id.generation << ". It might be a duplicate.";
    }
    EXPECT_EQ(pool.size(), num_ids);
    EXPECT_EQ(allocated_ids.size(), num_ids);
}

// Test interaction of release and allocate in a mixed scenario
TEST_F(IdPoolTest, MixedOperations) {
    Id id1 = pool.allocate(); // idx 0, gen 0
    Id id2 = pool.allocate(); // idx 1, gen 0
    Id id3 = pool.allocate(); // idx 2, gen 0
    EXPECT_EQ(pool.size(), 3);

    pool.release(id2); // Frees idx 1. Stored gen for idx 1 becomes 1.
    EXPECT_FALSE(pool.is_valid(id2));
    EXPECT_EQ(pool.size(), 2);

    Id id4 = pool.allocate(); // Should reuse idx 1, gen 1.
    EXPECT_TRUE(pool.is_valid(id4));
    EXPECT_EQ(id4.index, 1);
    EXPECT_EQ(id4.generation, 1);
    EXPECT_EQ(pool.size(), 3);

    pool.release(id1); // Frees idx 0. Stored gen for idx 0 becomes 1.
    EXPECT_FALSE(pool.is_valid(id1));
    Id id5 = pool.allocate(); // Should reuse idx 0, gen 1.
    EXPECT_TRUE(pool.is_valid(id5));
    EXPECT_EQ(id5.index, 0);
    EXPECT_EQ(id5.generation, 1);

    EXPECT_TRUE(pool.is_valid(id3)); // idx 2, gen 0 - should still be valid
    EXPECT_TRUE(pool.is_valid(id4)); // idx 1, gen 1 - should still be valid
    EXPECT_TRUE(pool.is_valid(id5)); // idx 0, gen 1 - should still be valid
    EXPECT_EQ(pool.size(), 3);
}

// Test releasing an invalid ID (index out of bounds)
TEST_F(IdPoolTest, ReleaseInvalidIndex) {
    Id id_invalid_index = {100, 0}; // Index 100 likely not allocated for an empty pool
    size_t current_size = pool.size();
    // No public way to check if generations_ vector grew, but release should be a no-op
    pool.release(id_invalid_index);
    EXPECT_EQ(pool.size(), current_size); // Size should not change
}

// Test releasing an ID with a stale generation
TEST_F(IdPoolTest, ReleaseStaleGeneration) {
    Id id1 = pool.allocate(); // idx 0, gen 0
    pool.release(id1);      // Stored gen for idx 0 becomes 1. id1 is now stale.

    size_t current_size = pool.size(); // Should be 0
    Id stale_id1 = {id1.index, id1.generation}; // Create a copy with the old generation

    pool.release(stale_id1); // Attempt to release the stale ID
    EXPECT_EQ(pool.size(), current_size); // Size should not change

    Id id2 = pool.allocate(); // idx 0, gen 1
    EXPECT_TRUE(pool.is_valid(id2));
    EXPECT_EQ(id2.index, 0);
    EXPECT_EQ(id2.generation, 1);
}

// Test is_valid for an ID with index out of bounds
TEST_F(IdPoolTest, IsValidInvalidIndex) {
    Id id_invalid_index = {100, 0};
    EXPECT_FALSE(pool.is_valid(id_invalid_index));
}

// Test is_valid for an ID with a valid index but non-matching (future) generation
TEST_F(IdPoolTest, IsValidFutureGeneration) {
    Id id1 = pool.allocate(); // idx 0, gen 0
    Id id_future_gen = {id1.index, id1.generation + 5}; // Same index, but generation is far ahead
    EXPECT_FALSE(pool.is_valid(id_future_gen));
}

// Test allocating up to a certain number, releasing some, then allocating more
TEST_F(IdPoolTest, AllocateReleaseAllocatePattern) {
    std::vector<Id> ids;
    for(int i = 0; i < 10; ++i) {
        ids.push_back(pool.allocate());
    }
    EXPECT_EQ(pool.size(), 10);

    pool.release(ids[3]); // Release index 3
    pool.release(ids[7]); // Release index 7
    EXPECT_EQ(pool.size(), 8);
    EXPECT_FALSE(pool.is_valid(ids[3]));
    EXPECT_FALSE(pool.is_valid(ids[7]));

    Id id_reused_7 = pool.allocate(); // Should reuse index 7, gen 1
    EXPECT_EQ(id_reused_7.index, ids[7].index);
    EXPECT_EQ(id_reused_7.generation, ids[7].generation + 1);
    EXPECT_TRUE(pool.is_valid(id_reused_7));
    EXPECT_EQ(pool.size(), 9);

    Id id_reused_3 = pool.allocate(); // Should reuse index 3, gen 1
    EXPECT_EQ(id_reused_3.index, ids[3].index);
    EXPECT_EQ(id_reused_3.generation, ids[3].generation + 1);
    EXPECT_TRUE(pool.is_valid(id_reused_3));
    EXPECT_EQ(pool.size(), 10);

    Id id_new = pool.allocate(); // Should be a new index (10)
    EXPECT_EQ(id_new.index, 10);
    EXPECT_EQ(id_new.generation, 0);
    EXPECT_TRUE(pool.is_valid(id_new));
    EXPECT_EQ(pool.size(), 11);
}

// Test allocating after pool is exhausted for new indices (if limit is small)
// This test requires a way to limit the pool or simulate exhaustion.
// The current IdPool grows dynamically, so true exhaustion is harder to test
// without allocating 2^32 IDs. We can test the throw condition if we could
// somehow force generations_.size() to approach numeric_limits<uint32_t>::max().
// For now, we'll assume the std::runtime_error for index exhaustion is hard to hit in unit tests.

// Test generation wrapping (extremely unlikely for uint32_t, conceptual)
// If generation could wrap, generations_[id.index]++ could become 0.
// If this 0 matched a newly allocated ID's generation 0, it could lead to issues.
// This is more of a design consideration for very long-lived pools or smaller generation types.
// Not practically testable for uint32_t generations.

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

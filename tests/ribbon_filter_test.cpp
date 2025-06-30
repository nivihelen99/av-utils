#include "gtest/gtest.h"
#include "ribbon_filter.h"
#include <string>
#include <vector>
#include <cstdint>

// Test fixture for RibbonFilter tests
class RibbonFilterTest : public ::testing::Test {
protected:
    // You can define helper functions or member variables here if needed
};

TEST_F(RibbonFilterTest, Construction) {
    RibbonFilter<int> filter(100);
    ASSERT_FALSE(filter.is_built());
    ASSERT_EQ(filter.size(), 0);
    // Capacity is an internal detail but should be non-zero if expected_items > 0
    ASSERT_GT(filter.capacity_slots(), 0);

    RibbonFilter<std::string, uint32_t> filter_str(500);
    ASSERT_FALSE(filter_str.is_built());
    ASSERT_EQ(filter_str.size(), 0);
    ASSERT_GT(filter_str.capacity_slots(), 0);
}

TEST_F(RibbonFilterTest, EmptyFilterConstructionAndBuild) {
    RibbonFilter<int> filter(0);
    ASSERT_FALSE(filter.is_built()); // Not built until build() is called
    ASSERT_EQ(filter.size(), 0);
    // For 0 items, block_size_ becomes 1, array_size_ = K_Indices * 1
    ASSERT_EQ(filter.capacity_slots(), 3 * 1); // K_Indices is 3 by default

    ASSERT_TRUE(filter.build());
    ASSERT_TRUE(filter.is_built());
    ASSERT_EQ(filter.size(), 0);
    ASSERT_FALSE(filter.might_contain(123));
}

TEST_F(RibbonFilterTest, AddAndBuildSuccessful) {
    RibbonFilter<std::string> filter(100);
    std::vector<std::string> items = {"apple", "banana", "cherry"};
    for (const auto& item : items) {
        filter.add(item);
    }
    ASSERT_FALSE(filter.is_built());
    ASSERT_TRUE(filter.build());
    ASSERT_TRUE(filter.is_built());
    ASSERT_EQ(filter.size(), items.size());
}

TEST_F(RibbonFilterTest, MightContainPositiveHits) {
    RibbonFilter<std::string> filter(100);
    std::vector<std::string> items = {"apple", "banana", "cherry", "date", "elderberry"};
    for (const auto& item : items) {
        filter.add(item);
    }
    ASSERT_TRUE(filter.build());

    for (const auto& item : items) {
        EXPECT_TRUE(filter.might_contain(item)) << "Filter should contain " << item;
    }
}

TEST_F(RibbonFilterTest, MightContainNegativeHits) {
    RibbonFilter<std::string> filter(100);
    std::vector<std::string> items_to_add = {"apple", "banana", "cherry"};
    for (const auto& item : items_to_add) {
        filter.add(item);
    }
    ASSERT_TRUE(filter.build());

    std::vector<std::string> items_not_added = {"date", "elderberry", "fig"};
    for (const auto& item : items_not_added) {
        // This might sometimes be true due to false positives, which is acceptable.
        // We are primarily testing that there are no false negatives.
        // For a robust test of false positive rate, many more items and iterations would be needed.
        // Here, we just check the basic functionality.
        // If it returns true, it's okay (FP). If false, that's the more common correct case.
        bool found = filter.might_contain(item);
        if (found) {
            std::cout << "Item '" << item << "' (not added) reported as present (False Positive)." << std::endl;
        }
        // No direct assertion here as FPs are expected. The main test is for positive hits.
    }
    // Add one specific check that is highly unlikely to be a false positive with few items.
    ASSERT_FALSE(filter.might_contain("a_very_long_and_unlikely_string_to_cause_collision"));
}


TEST_F(RibbonFilterTest, SizeAndIsBuiltMethods) {
    RibbonFilter<int> filter(10);
    ASSERT_EQ(filter.size(), 0);
    ASSERT_FALSE(filter.is_built());

    filter.add(1);
    filter.add(2);
    ASSERT_EQ(filter.size(), 0); // Size updates after build
    ASSERT_FALSE(filter.is_built());

    ASSERT_TRUE(filter.build());
    ASSERT_EQ(filter.size(), 2);
    ASSERT_TRUE(filter.is_built());

    // Build on already built filter
    ASSERT_TRUE(filter.build());
    ASSERT_EQ(filter.size(), 2);
    ASSERT_TRUE(filter.is_built());
}

TEST_F(RibbonFilterTest, BuildFailureTooManyItems) {
    // Expect build to fail if filter is significantly over capacity.
    // Sizing for 10 items, but adding 100.
    // The default sizing factor is ~1.23, so array_size will be ~12-15 for 10 expected items.
    // Adding 100 items into ~12-15 slots will very likely fail.
    RibbonFilter<int> filter(10);
    for (int i = 0; i < 100; ++i) {
        filter.add(i);
    }
    ASSERT_FALSE(filter.build()) << "Build should fail when grossly over capacity.";
    ASSERT_FALSE(filter.is_built());
    ASSERT_EQ(filter.size(), 0); // Size should be 0 after failed build
    ASSERT_FALSE(filter.might_contain(1)); // Should not contain anything
}

TEST_F(RibbonFilterTest, AddAfterBuildThrowsException) {
    RibbonFilter<std::string> filter(10);
    filter.add("test1");
    ASSERT_TRUE(filter.build());
    ASSERT_TRUE(filter.is_built());
    EXPECT_THROW(filter.add("test2"), std::runtime_error);
}

TEST_F(RibbonFilterTest, ConstCharSupport) {
    RibbonFilter<const char*> filter(10);
    filter.add("hello");
    filter.add("world");
    ASSERT_TRUE(filter.build());
    ASSERT_TRUE(filter.is_built());
    ASSERT_EQ(filter.size(), 2);

    EXPECT_TRUE(filter.might_contain("hello"));
    EXPECT_TRUE(filter.might_contain("world"));
    // For const char*, ensure different pointers to same string content are treated as same
    const char* hello_ptr1 = "hello";
    const char* hello_ptr2 = "hello"; // May or may not be same address due to string pooling
    std::string hello_str = "hello";
    const char* hello_ptr3 = hello_str.c_str();

    EXPECT_TRUE(filter.might_contain(hello_ptr1));
    EXPECT_TRUE(filter.might_contain(hello_ptr2));
    EXPECT_TRUE(filter.might_contain(hello_ptr3));

    EXPECT_FALSE(filter.might_contain("test"));
    EXPECT_FALSE(filter.might_contain("")); // Empty string test
}

TEST_F(RibbonFilterTest, DifferentFingerprintType) {
    RibbonFilter<int, uint32_t> filter(50);
    for(int i=0; i<50; ++i) {
        filter.add(i*100);
    }
    ASSERT_TRUE(filter.build());
    ASSERT_TRUE(filter.is_built());
    ASSERT_EQ(filter.size(), 50);

    EXPECT_TRUE(filter.might_contain(1000)); // 10*100
    EXPECT_TRUE(filter.might_contain(0));    // 0*100
    EXPECT_TRUE(filter.might_contain(4900)); // 49*100
    EXPECT_FALSE(filter.might_contain(1001));
    EXPECT_FALSE(filter.might_contain(5000));
}

TEST_F(RibbonFilterTest, ZeroItemBuild) {
    RibbonFilter<int> filter(10); // Expect 10, but add 0
    ASSERT_TRUE(filter.build());
    ASSERT_TRUE(filter.is_built());
    ASSERT_EQ(filter.size(), 0);
    EXPECT_FALSE(filter.might_contain(1));
    EXPECT_FALSE(filter.might_contain(0));
}

TEST_F(RibbonFilterTest, QueryNonBuiltFilter) {
    RibbonFilter<int> filter(10);
    filter.add(1);
    ASSERT_FALSE(filter.is_built());
    EXPECT_FALSE(filter.might_contain(1)) << "Querying non-built filter should return false.";
}

TEST_F(RibbonFilterTest, QueryFailedBuildFilter) {
    RibbonFilter<int> filter(1); // Small capacity
    filter.add(1);
    filter.add(2);
    filter.add(3);
    filter.add(4); // Likely to make it unpeelable for capacity of 1

    // It's hard to guarantee a build failure without making it massively overloaded.
    // Let's use the known overload from BuildFailureTooManyItems
    RibbonFilter<int> fail_filter(2);
    for (int i = 0; i < 20; ++i) { // 20 items for capacity of 2
        fail_filter.add(i);
    }

    ASSERT_FALSE(fail_filter.build());
    ASSERT_FALSE(fail_filter.is_built());
    ASSERT_EQ(fail_filter.size(), 0);
    EXPECT_FALSE(fail_filter.might_contain(1)) << "Querying failed-build filter should return false.";
}
// Note: Testing the exact false positive rate is complex and statistical,
// usually requiring many more items and trials than suitable for a unit test.
// These tests focus on correctness of the mechanics.

// Test with K=3 (default)
TEST_F(RibbonFilterTest, K3_DefaultBehavior) {
    RibbonFilter<int, uint16_t, 3> filter(100);
    for (int i = 0; i < 80; ++i) { // Load it reasonably
        filter.add(i);
    }
    ASSERT_TRUE(filter.build());
    EXPECT_TRUE(filter.might_contain(0));
    EXPECT_TRUE(filter.might_contain(79));
    EXPECT_FALSE(filter.might_contain(80));
    EXPECT_FALSE(filter.might_contain(-1));
}

// The current RibbonHasher is hardcoded for K_Indices == 3.
// To test K_Indices !=3, we would need a more flexible hasher or a specialized one.
// The static_assert in RibbonHasher will prevent compilation if K_Indices is not 3.
// If the intention is to support other K values, RibbonHasher must be updated.
// For now, we acknowledge this limitation.
/*
TEST_F(RibbonFilterTest, K_Other_NotSupportedByHasher) {
    // This test would demonstrate the static_assert if we tried to instantiate with K_Indices != 3
    // For example: RibbonFilter<int, uint16_t, 4> filter(100); // This would fail to compile
    // To make this a runtime check or a specific test, one might need to conditionally compile
    // or adjust the test setup. For now, it's a compile-time constraint.
    SUCCEED() << "K_Indices != 3 is currently a compile-time error due to Hasher constraints.";
}
*/

// Test to ensure fingerprint 0 is avoided by hasher and handled by filter if it were to occur
// The hasher maps 0 to 1. So, if an item hashes to a raw FP of 0, it becomes 1.
TEST_F(RibbonFilterTest, FingerprintZeroAvoidance) {
    // We need a way to force a primary hash that results in fingerprint 0
    // This is tricky without knowing the exact hash function details or being able to mock it.
    // However, the Hasher's get_fingerprint explicitly changes fp=0 to fp=1.
    // So, we test that an item added is found, assuming this mechanism works.

    // Let's consider an item that *might* hash to 0 before the adjustment.
    // This is difficult to guarantee.
    // Instead, we rely on the fact that the filter should work regardless.
    // The most direct test of `fp == 0 ? 1 : fp` is in the hasher itself,
    // but here we check filter integrity.

    RibbonFilter<int> filter(10);
    // Add items. One of them *could* hypothetically produce a raw fingerprint of 0.
    // The Hasher ensures it becomes 1.
    filter.add(0); // Value 0 is just an example item
    filter.add(12345);

    ASSERT_TRUE(filter.build());
    EXPECT_TRUE(filter.might_contain(0));
    EXPECT_TRUE(filter.might_contain(12345));

    // If we could mock get_primary_hash to return a value that leads to fp=0
    // and then fp=1, we could be more specific.
    // For now, this test ensures general functionality holds.
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

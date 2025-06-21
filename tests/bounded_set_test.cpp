#include "gtest/gtest.h"
#include "bounded_set.h" // Assuming BoundedSet.h is in the include path
#include <string>
#include <vector>
#include <stdexcept> // For std::runtime_error

// Test fixture for BoundedSet tests
class BoundedSetTest : public ::testing::Test {
protected:
    // You can define helper functions or member variables here if needed
};

TEST_F(BoundedSetTest, BasicFunctionality) {
    BoundedSet<int> s(3);

    // Test initial state
    EXPECT_EQ(s.size(), 0);
    EXPECT_EQ(s.capacity(), 3);
    EXPECT_TRUE(s.empty());

    // Test insertion
    EXPECT_TRUE(s.insert(10));   // [10]
    EXPECT_TRUE(s.insert(20));   // [10, 20]
    EXPECT_TRUE(s.insert(30));   // [10, 20, 30]

    EXPECT_EQ(s.size(), 3);
    EXPECT_FALSE(s.empty());

    // Test duplicate insertion
    EXPECT_FALSE(s.insert(20)); // No change: [10, 20, 30]
    EXPECT_EQ(s.size(), 3);

    // Test eviction
    EXPECT_TRUE(s.insert(40));   // [20, 30, 40] -> evicted 10
    EXPECT_EQ(s.size(), 3);

    // Test contains
    EXPECT_FALSE(s.contains(10));  // evicted
    EXPECT_TRUE(s.contains(20));
    EXPECT_TRUE(s.contains(30));
    EXPECT_TRUE(s.contains(40));
}

TEST_F(BoundedSetTest, FrontBackAccess) {
    BoundedSet<int> s(3);
    s.insert(10);
    s.insert(20);
    s.insert(30);

    ASSERT_FALSE(s.empty()); // Ensure not empty before accessing front/back
    EXPECT_EQ(s.front(), 10);  // oldest
    EXPECT_EQ(s.back(), 30);   // newest

    s.insert(40);  // evicts 10
    ASSERT_FALSE(s.empty());
    EXPECT_EQ(s.front(), 20);  // new oldest
    EXPECT_EQ(s.back(), 40);   // newest
}

TEST_F(BoundedSetTest, Iteration) {
    BoundedSet<int> s(4);
    s.insert(10);
    s.insert(20);
    s.insert(30);
    s.insert(40);

    std::vector<int> expected = {10, 20, 30, 40};
    std::vector<int> actual;

    for (const auto& val : s) {
        actual.push_back(val);
    }
    EXPECT_EQ(actual, expected);

    // Test as_vector
    auto vec = s.as_vector();
    EXPECT_EQ(vec, expected);
}

TEST_F(BoundedSetTest, Erase) {
    BoundedSet<int> s(3);
    s.insert(10);
    s.insert(20);
    s.insert(30);

    // Erase middle element
    EXPECT_TRUE(s.erase(20));
    EXPECT_EQ(s.size(), 2);
    EXPECT_FALSE(s.contains(20));
    EXPECT_TRUE(s.contains(10));
    EXPECT_TRUE(s.contains(30));

    // Erase non-existent element
    EXPECT_FALSE(s.erase(99));
    EXPECT_EQ(s.size(), 2);

    // Erase all
    s.clear();
    EXPECT_EQ(s.size(), 0);
    EXPECT_TRUE(s.empty());
}

TEST_F(BoundedSetTest, CapacityChanges) {
    BoundedSet<int> s(5);
    for (int i = 1; i <= 5; ++i) {
        s.insert(i);
    }
    EXPECT_EQ(s.size(), 5);

    // Reduce capacity - should evict oldest elements
    s.reserve(3);
    EXPECT_EQ(s.capacity(), 3);
    EXPECT_EQ(s.size(), 3);
    EXPECT_FALSE(s.contains(1));  // evicted
    EXPECT_FALSE(s.contains(2));  // evicted
    EXPECT_TRUE(s.contains(3));
    EXPECT_TRUE(s.contains(4));
    EXPECT_TRUE(s.contains(5));

    // Increase capacity
    s.reserve(6);
    EXPECT_EQ(s.capacity(), 6);
    EXPECT_EQ(s.size(), 3);  // size unchanged
}

TEST_F(BoundedSetTest, StringElements) {
    BoundedSet<std::string> dns_cache(3);

    dns_cache.insert("google.com");
    dns_cache.insert("github.com");
    dns_cache.insert("stackoverflow.com");

    EXPECT_TRUE(dns_cache.contains("google.com"));

    dns_cache.insert("reddit.com");  // evicts google.com
    EXPECT_FALSE(dns_cache.contains("google.com"));
    EXPECT_TRUE(dns_cache.contains("reddit.com"));
}

TEST_F(BoundedSetTest, EdgeCases) {
    // Test capacity of 1
    BoundedSet<int> s1(1);
    s1.insert(10);
    EXPECT_EQ(s1.size(), 1);
    s1.insert(20);  // evicts 10
    EXPECT_EQ(s1.size(), 1);
    EXPECT_FALSE(s1.contains(10));
    EXPECT_TRUE(s1.contains(20));

    // Test empty set operations
    BoundedSet<int> empty_set(5);
    EXPECT_FALSE(empty_set.contains(1));
    EXPECT_FALSE(empty_set.erase(1));

    EXPECT_THROW(empty_set.front(), std::runtime_error);
    EXPECT_THROW(empty_set.back(), std::runtime_error);

    // Test constructing with capacity 0
    EXPECT_THROW(BoundedSet<int> s_zero_cap(0), std::invalid_argument);
}

// The use case tests from the example file are more conceptual
// and demonstrate usage patterns rather than strict unit testable logic
// without mocking external dependencies (like send_query or drop_packet).
// They are valuable for understanding but are not directly ported here as unit tests.

// The demo_bounded_set function is also for demonstration and not a test.

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

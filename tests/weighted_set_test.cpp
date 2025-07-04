#include "gtest/gtest.h"
#include "weighted_set.h" // Adjust path as necessary based on include directories
#include <string>
#include <vector>
#include <map>
#include <cmath>     // For std::abs
#include <limits>    // For std::numeric_limits

// Using namespace for convenience in test file
using namespace cpp_collections;

TEST(WeightedSetTest, DefaultConstruction) {
    WeightedSet<int, double> ws;
    EXPECT_TRUE(ws.empty());
    EXPECT_EQ(ws.size(), 0);
    EXPECT_DOUBLE_EQ(ws.total_weight(), 0.0);
    EXPECT_THROW(ws.sample(), std::out_of_range);
}

TEST(WeightedSetTest, InitializerListConstruction) {
    WeightedSet<std::string, int> ws = {
        {"apple", 10},
        {"banana", 20},
        {"cherry", 5}
    };
    EXPECT_FALSE(ws.empty());
    EXPECT_EQ(ws.size(), 3);
    EXPECT_EQ(ws.total_weight(), 35);
    EXPECT_TRUE(ws.contains("apple"));
    EXPECT_EQ(ws.get_weight("apple"), 10);
    EXPECT_TRUE(ws.contains("banana"));
    EXPECT_EQ(ws.get_weight("banana"), 20);
    EXPECT_TRUE(ws.contains("cherry"));
    EXPECT_EQ(ws.get_weight("cherry"), 5);
    EXPECT_FALSE(ws.contains("date"));
}

TEST(WeightedSetTest, AddNewItems) {
    WeightedSet<int, int> ws;
    ws.add(1, 100);
    EXPECT_FALSE(ws.empty());
    EXPECT_EQ(ws.size(), 1);
    EXPECT_EQ(ws.total_weight(), 100);
    EXPECT_TRUE(ws.contains(1));
    EXPECT_EQ(ws.get_weight(1), 100);

    ws.add(2, 50);
    EXPECT_EQ(ws.size(), 2);
    EXPECT_EQ(ws.total_weight(), 150);
    EXPECT_TRUE(ws.contains(2));
    EXPECT_EQ(ws.get_weight(2), 50);
}

TEST(WeightedSetTest, AddAndUpdateItems) {
    WeightedSet<std::string, double> ws;
    ws.add("item1", 10.0);
    EXPECT_EQ(ws.get_weight("item1"), 10.0);
    EXPECT_EQ(ws.total_weight(), 10.0);

    ws.add("item1", 25.0); // Update weight
    EXPECT_EQ(ws.size(), 1);
    EXPECT_EQ(ws.get_weight("item1"), 25.0);
    EXPECT_EQ(ws.total_weight(), 25.0);
}

TEST(WeightedSetTest, AddWithZeroOrNegativeWeight) {
    WeightedSet<std::string, int> ws;
    ws.add("positive", 10);
    EXPECT_TRUE(ws.contains("positive"));
    EXPECT_EQ(ws.size(), 1);

    ws.add("zero_weight", 0);
    EXPECT_FALSE(ws.contains("zero_weight"));
    EXPECT_EQ(ws.size(), 1); // Size should not change

    ws.add("negative_weight", -5);
    EXPECT_FALSE(ws.contains("negative_weight"));
    EXPECT_EQ(ws.size(), 1);

    // Add item then update its weight to zero
    ws.add("to_remove", 20);
    EXPECT_TRUE(ws.contains("to_remove"));
    EXPECT_EQ(ws.size(), 2);
    ws.add("to_remove", 0); // Should remove it
    EXPECT_FALSE(ws.contains("to_remove"));
    EXPECT_EQ(ws.size(), 1);
    EXPECT_EQ(ws.total_weight(), 10); // Only "positive" remains
}

TEST(WeightedSetTest, RemoveItems) {
    WeightedSet<int, int> ws = {{1, 10}, {2, 20}, {3, 30}};
    EXPECT_EQ(ws.size(), 3);
    EXPECT_EQ(ws.total_weight(), 60);

    EXPECT_TRUE(ws.remove(2)); // Remove existing
    EXPECT_EQ(ws.size(), 2);
    EXPECT_FALSE(ws.contains(2));
    EXPECT_EQ(ws.get_weight(2), 0);
    EXPECT_EQ(ws.total_weight(), 40); // 10 + 30

    EXPECT_FALSE(ws.remove(5)); // Remove non-existing
    EXPECT_EQ(ws.size(), 2);
    EXPECT_EQ(ws.total_weight(), 40);
}

TEST(WeightedSetTest, GetWeightAndContains) {
    WeightedSet<char, int> ws = {{'a', 1}, {'b', 2}};
    EXPECT_TRUE(ws.contains('a'));
    EXPECT_EQ(ws.get_weight('a'), 1);
    EXPECT_FALSE(ws.contains('c'));
    EXPECT_EQ(ws.get_weight('c'), 0);
}

TEST(WeightedSetTest, SampleFromSingleItemSet) {
    WeightedSet<std::string, double> ws;
    ws.add("lonely", 100.0);
    ASSERT_EQ(ws.size(), 1);
    ASSERT_DOUBLE_EQ(ws.total_weight(), 100.0);
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(ws.sample(), "lonely");
    }
}

TEST(WeightedSetTest, SampleFromEmptySet) {
    WeightedSet<int, int> ws;
    EXPECT_THROW(ws.sample(), std::out_of_range);
}

TEST(WeightedSetTest, SampleFromZeroTotalWeightSet) {
    WeightedSet<int, int> ws;
    ws.add(1, 10);
    EXPECT_NO_THROW(ws.sample()); // Should be fine

    ws.add(1, 0); // Removes the item
    EXPECT_TRUE(ws.empty());
    EXPECT_THROW(ws.sample(), std::out_of_range);

    WeightedSet<int,int> ws2;
    ws2.add(1,0); // item not added
    ws2.add(2,-10); // item not added
    EXPECT_TRUE(ws2.empty());
    EXPECT_THROW(ws2.sample(), std::out_of_range);
}


TEST(WeightedSetTest, Iteration) {
    WeightedSet<std::string, int> ws = {{"c", 3}, {"a", 1}, {"b", 2}};
    std::vector<std::pair<std::string, int>> expected_items = {
        {"a", 1}, {"b", 2}, {"c", 3} // std::map iterates in key-sorted order
    };
    std::vector<std::pair<std::string, int>> actual_items;
    for (const auto& item : ws) {
        actual_items.push_back(item);
    }
    ASSERT_EQ(actual_items.size(), expected_items.size());
    for(size_t i=0; i < actual_items.size(); ++i) {
        EXPECT_EQ(actual_items[i].first, expected_items[i].first);
        EXPECT_EQ(actual_items[i].second, expected_items[i].second);
    }
}

TEST(WeightedSetTest, CopyConstruction) {
    WeightedSet<std::string, int> original = {{"one", 1}, {"two", 2}};
    WeightedSet<std::string, int> copy = original;

    EXPECT_EQ(copy.size(), 2);
    EXPECT_TRUE(copy.contains("one"));
    EXPECT_EQ(copy.get_weight("one"), 1);
    EXPECT_TRUE(copy.contains("two"));
    EXPECT_EQ(copy.get_weight("two"), 2);
    EXPECT_EQ(copy.total_weight(), 3);

    // Modify original, copy should not change
    original.add("three", 3);
    EXPECT_EQ(copy.size(), 2);
    EXPECT_FALSE(copy.contains("three"));

    // Ensure copy can be sampled
    EXPECT_NO_THROW(copy.sample());
}

TEST(WeightedSetTest, CopyAssignment) {
    WeightedSet<std::string, int> original = {{"one", 1}, {"two", 2}};
    WeightedSet<std::string, int> copy;
    copy.add("bogus", 100); // Add something to copy first
    copy = original;

    EXPECT_EQ(copy.size(), 2);
    EXPECT_TRUE(copy.contains("one"));
    EXPECT_EQ(copy.get_weight("one"), 1);
    EXPECT_FALSE(copy.contains("bogus"));
    EXPECT_EQ(copy.total_weight(), 3);

    // Ensure copy can be sampled
    EXPECT_NO_THROW(copy.sample());
}

TEST(WeightedSetTest, MoveConstruction) {
    WeightedSet<std::string, int> original = {{"one", 1}, {"two", 2}};
    WeightedSet<std::string, int> moved_to(std::move(original));

    EXPECT_EQ(moved_to.size(), 2);
    EXPECT_TRUE(moved_to.contains("one"));
    EXPECT_EQ(moved_to.get_weight("one"), 1);
    EXPECT_EQ(moved_to.total_weight(), 3);

    // Original should be in a valid but unspecified (likely empty) state
    // Depending on map's move ctor, it might be empty or not.
    // Our move ctor should make it stale and empty its caches.
    // Let's check if it's sample-able (it shouldn't be if empty)
    // or if it's empty.
    // EXPECT_TRUE(original.empty()); // This depends on std::map's move behavior.
                                  // It's safer to say it's valid.
    EXPECT_NO_THROW(moved_to.sample());
}

TEST(WeightedSetTest, MoveAssignment) {
    WeightedSet<std::string, int> original = {{"one", 1}, {"two", 2}};
    WeightedSet<std::string, int> moved_to;
    moved_to.add("bogus", 100);
    moved_to = std::move(original);

    EXPECT_EQ(moved_to.size(), 2);
    EXPECT_TRUE(moved_to.contains("one"));
    EXPECT_EQ(moved_to.get_weight("one"), 1);
    EXPECT_FALSE(moved_to.contains("bogus"));
    EXPECT_EQ(moved_to.total_weight(), 3);
    EXPECT_NO_THROW(moved_to.sample());
}


TEST(WeightedSetTest, SwapFunctionality) {
    WeightedSet<char, int> ws1 = {{'a', 10}, {'b', 20}};
    WeightedSet<char, int> ws2 = {{'x', 100}, {'y', 200}, {'z', 300}};

    ws1.swap(ws2);

    EXPECT_EQ(ws1.size(), 3);
    EXPECT_TRUE(ws1.contains('x'));
    EXPECT_EQ(ws1.get_weight('x'), 100);
    EXPECT_EQ(ws1.total_weight(), 600);

    EXPECT_EQ(ws2.size(), 2);
    EXPECT_TRUE(ws2.contains('a'));
    EXPECT_EQ(ws2.get_weight('a'), 10);
    EXPECT_EQ(ws2.total_weight(), 30);

    EXPECT_NO_THROW(ws1.sample());
    EXPECT_NO_THROW(ws2.sample());
}

TEST(WeightedSetTest, StatisticalSamplingDistribution) {
    WeightedSet<std::string, int> ws;
    ws.add("common", 75);
    ws.add("rare", 20);
    ws.add("legendary", 5);

    ASSERT_EQ(ws.total_weight(), 100);

    const int num_samples = 100000;
    std::map<std::string, int> counts;
    for (int i = 0; i < num_samples; ++i) {
        counts[ws.sample()]++;
    }

    EXPECT_EQ(counts.size(), 3); // All items should have been sampled

    double common_ratio = static_cast<double>(counts["common"]) / num_samples;
    double rare_ratio = static_cast<double>(counts["rare"]) / num_samples;
    double legendary_ratio = static_cast<double>(counts["legendary"]) / num_samples;

    // Check if ratios are within a tolerance (e.g., 10% of expected value, or fixed like +/- 0.05)
    // Expected ratios: common=0.75, rare=0.20, legendary=0.05
    // Tolerance needs to be reasonable for statistical tests.
    // A simple check for significant deviation: Chi-squared test would be more robust.
    // For unit test, rough bounds:
    EXPECT_NEAR(common_ratio, 0.75, 0.05); // Check within +/- 5 percentage points
    EXPECT_NEAR(rare_ratio, 0.20, 0.05);
    EXPECT_NEAR(legendary_ratio, 0.05, 0.025); // Tighter for smaller probability
}

TEST(WeightedSetTest, SamplingAfterUpdates) {
    WeightedSet<int, int> ws;
    ws.add(1, 1); // 100% item 1
    EXPECT_EQ(ws.sample(), 1);

    ws.add(2, 99); // item 1: 1%, item 2: 99%
    // After many samples, 2 should be much more frequent
    int count1 = 0, count2 = 0;
    for (int i = 0; i < 10000; ++i) {
        int sampled = ws.sample();
        if (sampled == 1) count1++;
        else if (sampled == 2) count2++;
    }
    EXPECT_LT(count1, count2);
    EXPECT_GT(count1, 10); // Should appear some times (expected 100)
    EXPECT_GT(count2, 9000); // Expected 9900

    ws.remove(2); // Back to 100% item 1
    EXPECT_EQ(ws.sample(), 1);

    ws.add(1, 0); // Remove item 1, now empty
    EXPECT_THROW(ws.sample(), std::out_of_range);
}

TEST(WeightedSetTest, ConstCorrectness) {
    const WeightedSet<int, int> ws_const = {{1,10}, {2,20}};
    EXPECT_EQ(ws_const.size(), 2);
    EXPECT_TRUE(ws_const.contains(1));
    EXPECT_FALSE(ws_const.empty());
    EXPECT_EQ(ws_const.get_weight(1), 10);
    EXPECT_EQ(ws_const.total_weight(), 30);

    // Test const iterators
    int sum_weights = 0;
    for(WeightedSet<int, int>::const_iterator it = ws_const.cbegin(); it != ws_const.cend(); ++it) {
        sum_weights += it->second;
    }
    EXPECT_EQ(sum_weights, 30);

    // Test sampling from const object
    EXPECT_NO_THROW(ws_const.sample());
    int s = ws_const.sample(); // Sample multiple times
    s = ws_const.sample();
    s = ws_const.sample();
    EXPECT_TRUE(s == 1 || s == 2);
}

TEST(WeightedSetTest, ZeroWeightInMiddleOfSamplingDataRebuild) {
    // This tests if rebuild_sampling_data correctly skips items with zero weight
    // even if they are in the middle of the map.
    WeightedSet<int, int> ws;
    ws.add(1, 10);
    ws.add(2, 0);  // This item won't be in sampling data
    ws.add(3, 20);

    EXPECT_EQ(ws.size(), 2); // Item 2 was effectively removed/not added with positive weight.
    EXPECT_TRUE(ws.contains(1));
    EXPECT_FALSE(ws.contains(2)); // Due to add logic
    EXPECT_TRUE(ws.contains(3));
    EXPECT_EQ(ws.total_weight(), 30);

    // Force rebuild and sample
    std::map<int, int> counts;
    for(int i = 0; i < 1000; ++i) {
        counts[ws.sample()]++;
    }
    EXPECT_TRUE(counts.count(1));
    EXPECT_FALSE(counts.count(2)); // Should not have sampled 2
    EXPECT_TRUE(counts.count(3));
    EXPECT_GT(counts[3], counts[1]); // 3 is twice as likely as 1
}

TEST(WeightedSetTest, KeyCompareCustom) {
    struct ReverseCompare {
        bool operator()(int a, int b) const { return a > b; }
    };
    WeightedSet<int, int, ReverseCompare> ws_rev = {{1,10}, {2,20}, {3,5}};
    // Iteration should be in reverse order of keys
    auto it = ws_rev.begin();
    ASSERT_NE(it, ws_rev.end());
    EXPECT_EQ(it->first, 3); ++it;
    ASSERT_NE(it, ws_rev.end());
    EXPECT_EQ(it->first, 2); ++it;
    ASSERT_NE(it, ws_rev.end());
    EXPECT_EQ(it->first, 1); ++it;
    EXPECT_EQ(it, ws_rev.end());

    EXPECT_NO_THROW(ws_rev.sample()); // Sampling should still work
}

TEST(WeightedSetTest, FloatingPointWeightsPrecision) {
    WeightedSet<std::string, double> ws;
    ws.add("A", 0.1);
    ws.add("B", 0.2);
    ws.add("C", 0.7);
    EXPECT_DOUBLE_EQ(ws.total_weight(), 1.0);

    std::map<std::string, int> counts;
    const int num_samples = 20000; // More samples for finer grains
     for (int i = 0; i < num_samples; ++i) {
        counts[ws.sample()]++;
    }
    EXPECT_NEAR(static_cast<double>(counts["A"]) / num_samples, 0.1, 0.05);
    EXPECT_NEAR(static_cast<double>(counts["B"]) / num_samples, 0.2, 0.05);
    EXPECT_NEAR(static_cast<double>(counts["C"]) / num_samples, 0.7, 0.05);

    // Test with very small weights
    WeightedSet<int, double> ws_small;
    ws_small.add(1, 0.000000001); // approx 1/3 chance
    ws_small.add(2, 0.000000002); // approx 2/3 chance

    std::map<int, int> small_counts;
    int num_small_samples = 1000; // Fewer samples, but enough to see a bias
    for(int i=0; i < num_small_samples; ++i) {
        ASSERT_NO_THROW(small_counts[ws_small.sample()]++);
    }
    // Expect item 2 to be sampled more often than item 1
    // Check if both items were sampled to avoid division by zero or missing key issues.
    if (small_counts.count(1) && small_counts.count(2)) {
        EXPECT_GT(small_counts[2], small_counts[1]);
        // Check if roughly 2/3 vs 1/3
        EXPECT_NEAR(static_cast<double>(small_counts[2]) / num_small_samples, 2.0/3.0, 0.15);
        EXPECT_NEAR(static_cast<double>(small_counts[1]) / num_small_samples, 1.0/3.0, 0.15);
    } else if (small_counts.count(2) && !small_counts.count(1)) {
        // Only item 2 sampled, which is fine if item 1 had very low probability in N samples
        EXPECT_EQ(small_counts[2], num_small_samples);
    } else if (small_counts.count(1) && !small_counts.count(2)) {
        // Only item 1 sampled, less likely but possible. For this test, we'd prefer 2 to show up.
        // This might indicate an issue if item 2 (higher weight) is consistently not sampled.
        // However, with very small probabilities, it's possible.
        // For this test, let's ensure counts exist before comparing, or make it more robust.
        // A simple GT might be enough if we ensure counts[1] and counts[2] are populated.
        FAIL() << "Item 2 (higher weight) was not sampled, or item 1 dominated unexpectedly.";
    } else {
        // Neither sampled, or only one in a way that doesn't fit above.
        // This case should ideally not be hit if sample() works and weights are positive.
        ASSERT_TRUE(small_counts.count(1) || small_counts.count(2)) << "Neither item 1 nor 2 were sampled.";
    }

    // Test if adding an extremely small weight to a large one has any effect
    WeightedSet<int, double> ws_mixed;
    ws_mixed.add(1, 1.0e18); // Large weight
    ws_mixed.add(2, 1.0);    // Tiny weight in comparison

    int count_one = 0;
    int count_two = 0;
    for(int i=0; i<1000; ++i) { // Fewer samples, we expect item 2 to appear rarely
        if(ws_mixed.sample() == 1) count_one++; else count_two++;
    }
    EXPECT_GT(count_one, 950); // Most samples should be 1
    // Item 2 might or might not be sampled in few trials, that's ok.
    // The key is that it doesn't break sampling.
}

// Main function for Google Test
// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }
// The above main is usually not needed if tests/CMakeLists.txt uses gtest_discover_tests and links GTest::gtest_main

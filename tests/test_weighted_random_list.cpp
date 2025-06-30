#include "gtest/gtest.h"
#include "WeightedRandomList.h" // Adjust path as necessary based on CMake include directories
#include <string>
#include <map>
#include <vector>
#include <numeric> // For std::accumulate if needed for manual weight sum

// Using namespace for convenience in test file
using namespace cpp_utils;

// Test fixture for WeightedRandomList
class WeightedRandomListTest : public ::testing::Test {
protected:
    WeightedRandomList<std::string> list_str_;
    WeightedRandomList<int, long long> list_int_; // Explicitly specify WeightType
};

TEST_F(WeightedRandomListTest, ConstructorAndInitialState) {
    EXPECT_EQ(list_str_.size(), 0);
    EXPECT_TRUE(list_str_.empty());
    EXPECT_EQ(list_str_.total_weight(), 0);

    WeightedRandomList<int> list_cap(10);
    EXPECT_EQ(list_cap.size(), 0); // Capacity is reserved, size is still 0
    EXPECT_TRUE(list_cap.empty());
    list_cap.reserve(20); // Should not throw, just reserves more
}

TEST_F(WeightedRandomListTest, PushBack) {
    list_str_.push_back("apple", 10);
    EXPECT_EQ(list_str_.size(), 1);
    EXPECT_FALSE(list_str_.empty());
    EXPECT_EQ(list_str_.total_weight(), 10);
    EXPECT_EQ(list_str_[0], "apple");
    EXPECT_EQ(list_str_.get_entry(0).second, 10);

    list_str_.push_back("banana", 20);
    EXPECT_EQ(list_str_.size(), 2);
    EXPECT_EQ(list_str_.total_weight(), 30); // 10 + 20
    EXPECT_EQ(list_str_[1], "banana");
    EXPECT_EQ(list_str_.get_entry(1).second, 20);

    // Test move semantics for value
    std::string cherry_str = "cherry";
    list_str_.push_back(std::move(cherry_str), 30);
    EXPECT_EQ(list_str_.size(), 3);
    EXPECT_EQ(list_str_.total_weight(), 60);
    EXPECT_EQ(list_str_[2], "cherry");
    // EXPECT_TRUE(cherry_str.empty()); // Behavior of moved-from string can vary
}

TEST_F(WeightedRandomListTest, PushBackNegativeWeight) {
    EXPECT_THROW(list_int_.push_back(1, -5), std::invalid_argument);
}

TEST_F(WeightedRandomListTest, UpdateWeight) {
    list_int_.push_back(100, 10);
    list_int_.push_back(200, 20);
    list_int_.push_back(300, 30); // Total weight = 60

    list_int_.update_weight(1, 25); // Update 200's weight from 20 to 25
    EXPECT_EQ(list_int_.get_entry(1).second, 25);
    EXPECT_EQ(list_int_.total_weight(), 65); // 10 + 25 + 30

    list_int_.update_weight(0, 5); // Update 100's weight from 10 to 5
    EXPECT_EQ(list_int_.get_entry(0).second, 5);
    EXPECT_EQ(list_int_.total_weight(), 60); // 5 + 25 + 30

    EXPECT_THROW(list_int_.update_weight(5, 10), std::out_of_range); // Index out of bounds
    EXPECT_THROW(list_int_.update_weight(0, -10), std::invalid_argument); // Negative weight
}

TEST_F(WeightedRandomListTest, GetRandomEmptyOrZeroWeight) {
    // Empty list
    auto item_opt = list_str_.get_random();
    EXPECT_FALSE(item_opt.has_value());

    // List with items but all zero weight
    list_str_.push_back("zero_one", 0);
    list_str_.push_back("zero_two", 0);
    EXPECT_EQ(list_str_.total_weight(), 0);
    item_opt = list_str_.get_random();
    EXPECT_FALSE(item_opt.has_value());

    // Mutable versions
    auto item_mut_opt = list_str_.get_random_mut();
    EXPECT_FALSE(item_mut_opt.has_value());
}

TEST_F(WeightedRandomListTest, GetRandomSingleItem) {
    list_int_.push_back(123, 10);
    auto item_opt = list_int_.get_random();
    ASSERT_TRUE(item_opt.has_value());
    EXPECT_EQ(item_opt.value().get(), 123);

    // Mutable
    auto item_mut_opt = list_int_.get_random_mut();
    ASSERT_TRUE(item_mut_opt.has_value());
    EXPECT_EQ(item_mut_opt.value().get(), 123);
    item_mut_opt.value().get() = 456;
    EXPECT_EQ(list_int_[0], 456);
}

TEST_F(WeightedRandomListTest, GetRandomDistribution) {
    list_str_.push_back("itemA", 10); // Expected ~10%
    list_str_.push_back("itemB", 80); // Expected ~80%
    list_str_.push_back("itemC", 10); // Expected ~10%
    // itemD has 0 weight, should not be selected
    list_str_.push_back("itemD_zero_weight", 0);


    ASSERT_EQ(list_str_.total_weight(), 100);

    std::map<std::string, int> counts;
    const int num_draws = 20000; // Increased draws for better statistical significance

    for (int i = 0; i < num_draws; ++i) {
        auto item_opt = list_str_.get_random();
        ASSERT_TRUE(item_opt.has_value()); // Should always get an item
        counts[item_opt.value().get()]++;
    }

    EXPECT_EQ(counts.count("itemD_zero_weight"), 0); // itemD should not be present or count 0

    // Check distribution (approximate)
    // Allow for some statistical variance, e.g., +/- 2.5% of total draws for 80% item
    // 80% of 20000 = 16000. 2.5% of 20000 = 500
    // 10% of 20000 = 2000.  2.5% of 20000 = 500

    // For itemA (10% weight)
    double pA = static_cast<double>(counts["itemA"]) / num_draws;
    EXPECT_NEAR(pA, 0.10, 0.025); // Expect 10%, allow margin of error 2.5%

    // For itemB (80% weight)
    double pB = static_cast<double>(counts["itemB"]) / num_draws;
    EXPECT_NEAR(pB, 0.80, 0.025); // Expect 80%, allow margin of error 2.5%

    // For itemC (10% weight)
    double pC = static_cast<double>(counts["itemC"]) / num_draws;
    EXPECT_NEAR(pC, 0.10, 0.025); // Expect 10%, allow margin of error 2.5%

    // Ensure all draws are accounted for by items with weight
    EXPECT_EQ(counts["itemA"] + counts["itemB"] + counts["itemC"], num_draws);
}


TEST_F(WeightedRandomListTest, GetRandomDistributionAfterUpdate) {
    list_str_.push_back("X", 25);
    list_str_.push_back("Y", 75); // Total 100

    list_str_.update_weight(0, 50); // X now 50
    list_str_.update_weight(1, 50); // Y now 50. Total still 100, but 50/50 distribution

    ASSERT_EQ(list_str_.total_weight(), 100);

    std::map<std::string, int> counts;
    const int num_draws = 10000;
    for (int i = 0; i < num_draws; ++i) {
        auto item_opt = list_str_.get_random();
        ASSERT_TRUE(item_opt.has_value());
        counts[item_opt.value().get()]++;
    }

    double pX = static_cast<double>(counts["X"]) / num_draws;
    EXPECT_NEAR(pX, 0.50, 0.03); // Expect 50%, allow margin 3%

    double pY = static_cast<double>(counts["Y"]) / num_draws;
    EXPECT_NEAR(pY, 0.50, 0.03); // Expect 50%, allow margin 3%
}


TEST_F(WeightedRandomListTest, ElementAccess) {
    list_str_.push_back("first", 1);
    list_str_.push_back("second", 1);

    // Const access
    const auto& const_list = list_str_;
    EXPECT_EQ(const_list[0], "first");
    EXPECT_EQ(const_list.at(1), "second");
    EXPECT_THROW(const_list.at(2), std::out_of_range);

    // Mutable access
    list_str_[0] = "new_first";
    EXPECT_EQ(list_str_[0], "new_first");
    list_str_.at(1) = "new_second";
    EXPECT_EQ(list_str_.at(1), "new_second");
    EXPECT_THROW(list_str_.at(2), std::out_of_range);
}

TEST_F(WeightedRandomListTest, GetEntry) {
    list_int_.push_back(10, 100);
    list_int_.push_back(20, 200);

    auto entry0 = list_int_.get_entry(0);
    EXPECT_EQ(entry0.first, 10);
    EXPECT_EQ(entry0.second, 100);

    auto entry1 = list_int_.get_entry(1);
    EXPECT_EQ(entry1.first, 20);
    EXPECT_EQ(entry1.second, 200);

    EXPECT_THROW(list_int_.get_entry(2), std::out_of_range);
}


TEST_F(WeightedRandomListTest, Clear) {
    list_str_.push_back("one", 1);
    list_str_.push_back("two", 2);
    ASSERT_FALSE(list_str_.empty());
    ASSERT_EQ(list_str_.size(), 2);
    ASSERT_EQ(list_str_.total_weight(), 3);

    list_str_.clear();
    EXPECT_TRUE(list_str_.empty());
    EXPECT_EQ(list_str_.size(), 0);
    EXPECT_EQ(list_str_.total_weight(), 0);

    // Check if it's usable after clear
    list_str_.push_back("three", 3);
    EXPECT_EQ(list_str_.size(), 1);
    EXPECT_EQ(list_str_.total_weight(), 3);
    auto item_opt = list_str_.get_random();
    ASSERT_TRUE(item_opt.has_value());
    EXPECT_EQ(item_opt.value().get(), "three");
}

TEST_F(WeightedRandomListTest, Reserve) {
    list_int_.reserve(100);
    // Functionality is primarily to avoid reallocations,
    // difficult to test behavior directly beyond it not crashing.
    // Size should still be 0.
    EXPECT_EQ(list_int_.size(), 0);
    EXPECT_TRUE(list_int_.empty());

    list_int_.push_back(1,1); // Should still work
    EXPECT_EQ(list_int_.size(), 1);
}

TEST_F(WeightedRandomListTest, GetRandomMutableAndModify) {
    list_str_.push_back("original", 100);
    list_str_.push_back("another", 1); // Low weight, less likely to be picked

    // Try a few times to increase chance of picking "original" if not perfectly random
    bool modified = false;
    for(int i=0; i<10; ++i) { // Try to get the first element
        auto item_mut_opt = list_str_.get_random_mut();
        ASSERT_TRUE(item_mut_opt.has_value());
        if (item_mut_opt.value().get() == "original") {
             item_mut_opt.value().get() = "modified";
             modified = true;
             break;
        }
    }
    ASSERT_TRUE(modified); // Ensure we actually modified it for the test to be meaningful

    // Verify modification
    bool found_modified = false;
    bool found_original_still = false;
    for(size_t i=0; i<list_str_.size(); ++i) {
        if (list_str_[i] == "modified") found_modified = true;
        if (list_str_[i] == "original") found_original_still = true;
    }
    EXPECT_TRUE(found_modified);
    EXPECT_FALSE(found_original_still); // "original" should be gone
    EXPECT_EQ(list_str_.get_entry(0).first, "modified"); // Assuming "original" was at index 0
}

// Test with a different weight type if we were to change the static_assert
// For now, WeightType is fixed to long long.
// If it were a template parameter for FenwickTree as well, we could test e.g. int.
/*
TEST_F(WeightedRandomListTest, DifferentWeightType) {
    WeightedRandomList<std::string, int> list_int_weights; // Hypothetical if WeightType was more flexible
    list_int_weights.push_back("item", 10);
    EXPECT_EQ(list_int_weights.total_weight(), 10);
}
*/

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

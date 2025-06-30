#include "gtest/gtest.h"
#include "multiset_counter.hpp" // Adjust path as necessary
#include <string>
#include <vector>
#include <list>
#include <set> // For std::multiset
#include <algorithm> // For std::sort in tests if needed

// Helper to compare multisets (vectors) ignoring order for some checks if needed,
// but canonical form should handle this.
template <typename T>
bool are_multisets_equal(std::vector<T> ms1, std::vector<T> ms2) {
    std::sort(ms1.begin(), ms1.end());
    std::sort(ms2.begin(), ms2.end());
    return ms1 == ms2;
}

// Test fixture for MultisetCounter tests
class MultisetCounterTest : public ::testing::Test {
protected:
    cpp_collections::multiset_counter<std::string> mc_str;
    cpp_collections::multiset_counter<int> mc_int;

    // Example multisets
    const std::vector<std::string> ms_fruits1 = {"apple", "banana"};
    const std::vector<std::string> ms_fruits1_ordered = {"banana", "apple"}; // Same as ms_fruits1
    const std::vector<std::string> ms_fruits2 = {"apple", "orange"};
    const std::vector<std::string> ms_fruits3 = {"apple", "banana", "apple"};
    const std::vector<std::string> ms_grape = {"grape"};

    const std::vector<int> ms_nums1 = {1, 2, 3};
    const std::vector<int> ms_nums1_ordered = {3, 1, 2};
    const std::vector<int> ms_nums2 = {1, 1, 2};
};

// Test Default Constructor
TEST_F(MultisetCounterTest, DefaultConstructor) {
    EXPECT_TRUE(mc_str.empty());
    EXPECT_EQ(mc_str.size(), 0);
    EXPECT_EQ(mc_str.total(), 0);
}

// Test Initializer List Constructor
TEST_F(MultisetCounterTest, InitializerListConstructor) {
    cpp_collections::multiset_counter<int> mc = {
        {1, 2, 3}, {3, 2, 1}, {1, 1, 2}, {1, 2, 3}, {4}
    };
    EXPECT_EQ(mc.count({1, 2, 3}), 3);
    EXPECT_EQ(mc.count({3, 1, 2}), 3); // Order doesn't matter
    EXPECT_EQ(mc.count({1, 1, 2}), 1);
    EXPECT_EQ(mc.count({4}), 1);
    EXPECT_EQ(mc.size(), 3); // Unique multisets: {1,2,3}, {1,1,2}, {4}
    EXPECT_EQ(mc.total(), 5);
}

// Test Add and Count
TEST_F(MultisetCounterTest, AddAndCount) {
    mc_str.add(ms_fruits1);
    EXPECT_EQ(mc_str.count(ms_fruits1), 1);
    EXPECT_EQ(mc_str.count(ms_fruits1_ordered), 1); // Check order invariance

    mc_str.add(ms_fruits1_ordered); // Add same multiset again
    EXPECT_EQ(mc_str.count(ms_fruits1), 2);

    mc_str.add(ms_fruits2);
    EXPECT_EQ(mc_str.count(ms_fruits2), 1);

    mc_str.add(ms_fruits3);
    EXPECT_EQ(mc_str.count(ms_fruits3), 1);
    EXPECT_EQ(mc_str.count({"apple", "apple", "banana"}), 1);


    EXPECT_EQ(mc_str.count({"non", "existent"}), 0);
}

TEST_F(MultisetCounterTest, AddWithCountParameter) {
    mc_int.add(ms_nums1, 5);
    EXPECT_EQ(mc_int.count(ms_nums1), 5);
    EXPECT_EQ(mc_int.count(ms_nums1_ordered), 5);

    mc_int.add(ms_nums1, 2);
    EXPECT_EQ(mc_int.count(ms_nums1), 7);

    mc_int.add(ms_nums2, 3);
    EXPECT_EQ(mc_int.count(ms_nums2), 3);

    // Add with negative count
    mc_int.add(ms_nums1, -4);
    EXPECT_EQ(mc_int.count(ms_nums1), 3);

    // Add with negative count to remove item
    mc_int.add(ms_nums1, -3);
    EXPECT_EQ(mc_int.count(ms_nums1), 0);
    EXPECT_FALSE(mc_int.contains(ms_nums1));

    // Add with zero count (should do nothing)
    mc_int.add(ms_nums2, 0);
    EXPECT_EQ(mc_int.count(ms_nums2), 3);
}


// Test OperatorBracketConst (alias for count)
TEST_F(MultisetCounterTest, OperatorBracketConst) {
    mc_str.add(ms_fruits1);
    EXPECT_EQ(mc_str[ms_fruits1], 1);
    EXPECT_EQ(mc_str[ms_fruits1_ordered], 1);
    EXPECT_EQ(mc_str[(std::vector<std::string>{"non", "existent"})], 0);
}

// Test Size, Empty, Total
TEST_F(MultisetCounterTest, SizeEmptyTotal) {
    EXPECT_TRUE(mc_str.empty());
    EXPECT_EQ(mc_str.size(), 0);
    EXPECT_EQ(mc_str.total(), 0);

    mc_str.add(ms_fruits1); // count = 1
    EXPECT_FALSE(mc_str.empty());
    EXPECT_EQ(mc_str.size(), 1); // 1 unique multiset
    EXPECT_EQ(mc_str.total(), 1); // sum of counts

    mc_str.add(ms_fruits1_ordered); // count = 2 for ms_fruits1
    EXPECT_EQ(mc_str.size(), 1);
    EXPECT_EQ(mc_str.total(), 2);

    mc_str.add(ms_fruits2); // count = 1 for ms_fruits2
    EXPECT_EQ(mc_str.size(), 2); // 2 unique multisets
    EXPECT_EQ(mc_str.total(), 3); // 2 + 1
}

// Test Clear
TEST_F(MultisetCounterTest, Clear) {
    mc_str.add(ms_fruits1);
    mc_str.add(ms_fruits2);
    ASSERT_FALSE(mc_str.empty());

    mc_str.clear();
    EXPECT_TRUE(mc_str.empty());
    EXPECT_EQ(mc_str.size(), 0);
    EXPECT_EQ(mc_str.total(), 0);
    EXPECT_EQ(mc_str.count(ms_fruits1), 0);
}

// Test Iterators
TEST_F(MultisetCounterTest, Iterators) {
    mc_int.add({1, 2}, 2);
    mc_int.add({1, 3}, 1);
    mc_int.add({2, 1}, 1); // Should merge with {1,2}, making its count 3

    std::map<std::vector<int>, int> expected_counts;
    expected_counts[{1, 2}] = 3; // canonical form
    expected_counts[{1, 3}] = 1; // canonical form

    std::map<std::vector<int>, int> actual_counts;
    for (const auto& pair : mc_int) { // Uses begin()/end()
        actual_counts[pair.first] = pair.second;
    }
    EXPECT_EQ(actual_counts, expected_counts);

    // Const iteration
    const auto& const_mc_int = mc_int;
    actual_counts.clear();
    for (const auto& pair : const_mc_int) { // Uses cbegin()/cend()
        actual_counts[pair.first] = pair.second;
    }
    EXPECT_EQ(actual_counts, expected_counts);
}

// Test MostCommon
TEST_F(MultisetCounterTest, MostCommon) {
    mc_str.add(ms_fruits1, 3); // {"apple", "banana"}
    mc_str.add(ms_fruits2, 5); // {"apple", "orange"}
    mc_str.add(ms_fruits3, 2); // {"apple", "banana", "apple"}
    mc_str.add(ms_grape, 5);   // {"grape"} - tie with ms_fruits2

    // Expected canonical forms for keys:
    std::vector<std::string> key_f1 = {"apple", "banana"};
    std::vector<std::string> key_f2 = {"apple", "orange"};
    std::vector<std::string> key_f3 = {"apple", "apple", "banana"};
    std::vector<std::string> key_g = {"grape"};


    auto common_all = mc_str.most_common();
    ASSERT_EQ(common_all.size(), 4);
    // Order: (count desc, key asc for ties)
    // Counts: f2:5, g:5, f1:3, f3:2
    // Tie between f2 and g: f2 < g? depends on vector comparison of strings
    // {"apple", "orange"} vs {"grape"} -> "apple" < "grape", so f2 comes before g
    EXPECT_EQ(common_all[0].first, key_f2); EXPECT_EQ(common_all[0].second, 5);
    EXPECT_EQ(common_all[1].first, key_g);  EXPECT_EQ(common_all[1].second, 5);
    EXPECT_EQ(common_all[2].first, key_f1); EXPECT_EQ(common_all[2].second, 3);
    EXPECT_EQ(common_all[3].first, key_f3); EXPECT_EQ(common_all[3].second, 2);

    auto common_top2 = mc_str.most_common(2);
    ASSERT_EQ(common_top2.size(), 2);
    EXPECT_EQ(common_top2[0].first, key_f2); EXPECT_EQ(common_top2[0].second, 5);
    EXPECT_EQ(common_top2[1].first, key_g);  EXPECT_EQ(common_top2[1].second, 5);

    auto common_top1 = mc_str.most_common(1);
    ASSERT_EQ(common_top1.size(), 1);
    EXPECT_EQ(common_top1[0].first, key_f2); EXPECT_EQ(common_top1[0].second, 5);

    auto common_n_gt_size = mc_str.most_common(10);
    ASSERT_EQ(common_n_gt_size.size(), 4);
    EXPECT_EQ(common_n_gt_size, common_all);

    cpp_collections::multiset_counter<int> empty_mc;
    EXPECT_TRUE(empty_mc.most_common().empty());
    EXPECT_TRUE(empty_mc.most_common(5).empty());
}

// Test Generic Add/Count Overloads
TEST_F(MultisetCounterTest, GenericAddCount) {
    std::list<std::string> list_items = {"config", "log", "config"};
    std::multiset<std::string> multiset_items = {"data", "data", "index"};

    mc_str.add(list_items);
    mc_str.add(multiset_items);
    mc_str.add(ms_fruits1); // Add a vector too

    EXPECT_EQ(mc_str.count(list_items), 1);
    EXPECT_EQ(mc_str.count(std::vector<std::string>{"config", "config", "log"}), 1);

    EXPECT_EQ(mc_str.count(multiset_items), 1);
    EXPECT_EQ(mc_str.count(std::vector<std::string>{"data", "data", "index"}), 1);

    EXPECT_EQ(mc_str[ms_fruits1], 1);

    EXPECT_EQ(mc_str.size(), 3);
    EXPECT_EQ(mc_str.total(), 3);
}

// Test Custom Comparator
TEST_F(MultisetCounterTest, CustomComparator) {
    // Counts multisets of integers, sorting items in descending order for canonical form
    cpp_collections::multiset_counter<int, std::greater<int>> mc_custom_comp;

    std::vector<int> items1 = {1, 5, 2}; // Canonical with std::greater: {5, 2, 1}
    std::vector<int> items2 = {2, 5, 1}; // Canonical with std::greater: {5, 2, 1}
    std::vector<int> items3 = {1, 2, 3}; // Canonical with std::greater: {3, 2, 1}

    mc_custom_comp.add(items1);
    mc_custom_comp.add(items2);
    mc_custom_comp.add(items3);

    EXPECT_EQ(mc_custom_comp.count({1, 2, 5}), 2);
    EXPECT_EQ(mc_custom_comp.count({5, 1, 2}), 2);
    EXPECT_EQ(mc_custom_comp.count({3, 2, 1}), 1);
    EXPECT_EQ(mc_custom_comp.count({1, 2, 3}), 1);

    EXPECT_EQ(mc_custom_comp.size(), 2);

    auto common = mc_custom_comp.most_common();
    ASSERT_EQ(common.size(), 2);
    // Expected canonical keys (sorted by std::greater<int>):
    std::vector<int> key1_custom = {5, 2, 1};
    std::vector<int> key2_custom = {3, 2, 1};

    // Order in most_common: count desc, then key asc (std::vector default less)
    // Counts: key1_custom: 2, key2_custom: 1
    EXPECT_EQ(common[0].first, key1_custom); EXPECT_EQ(common[0].second, 2);
    EXPECT_EQ(common[1].first, key2_custom); EXPECT_EQ(common[1].second, 1);
}

// Test with empty input multisets
TEST_F(MultisetCounterTest, EmptyInputMultisets) {
    std::vector<int> empty_ms = {};
    mc_int.add(empty_ms);
    EXPECT_EQ(mc_int.count(empty_ms), 1);
    EXPECT_EQ(mc_int.size(), 1);
    EXPECT_EQ(mc_int.total(), 1);

    mc_int.add(empty_ms, 2);
    EXPECT_EQ(mc_int.count(empty_ms), 3);
    EXPECT_EQ(mc_int.size(), 1);
    EXPECT_EQ(mc_int.total(), 3);

    mc_int.add({1,2});
    EXPECT_EQ(mc_int.size(), 2);
    EXPECT_EQ(mc_int.total(), 4); // 3 for empty, 1 for {1,2}

    auto common = mc_int.most_common();
    ASSERT_EQ(common.size(), 2);
    // Empty vector {} is "less than" {1,2}
    EXPECT_EQ(common[0].first, empty_ms); EXPECT_EQ(common[0].second, 3);
    EXPECT_EQ(common[1].first, std::vector<int>({1,2})); EXPECT_EQ(common[1].second, 1);
}

// Test rvalue add overload
TEST_F(MultisetCounterTest, RValueAdd) {
    mc_str.add(std::vector<std::string>{"a", "b"});
    EXPECT_EQ(mc_str.count({"a", "b"}), 1);

    mc_str.add(std::vector<std::string>{"b", "a"}, 2);
    EXPECT_EQ(mc_str.count({"a", "b"}), 3);

    std::vector<std::string> temp_ms = {"c", "d"};
    mc_str.add(std::move(temp_ms)); // temp_ms is now in a valid but unspecified state
    EXPECT_EQ(mc_str.count({"c", "d"}), 1);
    // EXPECT_TRUE(temp_ms.empty()); // Not guaranteed, but often true for vector move
}

// Main function for Google Test
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

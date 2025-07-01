#include "gtest/gtest.h"
#include "FrozenCounter.h"
#include "counter.h" // For constructing from mutable Counter
#include <string>
#include <vector>
#include <map> // For std::map to initialize
#include <unordered_map> // For testing std::hash

// Test fixture for FrozenCounter tests
class FrozenCounterTest : public ::testing::Test {
protected:
    // Per-test set-up and tear-down logic can go here if needed
};

TEST_F(FrozenCounterTest, DefaultConstruction) {
    cpp_collections::FrozenCounter<int> fc;
    EXPECT_TRUE(fc.empty());
    EXPECT_EQ(fc.size(), 0);
    EXPECT_EQ(fc.total(), 0);
    EXPECT_EQ(fc.count(123), 0);
    EXPECT_FALSE(fc.contains(123));
}

TEST_F(FrozenCounterTest, InitializerListConstruction) {
    cpp_collections::FrozenCounter<std::string> fc = {
        {"apple", 3}, {"banana", 2}, {"apple", 2}, {"orange", 1}, {"banana", 3}, {"grape", 0}, {"plum", -2}
    };
    EXPECT_FALSE(fc.empty());
    EXPECT_EQ(fc.size(), 3); // apple, banana, orange
    EXPECT_EQ(fc.total(), 11); // 5 (apple) + 5 (banana) + 1 (orange)

    EXPECT_EQ(fc.count("apple"), 5);
    EXPECT_EQ(fc["banana"], 5); // Test operator[]
    EXPECT_EQ(fc.count("orange"), 1);
    EXPECT_EQ(fc.count("grape"), 0); // Not included
    EXPECT_EQ(fc.count("plum"), 0);  // Not included
    EXPECT_TRUE(fc.contains("apple"));
    EXPECT_FALSE(fc.contains("grape"));
}

TEST_F(FrozenCounterTest, IteratorRangeConstructionVector) {
    std::vector<std::pair<char, int>> items = {{'a', 1}, {'b', 2}, {'a', 3}, {'c', 0}, {'d', -5}};
    cpp_collections::FrozenCounter<char> fc(items.begin(), items.end());

    EXPECT_EQ(fc.size(), 2); // a, b
    EXPECT_EQ(fc.total(), 6); // a:4, b:2
    EXPECT_EQ(fc.count('a'), 4);
    EXPECT_EQ(fc.count('b'), 2);
    EXPECT_EQ(fc.count('c'), 0);
    EXPECT_EQ(fc.count('d'), 0);
}

TEST_F(FrozenCounterTest, IteratorRangeConstructionMap) {
    std::map<int, int> items = {{1, 5}, {2, 3}, {1, 2}, {3, 0}}; // Map will consolidate {1,2} into {1,5} -> {1,5}
                                                                // So input to FC is effectively {1,5}, {2,3}, {3,0}
    // Note: std::map iterator yields std::pair<const Key, Value>, which is fine.
    // The build_from_range expects pairs where .first is key, .second is count.
    // For map: it->first is map's key, it->second is map's value (which we use as count)
    cpp_collections::FrozenCounter<int> fc(items.begin(), items.end());

    EXPECT_EQ(fc.size(), 2); // 1, 2 (3 has count 0)
    EXPECT_EQ(fc.total(), 8); // 1:5, 2:3 -> 5+3=8
    EXPECT_EQ(fc.count(1), 5);
    EXPECT_EQ(fc.count(2), 3);
    EXPECT_EQ(fc.count(3), 0);
}


TEST_F(FrozenCounterTest, ConstructionFromMutableCounter) {
    ::Counter<std::string> mutable_counter; // Use global Counter
    mutable_counter.add("hello", 3);
    mutable_counter.add("world", 2);
    mutable_counter.add("hello", 1);
    mutable_counter.add("test", 0); // Will be ignored

    cpp_collections::FrozenCounter<std::string> fc(mutable_counter);
    EXPECT_EQ(fc.size(), 2); // hello, world
    EXPECT_EQ(fc.total(), 6); // hello:4, world:2
    EXPECT_EQ(fc.count("hello"), 4);
    EXPECT_EQ(fc.count("world"), 2);
    EXPECT_EQ(fc.count("test"), 0);
}

TEST_F(FrozenCounterTest, CopyConstruction) {
    cpp_collections::FrozenCounter<std::string> fc1 = {{"a", 1}, {"b", 2}};
    cpp_collections::FrozenCounter<std::string> fc2(fc1);

    EXPECT_EQ(fc1.size(), fc2.size());
    EXPECT_EQ(fc1.total(), fc2.total());
    EXPECT_EQ(fc2.count("a"), 1);
    EXPECT_EQ(fc2.count("b"), 2);
    EXPECT_TRUE(fc1 == fc2);
}

TEST_F(FrozenCounterTest, MoveConstruction) {
    cpp_collections::FrozenCounter<std::string> fc1 = {{"a", 1}, {"b", 2}};
    cpp_collections::FrozenCounter<std::string> fc_copy_for_check(fc1); // Keep a copy for checking values

    cpp_collections::FrozenCounter<std::string> fc2(std::move(fc1));

    EXPECT_EQ(fc2.size(), fc_copy_for_check.size());
    EXPECT_EQ(fc2.total(), fc_copy_for_check.total());
    EXPECT_EQ(fc2.count("a"), 1);
    EXPECT_EQ(fc2.count("b"), 2);
    EXPECT_TRUE(fc2 == fc_copy_for_check);

    // fc1 should be in a valid but unspecified state (likely empty or resources moved)
    // For this implementation, it will be empty-ish (total_count_ zeroed)
    // EXPECT_TRUE(fc1.empty() || fc1.size()==0); // Check if it's empty or size 0
    // EXPECT_EQ(fc1.total(),0); // total_count_ is zeroed
}

TEST_F(FrozenCounterTest, CopyAssignment) {
    cpp_collections::FrozenCounter<std::string> fc1 = {{"a", 1}, {"b", 2}};
    cpp_collections::FrozenCounter<std::string> fc2;
    fc2 = fc1;

    EXPECT_EQ(fc1.size(), fc2.size());
    EXPECT_EQ(fc1.total(), fc2.total());
    EXPECT_EQ(fc2.count("a"), 1);
    EXPECT_EQ(fc2.count("b"), 2);
    EXPECT_TRUE(fc1 == fc2);
}

TEST_F(FrozenCounterTest, MoveAssignment) {
    cpp_collections::FrozenCounter<std::string> fc1 = {{"a", 1}, {"b", 2}};
    cpp_collections::FrozenCounter<std::string> fc_copy_for_check(fc1);

    cpp_collections::FrozenCounter<std::string> fc2;
    fc2 = std::move(fc1);

    EXPECT_EQ(fc2.size(), fc_copy_for_check.size());
    EXPECT_EQ(fc2.total(), fc_copy_for_check.total());
    EXPECT_EQ(fc2.count("a"), 1);
    EXPECT_EQ(fc2.count("b"), 2);
    EXPECT_TRUE(fc2 == fc_copy_for_check);
    // EXPECT_TRUE(fc1.empty() || fc1.size()==0);
    // EXPECT_EQ(fc1.total(),0);
}


TEST_F(FrozenCounterTest, Iteration) {
    cpp_collections::FrozenCounter<int> fc = {{10, 3}, {5, 2}, {15, 1}};
    // Expected sorted order: {5,2}, {10,3}, {15,1}

    std::vector<std::pair<const int, int>> expected_items = {{5, 2}, {10, 3}, {15, 1}};
    size_t i = 0;
    for (const auto& item : fc) {
        ASSERT_LT(i, expected_items.size());
        EXPECT_EQ(item.first, expected_items[i].first);
        EXPECT_EQ(item.second, expected_items[i].second);
        i++;
    }
    EXPECT_EQ(i, expected_items.size());

    // Test cbegin/cend
    i = 0;
    for (auto it = fc.cbegin(); it != fc.cend(); ++it) {
         ASSERT_LT(i, expected_items.size());
        EXPECT_EQ(it->first, expected_items[i].first);
        EXPECT_EQ(it->second, expected_items[i].second);
        i++;
    }
    EXPECT_EQ(i, expected_items.size());
}

TEST_F(FrozenCounterTest, MostCommon) {
    cpp_collections::FrozenCounter<std::string> fc = {
        {"apple", 5}, {"banana", 5}, {"orange", 1}, {"grape", 3}
    };
    // Expected order by count (desc), then key (asc for ties):
    // apple:5, banana:5 (apple before banana due to string comparison)
    // grape:3
    // orange:1

    auto common_all = fc.most_common();
    ASSERT_EQ(common_all.size(), 4);
    EXPECT_EQ(common_all[0].first, "apple"); EXPECT_EQ(common_all[0].second, 5);
    EXPECT_EQ(common_all[1].first, "banana"); EXPECT_EQ(common_all[1].second, 5);
    EXPECT_EQ(common_all[2].first, "grape"); EXPECT_EQ(common_all[2].second, 3);
    EXPECT_EQ(common_all[3].first, "orange"); EXPECT_EQ(common_all[3].second, 1);

    auto common_top2 = fc.most_common(2);
    ASSERT_EQ(common_top2.size(), 2);
    EXPECT_EQ(common_top2[0].first, "apple"); EXPECT_EQ(common_top2[0].second, 5);
    EXPECT_EQ(common_top2[1].first, "banana"); EXPECT_EQ(common_top2[1].second, 5);

    auto common_top0 = fc.most_common(0); // Should be same as all
    ASSERT_EQ(common_top0.size(), 4);
    EXPECT_EQ(common_top0[0].first, "apple");

    auto common_too_many = fc.most_common(10); // More than available
    ASSERT_EQ(common_too_many.size(), 4);
    EXPECT_EQ(common_too_many[0].first, "apple");
}

TEST_F(FrozenCounterTest, MostCommonEmpty) {
    cpp_collections::FrozenCounter<int> fc;
    auto common = fc.most_common();
    EXPECT_TRUE(common.empty());
    auto common_1 = fc.most_common(1);
    EXPECT_TRUE(common_1.empty());
}

TEST_F(FrozenCounterTest, ComparisonOperators) {
    cpp_collections::FrozenCounter<int> fc1 = {{1, 2}, {3, 4}};
    cpp_collections::FrozenCounter<int> fc2 = {{3, 4}, {1, 2}}; // Same, different order init
    cpp_collections::FrozenCounter<int> fc3 = {{1, 2}, {3, 5}}; // Different count
    cpp_collections::FrozenCounter<int> fc4 = {{1, 2}, {4, 4}}; // Different key
    cpp_collections::FrozenCounter<int> fc5 = {{1,2}}; // Different size

    EXPECT_TRUE(fc1 == fc2);
    EXPECT_FALSE(fc1 != fc2);

    EXPECT_FALSE(fc1 == fc3);
    EXPECT_TRUE(fc1 != fc3);

    EXPECT_FALSE(fc1 == fc4);
    EXPECT_TRUE(fc1 != fc4);

    EXPECT_FALSE(fc1 == fc5);
    EXPECT_TRUE(fc1 != fc5);

    cpp_collections::FrozenCounter<int> empty1, empty2;
    EXPECT_TRUE(empty1 == empty2);
}

TEST_F(FrozenCounterTest, CustomComparator) {
    struct ReverseCompare {
        bool operator()(const int& a, const int& b) const { return a > b; }
    };
    cpp_collections::FrozenCounter<int, ReverseCompare> fc = {{10, 1}, {20, 2}, {5, 3}};
    // Expected internal order (reverse): {20,2}, {10,1}, {5,3}
    // Total: 6, Size: 3

    EXPECT_EQ(fc.size(), 3);
    EXPECT_EQ(fc.total(), 6);
    EXPECT_EQ(fc.count(10), 1);
    EXPECT_EQ(fc.count(20), 2);
    EXPECT_EQ(fc.count(5), 3);

    auto it = fc.begin();
    EXPECT_EQ(it->first, 20); EXPECT_EQ(it->second, 2); ++it;
    EXPECT_EQ(it->first, 10); EXPECT_EQ(it->second, 1); ++it;
    EXPECT_EQ(it->first, 5);  EXPECT_EQ(it->second, 3); ++it;
    EXPECT_EQ(it, fc.end());

    // Most common with custom comparator for keys (tie-breaking)
    // Counts: 5:3, 20:2, 10:1
    auto common = fc.most_common();
    ASSERT_EQ(common.size(), 3);
    EXPECT_EQ(common[0].first, 5); EXPECT_EQ(common[0].second, 3);
    EXPECT_EQ(common[1].first, 20); EXPECT_EQ(common[1].second, 2);
    EXPECT_EQ(common[2].first, 10); EXPECT_EQ(common[2].second, 1);
}

TEST_F(FrozenCounterTest, StdHashSpecialization) {
    cpp_collections::FrozenCounter<std::string> fc1 = {{"a", 1}, {"b", 2}};
    cpp_collections::FrozenCounter<std::string> fc2 = {{"b", 2}, {"a", 1}}; // Same content
    cpp_collections::FrozenCounter<std::string> fc3 = {{"a", 1}, {"c", 2}}; // Different content

    std::hash<cpp_collections::FrozenCounter<std::string>> hasher;

    EXPECT_EQ(hasher(fc1), hasher(fc2));
    EXPECT_NE(hasher(fc1), hasher(fc3));

    // Test usage in unordered_map
    std::unordered_map<cpp_collections::FrozenCounter<std::string>, int> map_of_fcs;
    map_of_fcs[fc1] = 100;
    EXPECT_EQ(map_of_fcs[fc1], 100);

    map_of_fcs[fc2] = 200; // Should overwrite the entry for fc1
    EXPECT_EQ(map_of_fcs.size(), 1);
    EXPECT_EQ(map_of_fcs[fc1], 200);
    EXPECT_EQ(map_of_fcs[fc2], 200);

    map_of_fcs[fc3] = 300;
    EXPECT_EQ(map_of_fcs.size(), 2);
    EXPECT_EQ(map_of_fcs[fc3], 300);
}

TEST_F(FrozenCounterTest, AllocatorSupport) {
    // This is a compile-time check mostly, ensuring constructors exist.
    // Runtime check would require a custom allocator.
    using MyAllocator = std::allocator<std::pair<const int, int>>;
    MyAllocator alloc;

    cpp_collections::FrozenCounter<int, std::less<int>, MyAllocator> fc1(alloc);
    EXPECT_TRUE(fc1.empty());
    EXPECT_EQ(fc1.get_allocator(), alloc);

    cpp_collections::FrozenCounter<int, std::less<int>, MyAllocator> fc2(std::less<int>(), alloc);
    EXPECT_TRUE(fc2.empty());

    std::vector<std::pair<int, int>> data = {{1,1}};
    cpp_collections::FrozenCounter<int, std::less<int>, MyAllocator> fc3(data.begin(), data.end(), std::less<int>(), alloc);
    EXPECT_EQ(fc3.size(), 1);

    cpp_collections::FrozenCounter<int, std::less<int>, MyAllocator> fc4({ {1,1}, {2,2} }, std::less<int>(), alloc);
    EXPECT_EQ(fc4.size(), 2);

    cpp_collections::FrozenCounter<int, std::less<int>, MyAllocator> fc5(fc4); // Copy
    EXPECT_EQ(fc5.size(), 2);
    // Note: copy constructor usually copies allocator if std::allocator_traits::select_on_container_copy_construction is true for the allocator type
    // For std::allocator, it doesn't copy, the new container gets its default-constructed allocator or one passed explicitly.

    cpp_collections::FrozenCounter<int, std::less<int>, MyAllocator> fc6(fc4, alloc); // Copy with allocator
    EXPECT_EQ(fc6.size(), 2);
    EXPECT_EQ(fc6.get_allocator(), alloc);

    cpp_collections::FrozenCounter<int, std::less<int>, MyAllocator> fc7(std::move(fc4)); // Move
    EXPECT_EQ(fc7.size(), 2);
    // EXPECT_TRUE(fc4.empty() || fc4.size() == 0);

    // cpp_collections::FrozenCounter<int, std::less<int>, MyAllocator> fc8(std::move(fc5), alloc); // Move with allocator
    // EXPECT_EQ(fc8.size(), 2);
}

TEST_F(FrozenCounterTest, EdgeCaseZeroAndNegativeCountsInInput) {
    cpp_collections::FrozenCounter<std::string> fc = {
        {"positive", 5}, {"zero", 0}, {"negative", -2}, {"another_positive", 1}
    };
    EXPECT_EQ(fc.size(), 2); // positive, another_positive
    EXPECT_EQ(fc.total(), 6);
    EXPECT_TRUE(fc.contains("positive"));
    EXPECT_TRUE(fc.contains("another_positive"));
    EXPECT_FALSE(fc.contains("zero"));
    EXPECT_FALSE(fc.contains("negative"));
}

TEST_F(FrozenCounterTest, FindMethod) {
    cpp_collections::FrozenCounter<int> fc = {{10, 3}, {5, 2}, {15, 1}};
    // Internally: {5,2}, {10,3}, {15,1}

    auto it_5 = fc.find(5);
    ASSERT_NE(it_5, fc.end());
    EXPECT_EQ(it_5->first, 5);
    EXPECT_EQ(it_5->second, 2);

    auto it_10 = fc.find(10);
    ASSERT_NE(it_10, fc.end());
    EXPECT_EQ(it_10->first, 10);
    EXPECT_EQ(it_10->second, 3);

    auto it_15 = fc.find(15);
    ASSERT_NE(it_15, fc.end());
    EXPECT_EQ(it_15->first, 15);
    EXPECT_EQ(it_15->second, 1);

    auto it_missing = fc.find(100);
    EXPECT_EQ(it_missing, fc.end());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

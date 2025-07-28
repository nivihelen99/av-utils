#include "gtest/gtest.h"
#include "sorted_vector_map.h"
#include <string>
#include <vector>

// Test fixture for sorted_vector_map tests
class SortedVectorMapTest : public ::testing::Test {
protected:
    sorted_vector_map<int, std::string> map;
};

// Test default constructor
TEST_F(SortedVectorMapTest, DefaultConstructor) {
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(0, map.size());
}

// Test insertion and basic properties
TEST_F(SortedVectorMapTest, InsertAndSize) {
    auto result = map.insert({2, "banana"});
    EXPECT_TRUE(result.second);
    EXPECT_EQ(result.first->first, 2);
    EXPECT_EQ(result.first->second, "banana");
    EXPECT_EQ(map.size(), 1);
    EXPECT_FALSE(map.empty());

    map.insert({5, "apple"});
    map.insert({1, "date"});

    EXPECT_EQ(map.size(), 3);

    // Check sorted order
    auto it = map.begin();
    EXPECT_EQ(it->first, 1);
    ++it;
    EXPECT_EQ(it->first, 2);
    ++it;
    EXPECT_EQ(it->first, 5);
}

// Test find
TEST_F(SortedVectorMapTest, Find) {
    map.insert({2, "banana"});
    map.insert({5, "apple"});
    map.insert({1, "date"});

    auto it = map.find(2);
    ASSERT_NE(it, map.end());
    EXPECT_EQ(it->second, "banana");

    it = map.find(4);
    EXPECT_EQ(it, map.end());
}

// Test at
TEST_F(SortedVectorMapTest, At) {
    map.insert({2, "banana"});
    map.insert({5, "apple"});

    EXPECT_EQ(map.at(2), "banana");
    EXPECT_THROW(map.at(4), std::out_of_range);
}

// Test operator[]
TEST_F(SortedVectorMapTest, SubscriptOperator) {
    map[2] = "banana";
    map[5] = "apple";

    EXPECT_EQ(map[2], "banana");
    EXPECT_EQ(map[5], "apple");
    EXPECT_EQ(map.size(), 2);

    map[2] = "new banana";
    EXPECT_EQ(map[2], "new banana");
    EXPECT_EQ(map.size(), 2);

    EXPECT_EQ(map[3], "");
    EXPECT_EQ(map.size(), 3);
}

// Test erase
TEST_F(SortedVectorMapTest, Erase) {
    map.insert({2, "banana"});
    map.insert({5, "apple"});
    map.insert({1, "date"});

    // Erase by key
    EXPECT_EQ(map.erase(2), 1);
    EXPECT_EQ(map.size(), 2);
    EXPECT_EQ(map.find(2), map.end());

    // Erase non-existent key
    EXPECT_EQ(map.erase(3), 0);
    EXPECT_EQ(map.size(), 2);

    // Erase by iterator
    auto it = map.find(1);
    ASSERT_NE(it, map.end());
    auto next_it = map.erase(it);
    EXPECT_EQ(next_it->first, 5);
    EXPECT_EQ(map.size(), 1);
}

// Test clear
TEST_F(SortedVectorMapTest, Clear) {
    map.insert({2, "banana"});
    map.insert({5, "apple"});

    map.clear();
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.size(), 0);
}

// Test range constructor
TEST_F(SortedVectorMapTest, RangeConstructor) {
    std::vector<std::pair<int, std::string>> data = {{5, "apple"}, {2, "banana"}, {8, "cherry"}};
    sorted_vector_map<int, std::string> new_map(data.begin(), data.end());

    EXPECT_EQ(new_map.size(), 3);
    auto it = new_map.begin();
    EXPECT_EQ(it->first, 2);
    ++it;
    EXPECT_EQ(it->first, 5);
    ++it;
    EXPECT_EQ(it->first, 8);
}

// Test lower_bound and upper_bound
TEST_F(SortedVectorMapTest, Bounds) {
    map.insert({2, "banana"});
    map.insert({5, "apple"});
    map.insert({8, "cherry"});

    auto lower = map.lower_bound(5);
    EXPECT_EQ(lower->first, 5);

    auto upper = map.upper_bound(5);
    EXPECT_EQ(upper->first, 8);

    lower = map.lower_bound(4);
    EXPECT_EQ(lower->first, 5);

    upper = map.upper_bound(9);
    EXPECT_EQ(upper, map.end());
}

// Test equal_range
TEST_F(SortedVectorMapTest, EqualRange) {
    map.insert({2, "banana"});
    map.insert({5, "apple"});
    map.insert({8, "cherry"});

    auto range = map.equal_range(5);
    EXPECT_EQ(range.first->first, 5);
    EXPECT_EQ(range.second->first, 8);
}

// Test swap
TEST_F(SortedVectorMapTest, Swap) {
    map.insert({2, "banana"});
    map.insert({5, "apple"});

    sorted_vector_map<int, std::string> other_map;
    other_map.insert({1, "date"});
    other_map.insert({10, "fig"});

    map.swap(other_map);

    EXPECT_EQ(map.size(), 2);
    EXPECT_TRUE(map.find(1) != map.end());
    EXPECT_TRUE(map.find(10) != map.end());

    EXPECT_EQ(other_map.size(), 2);
    EXPECT_TRUE(other_map.find(2) != other_map.end());
    EXPECT_TRUE(other_map.find(5) != other_map.end());
}

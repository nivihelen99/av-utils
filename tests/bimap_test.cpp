#include "gtest/gtest.h"
#include "../include/bimap.h" // Assuming bimap.h is in 'include' dir, one level up
#include <string>
#include <vector>
#include <algorithm> // For STL algorithm tests
#include <map>       // For comparison data

// Test fixture for BiMap tests
class BiMapTest : public ::testing::Test {
protected:
    BiMap<std::string, int> bimap_str_int;
    BiMap<int, std::string> bimap_int_str;

    // Example data
    const std::string s1 = "one";
    const int i1 = 1;
    const std::string s2 = "two";
    const int i2 = 2;
    const std::string s3 = "three";
    const int i3 = 3;
};

// Test default constructor
TEST_F(BiMapTest, DefaultConstructor) {
    EXPECT_TRUE(bimap_str_int.empty());
    EXPECT_EQ(0, bimap_str_int.size());
    EXPECT_TRUE(bimap_int_str.empty());
    EXPECT_EQ(0, bimap_int_str.size());
}

// Test basic insert and retrieval
TEST_F(BiMapTest, BasicInsert) {
    // Insert first element
    EXPECT_TRUE(bimap_str_int.insert(s1, i1));
    EXPECT_EQ(1, bimap_str_int.size());
    EXPECT_FALSE(bimap_str_int.empty());

    // Check retrieval
    EXPECT_TRUE(bimap_str_int.contains_left(s1));
    EXPECT_TRUE(bimap_str_int.contains_right(i1));
    EXPECT_EQ(i1, bimap_str_int.at_left(s1));
    EXPECT_EQ(s1, bimap_str_int.at_right(i1));

    // Insert second element
    EXPECT_TRUE(bimap_str_int.insert(s2, i2));
    EXPECT_EQ(2, bimap_str_int.size());

    EXPECT_TRUE(bimap_str_int.contains_left(s2));
    EXPECT_TRUE(bimap_str_int.contains_right(i2));
    EXPECT_EQ(i2, bimap_str_int.at_left(s2));
    EXPECT_EQ(s2, bimap_str_int.at_right(i2));
}

// Test size and empty methods more thoroughly
TEST_F(BiMapTest, SizeEmpty) {
    EXPECT_TRUE(bimap_str_int.empty());
    EXPECT_EQ(0, bimap_str_int.size());

    bimap_str_int.insert(s1, i1);
    EXPECT_FALSE(bimap_str_int.empty());
    EXPECT_EQ(1, bimap_str_int.size());

    bimap_str_int.insert(s2, i2);
    EXPECT_FALSE(bimap_str_int.empty());
    EXPECT_EQ(2, bimap_str_int.size());

    bimap_str_int.erase_left(s1);
    EXPECT_FALSE(bimap_str_int.empty());
    EXPECT_EQ(1, bimap_str_int.size());

    bimap_str_int.erase_right(i2);
    EXPECT_TRUE(bimap_str_int.empty());
    EXPECT_EQ(0, bimap_str_int.size());
}

// --- Constructors and Assignment Tests ---

TEST_F(BiMapTest, CopyConstructor) {
    bimap_str_int.insert(s1, i1);
    bimap_str_int.insert(s2, i2);

    BiMap<std::string, int> copy_bimap(bimap_str_int);

    ASSERT_EQ(2, copy_bimap.size());
    EXPECT_TRUE(copy_bimap.contains_left(s1));
    EXPECT_TRUE(copy_bimap.contains_right(i1));
    EXPECT_EQ(i1, copy_bimap.at_left(s1));
    EXPECT_EQ(s1, copy_bimap.at_right(i1));

    EXPECT_TRUE(copy_bimap.contains_left(s2));
    EXPECT_TRUE(copy_bimap.contains_right(i2));
    EXPECT_EQ(i2, copy_bimap.at_left(s2));
    EXPECT_EQ(s2, copy_bimap.at_right(i2));

    // Ensure deep copy (modifying original doesn't affect copy)
    bimap_str_int.erase_left(s1);
    EXPECT_TRUE(copy_bimap.contains_left(s1)); // copy should still have s1
    EXPECT_FALSE(bimap_str_int.contains_left(s1)); // original should not
}

TEST_F(BiMapTest, MoveConstructor) {
    bimap_str_int.insert(s1, i1);
    bimap_str_int.insert(s2, i2);

    BiMap<std::string, int> moved_from_bimap(bimap_str_int); // copy to make source for move
    BiMap<std::string, int> moved_to_bimap(std::move(moved_from_bimap));

    ASSERT_EQ(2, moved_to_bimap.size());
    EXPECT_TRUE(moved_to_bimap.contains_left(s1));
    EXPECT_TRUE(moved_to_bimap.contains_right(i1));
    EXPECT_EQ(i1, moved_to_bimap.at_left(s1));
    EXPECT_EQ(s1, moved_to_bimap.at_right(i1));

    EXPECT_TRUE(moved_from_bimap.empty());
    EXPECT_EQ(0, moved_from_bimap.size());
}

TEST_F(BiMapTest, CopyAssignmentOperator) {
    bimap_str_int.insert(s1, i1);
    BiMap<std::string, int> target_bimap;
    target_bimap.insert(s2, i2);

    target_bimap = bimap_str_int;

    ASSERT_EQ(1, target_bimap.size());
    EXPECT_TRUE(target_bimap.contains_left(s1));
    EXPECT_EQ(i1, target_bimap.at_left(s1));
    EXPECT_FALSE(target_bimap.contains_left(s2));

    bimap_str_int.erase_left(s1);
    EXPECT_TRUE(target_bimap.contains_left(s1));

    target_bimap.insert(s3,i3);
    target_bimap = target_bimap; // Self-assignment
    ASSERT_EQ(2, target_bimap.size());
    EXPECT_TRUE(target_bimap.contains_left(s1));
    EXPECT_TRUE(target_bimap.contains_left(s3));
}

TEST_F(BiMapTest, MoveAssignmentOperator) {
    bimap_str_int.insert(s1, i1);
    bimap_str_int.insert(s2, i2);

    BiMap<std::string, int> source_bimap(bimap_str_int);
    BiMap<std::string, int> target_bimap;
    target_bimap.insert(s3, i3);

    target_bimap = std::move(source_bimap);

    ASSERT_EQ(2, target_bimap.size());
    EXPECT_TRUE(target_bimap.contains_left(s1));
    EXPECT_TRUE(target_bimap.contains_left(s2));
    EXPECT_FALSE(target_bimap.contains_left(s3));

    EXPECT_TRUE(source_bimap.empty());
    EXPECT_EQ(0, source_bimap.size());

    BiMap<std::string, int> self_move_target;
    self_move_target.insert("self_key", 123);
    self_move_target = std::move(self_move_target);
    ASSERT_TRUE(self_move_target.contains_left("self_key"));
    ASSERT_EQ(1, self_move_target.size());
}

// --- Insert Operations ---
TEST_F(BiMapTest, InsertLvalue) {
    ASSERT_TRUE(bimap_str_int.insert(s1, i1));
    EXPECT_EQ(1, bimap_str_int.size());
    EXPECT_TRUE(bimap_str_int.contains_left(s1));
    EXPECT_TRUE(bimap_str_int.contains_right(i1));

    ASSERT_FALSE(bimap_str_int.insert(s1, i2));
    EXPECT_EQ(1, bimap_str_int.size());
    EXPECT_EQ(i1, bimap_str_int.at_left(s1));

    ASSERT_FALSE(bimap_str_int.insert(s2, i1));
    EXPECT_EQ(1, bimap_str_int.size());
    EXPECT_EQ(s1, bimap_str_int.at_right(i1));

    ASSERT_FALSE(bimap_str_int.insert(s1, i1));
    EXPECT_EQ(1, bimap_str_int.size());
}

TEST_F(BiMapTest, InsertStdPair) {
    std::pair<std::string, int> p1(s1, i1);
    std::pair<std::string, int> p2_dup_left(s1, i2);
    std::pair<std::string, int> p3_dup_right(s2, i1);

    ASSERT_TRUE(bimap_str_int.insert(p1));
    EXPECT_EQ(1, bimap_str_int.size());
    EXPECT_TRUE(bimap_str_int.contains_left(p1.first));
    EXPECT_TRUE(bimap_str_int.contains_right(p1.second));

    ASSERT_FALSE(bimap_str_int.insert(p2_dup_left));
    EXPECT_EQ(1, bimap_str_int.size());

    ASSERT_FALSE(bimap_str_int.insert(p3_dup_right));
    EXPECT_EQ(1, bimap_str_int.size());
}

TEST_F(BiMapTest, InsertRvalue) {
    ASSERT_TRUE(bimap_str_int.insert(std::string("move_s1"), 101));
    EXPECT_EQ(1, bimap_str_int.size());
    EXPECT_TRUE(bimap_str_int.contains_left("move_s1"));
    EXPECT_TRUE(bimap_str_int.contains_right(101));

    ASSERT_TRUE(bimap_str_int.insert(s2, 102)); // const Left&, Right&&
    EXPECT_EQ(2, bimap_str_int.size());
    EXPECT_TRUE(bimap_str_int.contains_left(s2));
    EXPECT_TRUE(bimap_str_int.contains_right(102));

    ASSERT_TRUE(bimap_str_int.insert(std::string("move_s3"), i3)); // Left&&, const Right&
    EXPECT_EQ(3, bimap_str_int.size());
    EXPECT_TRUE(bimap_str_int.contains_left("move_s3"));
    EXPECT_TRUE(bimap_str_int.contains_right(i3));

    ASSERT_FALSE(bimap_str_int.insert(std::string("move_s1"), 104)); // Duplicate Left&&
    EXPECT_EQ(3, bimap_str_int.size());
    EXPECT_EQ(101, bimap_str_int.at_left("move_s1"));

    ASSERT_FALSE(bimap_str_int.insert(s1, 101)); // const Left&, Right&& - s1 is new, 101 is not
    EXPECT_EQ(3, bimap_str_int.size());
    EXPECT_EQ("move_s1", bimap_str_int.at_right(101));
}

TEST_F(BiMapTest, InsertInitializerList) {
    bimap_str_int.insert({ {s1, i1}, {s2, i2} });
    ASSERT_EQ(2, bimap_str_int.size());
    EXPECT_TRUE(bimap_str_int.contains_left(s1));
    EXPECT_TRUE(bimap_str_int.contains_right(i2));

    BiMap<std::string, int> bimap2;
    bimap2.insert({ {s1, i1}, {s1, i2}, {s3, i1} }); // s1,i2 fails due to s1; s3,i1 fails due to i1
    ASSERT_EQ(1, bimap2.size());
    EXPECT_TRUE(bimap2.contains_left(s1));
    EXPECT_TRUE(bimap2.contains_right(i1));
    EXPECT_EQ(i1, bimap2.at_left(s1));

    BiMap<std::string, int> bimap3;
    bimap3.insert({});
    ASSERT_TRUE(bimap3.empty());
}

TEST_F(BiMapTest, InsertRange) {
    std::vector<std::pair<std::string, int>> data = { {s1, i1}, {s2, i2} };
    bimap_str_int.insert(data.begin(), data.end());
    ASSERT_EQ(2, bimap_str_int.size());
    EXPECT_TRUE(bimap_str_int.contains_left(s1));
    EXPECT_TRUE(bimap_str_int.contains_right(i2));

    std::vector<std::pair<std::string, int>> data_dups = { {s3, i3}, {s1, 100}, {"s100", i1} };
    bimap_str_int.insert(data_dups.begin(), data_dups.end()); // Only {s3,i3} is new and non-conflicting
    ASSERT_EQ(3, bimap_str_int.size());
    EXPECT_TRUE(bimap_str_int.contains_left(s3));
    EXPECT_TRUE(bimap_str_int.contains_right(i3));
}

// --- insert_or_assign Operations ---
TEST_F(BiMapTest, InsertOrAssignLvalue) {
    bimap_str_int.insert_or_assign(s1, i1);
    ASSERT_EQ(1, bimap_str_int.size());
    EXPECT_EQ(i1, bimap_str_int.at_left(s1));
    EXPECT_EQ(s1, bimap_str_int.at_right(i1));

    bimap_str_int.insert_or_assign(s1, i2);
    ASSERT_EQ(1, bimap_str_int.size());
    EXPECT_EQ(i2, bimap_str_int.at_left(s1));
    EXPECT_EQ(s1, bimap_str_int.at_right(i2));
    EXPECT_FALSE(bimap_str_int.contains_right(i1));

    bimap_str_int.insert_or_assign(s2, i2);
    ASSERT_EQ(1, bimap_str_int.size());
    EXPECT_EQ(i2, bimap_str_int.at_left(s2));
    EXPECT_EQ(s2, bimap_str_int.at_right(i2));
    EXPECT_FALSE(bimap_str_int.contains_left(s1));

    bimap_str_int.insert_or_assign(s2, i2);
    ASSERT_EQ(1, bimap_str_int.size());

    bimap_str_int.insert(s3, i3);
    ASSERT_EQ(2, bimap_str_int.size()); // (s2,i2), (s3,i3)

    bimap_str_int.insert_or_assign(s2, i3); // Assign (s2, i3). This breaks (s3,i3).
                                          // Expected: (s2, i3)
    ASSERT_EQ(1, bimap_str_int.size());
    EXPECT_TRUE(bimap_str_int.contains_left(s2));
    EXPECT_TRUE(bimap_str_int.contains_right(i3));
    EXPECT_EQ(i3, bimap_str_int.at_left(s2));
    EXPECT_EQ(s2, bimap_str_int.at_right(i3));
    EXPECT_FALSE(bimap_str_int.contains_left(s3));
    EXPECT_FALSE(bimap_str_int.contains_right(i2));
}

TEST_F(BiMapTest, InsertOrAssignRvalue) {
    bimap_str_int.insert_or_assign(std::string("move_s1"), 101);
    ASSERT_EQ(1, bimap_str_int.size());
    EXPECT_TRUE(bimap_str_int.contains_left("move_s1"));
    EXPECT_TRUE(bimap_str_int.contains_right(101));

    bimap_str_int.insert_or_assign("move_s1", 102); // const L&, R&&
    ASSERT_EQ(1, bimap_str_int.size());
    EXPECT_EQ(102, bimap_str_int.at_left("move_s1"));
    EXPECT_FALSE(bimap_str_int.contains_right(101));

    bimap_str_int.insert_or_assign(std::string("move_s2"), 102); // L&&, const R&
    ASSERT_EQ(1, bimap_str_int.size());
    EXPECT_EQ(102, bimap_str_int.at_left("move_s2"));
    EXPECT_FALSE(bimap_str_int.contains_left("move_s1"));

    bimap_str_int.insert_or_assign(std::string("move_s3"), 103); // L&&, R&&
    ASSERT_EQ(2, bimap_str_int.size()); // ("move_s2", 102), ("move_s3", 103)

    bimap_str_int.insert_or_assign(std::string("move_s2"), 103); // L&&, R&&. Breaks ("move_s3", 103)
                                                              // Expected: ("move_s2", 103)
    ASSERT_EQ(1, bimap_str_int.size());
    EXPECT_TRUE(bimap_str_int.contains_left("move_s2"));
    EXPECT_TRUE(bimap_str_int.contains_right(103));
    EXPECT_EQ(103, bimap_str_int.at_left("move_s2"));
    EXPECT_EQ("move_s2", bimap_str_int.at_right(103));
    EXPECT_FALSE(bimap_str_int.contains_left("move_s3"));
    EXPECT_FALSE(bimap_str_int.contains_right(102));
}

// --- Access and Lookup ---
TEST_F(BiMapTest, AtLeftRight) {
    bimap_str_int.insert(s1, i1);
    EXPECT_EQ(i1, bimap_str_int.at_left(s1));
    EXPECT_EQ(s1, bimap_str_int.at_right(i1));

    const auto& const_bimap = bimap_str_int;
    EXPECT_EQ(i1, const_bimap.at_left(s1));
    EXPECT_EQ(s1, const_bimap.at_right(i1));

    EXPECT_THROW(bimap_str_int.at_left("nonexistent"), std::out_of_range);
    EXPECT_THROW(bimap_str_int.at_right(999), std::out_of_range);
    EXPECT_THROW(const_bimap.at_left("nonexistent"), std::out_of_range);
    EXPECT_THROW(const_bimap.at_right(999), std::out_of_range);
}

TEST_F(BiMapTest, FindLeftRight) {
    bimap_str_int.insert(s1, i1);

    const int* r_ptr = bimap_str_int.find_left(s1); // Right is int for bimap_str_int
    ASSERT_NE(nullptr, r_ptr);
    EXPECT_EQ(i1, *r_ptr);

    const std::string* l_ptr = bimap_str_int.find_right(i1); // Left is std::string
    ASSERT_NE(nullptr, l_ptr);
    EXPECT_EQ(s1, *l_ptr);

    const auto& const_bimap = bimap_str_int;
    const int* r_ptr_const = const_bimap.find_left(s1);
    ASSERT_NE(nullptr, r_ptr_const);
    EXPECT_EQ(i1, *r_ptr_const);

    const std::string* l_ptr_const = const_bimap.find_right(i1);
    ASSERT_NE(nullptr, l_ptr_const);
    EXPECT_EQ(s1, *l_ptr_const);

    EXPECT_EQ(nullptr, bimap_str_int.find_left("nonexistent"));
    EXPECT_EQ(nullptr, bimap_str_int.find_right(999));
    EXPECT_EQ(nullptr, const_bimap.find_left("nonexistent"));
    EXPECT_EQ(nullptr, const_bimap.find_right(999));
}

TEST_F(BiMapTest, ContainsLeftRight) {
    bimap_str_int.insert(s1, i1);
    EXPECT_TRUE(bimap_str_int.contains_left(s1));
    EXPECT_FALSE(bimap_str_int.contains_left(s2));
    EXPECT_TRUE(bimap_str_int.contains_right(i1));
    EXPECT_FALSE(bimap_str_int.contains_right(i2));

    const auto& const_bimap = bimap_str_int;
    EXPECT_TRUE(const_bimap.contains_left(s1));
    EXPECT_FALSE(const_bimap.contains_left(s2));
    EXPECT_TRUE(const_bimap.contains_right(i1));
    EXPECT_FALSE(const_bimap.contains_right(i2));
}

// --- Erasure Operations ---
TEST_F(BiMapTest, EraseLeftKey) {
    bimap_str_int.insert(s1, i1);
    bimap_str_int.insert(s2, i2);

    ASSERT_TRUE(bimap_str_int.erase_left(s1));
    EXPECT_EQ(1, bimap_str_int.size());
    EXPECT_FALSE(bimap_str_int.contains_left(s1));
    EXPECT_FALSE(bimap_str_int.contains_right(i1));
    EXPECT_TRUE(bimap_str_int.contains_left(s2));

    ASSERT_FALSE(bimap_str_int.erase_left(s1));
    EXPECT_EQ(1, bimap_str_int.size());

    ASSERT_TRUE(bimap_str_int.erase_left(s2));
    EXPECT_TRUE(bimap_str_int.empty());
}

TEST_F(BiMapTest, EraseRightKey) {
    bimap_str_int.insert(s1, i1);
    bimap_str_int.insert(s2, i2);

    ASSERT_TRUE(bimap_str_int.erase_right(i1));
    EXPECT_EQ(1, bimap_str_int.size());
    EXPECT_FALSE(bimap_str_int.contains_right(i1));
    EXPECT_FALSE(bimap_str_int.contains_left(s1));
    EXPECT_TRUE(bimap_str_int.contains_right(i2));

    ASSERT_FALSE(bimap_str_int.erase_right(i1));
    EXPECT_EQ(1, bimap_str_int.size());

    ASSERT_TRUE(bimap_str_int.erase_right(i2));
    EXPECT_TRUE(bimap_str_int.empty());
}

TEST_F(BiMapTest, EraseLeftIterator) {
    bimap_str_int.insert(s1, i1);
    bimap_str_int.insert(s2, i2);
    bimap_str_int.insert(s3, i3);

    BiMap<std::string, int>::left_const_iterator it_s2_const = bimap_str_int.left_cbegin();
    while(it_s2_const != bimap_str_int.left_cend() && it_s2_const->first != s2) { ++it_s2_const; }
    ASSERT_NE(bimap_str_int.left_cend(), it_s2_const) << "s2 not found for iterator erase test";

    auto next_it = bimap_str_int.erase_left(it_s2_const);
    EXPECT_EQ(2, bimap_str_int.size());
    EXPECT_FALSE(bimap_str_int.contains_left(s2));
    EXPECT_FALSE(bimap_str_int.contains_right(i2));
    if (next_it != bimap_str_int.left_end()) {
         EXPECT_TRUE(bimap_str_int.contains_left(next_it->first));
    }

    auto end_it_const = bimap_str_int.left_cend();
    ASSERT_NO_THROW({ bimap_str_int.erase_left(end_it_const); });
    EXPECT_EQ(2, bimap_str_int.size());
}


TEST_F(BiMapTest, EraseRightIterator) {
    bimap_str_int.insert(s1, i1);
    bimap_str_int.insert(s2, i2);
    bimap_str_int.insert(s3, i3);

    BiMap<std::string, int>::right_const_iterator it_i2_const = bimap_str_int.right_cbegin();
    while(it_i2_const != bimap_str_int.right_cend() && it_i2_const->first != i2) { ++it_i2_const; }
    ASSERT_NE(bimap_str_int.right_cend(), it_i2_const) << "i2 not found for iterator erase test";

    auto next_it = bimap_str_int.erase_right(it_i2_const);
    EXPECT_EQ(2, bimap_str_int.size());
    EXPECT_FALSE(bimap_str_int.contains_right(i2));
    EXPECT_FALSE(bimap_str_int.contains_left(s2));
     if (next_it != bimap_str_int.right_end()) {
         EXPECT_TRUE(bimap_str_int.contains_right(next_it->first));
    }

    auto end_it_const = bimap_str_int.right_cend();
    ASSERT_NO_THROW({ bimap_str_int.erase_right(end_it_const); });
    EXPECT_EQ(2, bimap_str_int.size());
}


// --- Size, State, and Clear --- (SizeEmpty already covers some)
TEST_F(BiMapTest, Clear) {
    bimap_str_int.insert(s1, i1);
    bimap_str_int.insert(s2, i2);
    ASSERT_FALSE(bimap_str_int.empty());
    ASSERT_EQ(2, bimap_str_int.size());

    bimap_str_int.clear();
    EXPECT_TRUE(bimap_str_int.empty());
    EXPECT_EQ(0, bimap_str_int.size());
    EXPECT_FALSE(bimap_str_int.contains_left(s1));
    EXPECT_FALSE(bimap_str_int.contains_right(i1));

    ASSERT_NO_THROW(bimap_str_int.clear());
    EXPECT_TRUE(bimap_str_int.empty());
    EXPECT_EQ(0, bimap_str_int.size());
}

// Main function might be needed if not linking with GTest_main

// --- Iterators and Views ---
TEST_F(BiMapTest, LeftViewIteration) {
    bimap_str_int.insert(s1, i1);
    bimap_str_int.insert(s2, i2);
    bimap_str_int.insert(s3, i3);

    std::map<std::string, int> expected_map = {{s1, i1}, {s2, i2}, {s3, i3}};
    std::map<std::string, int> actual_map;

    for (const auto& pair : bimap_str_int.left()) {
        actual_map.insert(pair);
    }
    EXPECT_EQ(expected_map, actual_map);

    // Const iteration
    const auto& const_bimap = bimap_str_int;
    actual_map.clear();
    for (const auto& pair : const_bimap.left()) {
        actual_map.insert(pair);
    }
    EXPECT_EQ(expected_map, actual_map);

    // Empty view
    BiMap<std::string, int> empty_bimap;
    int count = 0;
    for (const auto& pair : empty_bimap.left()) {
        count++; (void)pair; // Avoid unused variable warning
    }
    EXPECT_EQ(0, count);
}

TEST_F(BiMapTest, RightViewIteration) {
    bimap_int_str.insert(i1, s1);
    bimap_int_str.insert(i2, s2);
    bimap_int_str.insert(i3, s3);

    // For BiMap<int, std::string>, right() view iterates pairs of <std::string, int>
    // because right_to_left_ map is std::unordered_map<std::string, int>
    std::map<std::string, int> expected_map_right_view;
    expected_map_right_view[s1] = i1;
    expected_map_right_view[s2] = i2;
    expected_map_right_view[s3] = i3;

    std::map<std::string, int> actual_map_right_view;

    for (const auto& pair : bimap_int_str.right()) { // pair is const std::pair<const std::string, int>&
        actual_map_right_view.insert(pair);
    }
    EXPECT_EQ(expected_map_right_view, actual_map_right_view);

    // Const iteration
    const auto& const_bimap = bimap_int_str;
    actual_map_right_view.clear();
    for (const auto& pair : const_bimap.right()) {
        actual_map_right_view.insert(pair);
    }
    EXPECT_EQ(expected_map_right_view, actual_map_right_view);

    // Empty view
    BiMap<int, std::string> empty_bimap; // This is BiMap<int, std::string>
    int count = 0;
    for (const auto& pair : empty_bimap.right()) { // pair is <const std::string, int>
        count++; (void)pair;
    }
    EXPECT_EQ(0, count);
}

TEST_F(BiMapTest, DefaultIteration) { // Should be same as left view
    bimap_str_int.insert(s1, i1);
    bimap_str_int.insert(s2, i2);

    std::map<std::string, int> expected_map = {{s1, i1}, {s2, i2}};
    std::map<std::string, int> actual_map;

    for (const auto& pair : bimap_str_int) { // Uses BiMap::begin()/end()
        actual_map.insert(pair);
    }
    EXPECT_EQ(expected_map, actual_map);

    // Const iteration
    const auto& const_bimap = bimap_str_int;
    actual_map.clear();
    for (const auto& pair : const_bimap) { // Uses BiMap::cbegin()/cend()
        actual_map.insert(pair);
    }
    EXPECT_EQ(expected_map, actual_map);
}

TEST_F(BiMapTest, ViewIteratorsBeginEnd) {
    bimap_str_int.insert(s1, i1);

    // Left View
    ASSERT_NE(bimap_str_int.left().begin(), bimap_str_int.left().end());
    EXPECT_EQ(s1, bimap_str_int.left().begin()->first);
    EXPECT_EQ(i1, bimap_str_int.left().begin()->second);

    // Right View (use bimap_int_str for more direct key testing)
    // bimap_int_str is BiMap<int, std::string>
    // Its right() view iterates over <std::string, int> pairs
    bimap_int_str.insert(i1, s1); // (int, string) -> (string, int) for right view
    ASSERT_NE(bimap_int_str.right().begin(), bimap_int_str.right().end());
    EXPECT_EQ(s1, bimap_int_str.right().begin()->first); // key in right view is string
    EXPECT_EQ(i1, bimap_int_str.right().begin()->second); // value in right view is int

    // Const versions
    const auto& const_bimap_si = bimap_str_int;
    ASSERT_NE(const_bimap_si.left().cbegin(), const_bimap_si.left().cend());
    EXPECT_EQ(s1, const_bimap_si.left().cbegin()->first);

    const auto& const_bimap_is = bimap_int_str;
    ASSERT_NE(const_bimap_is.right().cbegin(), const_bimap_is.right().cend());
    EXPECT_EQ(s1, const_bimap_is.right().cbegin()->first); // key in right view is string
}
// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }

// --- emplace and try_emplace ---
TEST_F(BiMapTest, EmplaceOperation) {
    // Successful emplace
    auto result1 = bimap_str_int.emplace(s1, i1);
    ASSERT_TRUE(result1.second);
    ASSERT_NE(bimap_str_int.left_end(), result1.first);
    EXPECT_EQ(s1, result1.first->first);
    EXPECT_EQ(i1, result1.first->second);
    EXPECT_EQ(1, bimap_str_int.size());

    // Attempt to emplace when left key exists
    auto result2 = bimap_str_int.emplace(s1, i2); // s1 already exists
    ASSERT_FALSE(result2.second);
    ASSERT_NE(bimap_str_int.left_end(), result2.first);
    EXPECT_EQ(s1, result2.first->first); // Iterator to existing element
    EXPECT_EQ(i1, result2.first->second); // Original value
    EXPECT_EQ(1, bimap_str_int.size());

    // Attempt to emplace when right value exists
    // Current: (s1, i1)
    auto result3 = bimap_str_int.emplace(s2, i1); // i1 already exists
    ASSERT_FALSE(result3.second);
    EXPECT_EQ(bimap_str_int.left_end(), result3.first);
    EXPECT_EQ(1, bimap_str_int.size());
    EXPECT_TRUE(bimap_str_int.contains_left(s1));
    EXPECT_FALSE(bimap_str_int.contains_left(s2));

    auto result4 = bimap_str_int.emplace("hello_emplace", 42);
    ASSERT_TRUE(result4.second);
    EXPECT_EQ("hello_emplace", result4.first->first);
    EXPECT_EQ(42, result4.first->second);
    EXPECT_EQ(2, bimap_str_int.size());
}

TEST_F(BiMapTest, TryEmplaceLeftConstKey) {
    auto result1 = bimap_str_int.try_emplace_left(s1, i1);
    ASSERT_TRUE(result1.second);
    ASSERT_NE(bimap_str_int.left_end(), result1.first);
    EXPECT_EQ(s1, result1.first->first);
    EXPECT_EQ(i1, result1.first->second);
    EXPECT_EQ(1, bimap_str_int.size());

    auto result2 = bimap_str_int.try_emplace_left(s1, i2);
    ASSERT_FALSE(result2.second);
    ASSERT_NE(bimap_str_int.left_end(), result2.first);
    EXPECT_EQ(s1, result2.first->first);
    EXPECT_EQ(i1, result2.first->second);
    EXPECT_EQ(1, bimap_str_int.size());

    auto result3 = bimap_str_int.try_emplace_left(s2, i1);
    ASSERT_FALSE(result3.second);
    EXPECT_EQ(bimap_str_int.left_end(), result3.first);
    EXPECT_EQ(1, bimap_str_int.size());
    EXPECT_FALSE(bimap_str_int.contains_left(s2));
}

TEST_F(BiMapTest, TryEmplaceLeftRvalueKey) {
    auto result1 = bimap_str_int.try_emplace_left(std::string("move_s1"), i1);
    ASSERT_TRUE(result1.second);
    ASSERT_NE(bimap_str_int.left_end(), result1.first);
    EXPECT_EQ("move_s1", result1.first->first);
    EXPECT_EQ(i1, result1.first->second);
    EXPECT_EQ(1, bimap_str_int.size());

    std::string move_s1_copy = "move_s1";
    auto result2 = bimap_str_int.try_emplace_left(std::move(move_s1_copy), i2);
    ASSERT_FALSE(result2.second);
    ASSERT_NE(bimap_str_int.left_end(), result2.first);
    EXPECT_EQ("move_s1", result2.first->first);
    EXPECT_EQ(i1, result2.first->second);
    EXPECT_EQ(1, bimap_str_int.size());
    EXPECT_FALSE(move_s1_copy.empty());

    std::string move_s2 = "move_s2";
    auto result3 = bimap_str_int.try_emplace_left(std::move(move_s2), i1);
    ASSERT_FALSE(result3.second);
    EXPECT_EQ(bimap_str_int.left_end(), result3.first);
    EXPECT_EQ(1, bimap_str_int.size());
    EXPECT_FALSE(bimap_str_int.contains_left("move_s2"));
    EXPECT_FALSE(move_s2.empty());
}

TEST_F(BiMapTest, TryEmplaceRightConstKey) {
    // bimap_int_str is BiMap<int, std::string> (Left=int, Right=std::string)
    // try_emplace_right(const Right& k, Args&&... args) means k is std::string, Args constructs int

    auto result1 = bimap_int_str.try_emplace_right(s1, i1);
    ASSERT_TRUE(result1.second);
    ASSERT_NE(bimap_int_str.right_end(), result1.first);
    EXPECT_EQ(s1, result1.first->first);
    EXPECT_EQ(i1, result1.first->second);
    EXPECT_EQ(1, bimap_int_str.size());
    EXPECT_TRUE(bimap_int_str.contains_left(i1));
    EXPECT_TRUE(bimap_int_str.contains_right(s1));

    auto result2 = bimap_int_str.try_emplace_right(s1, i2);
    ASSERT_FALSE(result2.second);
    ASSERT_NE(bimap_int_str.right_end(), result2.first);
    EXPECT_EQ(s1, result2.first->first);
    EXPECT_EQ(i1, result2.first->second);
    EXPECT_EQ(1, bimap_int_str.size());

    auto result3 = bimap_int_str.try_emplace_right(s2, i1);
    ASSERT_FALSE(result3.second);
    EXPECT_EQ(bimap_int_str.right_end(), result3.first);
    EXPECT_EQ(1, bimap_int_str.size());
    EXPECT_FALSE(bimap_int_str.contains_right(s2));
}

TEST_F(BiMapTest, TryEmplaceRightRvalueKey) {
    // bimap_int_str is BiMap<int, std::string> (Left=int, Right=std::string)
    // try_emplace_right(Right&& k, Args&&... args) means k is std::string&&, Args constructs int

    std::string r_key_str1 = "move_s1";
    auto result1 = bimap_int_str.try_emplace_right(std::move(r_key_str1), i1);
    ASSERT_TRUE(result1.second);
    ASSERT_NE(bimap_int_str.right_end(), result1.first);
    EXPECT_EQ("move_s1", result1.first->first);
    EXPECT_EQ(i1, result1.first->second);
    EXPECT_EQ(1, bimap_int_str.size());
    EXPECT_TRUE(r_key_str1.empty() || r_key_str1 != "move_s1");

    std::string r_key_str1_copy = "move_s1";
    std::string r_key_str1_movable = "move_s1";
    auto result2 = bimap_int_str.try_emplace_right(std::move(r_key_str1_movable), i2);
    ASSERT_FALSE(result2.second);
    ASSERT_NE(bimap_int_str.right_end(), result2.first);
    EXPECT_EQ(r_key_str1_copy, result2.first->first);
    EXPECT_EQ(i1, result2.first->second);
    EXPECT_EQ(1, bimap_int_str.size());
    EXPECT_EQ("move_s1", r_key_str1_movable);

    std::string r_key_str2_movable = "move_s2";
    auto result3 = bimap_int_str.try_emplace_right(std::move(r_key_str2_movable), i1);
    ASSERT_FALSE(result3.second);
    EXPECT_EQ(bimap_int_str.right_end(), result3.first);
    EXPECT_EQ(1, bimap_int_str.size());
    EXPECT_FALSE(bimap_int_str.contains_right("move_s2"));
    EXPECT_EQ("move_s2", r_key_str2_movable);
}

// --- Swap Functionality ---
TEST_F(BiMapTest, MemberSwap) {
    bimap_str_int.insert(s1, i1);
    BiMap<std::string, int> other_bimap;
    other_bimap.insert(s2, i2);
    other_bimap.insert(s3, i3);

    bimap_str_int.swap(other_bimap);

    ASSERT_EQ(2, bimap_str_int.size());
    EXPECT_TRUE(bimap_str_int.contains_left(s2));
    EXPECT_TRUE(bimap_str_int.contains_right(i3));

    ASSERT_EQ(1, other_bimap.size());
    EXPECT_TRUE(other_bimap.contains_left(s1));
    EXPECT_TRUE(other_bimap.contains_right(i1));

    BiMap<std::string, int> empty_bimap;
    bimap_str_int.swap(empty_bimap);
    ASSERT_TRUE(bimap_str_int.empty());
    ASSERT_EQ(2, empty_bimap.size());
    EXPECT_TRUE(empty_bimap.contains_left(s2));
}

TEST_F(BiMapTest, NonMemberSwap) {
    bimap_str_int.insert(s1, i1);
    BiMap<std::string, int> other_bimap;
    other_bimap.insert(s2, i2);
    other_bimap.insert(s3, i3);

    using std::swap;
    swap(bimap_str_int, other_bimap);

    ASSERT_EQ(2, bimap_str_int.size());
    EXPECT_TRUE(bimap_str_int.contains_left(s2));

    ASSERT_EQ(1, other_bimap.size());
    EXPECT_TRUE(other_bimap.contains_left(s1));
}

// --- Comparison Operators ---
TEST_F(BiMapTest, EqualityOperators) {
    bimap_str_int.insert(s1, i1);
    bimap_str_int.insert(s2, i2);

    BiMap<std::string, int> same_bimap;
    same_bimap.insert(s1, i1);
    same_bimap.insert(s2, i2);

    BiMap<std::string, int> different_size_bimap;
    different_size_bimap.insert(s1, i1);

    BiMap<std::string, int> different_key_bimap;
    different_key_bimap.insert(s1, i1);
    different_key_bimap.insert(s3, i2);

    BiMap<std::string, int> different_value_bimap;
    different_value_bimap.insert(s1, i1);
    different_value_bimap.insert(s2, i3);

    BiMap<std::string, int> empty_bimap1;
    BiMap<std::string, int> empty_bimap2;


    EXPECT_TRUE(bimap_str_int == same_bimap);
    EXPECT_FALSE(bimap_str_int != same_bimap);

    EXPECT_FALSE(bimap_str_int == different_size_bimap);
    EXPECT_TRUE(bimap_str_int != different_size_bimap);

    EXPECT_FALSE(bimap_str_int == different_key_bimap);
    EXPECT_TRUE(bimap_str_int != different_key_bimap);

    EXPECT_FALSE(bimap_str_int == different_value_bimap);
    EXPECT_TRUE(bimap_str_int != different_value_bimap);

    EXPECT_TRUE(empty_bimap1 == empty_bimap2);
    EXPECT_FALSE(empty_bimap1 != empty_bimap2);
    EXPECT_FALSE(bimap_str_int == empty_bimap1);
}

// --- STL Algorithm Compatibility ---
TEST_F(BiMapTest, FindIf) {
    bimap_str_int.insert(s1, i1);
    bimap_str_int.insert(s2, i2);
    bimap_str_int.insert(s3, i3);

    auto it_left = std::find_if(bimap_str_int.left().begin(), bimap_str_int.left().end(),
        [](const auto& pair){ return pair.first == "two"; });
    ASSERT_NE(bimap_str_int.left().end(), it_left);
    EXPECT_EQ(s2, it_left->first);
    EXPECT_EQ(i2, it_left->second);

    auto it_right = std::find_if(bimap_str_int.right().begin(), bimap_str_int.right().end(),
        [](const auto& pair){ return pair.first == 2; });
    ASSERT_NE(bimap_str_int.right().end(), it_right);
    EXPECT_EQ(i2, it_right->first);
    EXPECT_EQ(s2, it_right->second);
}

TEST_F(BiMapTest, ForEach) {
    bimap_str_int.insert(s1, i1);
    bimap_str_int.insert(s2, i2);

    std::map<std::string, int> collected_left;
    std::for_each(bimap_str_int.left().begin(), bimap_str_int.left().end(),
        [&collected_left](const auto& pair){ collected_left.insert(pair); });
    ASSERT_EQ(2, collected_left.size());
    EXPECT_EQ(i1, collected_left[s1]);

    std::map<int, std::string> collected_right;
    std::for_each(bimap_str_int.right().begin(), bimap_str_int.right().end(),
        [&collected_right](const auto& pair){ collected_right.insert(pair); });
    ASSERT_EQ(2, collected_right.size());
    EXPECT_EQ(s1, collected_right[i1]);
}

TEST_F(BiMapTest, CountIf) {
    bimap_str_int.insert(s1, i1);
    bimap_str_int.insert(s2, i2);
    bimap_str_int.insert("hundred", 100);

    long count_left = std::count_if(bimap_str_int.left().begin(), bimap_str_int.left().end(),
        [](const auto& pair){ return pair.second > 10; });
    EXPECT_EQ(1, count_left);

    long count_right = std::count_if(bimap_str_int.right().begin(), bimap_str_int.right().end(),
        [](const auto& pair){ return pair.second.length() > 3; });
    EXPECT_EQ(1, count_right);
}

TEST_F(BiMapTest, TransformToVector) {
    bimap_str_int.insert(s1, i1);
    bimap_str_int.insert(s2, i2);

    std::vector<std::string> left_keys;
    std::transform(bimap_str_int.left().begin(), bimap_str_int.left().end(), std::back_inserter(left_keys),
        [](const auto& pair){ return pair.first; });
    std::sort(left_keys.begin(), left_keys.end());
    ASSERT_EQ(2, left_keys.size());
    if (s1 < s2) { // Ensure order for comparison if needed, though set/map might handle
      EXPECT_EQ(s1, left_keys[0]);
      EXPECT_EQ(s2, left_keys[1]);
    } else {
      EXPECT_EQ(s2, left_keys[0]);
      EXPECT_EQ(s1, left_keys[1]);
    }


    std::vector<int> right_keys;
    std::transform(bimap_str_int.right().begin(), bimap_str_int.right().end(), std::back_inserter(right_keys),
        [](const auto& pair){ return pair.first; });
    std::sort(right_keys.begin(), right_keys.end());
    ASSERT_EQ(2, right_keys.size());
     if (i1 < i2) {
        EXPECT_EQ(i1, right_keys[0]);
        EXPECT_EQ(i2, right_keys[1]);
    } else {
        EXPECT_EQ(i2, right_keys[0]);
        EXPECT_EQ(i1, right_keys[1]);
    }
}


// --- Edge Cases ---
TEST_F(BiMapTest, OperationsOnEmptyBiMap) {
    ASSERT_TRUE(bimap_str_int.empty());
    ASSERT_EQ(0, bimap_str_int.size());
    EXPECT_THROW(bimap_str_int.at_left(s1), std::out_of_range);
    EXPECT_THROW(bimap_str_int.at_right(i1), std::out_of_range);
    EXPECT_EQ(nullptr, bimap_str_int.find_left(s1));
    EXPECT_EQ(nullptr, bimap_str_int.find_right(i1));
    EXPECT_FALSE(bimap_str_int.contains_left(s1));
    EXPECT_FALSE(bimap_str_int.contains_right(i1));
    EXPECT_FALSE(bimap_str_int.erase_left(s1));
    EXPECT_FALSE(bimap_str_int.erase_right(i1));
    ASSERT_NO_THROW(bimap_str_int.clear());

    auto it_l = bimap_str_int.left().begin();
    auto end_l = bimap_str_int.left().end();
    EXPECT_EQ(it_l, end_l);

    auto it_r = bimap_str_int.right().begin();
    auto end_r = bimap_str_int.right().end();
    EXPECT_EQ(it_r, end_r);

    BiMap<std::string, int> other_empty;
    EXPECT_TRUE(bimap_str_int == other_empty);
    ASSERT_NO_THROW(bimap_str_int.swap(other_empty));
}

TEST_F(BiMapTest, OperationsOnSingleElementBiMap) {
    bimap_str_int.insert(s1, i1);
    ASSERT_EQ(1, bimap_str_int.size());

    EXPECT_EQ(i1, bimap_str_int.at_left(s1));
    EXPECT_EQ(s1, bimap_str_int.at_right(i1));
    EXPECT_TRUE(bimap_str_int.contains_left(s1));
    EXPECT_TRUE(bimap_str_int.contains_right(i1));

    int count = 0;
    for(const auto& p : bimap_str_int.left()) {
        EXPECT_EQ(s1, p.first);
        EXPECT_EQ(i1, p.second);
        count++;
    }
    EXPECT_EQ(1, count);

    ASSERT_TRUE(bimap_str_int.erase_left(s1));
    ASSERT_TRUE(bimap_str_int.empty());

    bimap_str_int.insert(s1, i1);
    ASSERT_TRUE(bimap_str_int.erase_right(i1));
    ASSERT_TRUE(bimap_str_int.empty());
}

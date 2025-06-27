#include "include/ScapegoatTree.h"
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <algorithm> // For std::sort, std::is_sorted
#include <set>

// Using namespace for convenience in test file
using namespace cpp_collections;

// Test fixture for ScapegoatTree tests
class ScapegoatTreeTest : public ::testing::Test {
protected:
    ScapegoatTree<int, std::string> tree;
    ScapegoatTree<std::string, int, std::greater<std::string>> reverse_tree; // Test with custom comparator

    void SetUp() override {
        // Common setup for tests, if any
    }

    void TearDown() override {
        // Common teardown for tests, if any
    }
};

TEST_F(ScapegoatTreeTest, Constructor) {
    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(tree.size(), 0);
    EXPECT_THROW(ScapegoatTree<int, int> tree_invalid_alpha_low(0.5), std::invalid_argument);
    EXPECT_THROW(ScapegoatTree<int, int> tree_invalid_alpha_high(1.0), std::invalid_argument);
    EXPECT_NO_THROW(ScapegoatTree<int, int> tree_valid_alpha(0.75));
}

TEST_F(ScapegoatTreeTest, InsertBasic) {
    EXPECT_TRUE(tree.insert(10, "ten"));
    EXPECT_EQ(tree.size(), 1);
    EXPECT_FALSE(tree.empty());
    EXPECT_TRUE(tree.contains(10));
    ASSERT_NE(tree.find(10), nullptr);
    EXPECT_EQ(*tree.find(10), "ten");

    EXPECT_FALSE(tree.insert(10, "ten_again")); // Key exists, update value
    EXPECT_EQ(tree.size(), 1); // Size should not change
    EXPECT_EQ(*tree.find(10), "ten_again");

    EXPECT_TRUE(tree.insert(5, "five"));
    EXPECT_EQ(tree.size(), 2);
    EXPECT_TRUE(tree.contains(5));
    ASSERT_NE(tree.find(5), nullptr);
    EXPECT_EQ(*tree.find(5), "five");

    EXPECT_TRUE(tree.insert(15, "fifteen"));
    EXPECT_EQ(tree.size(), 3);
    EXPECT_TRUE(tree.contains(15));
    ASSERT_NE(tree.find(15), nullptr);
    EXPECT_EQ(*tree.find(15), "fifteen");
}

TEST_F(ScapegoatTreeTest, FindNonExistent) {
    tree.insert(10, "ten");
    EXPECT_EQ(tree.find(100), nullptr);
    EXPECT_FALSE(tree.contains(100));
}

TEST_F(ScapegoatTreeTest, EraseBasic) {
    tree.insert(10, "ten");
    tree.insert(5, "five");
    EXPECT_EQ(tree.size(), 2);

    EXPECT_TRUE(tree.erase(10));
    EXPECT_EQ(tree.size(), 1);
    EXPECT_FALSE(tree.contains(10));
    EXPECT_EQ(tree.find(10), nullptr);
    EXPECT_TRUE(tree.contains(5)); // Other element should still be there

    EXPECT_FALSE(tree.erase(10)); // Already erased
    EXPECT_EQ(tree.size(), 1);

    EXPECT_TRUE(tree.erase(5));
    EXPECT_EQ(tree.size(), 0);
    EXPECT_TRUE(tree.empty());
    EXPECT_FALSE(tree.contains(5));
}

TEST_F(ScapegoatTreeTest, ReactivateNode) {
    tree.insert(10, "ten_v1");
    ASSERT_TRUE(tree.erase(10));
    EXPECT_EQ(tree.size(), 0);
    EXPECT_FALSE(tree.contains(10));

    EXPECT_TRUE(tree.insert(10, "ten_v2")); // Reactivates and updates
    EXPECT_EQ(tree.size(), 1);
    EXPECT_TRUE(tree.contains(10));
    ASSERT_NE(tree.find(10), nullptr);
    EXPECT_EQ(*tree.find(10), "ten_v2");
}

TEST_F(ScapegoatTreeTest, Clear) {
    tree.insert(10, "ten");
    tree.insert(5, "five");
    tree.insert(15, "fifteen");
    EXPECT_EQ(tree.size(), 3);

    tree.clear();
    EXPECT_EQ(tree.size(), 0);
    EXPECT_TRUE(tree.empty());
    EXPECT_FALSE(tree.contains(10));
    EXPECT_EQ(tree.find(5), nullptr);
}

TEST_F(ScapegoatTreeTest, MultipleInsertions) {
    const int num_insertions = 100;
    for (int i = 0; i < num_insertions; ++i) {
        EXPECT_TRUE(tree.insert(i, "val_" + std::to_string(i)));
    }
    EXPECT_EQ(tree.size(), num_insertions);
    for (int i = 0; i < num_insertions; ++i) {
        EXPECT_TRUE(tree.contains(i));
        ASSERT_NE(tree.find(i), nullptr);
        EXPECT_EQ(*tree.find(i), "val_" + std::to_string(i));
    }

    // Check that inserting existing keys updates them but doesn't change size
    for (int i = 0; i < num_insertions; ++i) {
        EXPECT_FALSE(tree.insert(i, "new_val_" + std::to_string(i)));
        EXPECT_EQ(tree.size(), num_insertions); // Size should remain the same
    }
    for (int i = 0; i < num_insertions; ++i) {
        ASSERT_NE(tree.find(i), nullptr);
        EXPECT_EQ(*tree.find(i), "new_val_" + std::to_string(i));
    }
}

TEST_F(ScapegoatTreeTest, InsertDeleteMix) {
    std::vector<int> keys = {10, 5, 15, 3, 7, 12, 17, 1, 4, 6, 8, 11, 13, 16, 18};
    for (int key : keys) {
        tree.insert(key, "val_" + std::to_string(key));
    }
    EXPECT_EQ(tree.size(), keys.size());

    // Erase some elements
    EXPECT_TRUE(tree.erase(7));
    EXPECT_TRUE(tree.erase(12));
    EXPECT_TRUE(tree.erase(1));
    EXPECT_EQ(tree.size(), keys.size() - 3);

    EXPECT_FALSE(tree.contains(7));
    EXPECT_TRUE(tree.contains(15));

    // Re-insert some, insert new ones
    EXPECT_TRUE(tree.insert(7, "new_seven")); // Reactivate
    EXPECT_EQ(tree.size(), keys.size() - 2);
    EXPECT_TRUE(tree.contains(7));
    EXPECT_EQ(*tree.find(7), "new_seven");

    EXPECT_TRUE(tree.insert(20, "twenty"));
    EXPECT_EQ(tree.size(), keys.size() - 1);
    EXPECT_TRUE(tree.contains(20));
}


TEST_F(ScapegoatTreeTest, IteratorBasicTraversal) {
    std::set<int> expected_keys;
    tree.insert(50, "fifty"); expected_keys.insert(50);
    tree.insert(30, "thirty"); expected_keys.insert(30);
    tree.insert(70, "seventy"); expected_keys.insert(70);
    tree.insert(20, "twenty"); expected_keys.insert(20);
    tree.insert(40, "forty"); expected_keys.insert(40);
    tree.insert(60, "sixty"); expected_keys.insert(60);
    tree.insert(80, "eighty"); expected_keys.insert(80);

    std::vector<int> iterated_keys;
    for (const auto& pair : tree) { // Uses iterator
        iterated_keys.push_back(pair.first);
    }

    EXPECT_EQ(iterated_keys.size(), expected_keys.size());
    EXPECT_TRUE(std::is_sorted(iterated_keys.begin(), iterated_keys.end()));

    std::vector<int> sorted_expected_keys(expected_keys.begin(), expected_keys.end());
    EXPECT_EQ(iterated_keys, sorted_expected_keys);

    // Test const iterator
    const auto& const_tree = tree;
    std::vector<int> const_iterated_keys;
    for (const auto& pair : const_tree) {
        const_iterated_keys.push_back(pair.first);
    }
    EXPECT_EQ(const_iterated_keys, sorted_expected_keys);
}

TEST_F(ScapegoatTreeTest, IteratorWithDeletions) {
    tree.insert(5, "E");
    tree.insert(2, "B");
    tree.insert(8, "H");
    tree.insert(1, "A");
    tree.insert(3, "C");
    tree.insert(7, "G");
    tree.insert(9, "I");
    tree.insert(4, "D"); // Not inserted
    tree.insert(6, "F");

    tree.erase(8); // H deleted
    tree.erase(4); // D was not inserted, but if it were, it would be deleted. No effect.
    tree.erase(1); // A deleted

    std::vector<int> expected_keys_after_delete = {2, 3, 5, 6, 7, 9}; // B, C, E, F, G, I
    std::vector<int> iterated_keys;
    for (const auto& pair : tree) {
        iterated_keys.push_back(pair.first);
    }

    EXPECT_EQ(iterated_keys.size(), expected_keys_after_delete.size());
    EXPECT_EQ(iterated_keys, expected_keys_after_delete);
}

TEST_F(ScapegoatTreeTest, EmptyTreeIteration) {
    int count = 0;
    for (const auto& pair : tree) {
        count++;
        (void)pair; // Suppress unused variable warning
    }
    EXPECT_EQ(count, 0);

    const auto& const_tree = tree;
    count = 0;
    for (const auto& pair : const_tree) {
        count++;
        (void)pair;
    }
    EXPECT_EQ(count, 0);
}

TEST_F(ScapegoatTreeTest, CustomComparator) {
    reverse_tree.insert("banana", 1);
    reverse_tree.insert("apple", 2);
    reverse_tree.insert("cherry", 3);

    EXPECT_EQ(reverse_tree.size(), 3);
    std::vector<std::string> keys;
    for (const auto& p : reverse_tree) {
        keys.push_back(p.first);
    }
    // Expected order with std::greater: cherry, banana, apple
    std::vector<std::string> expected = {"cherry", "banana", "apple"};
    EXPECT_EQ(keys, expected);

    ASSERT_NE(reverse_tree.find("apple"), nullptr);
    EXPECT_EQ(*reverse_tree.find("apple"), 2);
}


// Main function to run tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

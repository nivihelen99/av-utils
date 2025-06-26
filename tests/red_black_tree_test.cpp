#include "RedBlackTree.h"
#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include <random>
#include <set>

// Using namespace for convenience in test file
using namespace collections;

// Test fixture for RedBlackTree tests
class RedBlackTreeTest : public ::testing::Test {
protected:
    RedBlackTree<int, std::string> tree;

    // Helper to check all RBT properties
    void checkRBTProperties(const RedBlackTree<int, std::string>& rbt) {
        ASSERT_TRUE(rbt.checkProperty2()) << "Property 2 (Root is Black) failed.";
        // Property 3 (All leaves are black) is implicit by TNULL design
        ASSERT_TRUE(rbt.checkProperty4()) << "Property 4 (Red node children are black) failed.";
        ASSERT_TRUE(rbt.checkProperty5()) << "Property 5 (Same black height) failed.";
    }

    // Helper to verify tree structure and inorder traversal
    void verifyInorder(const RedBlackTree<int, std::string>& rbt, const std::vector<int>& expected_keys) {
        std::vector<int> actual_keys;
        std::function<void(std::shared_ptr<typename RedBlackTree<int, std::string>::Node>)> inorder_traversal =
            [&](std::shared_ptr<typename RedBlackTree<int, std::string>::Node> node) {
            if (node == nullptr || node == rbt.getTNULL()) {
                return;
            }
            inorder_traversal(node->left);
            actual_keys.push_back(node->key);
            inorder_traversal(node->right);
        };
        inorder_traversal(rbt.getRoot());
        ASSERT_EQ(actual_keys.size(), expected_keys.size());
        for (size_t i = 0; i < expected_keys.size(); ++i) {
            ASSERT_EQ(actual_keys[i], expected_keys[i]);
        }
    }
};

TEST_F(RedBlackTreeTest, IsEmptyInitially) {
    ASSERT_TRUE(tree.isEmpty());
    checkRBTProperties(tree);
}

TEST_F(RedBlackTreeTest, InsertSingleElement) {
    tree.insert(10, "ten");
    ASSERT_FALSE(tree.isEmpty());
    ASSERT_NE(tree.find(10), nullptr);
    ASSERT_EQ(*tree.find(10), "ten");
    checkRBTProperties(tree);
    verifyInorder(tree, {10});
}

TEST_F(RedBlackTreeTest, InsertMultipleElements) {
    tree.insert(10, "ten");
    tree.insert(5, "five");
    tree.insert(15, "fifteen");
    tree.insert(3, "three");
    tree.insert(7, "seven");
    tree.insert(12, "twelve");
    tree.insert(17, "seventeen");

    ASSERT_FALSE(tree.isEmpty());
    checkRBTProperties(tree);

    ASSERT_NE(tree.find(10), nullptr);
    ASSERT_EQ(*tree.find(10), "ten");
    ASSERT_NE(tree.find(5), nullptr);
    ASSERT_EQ(*tree.find(5), "five");
    ASSERT_NE(tree.find(17), nullptr);
    ASSERT_EQ(*tree.find(17), "seventeen");
    ASSERT_EQ(tree.find(100), nullptr); // Test find non-existent

    verifyInorder(tree, {3, 5, 7, 10, 12, 15, 17});
}

TEST_F(RedBlackTreeTest, InsertDuplicateKeyUpdatesValue) {
    tree.insert(10, "ten_v1");
    ASSERT_EQ(*tree.find(10), "ten_v1");
    checkRBTProperties(tree);

    tree.insert(10, "ten_v2");
    ASSERT_EQ(*tree.find(10), "ten_v2"); // Value should be updated
    checkRBTProperties(tree);
    verifyInorder(tree, {10}); // Structure should remain valid
}


TEST_F(RedBlackTreeTest, DeleteFromEmptyTree) {
    tree.remove(10); // Should not crash
    ASSERT_TRUE(tree.isEmpty());
    checkRBTProperties(tree);
}

TEST_F(RedBlackTreeTest, DeleteNonExistentKey) {
    tree.insert(10, "ten");
    tree.remove(20); // Should not affect the tree
    ASSERT_NE(tree.find(10), nullptr);
    checkRBTProperties(tree);
    verifyInorder(tree, {10});
}

TEST_F(RedBlackTreeTest, DeleteSingleElement) {
    tree.insert(10, "ten");
    tree.remove(10);
    ASSERT_TRUE(tree.isEmpty());
    ASSERT_EQ(tree.find(10), nullptr);
    checkRBTProperties(tree);
    verifyInorder(tree, {});
}

TEST_F(RedBlackTreeTest, DeleteRootNode) {
    tree.insert(10, "ten");
    tree.insert(5, "five");
    tree.insert(15, "fifteen");
    checkRBTProperties(tree);

    tree.remove(10); // Delete root
    ASSERT_EQ(tree.find(10), nullptr);
    ASSERT_NE(tree.find(5), nullptr);
    ASSERT_NE(tree.find(15), nullptr);
    checkRBTProperties(tree);
    verifyInorder(tree, {5, 15});
}

TEST_F(RedBlackTreeTest, DeleteNodeWithTwoChildren) {
    tree.insert(10, "ten");
    tree.insert(5, "five");
    tree.insert(15, "fifteen");
    tree.insert(3, "three");
    tree.insert(7, "seven");
    tree.insert(12, "twelve");
    tree.insert(17, "seventeen");
    checkRBTProperties(tree);

    tree.remove(15); // Node with two children (12, 17)
    ASSERT_EQ(tree.find(15), nullptr);
    checkRBTProperties(tree);
    verifyInorder(tree, {3, 5, 7, 10, 12, 17});

    tree.remove(5); // Node with two children (3, 7)
    ASSERT_EQ(tree.find(5), nullptr);
    checkRBTProperties(tree);
    verifyInorder(tree, {3, 7, 10, 12, 17});
}


TEST_F(RedBlackTreeTest, DeleteNodeWithOneRightChild) {
    tree.insert(10, "ten");
    tree.insert(5, "five");
    tree.insert(15, "fifteen");
    tree.insert(17, "seventeen"); // 15 has only right child 17
    checkRBTProperties(tree);

    tree.remove(15);
    ASSERT_EQ(tree.find(15), nullptr);
    ASSERT_NE(tree.find(17), nullptr);
    checkRBTProperties(tree);
    verifyInorder(tree, {5, 10, 17});
}

TEST_F(RedBlackTreeTest, DeleteNodeWithOneLeftChild) {
    tree.insert(10, "ten");
    tree.insert(5, "five");
    tree.insert(15, "fifteen");
    tree.insert(12, "twelve"); // 15 has only left child 12
    checkRBTProperties(tree);

    tree.remove(15);
    ASSERT_EQ(tree.find(15), nullptr);
    ASSERT_NE(tree.find(12), nullptr);
    checkRBTProperties(tree);
    verifyInorder(tree, {5, 10, 12});
}

TEST_F(RedBlackTreeTest, DeleteLeafNode) {
    tree.insert(10, "ten");
    tree.insert(5, "five");
    tree.insert(15, "fifteen");
    checkRBTProperties(tree);

    tree.remove(5); // Leaf node
    ASSERT_EQ(tree.find(5), nullptr);
    checkRBTProperties(tree);
    verifyInorder(tree, {10, 15});

    tree.remove(15); // Leaf node
    ASSERT_EQ(tree.find(15), nullptr);
    checkRBTProperties(tree);
    verifyInorder(tree, {10});
}

TEST_F(RedBlackTreeTest, ComprehensiveInsertDelete) {
    std::vector<int> keys = {10, 5, 15, 3, 7, 12, 17, 1, 4, 6, 8, 11, 13, 16, 18, 20, 22, 25, 30};
    for (int key : keys) {
        tree.insert(key, "val_" + std::to_string(key));
        checkRBTProperties(tree);
    }

    std::sort(keys.begin(), keys.end());
    verifyInorder(tree, keys);

    // Delete some elements
    std::vector<int> keys_to_delete = {7, 1, 13, 22, 10}; // Mix of leaf, internal, root-like
    for (int key : keys_to_delete) {
        tree.remove(key);
        keys.erase(std::remove(keys.begin(), keys.end(), key), keys.end());
        checkRBTProperties(tree);
        verifyInorder(tree, keys);
        ASSERT_FALSE(tree.contains(key));
    }

    // Add some back
    tree.insert(7, "new_seven");
    keys.push_back(7);
    std::sort(keys.begin(), keys.end());
    checkRBTProperties(tree);
    verifyInorder(tree, keys);
    ASSERT_TRUE(tree.contains(7));
    ASSERT_EQ(*tree.find(7), "new_seven");

    tree.insert(22, "new_twentytwo");
    keys.push_back(22);
    std::sort(keys.begin(), keys.end());
    checkRBTProperties(tree);
    verifyInorder(tree, keys);
    ASSERT_TRUE(tree.contains(22));


    // Delete all elements
    std::shuffle(keys.begin(), keys.end(), std::mt19937{std::random_device{}()}); // Delete in random order
    for (int key : keys) {
        tree.remove(key);
        checkRBTProperties(tree); // Check properties at each step
    }
    ASSERT_TRUE(tree.isEmpty());
    verifyInorder(tree, {});
}


TEST_F(RedBlackTreeTest, RandomizedOperations) {
    const int num_operations = 1000;
    const int key_range = 500;
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> op_dist(0, 2); // 0: insert, 1: delete, 2: find
    std::uniform_int_distribution<int> key_dist(0, key_range - 1);

    std::set<int> reference_set; // Use std::set as a reference

    for (int i = 0; i < num_operations; ++i) {
        int key = key_dist(rng);
        int operation = op_dist(rng);

        if (reference_set.empty() && operation == 1) { // Avoid deleting from empty set/tree
            operation = 0;
        }
        if (!reference_set.empty() && operation == 1) { // Ensure key exists for deletion if set is not empty
             // Pick a random key from the set to delete
            std::uniform_int_distribution<int> existing_key_idx_dist(0, reference_set.size() - 1);
            auto it = reference_set.begin();
            std::advance(it, existing_key_idx_dist(rng));
            key = *it;
        }


        if (operation == 0) { // Insert
            // std::cout << "Op: Insert " << key << std::endl;
            tree.insert(key, "val_" + std::to_string(key));
            reference_set.insert(key);
        } else if (operation == 1) { // Delete
             // std::cout << "Op: Delete " << key << std::endl;
            tree.remove(key);
            reference_set.erase(key);
        } else { // Find
            // std::cout << "Op: Find " << key << std::endl;
            std::string* found_val = tree.find(key);
            bool found_in_set = reference_set.count(key);
            ASSERT_EQ(found_val != nullptr, found_in_set);
            if (found_in_set) {
                ASSERT_EQ(*found_val, "val_" + std::to_string(key));
            }
        }

        // tree.printTree(); // For debugging
        // std::cout << "---" << std::endl;

        checkRBTProperties(tree);
        std::vector<int> sorted_keys(reference_set.begin(), reference_set.end());
        verifyInorder(tree, sorted_keys);
    }
    // Final check after all operations
    checkRBTProperties(tree);
    std::vector<int> final_keys(reference_set.begin(), reference_set.end());
    verifyInorder(tree, final_keys);
}

TEST_F(RedBlackTreeTest, ContainsMethod) {
    ASSERT_FALSE(tree.contains(10));
    tree.insert(10, "ten");
    ASSERT_TRUE(tree.contains(10));
    tree.insert(5, "five");
    ASSERT_TRUE(tree.contains(5));
    ASSERT_FALSE(tree.contains(15));
    tree.remove(10);
    ASSERT_FALSE(tree.contains(10));
    ASSERT_TRUE(tree.contains(5));
    checkRBTProperties(tree);
}


// Test specific rotation and fixup scenarios if possible, though this is hard
// without exposing internal node structure more or having very specific sequences.
// The property checks after each operation are the main safeguard.

// Example of a sequence that might trigger specific rotations:
TEST_F(RedBlackTreeTest, InsertTriggeringRotations) {
    // Sequence: 10, 20 (Left Rotate on 10's parent if 10 was root's left child, or on root)
    // Then 30 (another left rotate potentially)
    tree.insert(10, "ten");         // Root (B)
    checkRBTProperties(tree);
    tree.insert(20, "twenty");     // 10(B) -> 20(R)
    checkRBTProperties(tree);
    tree.insert(30, "thirty");     // Causes imbalance: 10(B) -> 20(R) -> 30(R)
                                   // Should become: 20(B) -> 10(R), 30(R) after rotations
    ASSERT_NE(tree.find(10), nullptr);
    ASSERT_NE(tree.find(20), nullptr);
    ASSERT_NE(tree.find(30), nullptr);
    checkRBTProperties(tree);
    verifyInorder(tree, {10, 20, 30});
    // Root should be 20 and black
    ASSERT_EQ(tree.getRoot()->key, 20);
    ASSERT_EQ(tree.getRoot()->color, Color::BLACK);
    ASSERT_EQ(tree.getRoot()->left->key, 10);
    ASSERT_EQ(tree.getRoot()->left->color, Color::RED);
    ASSERT_EQ(tree.getRoot()->right->key, 30);
    ASSERT_EQ(tree.getRoot()->right->color, Color::RED);

    // Insert 5 (Right rotate on 10)
    // 20(B) -> 10(R) -> 5(R)
    // Becomes 20(B) -> 5(B) -> 10(R)  (after color flips and rotations)
    // Actually: 20(B), L:10(B), R:30(B). Insert 5 -> 10(B) has L:5(R)
    tree.insert(5, "five");
    checkRBTProperties(tree);
    verifyInorder(tree, {5, 10, 20, 30});
    ASSERT_EQ(tree.getRoot()->key, 20);
    ASSERT_EQ(tree.getRoot()->left->key, 10); // Or 5 depending on exact rotations and recoloring
                                             // The key is that properties hold

    // Insert 25
    tree.insert(25, "twenty-five");
    checkRBTProperties(tree);
    verifyInorder(tree, {5, 10, 20, 25, 30});
}

TEST_F(RedBlackTreeTest, DeleteTriggeringFixups) {
    // Build a moderately complex tree
    std::vector<int> initial_keys = {10, 5, 20, 3, 7, 15, 25, 12, 17};
    for (int key : initial_keys) {
        tree.insert(key, "val_" + std::to_string(key));
    }
    checkRBTProperties(tree);
    std::sort(initial_keys.begin(), initial_keys.end());
    verifyInorder(tree, initial_keys);

    // Delete a node that is black and has a red sibling (simple case for fixDelete)
    // Example: Delete 3 (leaf, black after some insertions). Its sibling 7 might be red or its parent's other child path is complex.
    // This is tricky to ensure without stepping through debugger or printing tree state.
    // The randomized test is more robust for broader coverage.

    // Delete 3 (likely a black leaf or red leaf becoming black)
    tree.remove(3);
    initial_keys.erase(std::remove(initial_keys.begin(), initial_keys.end(), 3), initial_keys.end());
    checkRBTProperties(tree);
    verifyInorder(tree, initial_keys);

    // Delete 12 (might be black, and its removal might cause issues)
    tree.remove(12);
    initial_keys.erase(std::remove(initial_keys.begin(), initial_keys.end(), 12), initial_keys.end());
    checkRBTProperties(tree);
    verifyInorder(tree, initial_keys);

    // Delete root (10)
    tree.remove(10);
    initial_keys.erase(std::remove(initial_keys.begin(), initial_keys.end(), 10), initial_keys.end());
    checkRBTProperties(tree);
    verifyInorder(tree, initial_keys);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

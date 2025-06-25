#include "gtest/gtest.h"
#include "treap.h" // Adjust path as necessary
#include <string>
#include <vector>
#include <algorithm> // For std::is_sorted, std::shuffle
#include <map>       // For comparison
#include <random>    // For std::default_random_engine for shuffle

// Test fixture for Treap tests
class TreapTest : public ::testing::Test {
protected:
    Treap<int, std::string> treap_int_string;
    Treap<std::string, int> treap_string_int;
};

// Test default constructor and initial state
TEST_F(TreapTest, Initialization) {
    EXPECT_EQ(treap_int_string.size(), 0);
    EXPECT_TRUE(treap_int_string.empty());
    EXPECT_EQ(treap_int_string.begin(), treap_int_string.end());

    EXPECT_EQ(treap_string_int.size(), 0);
    EXPECT_TRUE(treap_string_int.empty());
    EXPECT_EQ(treap_string_int.begin(), treap_string_int.end());
}

// Test insertion of new elements
TEST_F(TreapTest, InsertNewElements) {
    auto result1 = treap_int_string.insert(10, "Apple");
    EXPECT_TRUE(result1.second); // New element
    EXPECT_EQ(result1.first->first, 10);
    EXPECT_EQ(result1.first->second, "Apple");
    EXPECT_EQ(treap_int_string.size(), 1);
    EXPECT_FALSE(treap_int_string.empty());

    auto result2 = treap_int_string.insert(5, "Banana");
    EXPECT_TRUE(result2.second);
    EXPECT_EQ(result2.first->first, 5);
    EXPECT_EQ(treap_int_string.size(), 2);

    auto result3 = treap_int_string.insert(15, "Cherry");
    EXPECT_TRUE(result3.second);
    EXPECT_EQ(result3.first->first, 15);
    EXPECT_EQ(treap_int_string.size(), 3);

    EXPECT_TRUE(treap_int_string.contains(10));
    EXPECT_TRUE(treap_int_string.contains(5));
    EXPECT_TRUE(treap_int_string.contains(15));
}

// Test insertion of duplicate keys (should update value)
TEST_F(TreapTest, InsertDuplicateKeys) {
    treap_int_string.insert(10, "Apple");
    EXPECT_EQ(treap_int_string.size(), 1);
    EXPECT_EQ(*treap_int_string.find(10), "Apple");

    auto result = treap_int_string.insert(10, "Apricot");
    EXPECT_FALSE(result.second); // Not a new element, updated
    EXPECT_EQ(result.first->first, 10);
    EXPECT_EQ(result.first->second, "Apricot");
    EXPECT_EQ(treap_int_string.size(), 1); // Size should not change
    EXPECT_EQ(*treap_int_string.find(10), "Apricot");
}

// Test operator[] for insertion and access
TEST_F(TreapTest, OperatorSquareBrackets) {
    treap_int_string[10] = "Apple";
    EXPECT_EQ(treap_int_string.size(), 1);
    EXPECT_EQ(*treap_int_string.find(10), "Apple");

    treap_int_string[5] = "Banana";
    EXPECT_EQ(treap_int_string.size(), 2);
    EXPECT_EQ(*treap_int_string.find(5), "Banana");

    // Access existing
    EXPECT_EQ(treap_int_string[10], "Apple");

    // Update existing
    treap_int_string[10] = "Apricot";
    EXPECT_EQ(treap_int_string.size(), 2); // Size should not change
    EXPECT_EQ(*treap_int_string.find(10), "Apricot");
    EXPECT_EQ(treap_int_string[10], "Apricot");

    // Access non-existing (should insert default value for string - empty string)
    EXPECT_EQ(treap_int_string[20], "");
    EXPECT_EQ(treap_int_string.size(), 3);
    EXPECT_TRUE(treap_int_string.contains(20));
    EXPECT_EQ(*treap_int_string.find(20), "");
}

// Test operator[] with rvalue key (std::string)
TEST_F(TreapTest, OperatorSquareBracketsRvalueKey) {
    treap_string_int[std::string("Hello")] = 1;
    EXPECT_EQ(treap_string_int.size(), 1);
    EXPECT_TRUE(treap_string_int.contains("Hello"));
    EXPECT_EQ(treap_string_int["Hello"], 1);

    std::string key = "World";
    treap_string_int[std::move(key)] = 2; // key is now in unspecified state
    EXPECT_EQ(treap_string_int.size(), 2);
    EXPECT_TRUE(treap_string_int.contains("World"));
    EXPECT_EQ(treap_string_int["World"], 2);

    // Update using rvalue key
    treap_string_int[std::string("Hello")] = 100;
    EXPECT_EQ(treap_string_int.size(), 2);
    EXPECT_EQ(treap_string_int["Hello"], 100);
}


// Test finding elements
TEST_F(TreapTest, FindElements) {
    treap_int_string.insert(10, "Apple");
    treap_int_string.insert(5, "Banana");

    // Find existing
    std::string* val_ptr = treap_int_string.find(10);
    ASSERT_NE(val_ptr, nullptr);
    EXPECT_EQ(*val_ptr, "Apple");

    const Treap<int, std::string>& const_treap = treap_int_string;
    const std::string* c_val_ptr = const_treap.find(5);
    ASSERT_NE(c_val_ptr, nullptr);
    EXPECT_EQ(*c_val_ptr, "Banana");

    // Find non-existing
    EXPECT_EQ(treap_int_string.find(99), nullptr);
    EXPECT_EQ(const_treap.find(999), nullptr);

    // Test contains
    EXPECT_TRUE(treap_int_string.contains(10));
    EXPECT_TRUE(const_treap.contains(5));
    EXPECT_FALSE(treap_int_string.contains(99));
    EXPECT_FALSE(const_treap.contains(999));
}

// Test deletion of elements
TEST_F(TreapTest, EraseElements) {
    treap_int_string.insert(10, "Apple");
    treap_int_string.insert(5, "Banana");
    treap_int_string.insert(15, "Cherry");
    treap_int_string.insert(3, "Date");
    treap_int_string.insert(7, "Elderberry");
    EXPECT_EQ(treap_int_string.size(), 5);

    // Erase a leaf node (3)
    EXPECT_TRUE(treap_int_string.erase(3));
    EXPECT_EQ(treap_int_string.size(), 4);
    EXPECT_FALSE(treap_int_string.contains(3));

    // Erase a node with one child (e.g. 7 after 3 is gone, or 15)
    // Let's erase 7 (if it became a leaf or has one child after 3's deletion)
    // Or erase 15 (likely a leaf or one child)
    EXPECT_TRUE(treap_int_string.erase(15)); // 15 is likely a leaf or has one child
    EXPECT_EQ(treap_int_string.size(), 3);
    EXPECT_FALSE(treap_int_string.contains(15));

    // Erase a node with two children (e.g. 5 or 10)
    // Let's erase 5 (original children 3, 7. After 3 deleted, 7 is child. After 7 deleted, 5 is leaf)
    // The structure changes, so let's erase a known internal node.
    // Current: 10, 5, 7. If 10 is root, 5 left, 7 right of 5 or 10.
    // Re-insert to make structure more predictable for this test point if needed, or just test erase.
    // Erase 5.
    EXPECT_TRUE(treap_int_string.erase(5));
    EXPECT_EQ(treap_int_string.size(), 2);
    EXPECT_FALSE(treap_int_string.contains(5));

    // Erase remaining elements
    EXPECT_TRUE(treap_int_string.erase(10));
    EXPECT_EQ(treap_int_string.size(), 1);
    EXPECT_FALSE(treap_int_string.contains(10));

    EXPECT_TRUE(treap_int_string.erase(7)); // This was inserted as "Elderberry"
    EXPECT_EQ(treap_int_string.size(), 0);
    EXPECT_FALSE(treap_int_string.contains(7));
    EXPECT_TRUE(treap_int_string.empty());

    // Erase from empty treap
    EXPECT_FALSE(treap_int_string.erase(100));
    EXPECT_EQ(treap_int_string.size(), 0);

    // Erase non-existing element
    treap_int_string.insert(1, "One");
    EXPECT_FALSE(treap_int_string.erase(100));
    EXPECT_EQ(treap_int_string.size(), 1);
}

// Test iteration
TEST_F(TreapTest, Iteration) {
    treap_int_string.insert(10, "J"); // J for 10
    treap_int_string.insert(5,  "E"); // E for 5
    treap_int_string.insert(15, "O"); // O for 15
    treap_int_string.insert(3,  "C"); // C for 3
    treap_int_string.insert(7,  "G"); // G for 7
    treap_int_string.insert(12, "L"); // L for 12
    treap_int_string.insert(17, "Q"); // Q for 17

    std::vector<int> expected_keys = {3, 5, 7, 10, 12, 15, 17};
    std::vector<std::string> expected_values = {"C", "E", "G", "J", "L", "O", "Q"};

    std::vector<int> actual_keys;
    std::vector<std::string> actual_values;
    for (const auto& pair : treap_int_string) {
        actual_keys.push_back(pair.first);
        actual_values.push_back(pair.second);
    }

    EXPECT_EQ(actual_keys, expected_keys);
    EXPECT_EQ(actual_values, expected_values);
    EXPECT_TRUE(std::is_sorted(actual_keys.begin(), actual_keys.end()));

    // Test const iteration
    const Treap<int, std::string>& const_treap = treap_int_string;
    actual_keys.clear();
    actual_values.clear();
    for (const auto& pair : const_treap) {
        actual_keys.push_back(pair.first);
        actual_values.push_back(pair.second);
    }
    EXPECT_EQ(actual_keys, expected_keys);
    EXPECT_EQ(actual_values, expected_values);

    // Test cbegin/cend
    actual_keys.clear();
    actual_values.clear();
    for (auto it = const_treap.cbegin(); it != const_treap.cend(); ++it) {
        actual_keys.push_back(it->first);
        actual_values.push_back(it->second);
    }
    EXPECT_EQ(actual_keys, expected_keys);
    EXPECT_EQ(actual_values, expected_values);
}

// Test empty treap iteration
TEST_F(TreapTest, EmptyIteration) {
    int count = 0;
    for (const auto& pair : treap_int_string) {
        count++;
        (void)pair; // Suppress unused variable warning
    }
    EXPECT_EQ(count, 0);

    const Treap<int, std::string>& const_treap = treap_int_string;
    count = 0;
    for (const auto& pair : const_treap) {
        count++;
        (void)pair; // Suppress unused variable warning
    }
    EXPECT_EQ(count, 0);
    EXPECT_EQ(const_treap.begin(), const_treap.end());
    EXPECT_EQ(const_treap.cbegin(), const_treap.cend());
}


// Test clear operation
TEST_F(TreapTest, Clear) {
    treap_int_string.insert(10, "Apple");
    treap_int_string.insert(5, "Banana");
    EXPECT_EQ(treap_int_string.size(), 2);
    EXPECT_FALSE(treap_int_string.empty());

    treap_int_string.clear();
    EXPECT_EQ(treap_int_string.size(), 0);
    EXPECT_TRUE(treap_int_string.empty());
    EXPECT_FALSE(treap_int_string.contains(10));
    EXPECT_FALSE(treap_int_string.contains(5));
    EXPECT_EQ(treap_int_string.begin(), treap_int_string.end());
}

// Test with string keys
TEST_F(TreapTest, StringKeys) {
    treap_string_int.insert("David", 30);
    treap_string_int.insert("Alice", 25);
    treap_string_int.insert("Charlie", 35);
    treap_string_int.insert("Bob", 28);
    EXPECT_EQ(treap_string_int.size(), 4);

    std::vector<std::string> expected_keys = {"Alice", "Bob", "Charlie", "David"};
    std::vector<int> expected_values = {25, 28, 35, 30};

    std::vector<std::string> actual_keys;
    std::vector<int> actual_values;
    for (const auto& pair : treap_string_int) {
        actual_keys.push_back(pair.first);
        actual_values.push_back(pair.second);
    }
    EXPECT_EQ(actual_keys, expected_keys);
    EXPECT_EQ(actual_values, expected_values);

    EXPECT_EQ(*treap_string_int.find("Alice"), 25);
    treap_string_int["Alice"] = 26; // Update
    EXPECT_EQ(*treap_string_int.find("Alice"), 26);
    EXPECT_EQ(treap_string_int.size(), 4);
}

// Test move constructor
TEST_F(TreapTest, MoveConstructor) {
    treap_int_string.insert(10, "Ten");
    treap_int_string.insert(5, "Five");
    treap_int_string[20] = "Twenty";

    Treap<int, std::string> moved_treap(std::move(treap_int_string));

    EXPECT_EQ(treap_int_string.size(), 0); // Original should be empty
    EXPECT_TRUE(treap_int_string.empty());
    EXPECT_EQ(treap_int_string.begin(), treap_int_string.end());


    EXPECT_EQ(moved_treap.size(), 3);
    EXPECT_TRUE(moved_treap.contains(5));
    EXPECT_TRUE(moved_treap.contains(10));
    EXPECT_TRUE(moved_treap.contains(20));
    EXPECT_EQ(*moved_treap.find(5), "Five");
    EXPECT_EQ(moved_treap[10], "Ten"); // Test operator[] access on moved treap

    std::vector<int> keys;
    for(const auto& p : moved_treap) {
        keys.push_back(p.first);
    }
    std::vector<int> expected_keys = {5, 10, 20};
    EXPECT_EQ(keys, expected_keys);
}

// Test move assignment operator
TEST_F(TreapTest, MoveAssignment) {
    treap_int_string.insert(10, "Ten");
    treap_int_string.insert(5, "Five");

    Treap<int, std::string> another_treap;
    another_treap.insert(100, "Hundred");
    another_treap.insert(50, "Fifty");
    another_treap.insert(150, "OneFifty");

    another_treap = std::move(treap_int_string);

    EXPECT_EQ(treap_int_string.size(), 0);
    EXPECT_TRUE(treap_int_string.empty());

    EXPECT_EQ(another_treap.size(), 2);
    EXPECT_TRUE(another_treap.contains(5));
    EXPECT_TRUE(another_treap.contains(10));
    EXPECT_FALSE(another_treap.contains(100)); // Should not have old elements
    EXPECT_EQ(*another_treap.find(10), "Ten");

    std::vector<int> keys;
    for(const auto& p : another_treap) {
        keys.push_back(p.first);
    }
    std::vector<int> expected_keys = {5, 10};
    EXPECT_EQ(keys, expected_keys);
}

// Test large number of insertions and deletions to check for stability and order
TEST_F(TreapTest, StressTest) {
    const int num_elements = 1000;
    std::vector<int> keys(num_elements);
    for (int i = 0; i < num_elements; ++i) {
        keys[i] = i;
    }

    auto rng = std::default_random_engine {};
    std::shuffle(std::begin(keys), std::end(keys), rng); // Shuffle keys for random insertion order

    // Insert elements
    for (int key : keys) {
        treap_int_string.insert(key, "value_" + std::to_string(key));
    }
    EXPECT_EQ(treap_int_string.size(), num_elements);

    // Check if all elements are present and sorted
    std::vector<int> current_keys;
    for (const auto& pair : treap_int_string) {
        current_keys.push_back(pair.first);
        EXPECT_EQ(pair.second, "value_" + std::to_string(pair.first));
    }
    std::sort(keys.begin(), keys.end()); // Sort original keys for comparison
    EXPECT_EQ(current_keys, keys);
    EXPECT_TRUE(std::is_sorted(current_keys.begin(), current_keys.end()));

    // Delete elements in a different shuffled order
    std::shuffle(std::begin(keys), std::end(keys), rng);
    for (int i = 0; i < num_elements / 2; ++i) {
        EXPECT_TRUE(treap_int_string.erase(keys[i]));
    }
    EXPECT_EQ(treap_int_string.size(), num_elements - (num_elements / 2));

    // Delete remaining elements
    for (int i = num_elements / 2; i < num_elements; ++i) {
        EXPECT_TRUE(treap_int_string.erase(keys[i]));
    }
    EXPECT_EQ(treap_int_string.size(), 0);
    EXPECT_TRUE(treap_int_string.empty());
}

// Test insert rvalue key/value
TEST_F(TreapTest, InsertRvalue) {
    Treap<std::string, std::string> treap;
    std::string key1 = "key1";
    std::string val1 = "val1";

    auto res1 = treap.insert(std::move(key1), std::move(val1));
    EXPECT_TRUE(res1.second);
    EXPECT_EQ(res1.first->first, "key1");
    EXPECT_EQ(res1.first->second, "val1");
    EXPECT_EQ(treap.size(), 1);
    // key1 and val1 are in valid but unspecified state.

    std::string key2 = "key2";
    // Test insert rvalue key, lvalue value (if supported by Treap::insert, current one requires Value&&)
    // Current Treap::insert(Key&&, Value&&) expects Value&&.
    // Let's make val2 rvalue too.
    std::string val2_orig = "val2_orig";
    auto res2 = treap.insert(std::move(key2), std::string("val2")); // string("val2") is rvalue
    EXPECT_TRUE(res2.second);
    EXPECT_EQ(res2.first->first, "key2");
    EXPECT_EQ(res2.first->second, "val2");
    EXPECT_EQ(treap.size(), 2);

    // Test insert literal key (rvalue), literal value (rvalue)
    auto res3 = treap.insert("key3", "val3");
    EXPECT_TRUE(res3.second);
    EXPECT_EQ(res3.first->first, "key3");
    EXPECT_EQ(res3.first->second, "val3");
    EXPECT_EQ(treap.size(), 3);

    // Check all are present
    EXPECT_TRUE(treap.contains("key1"));
    EXPECT_TRUE(treap.contains("key2"));
    EXPECT_TRUE(treap.contains("key3"));
    EXPECT_EQ(*treap.find("key1"), "val1");
    EXPECT_EQ(*treap.find("key2"), "val2");
    EXPECT_EQ(*treap.find("key3"), "val3");
}

// Test iterator behavior, especially after modifications
TEST_F(TreapTest, IteratorValidityAndBehavior) {
    treap_int_string.insert(10, "A");
    treap_int_string.insert(20, "B");
    treap_int_string.insert(5, "C");

    // Get an iterator
    auto it = treap_int_string.begin(); // Should point to {5, "C"}
    ASSERT_NE(it, treap_int_string.end());
    EXPECT_EQ(it->first, 5);
    EXPECT_EQ(it->second, "C");

    ++it; // Should point to {10, "A"}
    ASSERT_NE(it, treap_int_string.end());
    EXPECT_EQ(it->first, 10);
    EXPECT_EQ(it->second, "A");

    // Test post-increment
    auto it_prev = it++; // it_prev = {10, "A"}, it = {20, "B"}
    ASSERT_NE(it_prev, treap_int_string.end());
    EXPECT_EQ(it_prev->first, 10);
    ASSERT_NE(it, treap_int_string.end());
    EXPECT_EQ(it->first, 20);
    EXPECT_EQ(it->second, "B");

    ++it; // Should point to end()
    EXPECT_EQ(it, treap_int_string.end());

    // Test comparison
    auto it1 = treap_int_string.begin();
    auto it2 = treap_int_string.begin();
    EXPECT_EQ(it1, it2);
    ++it2;
    EXPECT_NE(it1, it2);
}

// Test edge case: Treap with one element
TEST_F(TreapTest, SingleElementTreap) {
    treap_int_string.insert(100, "Solo");
    EXPECT_EQ(treap_int_string.size(), 1);
    EXPECT_FALSE(treap_int_string.empty());
    EXPECT_TRUE(treap_int_string.contains(100));
    EXPECT_EQ(*treap_int_string.find(100), "Solo");

    auto it = treap_int_string.begin();
    ASSERT_NE(it, treap_int_string.end());
    EXPECT_EQ(it->first, 100);
    ++it;
    EXPECT_EQ(it, treap_int_string.end());

    EXPECT_TRUE(treap_int_string.erase(100));
    EXPECT_TRUE(treap_int_string.empty());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

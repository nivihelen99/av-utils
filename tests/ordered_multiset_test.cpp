#include "gtest/gtest.h"
#include "ordered_multiset.h" // Adjust path if necessary
#include <string>
#include <vector>
#include <algorithm> // For std::equal, std::is_permutation
#include <iterator> // For std::distance

// Helper to convert OrderedMultiset to vector for easier comparison of order
template <typename T, typename Hash, typename KeyEqual>
std::vector<T> to_vector(const cpp_utils::OrderedMultiset<T, Hash, KeyEqual>& oms) {
    std::vector<T> vec;
    for (const auto& item : oms) {
        vec.push_back(item);
    }
    return vec;
}

// Test fixture for OrderedMultiset
class OrderedMultisetTest : public ::testing::Test {
protected:
    cpp_utils::OrderedMultiset<int> oms_int;
    cpp_utils::OrderedMultiset<std::string> oms_str;
};

// Test Default Constructor
TEST_F(OrderedMultisetTest, DefaultConstructor) {
    EXPECT_TRUE(oms_int.empty());
    EXPECT_EQ(0, oms_int.size());
    EXPECT_EQ(oms_int.begin(), oms_int.end());
}

// Test Initializer List Constructor
TEST_F(OrderedMultisetTest, InitializerListConstructor) {
    cpp_utils::OrderedMultiset<int> oms = {1, 2, 2, 3, 1};
    EXPECT_FALSE(oms.empty());
    EXPECT_EQ(5, oms.size());
    EXPECT_EQ(2, oms.count(1));
    EXPECT_EQ(2, oms.count(2));
    EXPECT_EQ(1, oms.count(3));
    EXPECT_EQ(0, oms.count(4));
    std::vector<int> expected_order = {1, 2, 2, 3, 1};
    EXPECT_EQ(expected_order, to_vector(oms));
}

// Test Insert Lvalue
TEST_F(OrderedMultisetTest, InsertLvalue) {
    oms_int.insert(10);
    oms_int.insert(20);
    oms_int.insert(10);

    EXPECT_EQ(3, oms_int.size());
    EXPECT_EQ(2, oms_int.count(10));
    EXPECT_EQ(1, oms_int.count(20));
    std::vector<int> expected_order = {10, 20, 10};
    EXPECT_EQ(expected_order, to_vector(oms_int));

    auto result = oms_int.insert(30);
    EXPECT_NE(oms_int.end(), result.first);
    EXPECT_EQ(30, *result.first);
    EXPECT_TRUE(result.second); // always true for multiset
    EXPECT_EQ(4, oms_int.size());
    EXPECT_EQ(1, oms_int.count(30));
}

// Test Insert Rvalue
TEST_F(OrderedMultisetTest, InsertRvalue) {
    oms_str.insert("apple");
    oms_str.insert(std::string("banana"));
    oms_str.insert("apple");

    EXPECT_EQ(3, oms_str.size());
    EXPECT_EQ(2, oms_str.count("apple"));
    EXPECT_EQ(1, oms_str.count("banana"));
    std::vector<std::string> expected_order = {"apple", "banana", "apple"};
    EXPECT_EQ(expected_order, to_vector(oms_str));
}

// Test Erase Single Instance
TEST_F(OrderedMultisetTest, EraseSingleInstance) {
    oms_int.insert(1); oms_int.insert(2); oms_int.insert(1); oms_int.insert(3); oms_int.insert(1);
    // Order: 1, 2, 1, 3, 1

    EXPECT_EQ(3, oms_int.count(1));
    EXPECT_EQ(1, oms_int.erase(1)); // Removes one '1' (the last inserted '1')
    EXPECT_EQ(2, oms_int.count(1));
    EXPECT_EQ(4, oms_int.size());
    std::vector<int> expected_order1 = {1, 2, 1, 3}; // The last '1' was at the end of its group
    EXPECT_EQ(expected_order1, to_vector(oms_int));

    EXPECT_EQ(1, oms_int.erase(1));
    EXPECT_EQ(1, oms_int.count(1));
    EXPECT_EQ(3, oms_int.size());
    std::vector<int> expected_order2 = {1, 2, 3};
    EXPECT_EQ(expected_order2, to_vector(oms_int));

    EXPECT_EQ(1, oms_int.erase(2));
    EXPECT_EQ(0, oms_int.count(2));
    EXPECT_EQ(2, oms_int.size());
    std::vector<int> expected_order3 = {1, 3};
    EXPECT_EQ(expected_order3, to_vector(oms_int));

    EXPECT_EQ(0, oms_int.erase(4)); // Non-existent
    EXPECT_EQ(2, oms_int.size());
}

// Test Erase All Instances
TEST_F(OrderedMultisetTest, EraseAllInstances) {
    oms_int.insert(1); oms_int.insert(2); oms_int.insert(1); oms_int.insert(3); oms_int.insert(1);
    // Order: 1, 2, 1, 3, 1

    EXPECT_EQ(3, oms_int.count(1));
    EXPECT_EQ(3, oms_int.erase_all(1));
    EXPECT_EQ(0, oms_int.count(1));
    EXPECT_EQ(2, oms_int.size());
    std::vector<int> expected_order = {2, 3};
    EXPECT_EQ(expected_order, to_vector(oms_int));

    EXPECT_EQ(0, oms_int.erase_all(1)); // Already removed
    EXPECT_EQ(0, oms_int.erase_all(4)); // Non-existent
    EXPECT_EQ(2, oms_int.size());

    oms_int.insert(2); // Order: 2, 3, 2
    EXPECT_EQ(2, oms_int.erase_all(2));
    EXPECT_EQ(0, oms_int.count(2));
    EXPECT_EQ(1, oms_int.size());
    std::vector<int> expected_order2 = {3};
    EXPECT_EQ(expected_order2, to_vector(oms_int));
}

// Test Count and Contains
TEST_F(OrderedMultisetTest, CountAndContains) {
    oms_int.insert(10); oms_int.insert(20); oms_int.insert(10);
    EXPECT_EQ(2, oms_int.count(10));
    EXPECT_TRUE(oms_int.contains(10));
    EXPECT_EQ(1, oms_int.count(20));
    EXPECT_TRUE(oms_int.contains(20));
    EXPECT_EQ(0, oms_int.count(30));
    EXPECT_FALSE(oms_int.contains(30));
}

// Test Size, Empty, Clear
TEST_F(OrderedMultisetTest, SizeEmptyClear) {
    EXPECT_TRUE(oms_int.empty());
    EXPECT_EQ(0, oms_int.size());

    oms_int.insert(1);
    EXPECT_FALSE(oms_int.empty());
    EXPECT_EQ(1, oms_int.size());

    oms_int.insert(1);
    EXPECT_FALSE(oms_int.empty());
    EXPECT_EQ(2, oms_int.size());

    oms_int.clear();
    EXPECT_TRUE(oms_int.empty());
    EXPECT_EQ(0, oms_int.size());
    EXPECT_EQ(0, oms_int.count(1));
}

// Test Iteration (Forward and Reverse)
TEST_F(OrderedMultisetTest, Iteration) {
    cpp_utils::OrderedMultiset<int> oms = {1, 2, 2, 3, 1, 4};
    std::vector<int> expected_fwd = {1, 2, 2, 3, 1, 4};
    std::vector<int> actual_fwd;
    for (const auto& item : oms) {
        actual_fwd.push_back(item);
    }
    EXPECT_EQ(expected_fwd, actual_fwd);

    // Const iteration
    const auto& const_oms = oms;
    actual_fwd.clear();
    for (const auto& item : const_oms) {
        actual_fwd.push_back(item);
    }
    EXPECT_EQ(expected_fwd, actual_fwd);

    std::vector<int> expected_rev = {4, 1, 3, 2, 2, 1};
    std::vector<int> actual_rev;
    for (auto it = oms.rbegin(); it != oms.rend(); ++it) {
        actual_rev.push_back(*it);
    }
    EXPECT_EQ(expected_rev, actual_rev);

    // Const reverse iteration
    actual_rev.clear();
    for (auto it = const_oms.rbegin(); it != const_oms.rend(); ++it) {
        actual_rev.push_back(*it);
    }
    EXPECT_EQ(expected_rev, actual_rev);
}

// Test Copy Constructor
TEST_F(OrderedMultisetTest, CopyConstructor) {
    oms_int.insert(10); oms_int.insert(20); oms_int.insert(10);
    cpp_utils::OrderedMultiset<int> oms_copy(oms_int);

    EXPECT_EQ(oms_int.size(), oms_copy.size());
    EXPECT_EQ(to_vector(oms_int), to_vector(oms_copy));
    EXPECT_EQ(oms_int.count(10), oms_copy.count(10));
    EXPECT_EQ(oms_int.count(20), oms_copy.count(20));

    // Ensure deep copy by modifying original
    oms_int.insert(30);
    EXPECT_NE(oms_int.size(), oms_copy.size());
    EXPECT_FALSE(oms_copy.contains(30));
    oms_copy.erase(10);
    EXPECT_NE(oms_int.count(10), oms_copy.count(10));
}

// Test Copy Assignment Operator
TEST_F(OrderedMultisetTest, CopyAssignmentOperator) {
    oms_int.insert(10); oms_int.insert(20); oms_int.insert(10);
    cpp_utils::OrderedMultiset<int> oms_copy;
    oms_copy.insert(100); // Add some initial data
    oms_copy = oms_int;

    EXPECT_EQ(oms_int.size(), oms_copy.size());
    EXPECT_EQ(to_vector(oms_int), to_vector(oms_copy));
    EXPECT_EQ(oms_int.count(10), oms_copy.count(10));

    // Ensure deep copy
    oms_int.insert(30);
    EXPECT_NE(oms_int.size(), oms_copy.size());
    EXPECT_FALSE(oms_copy.contains(30));

    // Self-assignment
    oms_copy = oms_copy;
    EXPECT_EQ(2, oms_copy.count(10)); // Should remain unchanged
}

// Test Move Constructor
TEST_F(OrderedMultisetTest, MoveConstructor) {
    oms_int.insert(1); oms_int.insert(2); oms_int.insert(1);
    std::vector<int> expected_order = {1, 2, 1};
    size_t original_size = oms_int.size();

    cpp_utils::OrderedMultiset<int> oms_moved(std::move(oms_int));

    EXPECT_EQ(original_size, oms_moved.size());
    EXPECT_EQ(expected_order, to_vector(oms_moved));
    EXPECT_EQ(2, oms_moved.count(1));
    EXPECT_TRUE(oms_int.empty() || oms_int.size() == 0); // Original is valid but empty or small
}

// Test Move Assignment Operator
TEST_F(OrderedMultisetTest, MoveAssignmentOperator) {
    oms_int.insert(1); oms_int.insert(2); oms_int.insert(1);
    std::vector<int> expected_order = {1, 2, 1};
    size_t original_size = oms_int.size();

    cpp_utils::OrderedMultiset<int> oms_moved;
    oms_moved.insert(100); // Initial data
    oms_moved = std::move(oms_int);

    EXPECT_EQ(original_size, oms_moved.size());
    EXPECT_EQ(expected_order, to_vector(oms_moved));
    EXPECT_EQ(2, oms_moved.count(1));
    EXPECT_TRUE(oms_int.empty() || oms_int.size() == 0);

    // Self-assignment (should be safe, though not typical for moved objects)
    // cpp_utils::OrderedMultiset<int>& ref_moved = oms_moved;
    // oms_moved = std::move(ref_moved);
    // This is tricky and depends on exact guarantees. Standard library containers are safe.
    // Assuming our implementation is safe (clears then moves).
    // EXPECT_EQ(original_size, oms_moved.size());
}

// Test Swap (Member and Non-Member)
TEST_F(OrderedMultisetTest, Swap) {
    oms_int.insert(1); oms_int.insert(1);
    cpp_utils::OrderedMultiset<int> oms_other;
    oms_other.insert(2); oms_other.insert(3); oms_other.insert(3);

    std::vector<int> oms_int_expected_before = {1, 1};
    std::vector<int> oms_other_expected_before = {2, 3, 3};
    EXPECT_EQ(oms_int_expected_before, to_vector(oms_int));
    EXPECT_EQ(oms_other_expected_before, to_vector(oms_other));

    oms_int.swap(oms_other); // Member swap

    EXPECT_EQ(oms_other_expected_before, to_vector(oms_int)); // oms_int now has other's elements
    EXPECT_EQ(oms_int_expected_before, to_vector(oms_other)); // oms_other now has int's elements

    // Non-member swap
    swap(oms_int, oms_other); // Should swap them back

    EXPECT_EQ(oms_int_expected_before, to_vector(oms_int));
    EXPECT_EQ(oms_other_expected_before, to_vector(oms_other));
}

// Test Comparison Operators (== and !=)
TEST_F(OrderedMultisetTest, ComparisonOperators) {
    cpp_utils::OrderedMultiset<int> oms1 = {1, 2, 1};
    cpp_utils::OrderedMultiset<int> oms2 = {1, 2, 1};
    cpp_utils::OrderedMultiset<int> oms3 = {1, 1, 2}; // Different order
    cpp_utils::OrderedMultiset<int> oms4 = {1, 2, 1, 3}; // Different size/content
    cpp_utils::OrderedMultiset<int> oms_empty1;
    cpp_utils::OrderedMultiset<int> oms_empty2;

    EXPECT_TRUE(oms1 == oms2);
    EXPECT_FALSE(oms1 != oms2);

    EXPECT_FALSE(oms1 == oms3);
    EXPECT_TRUE(oms1 != oms3);

    EXPECT_FALSE(oms1 == oms4);
    EXPECT_TRUE(oms1 != oms4);

    EXPECT_FALSE(oms3 == oms4);
    EXPECT_TRUE(oms3 != oms4);

    EXPECT_TRUE(oms_empty1 == oms_empty2);
    EXPECT_FALSE(oms_empty1 != oms_empty2);

    EXPECT_FALSE(oms1 == oms_empty1);
    EXPECT_TRUE(oms1 != oms_empty1);
}

// Test with String Data
TEST_F(OrderedMultisetTest, StringData) {
    oms_str.insert("apple");
    oms_str.insert("banana");
    oms_str.insert("apple");
    oms_str.insert("orange");

    EXPECT_EQ(4, oms_str.size());
    EXPECT_EQ(2, oms_str.count("apple"));
    EXPECT_EQ(1, oms_str.count("banana"));
    EXPECT_EQ(1, oms_str.count("orange"));
    EXPECT_TRUE(oms_str.contains("apple"));
    EXPECT_FALSE(oms_str.contains("grape"));

    std::vector<std::string> expected_order = {"apple", "banana", "apple", "orange"};
    EXPECT_EQ(expected_order, to_vector(oms_str));

    oms_str.erase("apple"); // Erase one "apple"
    expected_order = {"apple", "banana", "orange"}; // The second "apple" was removed
    EXPECT_EQ(expected_order, to_vector(oms_str));
    EXPECT_EQ(1, oms_str.count("apple"));

    oms_str.erase_all("apple");
    expected_order = {"banana", "orange"};
    EXPECT_EQ(expected_order, to_vector(oms_str));
    EXPECT_EQ(0, oms_str.count("apple"));
}

// Test Edge Case: Empty Multiset Operations
TEST_F(OrderedMultisetTest, EmptyMultisetOperations) {
    EXPECT_EQ(0, oms_int.erase(1));
    EXPECT_EQ(0, oms_int.erase_all(1));
    EXPECT_EQ(0, oms_int.count(1));
    EXPECT_FALSE(oms_int.contains(1));
    EXPECT_TRUE(oms_int.begin() == oms_int.end());
    EXPECT_TRUE(oms_int.rbegin() == oms_int.rend());
}

// Test Edge Case: All Elements Identical
TEST_F(OrderedMultisetTest, AllElementsIdentical) {
    oms_int.insert(5);
    oms_int.insert(5);
    oms_int.insert(5);

    EXPECT_EQ(3, oms_int.size());
    EXPECT_EQ(3, oms_int.count(5));
    EXPECT_TRUE(oms_int.contains(5));
    std::vector<int> expected = {5, 5, 5};
    EXPECT_EQ(expected, to_vector(oms_int));

    oms_int.erase(5);
    EXPECT_EQ(2, oms_int.size());
    EXPECT_EQ(2, oms_int.count(5));
    expected = {5, 5};
    EXPECT_EQ(expected, to_vector(oms_int));

    oms_int.erase_all(5);
    EXPECT_TRUE(oms_int.empty());
    EXPECT_EQ(0, oms_int.count(5));
}

// Test interaction of erase and iterators (more complex)
TEST_F(OrderedMultisetTest, EraseAndIteratorsComplex) {
    cpp_utils::OrderedMultiset<int> oms = {10, 20, 10, 30, 10, 40, 10};
    // Order: 10, 20, 10, 30, 10, 40, 10
    // Iterators for 10 point to indices 0, 2, 4, 6 (conceptual)

    // Erase the specific instance of 10 that was inserted last
    // (which element_positions_ should give us)
    oms.erase(10);
    // Expected: 10, 20, 10, 30, 10, 40
    std::vector<int> expected1 = {10, 20, 10, 30, 10, 40};
    EXPECT_EQ(expected1, to_vector(oms));
    EXPECT_EQ(3, oms.count(10));

    // Erase another 10 (again, the one considered "last" among the remaining 10s by element_positions_)
    oms.erase(10);
    // Expected: 10, 20, 10, 30, 40
    std::vector<int> expected2 = {10, 20, 10, 30, 40};
    EXPECT_EQ(expected2, to_vector(oms));
    EXPECT_EQ(2, oms.count(10));

    // Erase 20
    oms.erase(20);
    // Expected: 10, 10, 30, 40
    std::vector<int> expected3 = {10, 10, 30, 40};
    EXPECT_EQ(expected3, to_vector(oms));
    EXPECT_EQ(2, oms.count(10));
    EXPECT_EQ(0, oms.count(20));

    // Erase all remaining 10s
    oms.erase_all(10);
    // Expected: 30, 40
    std::vector<int> expected4 = {30, 40};
    EXPECT_EQ(expected4, to_vector(oms));
    EXPECT_EQ(0, oms.count(10));
}

// Test that iterators in element_positions_ are correctly managed
// This is implicitly tested by erase and erase_all, but an explicit check can be good.
// It's hard to check element_positions_ directly without friend class or public access.
// Correct behavior of erase/erase_all with correct final order is strong evidence.
// This test mainly ensures that after multiple erasures of the same value,
// the internal state of element_positions_ remains consistent for subsequent operations.
TEST_F(OrderedMultisetTest, InternalIteratorManagementOnErase) {
    cpp_utils::OrderedMultiset<int> oms;
    oms.insert(1); // list: {1_a}, map: {1: [it_a]}
    oms.insert(2); // list: {1_a, 2_b}, map: {1: [it_a], 2: [it_b]}
    oms.insert(1); // list: {1_a, 2_b, 1_c}, map: {1: [it_a, it_c], 2: [it_b]}
    oms.insert(3); // list: {1_a, 2_b, 1_c, 3_d}, map: {1: [it_a, it_c], 2: [it_b], 3: [it_d]}
    oms.insert(1); // list: {1_a, 2_b, 1_c, 3_d, 1_e}, map: {1: [it_a, it_c, it_e], 2: [it_b], 3: [it_d]}

    std::vector<int> expected_initial = {1, 2, 1, 3, 1};
    EXPECT_EQ(expected_initial, to_vector(oms));
    EXPECT_EQ(3, oms.count(1));

    // Erase one '1'. The one pointed to by it_e (last in vector for '1') should be removed.
    // List becomes {1_a, 2_b, 1_c, 3_d}. Map becomes {1: [it_a, it_c], 2: [it_b], 3: [it_d]}
    oms.erase(1);
    std::vector<int> expected_after_erase1 = {1, 2, 1, 3};
    EXPECT_EQ(expected_after_erase1, to_vector(oms));
    EXPECT_EQ(2, oms.count(1));

    // Erase another '1'. The one pointed to by it_c should be removed.
    // List becomes {1_a, 2_b, 3_d}. Map becomes {1: [it_a], 2: [it_b], 3: [it_d]}
    oms.erase(1);
    std::vector<int> expected_after_erase2 = {1, 2, 3};
    EXPECT_EQ(expected_after_erase2, to_vector(oms));
    EXPECT_EQ(1, oms.count(1));

    // Insert a '1' again.
    // List becomes {1_a, 2_b, 3_d, 1_f}. Map becomes {1: [it_a, it_f], 2: [it_b], 3: [it_d]}
    oms.insert(1);
    std::vector<int> expected_after_insert_again = {1, 2, 3, 1};
    EXPECT_EQ(expected_after_insert_again, to_vector(oms));
    EXPECT_EQ(2, oms.count(1));

    // Erase all '1's.
    // List becomes {2_b, 3_d}. Map becomes {2: [it_b], 3: [it_d]} (key 1 removed)
    oms.erase_all(1);
    std::vector<int> expected_after_erase_all = {2, 3};
    EXPECT_EQ(expected_after_erase_all, to_vector(oms));
    EXPECT_EQ(0, oms.count(1));
    EXPECT_EQ(2, oms.size());
}

// Custom struct for testing with user-defined types
struct MyStruct {
    int id;
    std::string name;

    // Needed for std::unordered_map key if not providing custom hash/equal
    bool operator==(const MyStruct& other) const {
        return id == other.id && name == other.name;
    }
};

// Custom hash for MyStruct
struct MyStructHash {
    std::size_t operator()(const MyStruct& s) const {
        return std::hash<int>()(s.id) ^ (std::hash<std::string>()(s.name) << 1);
    }
};

TEST_F(OrderedMultisetTest, CustomType) {
    cpp_utils::OrderedMultiset<MyStruct, MyStructHash> oms_custom;
    MyStruct s1{1, "one"};
    MyStruct s2{2, "two"};
    MyStruct s1_dup{1, "one"};

    oms_custom.insert(s1);
    oms_custom.insert(s2);
    oms_custom.insert(s1_dup); // Same value as s1

    EXPECT_EQ(3, oms_custom.size());
    EXPECT_EQ(2, oms_custom.count(s1));
    EXPECT_EQ(1, oms_custom.count(s2));
    EXPECT_TRUE(oms_custom.contains(s1_dup));

    std::vector<MyStruct> expected_order = {s1, s2, s1_dup};
    EXPECT_EQ(expected_order, to_vector(oms_custom));

    oms_custom.erase(s1);
    EXPECT_EQ(2, oms_custom.size());
    EXPECT_EQ(1, oms_custom.count(s1));
    expected_order = {s1, s2}; // The s1_dup instance was removed
    EXPECT_EQ(expected_order, to_vector(oms_custom));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

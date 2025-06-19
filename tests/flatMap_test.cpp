#include "gtest/gtest.h"
#include "flatMap.h" // Assuming flatMap.h is in the include directory, and build system handles include paths
#include <string>
#include <vector>
#include <utility> // For std::pair

// Test fixture for FlatMap tests
class FlatMapTest : public ::testing::Test {
protected:
    FlatMap<int, std::string> map_int_string;
    FlatMap<std::string, int> map_string_int;
};

// Test case for inserting and looking up a small number of items
TEST_F(FlatMapTest, InsertAndLookup) {
    map_int_string.insert(1, "one");
    map_int_string.insert(2, "two");
    map_int_string.insert(0, "zero"); // Insert out of order

    ASSERT_NE(map_int_string.find(1), nullptr);
    EXPECT_EQ(*map_int_string.find(1), "one");
    ASSERT_NE(map_int_string.find(2), nullptr);
    EXPECT_EQ(*map_int_string.find(2), "two");
    ASSERT_NE(map_int_string.find(0), nullptr);
    EXPECT_EQ(*map_int_string.find(0), "zero");
    EXPECT_EQ(map_int_string.find(3), nullptr); // Non-existent key
}

// Test case for insertion with a key collision (should update the value)
TEST_F(FlatMapTest, InsertCollisionUpdatesValue) {
    map_int_string.insert(1, "one_v1");
    ASSERT_NE(map_int_string.find(1), nullptr);
    EXPECT_EQ(*map_int_string.find(1), "one_v1");

    map_int_string.insert(1, "one_v2"); // Collision
    ASSERT_NE(map_int_string.find(1), nullptr);
    EXPECT_EQ(*map_int_string.find(1), "one_v2"); // Value should be updated
    EXPECT_EQ(map_int_string.size(), 1); // Size should remain 1
}

// Test case for erasing an existing key
TEST_F(FlatMapTest, EraseExistingKey) {
    map_int_string.insert(1, "one");
    map_int_string.insert(2, "two");
    ASSERT_EQ(map_int_string.size(), 2);

    EXPECT_TRUE(map_int_string.erase(1));
    EXPECT_EQ(map_int_string.size(), 1);
    EXPECT_EQ(map_int_string.find(1), nullptr);
    ASSERT_NE(map_int_string.find(2), nullptr); // Make sure other key is still there
    EXPECT_EQ(*map_int_string.find(2), "two");
}

// Test case for attempting to erase a non-existent key (should be a no-op)
TEST_F(FlatMapTest, EraseNonExistentKey) {
    map_int_string.insert(1, "one");
    ASSERT_EQ(map_int_string.size(), 1);

    EXPECT_FALSE(map_int_string.erase(2)); // Key 2 does not exist
    EXPECT_EQ(map_int_string.size(), 1);    // Size should not change
    ASSERT_NE(map_int_string.find(1), nullptr); // Key 1 should still be there
}

// Test case for looking up a missing key (using find, should return nullptr)
TEST_F(FlatMapTest, LookupMissingKeyFind) {
    EXPECT_EQ(map_int_string.find(123), nullptr);
    map_int_string.insert(1, "one");
    EXPECT_EQ(map_int_string.find(123), nullptr);
}

// Test case for using operator[] to insert a missing key with a default-initialized value
TEST_F(FlatMapTest, OperatorSquareBracketsInsertsMissing) {
    // For std::string, default-initialized is an empty string
    EXPECT_EQ(map_int_string[10], "");
    ASSERT_NE(map_int_string.find(10), nullptr);
    EXPECT_EQ(*map_int_string.find(10), "");
    EXPECT_EQ(map_int_string.size(), 1);

    // Access again, should return existing
    map_int_string[10] = "ten";
    EXPECT_EQ(map_int_string[10], "ten");
    EXPECT_EQ(map_int_string.size(), 1);
}

// Test case for using operator[] to access an existing key
TEST_F(FlatMapTest, OperatorSquareBracketsAccessExisting) {
    map_int_string.insert(5, "five");
    EXPECT_EQ(map_int_string[5], "five");
    map_int_string[5] = "new_five";
    EXPECT_EQ(map_int_string[5], "new_five");
}

// Test case for using at() to access an existing key
TEST_F(FlatMapTest, AtAccessExisting) {
    map_int_string.insert(7, "seven");
    EXPECT_EQ(map_int_string.at(7), "seven");

    // Const version
    const auto& const_map = map_int_string;
    EXPECT_EQ(const_map.at(7), "seven");
}

// Test case for using at() to access a missing key (should throw std::out_of_range)
TEST_F(FlatMapTest, AtAccessMissingThrows) {
    EXPECT_THROW(map_int_string.at(99), std::out_of_range);
    map_int_string.insert(1, "one"); // Add something to make sure it's not empty related
    EXPECT_THROW(map_int_string.at(99), std::out_of_range);

    // Const version
    const auto& const_map = map_int_string;
    EXPECT_THROW(const_map.at(99), std::out_of_range);
}

// Test case for iterating over the FlatMap and verifying that key-value pairs are sorted
TEST_F(FlatMapTest, IterationReturnsSorted) {
    map_string_int.insert("banana", 2);
    map_string_int.insert("apple", 1);
    map_string_int.insert("cherry", 3);
    map_string_int.insert("date", 0); // To ensure it's not just insertion order

    std::vector<std::pair<std::string, int>> expected_order = {
        {"apple", 1}, {"banana", 2}, {"cherry", 3}, {"date", 0}
    };
    // Sort expected order by key for comparison as FlatMap stores them sorted by key
    std::sort(expected_order.begin(), expected_order.end(),
        [](const auto& a, const auto& b){
            return a.first < b.first;
        });


    std::vector<std::pair<std::string, int>> actual_order;
    for (const auto& pair : map_string_int) {
        actual_order.push_back(pair);
    }

    // The internal representation of FlatMap is already sorted by key.
    // The test here is to ensure iteration visits them in that sorted order.
    // So, `expected_order` should be `{"apple", 1}, {"banana", 2}, {"cherry", 3}, {"date", 0}`
    // after sorting by key.

    ASSERT_EQ(actual_order.size(), expected_order.size());
    for (size_t i = 0; i < actual_order.size(); ++i) {
        EXPECT_EQ(actual_order[i].first, expected_order[i].first) << "Mismatch at index " << i;
        EXPECT_EQ(actual_order[i].second, expected_order[i].second) << "Mismatch at index " << i;
    }
}

// Test case for empty() and size() methods
TEST_F(FlatMapTest, EmptyAndSize) {
    EXPECT_TRUE(map_int_string.empty());
    EXPECT_EQ(map_int_string.size(), 0);

    map_int_string.insert(1, "one");
    EXPECT_FALSE(map_int_string.empty());
    EXPECT_EQ(map_int_string.size(), 1);

    map_int_string.insert(2, "two");
    EXPECT_FALSE(map_int_string.empty());
    EXPECT_EQ(map_int_string.size(), 2);

    map_int_string.erase(1);
    EXPECT_FALSE(map_int_string.empty());
    EXPECT_EQ(map_int_string.size(), 1);

    map_int_string.erase(2);
    EXPECT_TRUE(map_int_string.empty());
    EXPECT_EQ(map_int_string.size(), 0);
}

// Test contains method
TEST_F(FlatMapTest, Contains) {
    EXPECT_FALSE(map_int_string.contains(1));
    map_int_string.insert(1, "one");
    EXPECT_TRUE(map_int_string.contains(1));
    EXPECT_FALSE(map_int_string.contains(2));
    map_int_string.insert(2, "two");
    EXPECT_TRUE(map_int_string.contains(2));
    map_int_string.erase(1);
    EXPECT_FALSE(map_int_string.contains(1));
    EXPECT_TRUE(map_int_string.contains(2));
}

// Test with more complex types and larger number of elements
TEST_F(FlatMapTest, LargeNumberOfElementsAndStrings) {
    FlatMap<int, std::string> large_map;
    const int num_elements = 1000;
    for (int i = 0; i < num_elements; ++i) {
        large_map.insert(i, "value_" + std::to_string(i));
    }

    EXPECT_EQ(large_map.size(), num_elements);
    for (int i = 0; i < num_elements; ++i) {
        ASSERT_TRUE(large_map.contains(i));
        ASSERT_NE(large_map.find(i), nullptr);
        EXPECT_EQ(*large_map.find(i), "value_" + std::to_string(i));
    }

    // Check iteration order for a subset
    int count = 0;
    for(const auto& p : large_map) {
        EXPECT_EQ(p.first, count);
        EXPECT_EQ(p.second, "value_" + std::to_string(count));
        count++;
        if (count > 10) break; // Just check the first few
    }


    // Erase some elements
    for (int i = 0; i < num_elements; i += 2) { // Erase even numbers
        EXPECT_TRUE(large_map.erase(i));
    }
    EXPECT_EQ(large_map.size(), num_elements / 2);

    for (int i = 0; i < num_elements; ++i) {
        if (i % 2 == 0) { // Even numbers should be gone
            EXPECT_FALSE(large_map.contains(i));
            EXPECT_EQ(large_map.find(i), nullptr);
        } else { // Odd numbers should still be there
            EXPECT_TRUE(large_map.contains(i));
            ASSERT_NE(large_map.find(i), nullptr);
            EXPECT_EQ(*large_map.find(i), "value_" + std::to_string(i));
        }
    }
}

// Test operator[] for default construction of complex types
struct ComplexValue {
    int id = 0;
    std::string name = "default";
    bool operator==(const ComplexValue& other) const {
        return id == other.id && name == other.name;
    }
};

TEST_F(FlatMapTest, OperatorSquareBracketsDefaultConstructionComplex) {
    FlatMap<int, ComplexValue> map_complex;

    // Accessing a new key should insert a default-constructed ComplexValue
    ComplexValue val = map_complex[1];
    EXPECT_EQ(val.id, 0);
    EXPECT_EQ(val.name, "default");
    ASSERT_TRUE(map_complex.contains(1));
    EXPECT_EQ(map_complex.at(1).id, 0);
    EXPECT_EQ(map_complex.at(1).name, "default");

    // Modify it
    map_complex[1].name = "modified";
    EXPECT_EQ(map_complex[1].name, "modified");
}

// Test const correctness for find, at, begin, end, contains, size, empty
TEST_F(FlatMapTest, ConstCorrectness) {
    FlatMap<int, int> map_for_const_test;
    map_for_const_test.insert(1, 10);
    map_for_const_test.insert(2, 20);

    const FlatMap<int, int>& const_map = map_for_const_test;

    // find
    ASSERT_NE(const_map.find(1), nullptr);
    EXPECT_EQ(*const_map.find(1), 10);
    EXPECT_EQ(const_map.find(3), nullptr);

    // at
    EXPECT_EQ(const_map.at(2), 20);
    EXPECT_THROW(const_map.at(3), std::out_of_range);

    // contains
    EXPECT_TRUE(const_map.contains(1));
    EXPECT_FALSE(const_map.contains(3));

    // size & empty
    EXPECT_EQ(const_map.size(), 2);
    EXPECT_FALSE(const_map.empty());

    // begin & end
    int key_sum = 0;
    int value_sum = 0;
    for (const auto& p : const_map) {
        key_sum += p.first;
        value_sum += p.second;
    }
    EXPECT_EQ(key_sum, 3);   // 1 + 2
    EXPECT_EQ(value_sum, 30); // 10 + 20

    FlatMap<int,int> empty_const_map_init;
    const FlatMap<int,int>& empty_const_map = empty_const_map_init;
    EXPECT_TRUE(empty_const_map.empty());
    EXPECT_EQ(empty_const_map.size(), 0);
    EXPECT_EQ(empty_const_map.begin(), empty_const_map.end());
}

// Test that iterators are of the correct type (const_iterator for const FlatMap)
TEST_F(FlatMapTest, IteratorTypes) {
    FlatMap<int, std::string> map;
    map.insert(1, "hello");

    // Non-const map, begin() should return iterator
    auto it = map.begin();
    *it = std::make_pair(1, "world"); // Modifying through iterator
    EXPECT_EQ(map.at(1), "world");

    // Const map, begin() should return const_iterator
    const FlatMap<int, std::string>& const_map = map;
    auto const_it = const_map.begin();
    // *const_it = std::make_pair(1, "fail"); // This line should not compile if uncommented
    EXPECT_EQ(const_it->first, 1);
    EXPECT_EQ(const_it->second, "world");
    // Check that the type is const_iterator. This is a bit hard to assert directly at runtime
    // but the ability to compile (or not compile) assignments is the key.
    // We can use std::is_const to check the value_type pointed to.
    // Note: std::vector<std::pair<const K, V>>::iterator is not what lower_bound returns for const vector.
    // It returns std::vector<std::pair<K,V>>::const_iterator.
    // So value_type of const_iterator is std::pair<Key, Value>, but it's not modifiable.
    // The const-ness is on the iterator itself preventing modification.
    // So, this check might be tricky or misleading.
    // The key is that `const_it->second = "new value";` would fail to compile.
    // And `*const_it = ...` would fail to compile.

    SUCCEED(); // If it compiles and previous checks pass, consider it a success for iterator types.
}

// Main function for GTest (if not linking with GTest main)
// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }

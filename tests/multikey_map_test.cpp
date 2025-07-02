#include "gtest/gtest.h"
#include "multikey_map.h" // Assuming this is the header for MultiKeyMap
#include <string>
#include <vector>
#include <tuple>

// Custom struct for testing complex key types
struct TestKeyStruct {
    int id;
    std::string name;
    double value;

    bool operator==(const TestKeyStruct& other) const {
        return id == other.id && name == other.name && value == other.value;
    }
};

// Hash specialization for TestKeyStruct
namespace std {
template <>
struct hash<TestKeyStruct> {
    size_t operator()(const TestKeyStruct& s) const {
        size_t seed = 0;
        hash_combine(seed, s.id);
        hash_combine(seed, s.name);
        hash_combine(seed, s.value);
        return seed;
    }
};
} // namespace std


// Test fixture for MultiKeyMap tests
class MultiKeyMapTest : public ::testing::Test {
protected:
    // Example: MultiKeyMap<ValueType, KeyType1, KeyType2, ...>
    MultiKeyMap<std::string, int, std::string> map_is_s; // int, string -> string
    MultiKeyMap<int, std::string, double, char> map_sdc_i; // string, double, char -> int
    MultiKeyMap<double, TestKeyStruct> map_cks_d; // TestKeyStruct -> double
};

// Test default constructor and initial state
TEST_F(MultiKeyMapTest, DefaultConstructor) {
    EXPECT_TRUE(map_is_s.empty());
    EXPECT_EQ(0, map_is_s.size());
    MultiKeyMap<int, int> empty_map;
    EXPECT_TRUE(empty_map.empty());
}

// Test insertion and basic find
TEST_F(MultiKeyMapTest, InsertAndFind) {
    map_is_s.insert(1, "apple", "Red Fruit");
    ASSERT_EQ(1, map_is_s.size());
    ASSERT_FALSE(map_is_s.empty());

    auto it = map_is_s.find(1, "apple");
    ASSERT_NE(it, map_is_s.end());
    EXPECT_EQ("Red Fruit", it->second);

    const auto& cmap_is_s = map_is_s;
    auto cit = cmap_is_s.find(1, "apple");
    ASSERT_NE(cit, cmap_is_s.cend());
    EXPECT_EQ("Red Fruit", cit->second);
}

// Test emplace
TEST_F(MultiKeyMapTest, Emplace) {
    auto result = map_is_s.emplace(2, "banana", "Yellow Fruit");
    EXPECT_TRUE(result.second); // Insertion should happen
    EXPECT_EQ("Yellow Fruit", result.first->second);
    EXPECT_EQ(1, map_is_s.size());

    // Try emplacing duplicate
    result = map_is_s.emplace(2, "banana", "Another Yellow Fruit");
    EXPECT_FALSE(result.second); // Insertion should not happen
    EXPECT_EQ("Yellow Fruit", result.first->second); // Should point to existing
    EXPECT_EQ(1, map_is_s.size());
}

// Test try_emplace
TEST_F(MultiKeyMapTest, TryEmplace) {
    auto result = map_is_s.try_emplace(3, "cherry", "Red Small Fruit");
    EXPECT_TRUE(result.second);
    EXPECT_EQ("Red Small Fruit", result.first->second);
    EXPECT_EQ(1, map_is_s.size());

    result = map_is_s.try_emplace(3, "cherry", "Another Red Fruit");
    EXPECT_FALSE(result.second); // Key exists, no insertion or update
    EXPECT_EQ("Red Small Fruit", result.first->second); // Points to existing
    EXPECT_EQ(1, map_is_s.size());

    // Try emplace with a new value for a new key component but same first key
    result = map_is_s.try_emplace(3, "cranberry", "Tart Red Fruit");
    EXPECT_TRUE(result.second);
    EXPECT_EQ("Tart Red Fruit", result.first->second);
    EXPECT_EQ(2, map_is_s.size());
}


// Test at() method for access and exceptions
TEST_F(MultiKeyMapTest, AtMethod) {
    map_is_s.insert(1, "apple", "Red Fruit");
    EXPECT_EQ("Red Fruit", map_is_s.at(1, "apple"));

    const auto& cmap_is_s = map_is_s;
    EXPECT_EQ("Red Fruit", cmap_is_s.at(1, "apple"));

    EXPECT_THROW(map_is_s.at(2, "nonexistent"), std::out_of_range);
    EXPECT_THROW(cmap_is_s.at(2, "nonexistent"), std::out_of_range);
}

// Test operator() for access and insertion
TEST_F(MultiKeyMapTest, CallOperator) {
    map_is_s(1, "date") = "Brown Fruit"; // Insert
    EXPECT_EQ("Brown Fruit", map_is_s.at(1, "date"));
    EXPECT_EQ(1, map_is_s.size());

    map_is_s(1, "date") = "Sweet Brown Fruit"; // Update
    EXPECT_EQ("Sweet Brown Fruit", map_is_s.at(1, "date"));
    EXPECT_EQ(1, map_is_s.size());

    // Accessing non-existent key creates default-constructed value
    EXPECT_EQ("", map_is_s(2, "fig")); // Assuming std::string default constructor
    EXPECT_EQ(2, map_is_s.size());
    EXPECT_TRUE(map_is_s.contains(2, "fig"));
}

// Test operator[] with std::tuple
TEST_F(MultiKeyMapTest, BracketOperatorWithTuple) {
    using KeyTuple = typename decltype(map_is_s)::KeyTuple;
    KeyTuple kt1 = {1, "elderberry"};

    map_is_s[kt1] = "Dark Berry"; // Insert
    EXPECT_EQ("Dark Berry", map_is_s.at_tuple(kt1));
    EXPECT_EQ(1, map_is_s.size());

    map_is_s[kt1] = "Sweet Dark Berry"; // Update
    EXPECT_EQ("Sweet Dark Berry", map_is_s.at_tuple(kt1));
    EXPECT_EQ(1, map_is_s.size());

    KeyTuple kt2 = {2, "fig"};
    EXPECT_EQ("", map_is_s[kt2]); // Default construction
    EXPECT_EQ(2, map_is_s.size());
    EXPECT_TRUE(map_is_s.contains_tuple(kt2));

    KeyTuple kt3 = {3, "grape"};
    map_is_s[std::move(kt3)] = "Green or Purple"; // Move overload for key
    EXPECT_TRUE(map_is_s.contains(3, "grape"));
    EXPECT_EQ("Green or Purple", map_is_s.at(3, "grape"));

}


// Test contains method
TEST_F(MultiKeyMapTest, Contains) {
    map_is_s.insert(1, "apple", "Red Fruit");
    EXPECT_TRUE(map_is_s.contains(1, "apple"));
    EXPECT_FALSE(map_is_s.contains(1, "banana"));
    EXPECT_FALSE(map_is_s.contains(2, "apple"));
}

// Test erase method
TEST_F(MultiKeyMapTest, Erase) {
    map_is_s.insert(1, "apple", "Red Fruit");
    map_is_s.insert(2, "banana", "Yellow Fruit");
    ASSERT_EQ(2, map_is_s.size());

    EXPECT_EQ(1, map_is_s.erase(1, "apple")); // Erase existing
    EXPECT_EQ(1, map_is_s.size());
    EXPECT_FALSE(map_is_s.contains(1, "apple"));

    EXPECT_EQ(0, map_is_s.erase(1, "apple")); // Erase non-existing
    EXPECT_EQ(1, map_is_s.size());

    // Erase by iterator
    auto it = map_is_s.find(2, "banana");
    ASSERT_NE(it, map_is_s.end());
    auto next_it = map_is_s.erase(it);
    EXPECT_EQ(map_is_s.end(), next_it); // Simplified check, might be more specific if map had more elements
    EXPECT_EQ(0, map_is_s.size());
    EXPECT_TRUE(map_is_s.empty());
}

// Test clear method
TEST_F(MultiKeyMapTest, Clear) {
    map_is_s.insert(1, "apple", "Red Fruit");
    map_is_s.insert(2, "banana", "Yellow Fruit");
    ASSERT_FALSE(map_is_s.empty());

    map_is_s.clear();
    EXPECT_TRUE(map_is_s.empty());
    EXPECT_EQ(0, map_is_s.size());
    EXPECT_FALSE(map_is_s.contains(1, "apple")); // Ensure elements are gone
}

// Test iteration
TEST_F(MultiKeyMapTest, Iteration) {
    map_is_s.insert(1, "apple", "Red");
    map_is_s.insert(2, "banana", "Yellow");
    map_is_s.insert(3, "cherry", "Red Small");

    int count = 0;
    std::vector<std::tuple<int, std::string>> expected_keys = {
        {1, "apple"}, {2, "banana"}, {3, "cherry"}
    };
    std::vector<std::string> expected_values = {"Red", "Yellow", "Red Small"};

    for (const auto& pair : map_is_s) {
        bool found = false;
        for (size_t i = 0; i < expected_keys.size(); ++i) {
            if (pair.first == expected_keys[i] && pair.second == expected_values[i]) {
                found = true;
                // Remove to ensure we don't match twice and all are found
                expected_keys.erase(expected_keys.begin() + i);
                expected_values.erase(expected_values.begin() + i);
                break;
            }
        }
        EXPECT_TRUE(found) << "Unexpected pair: ("
                           << std::get<0>(pair.first) << ", " << std::get<1>(pair.first)
                           << ") -> " << pair.second;
        count++;
    }
    EXPECT_EQ(3, count);
    EXPECT_TRUE(expected_keys.empty()) << "Not all expected keys were found during iteration.";
}

// Test with multiple key types
TEST_F(MultiKeyMapTest, MultipleKeyTypes) {
    map_sdc_i.insert("key1", 3.14, 'a', 100);
    map_sdc_i.insert("key2", 2.71, 'b', 200);

    EXPECT_EQ(100, map_sdc_i.at("key1", 3.14, 'a'));
    EXPECT_TRUE(map_sdc_i.contains("key2", 2.71, 'b'));
    EXPECT_FALSE(map_sdc_i.contains("key1", 3.14, 'x'));
}

// Test with custom struct as key
TEST_F(MultiKeyMapTest, CustomStructKey) {
    TestKeyStruct k1 = {1, "one", 1.0};
    TestKeyStruct k2 = {2, "two", 2.0};

    map_cks_d.insert(k1, 100.0);
    map_cks_d.insert(k2, 200.0);

    ASSERT_EQ(2, map_cks_d.size());
    EXPECT_DOUBLE_EQ(100.0, map_cks_d.at(k1));

    // Test find with a temporary equivalent key
    EXPECT_DOUBLE_EQ(200.0, map_cks_d.at({2, "two", 2.0}));

    EXPECT_TRUE(map_cks_d.contains({1, "one", 1.0}));
    EXPECT_FALSE(map_cks_d.contains({3, "three", 3.0}));
}

// Test initializer list constructor
TEST_F(MultiKeyMapTest, InitializerListConstructor) {
    using KeyTuple = MultiKeyMap<std::string, int, std::string>::KeyTuple;
    MultiKeyMap<std::string, int, std::string> map_init = {
        {std::make_tuple(1, "one"), "uno"},
        {std::make_tuple(2, "two"), "dos"},
        {KeyTuple{3, "three"}, "tres"} // Another way to create tuple
    };

    ASSERT_EQ(3, map_init.size());
    EXPECT_EQ("uno", map_init.at(1, "one"));
    EXPECT_EQ("dos", map_init.at(2, "two"));
    EXPECT_EQ("tres", map_init.at_tuple({3, "three"}));
}

// Test copy constructor
TEST_F(MultiKeyMapTest, CopyConstructor) {
    map_is_s.insert(1, "copy", "original_value");
    MultiKeyMap<std::string, int, std::string> copy_map(map_is_s);

    ASSERT_EQ(1, copy_map.size());
    EXPECT_EQ("original_value", copy_map.at(1, "copy"));
    // Ensure modifying copy doesn't affect original
    copy_map.insert(2, "new", "new_value");
    EXPECT_EQ(1, map_is_s.size());
    EXPECT_FALSE(map_is_s.contains(2, "new"));
}

// Test copy assignment
TEST_F(MultiKeyMapTest, CopyAssignment) {
    map_is_s.insert(1, "assign", "val1");
    MultiKeyMap<std::string, int, std::string> assigned_map;
    assigned_map.insert(10, "old", "old_val");

    assigned_map = map_is_s;
    ASSERT_EQ(1, assigned_map.size());
    EXPECT_EQ("val1", assigned_map.at(1, "assign"));
    EXPECT_FALSE(assigned_map.contains(10, "old"));

    // Ensure modifying assigned_map doesn't affect original
    assigned_map.insert(2, "new_in_assigned", "val2");
    EXPECT_EQ(1, map_is_s.size());
}

// Test move constructor
TEST_F(MultiKeyMapTest, MoveConstructor) {
    map_is_s.insert(1, "move_val", "original_move");
    MultiKeyMap<std::string, int, std::string> moved_map(std::move(map_is_s));

    ASSERT_EQ(1, moved_map.size());
    EXPECT_EQ("original_move", moved_map.at(1, "move_val"));
    // Original map_is_s should be in a valid but unspecified state (likely empty)
    // EXPECT_TRUE(map_is_s.empty()); // This depends on std::unordered_map move behavior
}

// Test move assignment
TEST_F(MultiKeyMapTest, MoveAssignment) {
    map_is_s.insert(1, "move_assign_val", "val_to_move");
    MultiKeyMap<std::string, int, std::string> target_map;
    target_map.insert(100, "target_old", "old_target_val");

    target_map = std::move(map_is_s);
    ASSERT_EQ(1, target_map.size());
    EXPECT_EQ("val_to_move", target_map.at(1, "move_assign_val"));
    EXPECT_FALSE(target_map.contains(100, "target_old"));
    // EXPECT_TRUE(map_is_s.empty()); // Original map_is_s state
}

// Test swap
TEST_F(MultiKeyMapTest, Swap) {
    map_is_s.insert(1, "map1_key", "map1_val");
    MultiKeyMap<std::string, int, std::string> map2_is_s;
    map2_is_s.insert(2, "map2_key", "map2_val");

    map_is_s.swap(map2_is_s);

    EXPECT_TRUE(map_is_s.contains(2, "map2_key"));
    EXPECT_EQ("map2_val", map_is_s.at(2, "map2_key"));
    EXPECT_FALSE(map_is_s.contains(1, "map1_key"));
    EXPECT_EQ(1, map_is_s.size());

    EXPECT_TRUE(map2_is_s.contains(1, "map1_key"));
    EXPECT_EQ("map1_val", map2_is_s.at(1, "map1_key"));
    EXPECT_FALSE(map2_is_s.contains(2, "map2_key"));
    EXPECT_EQ(1, map2_is_s.size());

    // Test non-member swap
    swap(map_is_s, map2_is_s); // Swap back
    EXPECT_TRUE(map_is_s.contains(1, "map1_key"));
    EXPECT_TRUE(map2_is_s.contains(2, "map2_key"));
}


// Test hash policies and bucket interface (basic pass-through checks)
TEST_F(MultiKeyMapTest, HashPolicyAndBuckets) {
    map_is_s.insert(1, "a", "1a");
    map_is_s.insert(2, "b", "2b");
    map_is_s.insert(3, "c", "3c");

    EXPECT_GT(map_is_s.bucket_count(), 0);
    EXPECT_NO_THROW(map_is_s.load_factor());
    EXPECT_NO_THROW(map_is_s.max_load_factor());
    EXPECT_NO_THROW(map_is_s.max_load_factor(2.0f));

    size_t bucket_for_key = map_is_s.bucket(1, "a");
    EXPECT_LT(bucket_for_key, map_is_s.bucket_count());
    EXPECT_GT(map_is_s.bucket_size(bucket_for_key), 0);

    map_is_s.rehash(100);
    EXPECT_GE(map_is_s.bucket_count(), 100 / map_is_s.max_load_factor());

    map_is_s.reserve(50); // Reserve space for 50 elements
    // Actual bucket_count change due to reserve is implementation-defined based on load factor
}

// Test equality operators
TEST_F(MultiKeyMapTest, EqualityOperators) {
    MultiKeyMap<std::string, int, std::string> map_a;
    MultiKeyMap<std::string, int, std::string> map_b;

    map_a.insert(1, "key", "valueA");
    map_b.insert(1, "key", "valueA");
    EXPECT_TRUE(map_a == map_b);
    EXPECT_FALSE(map_a != map_b);

    map_b.insert(2, "key2", "valueB");
    EXPECT_FALSE(map_a == map_b);
    EXPECT_TRUE(map_a != map_b);

    map_a.insert(2, "key2", "valueDifferent");
    EXPECT_FALSE(map_a == map_b); // Different values for same key structure

    map_a.at(2,"key2") = "valueB"; // Make values same
    EXPECT_TRUE(map_a == map_b);
}

// Test with empty key set (edge case, though our current design requires at least one KeyType)
// This test is more conceptual for variadic templates.
// MultiKeyMap<std::string> map_no_keys; // This would not compile with current design.
// To support this, KeyTuple would need specialization for empty pack,
// and TupleHash would need to handle it (e.g. return 0).
// For now, we assume KeyTypes... is not empty.

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

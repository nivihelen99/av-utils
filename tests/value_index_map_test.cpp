#include "../include/value_index_map.h"
#include "gtest/gtest.h" // Google Test header
#include <string>
#include <vector>
#include <optional>
#include <stdexcept> // For std::logic_error etc.

// Test fixture for ValueIndexMap tests, if needed for setup/teardown
class ValueIndexMapTest : public ::testing::Test {
protected:
    // Per-test-suite set-up.
    // Called before the first test in this test suite.
    // Can be static if shared resources are static.
    static void SetUpTestSuite() {}

    // Per-test-suite tear-down.
    // Called after the last test in this test suite.
    // Can be static if shared resources are static.
    static void TearDownTestSuite() {}

    // You can define per-test set-up and tear-down logic as usual.
    void SetUp() override {
        // Code here will be called immediately after the constructor (right
        // before each test).
    }

    void TearDown() override {
        // Code here will be called immediately after each test (right
        // before the destructor).
    }
};

TEST_F(ValueIndexMapTest, BasicOperationsInt) {
    ValueIndexMap<int> map;
    ASSERT_EQ(map.size(), 0);
    ASSERT_TRUE(map.empty());

    size_t idx1 = map.insert(100);
    ASSERT_EQ(idx1, 0);
    ASSERT_EQ(map.size(), 1);
    ASSERT_FALSE(map.empty());
    ASSERT_TRUE(map.contains(100));
    ASSERT_FALSE(map.contains(200));
    ASSERT_EQ(map.index_of(100).value_or(-1), idx1);
    ASSERT_NE(map.value_at(idx1), nullptr);
    ASSERT_EQ(*map.value_at(idx1), 100);

    size_t idx2 = map.insert(200);
    ASSERT_EQ(idx2, 1);
    ASSERT_EQ(map.size(), 2);
    ASSERT_TRUE(map.contains(200));
    ASSERT_EQ(map.index_of(200).value_or(-1), idx2);
    ASSERT_NE(map.value_at(idx2), nullptr);
    ASSERT_EQ(*map.value_at(idx2), 200);

    // Idempotency
    size_t idx1_again = map.insert(100);
    ASSERT_EQ(idx1_again, idx1);
    ASSERT_EQ(map.size(), 2); // Size should not change

    // value_at out of bounds
    ASSERT_EQ(map.value_at(map.size()), nullptr);
    ASSERT_EQ(map.value_at(99), nullptr);

    // index_of non-existent
    ASSERT_FALSE(map.index_of(300).has_value());
}

TEST_F(ValueIndexMapTest, BasicOperationsString) {
    ValueIndexMap<std::string> map;
    ASSERT_EQ(map.size(), 0);

    size_t idx_apple = map.insert("apple");
    ASSERT_EQ(idx_apple, 0);
    ASSERT_EQ(map.size(), 1);
    ASSERT_TRUE(map.contains("apple"));
    ASSERT_EQ(map.index_of("apple").value_or(-1), idx_apple);
    ASSERT_NE(map.value_at(idx_apple), nullptr);
    ASSERT_EQ(*map.value_at(idx_apple), "apple");

    size_t idx_banana = map.insert("banana");
    ASSERT_EQ(idx_banana, 1);
    ASSERT_EQ(map.size(), 2);

    // Move semantics for insert
    std::string orange_str = "orange";
    size_t idx_orange = map.insert(std::move(orange_str));
    ASSERT_EQ(idx_orange, 2);
    ASSERT_EQ(map.size(), 3);
    ASSERT_TRUE(map.contains("orange"));
    ASSERT_NE(map.value_at(idx_orange), nullptr);
    ASSERT_EQ(*map.value_at(idx_orange), "orange");
    // orange_str is in a valid but unspecified state (likely empty)
    // EXPECT_TRUE(orange_str.empty()); // This behavior depends on std::string move
}

TEST_F(ValueIndexMapTest, Clear) {
    ValueIndexMap<int> map;
    map.insert(10);
    map.insert(20);
    ASSERT_EQ(map.size(), 2);
    map.clear();
    ASSERT_EQ(map.size(), 0);
    ASSERT_TRUE(map.empty());
    ASSERT_FALSE(map.contains(10));
    ASSERT_FALSE(map.index_of(10).has_value());
    ASSERT_EQ(map.value_at(0), nullptr);

    // Insert after clear
    size_t idx = map.insert(30);
    ASSERT_EQ(idx, 0);
    ASSERT_EQ(map.size(), 1);
    ASSERT_TRUE(map.contains(30));
}

TEST_F(ValueIndexMapTest, Seal) {
    ValueIndexMap<int> map;
    map.insert(1);
    map.seal();
    ASSERT_TRUE(map.is_sealed());

    ASSERT_THROW(map.insert(2), std::logic_error);
    ASSERT_EQ(map.size(), 1); // Size should not change

    ASSERT_THROW(map.clear(), std::logic_error);
    ASSERT_EQ(map.size(), 1); // Size should not change

    // Reads should still work
    ASSERT_TRUE(map.contains(1));
    ASSERT_EQ(map.index_of(1).value_or(-1), 0);
    ASSERT_NE(map.value_at(0), nullptr);
    ASSERT_EQ(*map.value_at(0), 1);
}

TEST_F(ValueIndexMapTest, Iterators) {
    ValueIndexMap<std::string> map;
    map.insert("first");
    map.insert("second");
    map.insert("third");

    std::vector<std::string> expected_values = {"first", "second", "third"};
    size_t i = 0;
    for (const auto& val : map) { // Uses map.begin(), map.end()
        ASSERT_LT(i, expected_values.size());
        ASSERT_EQ(val, expected_values[i]);
        i++;
    }
    ASSERT_EQ(i, expected_values.size());

    // Test cbegin/cend
    i = 0;
    for (auto it = map.cbegin(); it != map.cend(); ++it) {
        ASSERT_LT(i, expected_values.size());
        ASSERT_EQ(*it, expected_values[i]);
        i++;
    }
    ASSERT_EQ(i, expected_values.size());

    ValueIndexMap<int> empty_map;
    int count = 0;
    for (const auto& val : empty_map) {
        count++;
    }
    ASSERT_EQ(count, 0);
    ASSERT_TRUE(std::begin(empty_map) == std::end(empty_map));
}

TEST_F(ValueIndexMapTest, SerializationDeserialization) {
    ValueIndexMap<std::string> original_map;
    original_map.insert("one");
    original_map.insert("two");
    original_map.insert("three");

    const std::vector<std::string>& serialized_data = original_map.get_values_for_serialization();
    ASSERT_EQ(serialized_data.size(), 3);

    // Deserialize using copy constructor
    ValueIndexMap<std::string> map_from_copy(serialized_data);
    ASSERT_EQ(map_from_copy.size(), 3);
    ASSERT_EQ(map_from_copy.index_of("one").value_or(-1), 0);
    ASSERT_EQ(map_from_copy.index_of("two").value_or(-1), 1);
    ASSERT_EQ(map_from_copy.index_of("three").value_or(-1), 2);
    ASSERT_NE(map_from_copy.value_at(1), nullptr);
    ASSERT_EQ(*map_from_copy.value_at(1), "two");

    // Deserialize using move constructor
    std::vector<std::string> data_to_move = {"alpha", "beta", "gamma"};
    ValueIndexMap<std::string> map_from_move(std::move(data_to_move));
    // data_to_move is in a valid but unspecified state (likely empty)
    ASSERT_TRUE(data_to_move.empty() || data_to_move.capacity() == 0);
    ASSERT_EQ(map_from_move.size(), 3);
    ASSERT_EQ(map_from_move.index_of("alpha").value_or(-1), 0);
    ASSERT_EQ(map_from_move.index_of("beta").value_or(-1), 1);
    ASSERT_EQ(map_from_move.index_of("gamma").value_or(-1), 2);

    // Test deserialization with duplicates in input (should throw)
    std::vector<std::string> duplicate_data = {"x", "y", "x"};
    ASSERT_THROW(ValueIndexMap<std::string> map_dup(duplicate_data), std::invalid_argument);

    std::vector<std::string> duplicate_data_to_move = {"x", "y", "x"};
    ASSERT_THROW(ValueIndexMap<std::string> map_dup_move(std::move(duplicate_data_to_move)), std::invalid_argument);
}

TEST_F(ValueIndexMapTest, Erase) {
    ValueIndexMap<std::string> map;
    map.insert("a"); // 0
    map.insert("b"); // 1
    map.insert("c"); // 2
    map.insert("d"); // 3
    map.insert("e"); // 4
    ASSERT_EQ(map.size(), 5);

    // Erase by value (middle)
    ASSERT_TRUE(map.erase("c")); // "c" at index 2. "e" (last) moves to index 2.
    ASSERT_EQ(map.size(), 4);
    ASSERT_FALSE(map.contains("c"));
    ASSERT_FALSE(map.index_of("c").has_value());
    ASSERT_TRUE(map.contains("e"));
    ASSERT_EQ(map.index_of("e").value_or(-1), 2); // "e" moved from index 4 to 2
    ASSERT_NE(map.value_at(2), nullptr);
    ASSERT_EQ(*map.value_at(2), "e");
    ASSERT_EQ(map.index_of("d").value_or(-1), 3); // "d" should be unaffected

    // Erase by value (actual last element)
    // Current state: a(0), b(1), e(2), d(3)
    ASSERT_TRUE(map.erase("d")); // "d" at index 3 (last)
    ASSERT_EQ(map.size(), 3);
    ASSERT_FALSE(map.contains("d"));
    ASSERT_EQ(map.index_of("e").value_or(-1), 2); // "e" unaffected

    // Erase by value (first element)
    // Current state: a(0), b(1), e(2)
    ASSERT_TRUE(map.erase("a")); // "a" at index 0. "e" (last) moves to index 0
    ASSERT_EQ(map.size(), 2);
    ASSERT_FALSE(map.contains("a"));
    ASSERT_TRUE(map.contains("e"));
    ASSERT_EQ(map.index_of("e").value_or(-1), 0); // "e" moved from index 2 to 0
    ASSERT_NE(map.value_at(0), nullptr);
    ASSERT_EQ(*map.value_at(0), "e");
    ASSERT_EQ(map.index_of("b").value_or(-1), 1); // "b" unaffected

    // Erase non-existent value
    ASSERT_FALSE(map.erase("z"));
    ASSERT_EQ(map.size(), 2);

    // Erase by index
    // Current state: e(0), b(1)
    ASSERT_TRUE(map.erase_at_index(0)); // Erase "e". "b" (last) moves to index 0
    ASSERT_EQ(map.size(), 1);
    ASSERT_FALSE(map.contains("e"));
    ASSERT_TRUE(map.contains("b"));
    ASSERT_EQ(map.index_of("b").value_or(-1), 0);
    ASSERT_NE(map.value_at(0), nullptr);
    ASSERT_EQ(*map.value_at(0), "b");

    // Erase last remaining element by index
    ASSERT_TRUE(map.erase_at_index(0));
    ASSERT_EQ(map.size(), 0);
    ASSERT_TRUE(map.empty());

    // Erase from empty map
    ASSERT_FALSE(map.erase("any"));
    ASSERT_FALSE(map.erase_at_index(0));

    // Seal and erase
    map.insert("final");
    map.seal();
    ASSERT_THROW(map.erase("final"), std::logic_error);
    ASSERT_THROW(map.erase_at_index(0), std::logic_error);
    ASSERT_EQ(map.size(), 1);
}

// Custom struct for testing custom hash/equality
struct CustomType {
    int id;
    std::string data;

    bool operator==(const CustomType& other) const {
        return id == other.id && data == other.data;
    }
};

// Custom hash for CustomType
struct CustomTypeHash {
    std::size_t operator()(const CustomType& ct) const {
        // Simple hash combination
        return std::hash<int>()(ct.id) ^ (std::hash<std::string>()(ct.data) << 1);
    }
};

TEST_F(ValueIndexMapTest, CustomHashEquality) {
    ValueIndexMap<CustomType, CustomTypeHash> map;

    CustomType val1 = {1, "hello"};
    CustomType val2 = {2, "world"};
    CustomType val1_again = {1, "hello"}; // Same as val1

    size_t idx1 = map.insert(val1);
    ASSERT_EQ(idx1, 0);
    ASSERT_EQ(map.size(), 1);
    ASSERT_TRUE(map.contains(val1));
    ASSERT_TRUE(map.index_of(val1).has_value());
    ASSERT_EQ(map.index_of(val1).value(), idx1);
    ASSERT_NE(map.value_at(idx1), nullptr);
    EXPECT_EQ(map.value_at(idx1)->id, 1);
    EXPECT_EQ(map.value_at(idx1)->data, "hello");


    size_t idx2 = map.insert(val2);
    ASSERT_EQ(idx2, 1);
    ASSERT_EQ(map.size(), 2);

    size_t idx1_check = map.insert(val1_again); // Should be idempotent
    ASSERT_EQ(idx1_check, idx1);
    ASSERT_EQ(map.size(), 2);

    ASSERT_TRUE(map.contains({1, "hello"}));
    ASSERT_FALSE(map.contains({3, "test"}));
}

TEST_F(ValueIndexMapTest, CopyMoveSemantics) {
    ValueIndexMap<std::string> map1;
    map1.insert("one");
    map1.insert("two");

    // Copy constructor
    ValueIndexMap<std::string> map2 = map1;
    ASSERT_EQ(map2.size(), 2);
    EXPECT_TRUE(map2.contains("one") && map2.contains("two"));
    ASSERT_EQ(map1.size(), 2); // map1 unchanged

    // Copy assignment
    ValueIndexMap<std::string> map3;
    map3.insert("temp");
    map3 = map1;
    ASSERT_EQ(map3.size(), 2);
    EXPECT_TRUE(map3.contains("one") && map3.contains("two"));
    ASSERT_EQ(map1.size(), 2); // map1 unchanged

    // Move constructor
    ValueIndexMap<std::string> map4 = std::move(map1);
    ASSERT_EQ(map4.size(), 2);
    EXPECT_TRUE(map4.contains("one") && map4.contains("two"));
    // map1 is in a valid but unspecified state (likely empty)
    EXPECT_TRUE(map1.empty() || map1.size()==0);

    // Re-initialize map1 for move assignment test
    map1.insert("three"); // map1 might be empty or already cleared by move. This ensures it has content.
    map1.insert("four");
    ASSERT_EQ(map1.size(), 2);

    // Move assignment
    ValueIndexMap<std::string> map5;
    map5.insert("another temp");
    map5 = std::move(map1);
    ASSERT_EQ(map5.size(), 2);
    EXPECT_TRUE(map5.contains("three") && map5.contains("four"));
    EXPECT_TRUE(map1.empty() || map1.size()==0);

    // Test sealed status with copy/move
    ValueIndexMap<int> sealed_orig;
    sealed_orig.insert(100);
    sealed_orig.seal();
    ASSERT_TRUE(sealed_orig.is_sealed());

    ValueIndexMap<int> sealed_copy_ctor = sealed_orig;
    EXPECT_TRUE(sealed_copy_ctor.is_sealed());
    ASSERT_EQ(sealed_copy_ctor.size(), 1);
    ASSERT_THROW(sealed_copy_ctor.insert(200), std::logic_error);

    ValueIndexMap<int> sealed_assign;
    sealed_assign = sealed_orig; // Assign from sealed_orig
    EXPECT_TRUE(sealed_assign.is_sealed());
    ASSERT_EQ(sealed_assign.size(), 1);
    ASSERT_THROW(sealed_assign.insert(200), std::logic_error);

    ValueIndexMap<int> sealed_move_ctor = std::move(sealed_copy_ctor); // move from a sealed copy
    EXPECT_TRUE(sealed_move_ctor.is_sealed());
    ASSERT_EQ(sealed_move_ctor.size(), 1);
    ASSERT_THROW(sealed_move_ctor.insert(300), std::logic_error);
    // sealed_copy_ctor should be empty and not sealed
    EXPECT_FALSE(sealed_copy_ctor.is_sealed());
    EXPECT_TRUE(sealed_copy_ctor.empty());


    ValueIndexMap<int> sealed_move_assign;
    sealed_move_assign = std::move(sealed_assign); // move from another sealed copy
    EXPECT_TRUE(sealed_move_assign.is_sealed());
    ASSERT_EQ(sealed_move_assign.size(), 1);
    ASSERT_THROW(sealed_move_assign.insert(300), std::logic_error);
    // sealed_assign should be empty and not sealed
    EXPECT_FALSE(sealed_assign.is_sealed());
    EXPECT_TRUE(sealed_assign.empty());
}

// It's good practice to have a main function in one of your test files,
// or link with gtest_main. CMake setup in tests/CMakeLists.txt handles this
// by linking with GTest::gtest_main.
// So, no main() function is needed here.

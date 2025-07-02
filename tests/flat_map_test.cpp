#include "gtest/gtest.h"
#include "flat_map.h" // Assuming flat_map.h is in the include path configured by CMake
#include <string>
#include <vector>
#include <algorithm> // For std::is_sorted
#include <stdexcept> // For std::out_of_range

// Test fixture for FlatMap tests
class FlatMapTest : public ::testing::Test {
protected:
    FlatMap<int, std::string> map_int_str;
    FlatMap<std::string, int, std::greater<std::string>> map_str_int_greater;
};

TEST_F(FlatMapTest, ConstructionAndBasicProperties) {
    FlatMap<int, std::string> map1;
    EXPECT_TRUE(map1.empty());
    EXPECT_EQ(0, map1.size());

    FlatMap<std::string, int, std::greater<std::string>> map2; // Custom comparator
    EXPECT_TRUE(map2.empty());
    EXPECT_EQ(0, map2.size());
}

TEST_F(FlatMapTest, InsertAndFind) {
    // Insert new elements
    EXPECT_TRUE(map_int_str.insert(10, "ten"));
    EXPECT_EQ(1, map_int_str.size());
    EXPECT_TRUE(map_int_str.insert(5, "five"));
    EXPECT_EQ(2, map_int_str.size());
    EXPECT_TRUE(map_int_str.insert(15, "fifteen"));
    EXPECT_EQ(3, map_int_str.size());

    // Check values using find
    ASSERT_NE(nullptr, map_int_str.find(10));
    EXPECT_EQ("ten", *map_int_str.find(10));
    ASSERT_NE(nullptr, map_int_str.find(5));
    EXPECT_EQ("five", *map_int_str.find(5));
    ASSERT_NE(nullptr, map_int_str.find(15));
    EXPECT_EQ("fifteen", *map_int_str.find(15));

    // Try to find non-existent key
    EXPECT_EQ(nullptr, map_int_str.find(100));

    // Insert (update) existing element
    EXPECT_FALSE(map_int_str.insert(10, "TEN_UPDATED"));
    EXPECT_EQ(3, map_int_str.size()); // Size should not change
    ASSERT_NE(nullptr, map_int_str.find(10));
    EXPECT_EQ("TEN_UPDATED", *map_int_str.find(10));
}

TEST_F(FlatMapTest, Contains) {
    map_int_str.insert(1, "1.1"); // Value type is string
    map_int_str.insert(2, "2.2");

    EXPECT_TRUE(map_int_str.contains(1));
    EXPECT_TRUE(map_int_str.contains(2));
    EXPECT_FALSE(map_int_str.contains(3));
}

TEST_F(FlatMapTest, At) {
    map_str_int_greater.insert("apple", 1); // Using the other map instance for variety
    map_str_int_greater.insert("banana", 2);

    EXPECT_EQ(1, map_str_int_greater.at("apple"));
    EXPECT_EQ(2, map_str_int_greater.at("banana"));
    map_str_int_greater.at("apple") = 100; // Modify using at()
    EXPECT_EQ(100, map_str_int_greater.at("apple"));

    EXPECT_THROW(map_str_int_greater.at("cherry"), std::out_of_range);
}

TEST_F(FlatMapTest, ConstAt) {
    map_int_str.insert(1, "one");
    map_int_str.insert(2, "two");
    const auto& const_map = map_int_str;

    EXPECT_EQ("one", const_map.at(1));
    EXPECT_EQ("two", const_map.at(2));
    EXPECT_THROW(const_map.at(3), std::out_of_range);
}


TEST_F(FlatMapTest, OperatorSquareBrackets) {
    // Access existing
    map_int_str.insert(1, "one");
    EXPECT_EQ("one", map_int_str[1]);
    map_int_str[1] = "ONE_MODIFIED";
    EXPECT_EQ("ONE_MODIFIED", map_int_str[1]);
    EXPECT_EQ(1, map_int_str.size());

    // Access non-existing (should insert default)
    EXPECT_EQ("", map_int_str[2]); // Default for std::string is ""
    EXPECT_EQ(2, map_int_str.size());
    EXPECT_TRUE(map_int_str.contains(2));
    EXPECT_EQ("", map_int_str.at(2));

    map_int_str[2] = "two";
    EXPECT_EQ("two", map_int_str[2]);

    FlatMap<int, int> int_map; // Test with default-constructible int
    int_map[5] = 50;
    EXPECT_EQ(50, int_map[5]);
    EXPECT_EQ(0, int_map[10]); // Default int is 0
    EXPECT_EQ(2, int_map.size());
}

TEST_F(FlatMapTest, Erase) {
    map_int_str.insert(10, "A");
    map_int_str.insert(20, "B");
    map_int_str.insert(30, "C");
    EXPECT_EQ(3, map_int_str.size());

    // Erase existing
    EXPECT_TRUE(map_int_str.erase(20));
    EXPECT_EQ(2, map_int_str.size());
    EXPECT_FALSE(map_int_str.contains(20));
    EXPECT_EQ(nullptr, map_int_str.find(20));

    // Erase non-existing
    EXPECT_FALSE(map_int_str.erase(100));
    EXPECT_EQ(2, map_int_str.size());

    // Erase remaining
    EXPECT_TRUE(map_int_str.erase(10));
    EXPECT_TRUE(map_int_str.erase(30));
    EXPECT_TRUE(map_int_str.empty());
}

TEST_F(FlatMapTest, IterationAndOrder) {
    map_int_str.insert(30, "thirty");
    map_int_str.insert(10, "ten");
    map_int_str.insert(40, "forty");
    map_int_str.insert(20, "twenty");

    std::vector<int> keys_retrieved;
    for (const auto& pair : map_int_str) {
        keys_retrieved.push_back(pair.first);
    }
    ASSERT_EQ(4, keys_retrieved.size());
    EXPECT_TRUE(std::is_sorted(keys_retrieved.begin(), keys_retrieved.end()));
    EXPECT_EQ(10, keys_retrieved[0]);
    EXPECT_EQ("ten", map_int_str.at(10));
    EXPECT_EQ(20, keys_retrieved[1]);
    EXPECT_EQ("twenty", map_int_str.at(20));
    EXPECT_EQ(30, keys_retrieved[2]);
    EXPECT_EQ("thirty", map_int_str.at(30));
    EXPECT_EQ(40, keys_retrieved[3]);
    EXPECT_EQ("forty", map_int_str.at(40));

    // Test const iteration
    const FlatMap<int, std::string>& const_map = map_int_str;
    keys_retrieved.clear();
    for (const auto& pair : const_map) {
        keys_retrieved.push_back(pair.first);
    }
    ASSERT_EQ(4, keys_retrieved.size());
    EXPECT_TRUE(std::is_sorted(keys_retrieved.begin(), keys_retrieved.end()));
}

TEST_F(FlatMapTest, ConstCorrectness) {
    map_int_str.insert(1, "10"); // string value
    map_int_str.insert(2, "20");

    const FlatMap<int, std::string>& const_map = map_int_str;

    // Test const find
    const std::string* val_ptr = const_map.find(1);
    ASSERT_NE(nullptr, val_ptr);
    EXPECT_EQ("10", *val_ptr);
    EXPECT_EQ(nullptr, const_map.find(3));

    // Test const at (already in ConstAt test, but good to have here too)
    EXPECT_EQ("20", const_map.at(2));
    EXPECT_THROW(const_map.at(3), std::out_of_range);

    // Test const contains
    EXPECT_TRUE(const_map.contains(1));
    EXPECT_FALSE(const_map.contains(3));

    // Test const iteration (already partially in test_iteration_and_order)
    std::string concatenated_values;
    for (const auto& pair : const_map) {
        concatenated_values += pair.second;
    }
    // Order is 10, 20. So "1020"
    EXPECT_EQ("1020", concatenated_values);

    EXPECT_EQ(2, const_map.size());
    EXPECT_FALSE(const_map.empty());
}

TEST_F(FlatMapTest, CustomComparator) {
    map_str_int_greater.insert("zebra", 10); // Using the fixture's custom comparator map
    map_str_int_greater.insert("apple", 20);
    map_str_int_greater.insert("monkey", 30);

    EXPECT_EQ(3, map_str_int_greater.size());
    EXPECT_EQ(10, map_str_int_greater.at("zebra"));

    std::vector<std::string> keys_retrieved;
    for(const auto& p : map_str_int_greater) {
        keys_retrieved.push_back(p.first);
    }
    // Check if sorted in descending order (std::greater for strings)
    ASSERT_EQ(3, keys_retrieved.size());
    EXPECT_EQ("zebra", keys_retrieved[0]);  // z > m > a
    EXPECT_EQ("monkey", keys_retrieved[1]);
    EXPECT_EQ("apple", keys_retrieved[2]);

    // Test operator[] with custom comparator
    map_str_int_greater["yak"] = 40; // Should insert according to std::greater
    EXPECT_EQ("zebra", map_str_int_greater.begin()->first); // yak comes before zebra with std::greater if we consider string comparison
                                                       // Actually, no, "yak" < "zebra" lexicographically.
                                                       // std::greater means larger elements come first.
                                                       // 'z' > 'y' > 'm' > 'a'

    // Let's re-verify the order with std::greater<std::string>
    // "zebra", "yak", "monkey", "apple"
    keys_retrieved.clear();
    for(const auto& p : map_str_int_greater) {
        keys_retrieved.push_back(p.first);
    }
    EXPECT_EQ("zebra", keys_retrieved[0]);
    EXPECT_EQ("yak", keys_retrieved[1]);
    EXPECT_EQ("monkey", keys_retrieved[2]);
    EXPECT_EQ("apple", keys_retrieved[3]);


    map_str_int_greater["cat"] = 5;
    keys_retrieved.clear();
    for(const auto& p : map_str_int_greater) {
        keys_retrieved.push_back(p.first);
    }
    // "zebra", "yak", "monkey", "cat", "apple"
    EXPECT_EQ("zebra", keys_retrieved[0]);
    EXPECT_EQ("yak", keys_retrieved[1]);
    EXPECT_EQ("monkey", keys_retrieved[2]);
    EXPECT_EQ("cat", keys_retrieved[3]);
    EXPECT_EQ("apple", keys_retrieved[4]);


    EXPECT_EQ("zebra", map_str_int_greater.begin()->first);
    EXPECT_EQ("apple", std::prev(map_str_int_greater.end())->first);
}

// It's good practice to have a main if not using gtest_main,
// but since tests/CMakeLists.txt links GTest::gtest_main, this is not strictly needed.
// However, to be absolutely sure it can compile independently for quick checks:
// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }

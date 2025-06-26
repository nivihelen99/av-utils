#include "gtest/gtest.h"
#include "unordered_multiset.h" // Assuming header is in include/
#include <string>
#include <vector>
#include <algorithm> // For std::sort, std::is_permutation (if needed for more complex checks)
#include <map> // For convenience in checking counts

// Test fixture for common setup
class UnorderedMultisetTest : public ::testing::Test {
protected:
    cpp_utils::UnorderedMultiset<int> ms_int;
    cpp_utils::UnorderedMultiset<std::string> ms_string;

    // Helper to get items and their counts for comparison
    template<typename T, typename Hash, typename KeyEqual>
    std::map<T, size_t> get_counts(const cpp_utils::UnorderedMultiset<T, Hash, KeyEqual>& ms) {
        std::map<T, size_t> counts;
        for (const auto& pair : ms) { // Iterates unique_key-count pairs from underlying map
            counts[pair.first] = pair.second;
        }
        return counts;
    }
};

// Test Cases

TEST_F(UnorderedMultisetTest, DefaultConstructor) {
    EXPECT_TRUE(ms_int.empty());
    EXPECT_EQ(ms_int.size(), 0);
    EXPECT_EQ(ms_int.begin(), ms_int.end());

    cpp_utils::UnorderedMultiset<std::string, std::hash<std::string>, std::equal_to<std::string>> ms_str_explicit_params;
    EXPECT_TRUE(ms_str_explicit_params.empty());
}

TEST_F(UnorderedMultisetTest, InsertAndCount) {
    ms_int.insert(10);
    EXPECT_EQ(ms_int.count(10), 1);
    EXPECT_EQ(ms_int.size(), 1);
    EXPECT_FALSE(ms_int.empty());

    ms_int.insert(10);
    EXPECT_EQ(ms_int.count(10), 2);
    EXPECT_EQ(ms_int.size(), 2);

    ms_int.insert(20);
    EXPECT_EQ(ms_int.count(10), 2);
    EXPECT_EQ(ms_int.count(20), 1);
    EXPECT_EQ(ms_int.size(), 3);

    EXPECT_EQ(ms_int.count(30), 0); // Non-existent
}

TEST_F(UnorderedMultisetTest, InsertRValue) {
    ms_string.insert("hello");
    EXPECT_EQ(ms_string.count("hello"), 1);
    ms_string.insert(std::string("world"));
    EXPECT_EQ(ms_string.count("world"), 1);
    ms_string.insert(std::string("hello"));
    EXPECT_EQ(ms_string.count("hello"), 2);
    EXPECT_EQ(ms_string.size(), 3);
}

TEST_F(UnorderedMultisetTest, Contains) {
    ms_int.insert(5);
    ms_int.insert(5);
    EXPECT_TRUE(ms_int.contains(5));
    EXPECT_FALSE(ms_int.contains(10));
}

TEST_F(UnorderedMultisetTest, EraseSingleInstance) {
    ms_int.insert(10);
    ms_int.insert(10);
    ms_int.insert(10);
    ms_int.insert(20);
    EXPECT_EQ(ms_int.size(), 4);
    EXPECT_EQ(ms_int.count(10), 3);

    size_t erased_count = ms_int.erase(10);
    EXPECT_EQ(erased_count, 1);
    EXPECT_EQ(ms_int.count(10), 2);
    EXPECT_EQ(ms_int.size(), 3);
    EXPECT_TRUE(ms_int.contains(10));

    erased_count = ms_int.erase(10);
    EXPECT_EQ(erased_count, 1);
    EXPECT_EQ(ms_int.count(10), 1);
    EXPECT_EQ(ms_int.size(), 2);

    erased_count = ms_int.erase(10);
    EXPECT_EQ(erased_count, 1);
    EXPECT_EQ(ms_int.count(10), 0);
    EXPECT_FALSE(ms_int.contains(10));
    EXPECT_EQ(ms_int.size(), 1); // Only 20 should be left

    erased_count = ms_int.erase(10); // Erase non-existent (already fully erased)
    EXPECT_EQ(erased_count, 0);
    EXPECT_EQ(ms_int.count(10), 0);
    EXPECT_EQ(ms_int.size(), 1);

    erased_count = ms_int.erase(30); // Erase completely non-existent
    EXPECT_EQ(erased_count, 0);
    EXPECT_EQ(ms_int.size(), 1);

    EXPECT_EQ(ms_int.count(20), 1);
}

TEST_F(UnorderedMultisetTest, EraseAllInstances) {
    ms_string.insert("apple");
    ms_string.insert("banana");
    ms_string.insert("apple");
    ms_string.insert("orange");
    ms_string.insert("apple");
    // State: apple:3, banana:1, orange:1. Total size: 5
    EXPECT_EQ(ms_string.size(), 5);
    EXPECT_EQ(ms_string.count("apple"), 3);

    size_t erased_count = ms_string.erase_all("apple");
    EXPECT_EQ(erased_count, 3);
    EXPECT_EQ(ms_string.count("apple"), 0);
    EXPECT_FALSE(ms_string.contains("apple"));
    EXPECT_EQ(ms_string.size(), 2); // banana and orange left

    erased_count = ms_string.erase_all("apple"); // Already erased
    EXPECT_EQ(erased_count, 0);
    EXPECT_EQ(ms_string.size(), 2);

    erased_count = ms_string.erase_all("grape"); // Non-existent
    EXPECT_EQ(erased_count, 0);
    EXPECT_EQ(ms_string.size(), 2);

    EXPECT_TRUE(ms_string.contains("banana"));
    EXPECT_TRUE(ms_string.contains("orange"));
}

TEST_F(UnorderedMultisetTest, Clear) {
    ms_int.insert(1);
    ms_int.insert(1);
    ms_int.insert(2);
    EXPECT_FALSE(ms_int.empty());
    EXPECT_EQ(ms_int.size(), 3);

    ms_int.clear();
    EXPECT_TRUE(ms_int.empty());
    EXPECT_EQ(ms_int.size(), 0);
    EXPECT_EQ(ms_int.count(1), 0);
    EXPECT_EQ(ms_int.count(2), 0);
    EXPECT_EQ(ms_int.begin(), ms_int.end());
}

TEST_F(UnorderedMultisetTest, Iteration) {
    ms_string.insert("a");
    ms_string.insert("b");
    ms_string.insert("a");
    ms_string.insert("c");
    ms_string.insert("b");
    ms_string.insert("a");
    // Expected counts: a:3, b:2, c:1. Total size: 6

    std::map<std::string, size_t> expected_counts = {{"a", 3}, {"b", 2}, {"c", 1}};
    std::map<std::string, size_t> actual_counts = get_counts(ms_string);

    EXPECT_EQ(actual_counts.size(), 3); // 3 unique elements
    EXPECT_EQ(actual_counts, expected_counts);

    size_t unique_elements_iterated = 0;
    for (auto it = ms_string.begin(); it != ms_string.end(); ++it) {
        unique_elements_iterated++;
        EXPECT_TRUE(expected_counts.count(it->first)); // key from map iterator
        EXPECT_EQ(it->second, expected_counts.at(it->first)); // value from map iterator is the count
    }
    EXPECT_EQ(unique_elements_iterated, expected_counts.size());

    // Const iteration
    const auto& const_ms_string = ms_string;
    actual_counts.clear();
     for (const auto& pair : const_ms_string) {
        actual_counts[pair.first] = pair.second;
    }
    EXPECT_EQ(actual_counts, expected_counts);
}

TEST_F(UnorderedMultisetTest, EmptySetOperations) {
    EXPECT_TRUE(ms_int.empty());
    EXPECT_EQ(ms_int.size(), 0);
    EXPECT_EQ(ms_int.count(123), 0);
    EXPECT_FALSE(ms_int.contains(123));
    EXPECT_EQ(ms_int.erase(123), 0);
    EXPECT_EQ(ms_int.erase_all(123), 0);
    EXPECT_EQ(ms_int.begin(), ms_int.end());
}

TEST_F(UnorderedMultisetTest, SwapMember) {
    ms_int.insert(1);
    ms_int.insert(1);
    ms_int.insert(2);

    cpp_utils::UnorderedMultiset<int> other_ms;
    other_ms.insert(30);
    other_ms.insert(40);
    other_ms.insert(40);
    other_ms.insert(40);

    auto counts1_orig = get_counts(ms_int);
    size_t size1_orig = ms_int.size();
    auto counts2_orig = get_counts(other_ms);
    size_t size2_orig = other_ms.size();

    ms_int.swap(other_ms);

    EXPECT_EQ(get_counts(ms_int), counts2_orig);
    EXPECT_EQ(ms_int.size(), size2_orig);
    EXPECT_EQ(get_counts(other_ms), counts1_orig);
    EXPECT_EQ(other_ms.size(), size1_orig);
}

TEST_F(UnorderedMultisetTest, SwapNonMember) {
    ms_string.insert("x");
    ms_string.insert("y");
    ms_string.insert("x");

    cpp_utils::UnorderedMultiset<std::string> other_ms_str;
    other_ms_str.insert("a");
    other_ms_str.insert("b");

    auto counts1_orig = get_counts(ms_string);
    size_t size1_orig = ms_string.size();
    auto counts2_orig = get_counts(other_ms_str);
    size_t size2_orig = other_ms_str.size();

    swap(ms_string, other_ms_str); // Calls non-member swap

    EXPECT_EQ(get_counts(ms_string), counts2_orig);
    EXPECT_EQ(ms_string.size(), size2_orig);
    EXPECT_EQ(get_counts(other_ms_str), counts1_orig);
    EXPECT_EQ(other_ms_str.size(), size1_orig);
}

// Test with a custom struct and custom hash/equal
struct MyData {
    int id;
    std::string name;

    bool operator==(const MyData& other) const {
        return id == other.id && name == other.name;
    }
};

struct MyDataHash {
    std::size_t operator()(const MyData& d) const {
        return std::hash<int>()(d.id) ^ (std::hash<std::string>()(d.name) << 1);
    }
};

TEST_F(UnorderedMultisetTest, CustomTypeAndHash) {
    cpp_utils::UnorderedMultiset<MyData, MyDataHash> ms_custom;
    MyData d1{1, "Alice"};
    MyData d2{2, "Bob"};
    MyData d1_again{1, "Alice"};

    ms_custom.insert(d1);
    ms_custom.insert(d2);
    ms_custom.insert(d1_again); // Same as d1

    EXPECT_EQ(ms_custom.size(), 3);
    EXPECT_EQ(ms_custom.count(d1), 2);
    EXPECT_EQ(ms_custom.count(d2), 1);
    EXPECT_TRUE(ms_custom.contains(MyData{1, "Alice"}));

    ms_custom.erase(MyData{1, "Alice"});
    EXPECT_EQ(ms_custom.count(d1), 1);
    EXPECT_EQ(ms_custom.size(), 2);

    ms_custom.erase_all(d1);
    EXPECT_EQ(ms_custom.count(d1), 0);
    EXPECT_EQ(ms_custom.size(), 1); // Only d2 left
    EXPECT_TRUE(ms_custom.contains(d2));
}

TEST_F(UnorderedMultisetTest, ConstructorWithHashAndEqual) {
    MyDataHash hasher;
    std::equal_to<MyData> equator; // Default is fine, but can be explicit
    cpp_utils::UnorderedMultiset<MyData, MyDataHash, std::equal_to<MyData>> ms_custom_explicit(hasher, equator);

    MyData d1{1, "Test"};
    ms_custom_explicit.insert(d1);
    EXPECT_EQ(ms_custom_explicit.count(d1), 1);
    EXPECT_TRUE(ms_custom_explicit.contains(d1));
}

/*
// Optional: Main function for running tests if not part of a larger test runner
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
*/

// For GTest printing of MyData if needed in more complex EXPECT_EQ failures.
// std::ostream& operator<<(std::ostream& os, const MyData& d) {
//     return os << "MyData{id=" << d.id << ", name=\"" << d.name << "\"}";
// }

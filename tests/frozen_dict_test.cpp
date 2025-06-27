#include "FrozenDict.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <map> // For ordered input to test construction

// Custom types for testing
struct TestKey {
    int id;
    std::string name;

    TestKey(int i = 0, std::string n = "") : id(i), name(std::move(n)) {}

    bool operator==(const TestKey& other) const {
        return id == other.id && name == other.name;
    }
    // For std::map and FrozenDict's internal sorting
    bool operator<(const TestKey& other) const {
        if (id != other.id) return id < other.id;
        return name < other.name;
    }
};

struct TestValue {
    double val;
    std::string desc;

    TestValue(double v = 0.0, std::string d = "") : val(v), desc(std::move(d)) {}

    bool operator==(const TestValue& other) const {
        return val == other.val && desc == other.desc;
    }
};

// Hash specializations for custom types
namespace std {
template <>
struct hash<TestKey> {
    std::size_t operator()(const TestKey& k) const {
        return std::hash<int>{}(k.id) ^ (std::hash<std::string>{}(k.name) << 1);
    }
};

template <>
struct hash<TestValue> {
    std::size_t operator()(const TestValue& v) const {
        return std::hash<double>{}(v.val) ^ (std::hash<std::string>{}(v.desc) << 1);
    }
};
} // namespace std

// Define types for easier use in tests
using FD_StringInt = cpp_collections::FrozenDict<std::string, int>;
using FD_IntString = cpp_collections::FrozenDict<int, std::string>;
// Explicitly define TestKey related FrozenDict with necessary functors
using FD_TestKeyTestValue = cpp_collections::FrozenDict<TestKey, TestValue, std::hash<TestKey>, std::equal_to<TestKey>, std::less<TestKey>>;


TEST(FrozenDictTest, DefaultConstructor) {
    FD_StringInt fd;
    EXPECT_TRUE(fd.empty());
    EXPECT_EQ(fd.size(), 0);
}

TEST(FrozenDictTest, InitializerListConstructor) {
    FD_StringInt fd = {{"apple", 1}, {"banana", 2}, {"cherry", 3}};
    EXPECT_FALSE(fd.empty());
    EXPECT_EQ(fd.size(), 3);
    EXPECT_EQ(fd.at("apple"), 1);
    EXPECT_EQ(fd["banana"], 2); // Test operator[]
    EXPECT_TRUE(fd.contains("cherry"));
    EXPECT_FALSE(fd.contains("date"));
}

TEST(FrozenDictTest, InitializerListConstructorDuplicateKeys) {
    // "Last one wins" policy is implemented by build_from_range using std::map
    FD_StringInt fd = {{"apple", 1}, {"banana", 2}, {"apple", 100}};
    EXPECT_EQ(fd.size(), 2); // "apple" should only appear once
    EXPECT_EQ(fd.at("apple"), 100); // Last value for "apple" should be kept
    EXPECT_EQ(fd.at("banana"), 2);
}

TEST(FrozenDictTest, IteratorConstructor) {
    std::vector<std::pair<const std::string, int>> source_vector = {{"one", 1}, {"two", 2}, {"three", 3}};
    FD_StringInt fd(source_vector.begin(), source_vector.end());
    EXPECT_EQ(fd.size(), 3);
    EXPECT_EQ(fd.at("one"), 1);
    EXPECT_EQ(fd.at("two"), 2);

    std::map<std::string, int> source_map = {{"a", 10}, {"b", 20}};
    FD_StringInt fd_from_map(source_map.begin(), source_map.end());
    EXPECT_EQ(fd_from_map.size(), 2);
    EXPECT_EQ(fd_from_map.at("a"), 10);
}

TEST(FrozenDictTest, CopyConstructor) {
    FD_StringInt fd1 = {{"a", 1}, {"b", 2}};
    FD_StringInt fd2 = fd1;
    EXPECT_EQ(fd1.size(), fd2.size());
    EXPECT_EQ(fd1.at("a"), fd2.at("a"));
    EXPECT_EQ(fd1.at("b"), fd2.at("b"));
    EXPECT_EQ(fd1, fd2);
}

TEST(FrozenDictTest, MoveConstructor) {
    FD_StringInt fd1 = {{"a", 1}, {"b", 2}};
    FD_StringInt fd_temp = fd1; // To verify content
    FD_StringInt fd2 = std::move(fd1);

    EXPECT_EQ(fd2.size(), fd_temp.size());
    EXPECT_EQ(fd2.at("a"), fd_temp.at("a"));
    EXPECT_EQ(fd2.at("b"), fd_temp.at("b"));
    // fd1 is in a valid but unspecified state after move
    EXPECT_TRUE(fd1.empty() || fd1.size() == 0); // Typical post-move state for vector-based
}

TEST(FrozenDictTest, CopyAssignment) {
    FD_StringInt fd1 = {{"a", 1}, {"b", 2}};
    FD_StringInt fd2;
    fd2 = fd1;
    EXPECT_EQ(fd1.size(), fd2.size());
    EXPECT_EQ(fd1.at("a"), fd2.at("a"));
    EXPECT_EQ(fd1.at("b"), fd2.at("b"));
    EXPECT_EQ(fd1, fd2);
}

TEST(FrozenDictTest, MoveAssignment) {
    FD_StringInt fd1 = {{"a", 1}, {"b", 2}};
    FD_StringInt fd_temp = fd1;
    FD_StringInt fd2;
    fd2 = std::move(fd1);

    EXPECT_EQ(fd2.size(), fd_temp.size());
    EXPECT_EQ(fd2.at("a"), fd_temp.at("a"));
    EXPECT_EQ(fd2.at("b"), fd_temp.at("b"));
    EXPECT_TRUE(fd1.empty() || fd1.size() == 0);
}


TEST(FrozenDictTest, LookupMethods) {
    FD_IntString fd = {{1, "one"}, {2, "two"}, {3, "three"}};

    // at()
    EXPECT_EQ(fd.at(1), "one");
    EXPECT_THROW(fd.at(4), std::out_of_range);

    // operator[]
    EXPECT_EQ(fd[2], "two");
    EXPECT_THROW(fd[5], std::out_of_range);

    // count()
    EXPECT_EQ(fd.count(1), 1);
    EXPECT_EQ(fd.count(4), 0);

    // contains()
    EXPECT_TRUE(fd.contains(3));
    EXPECT_FALSE(fd.contains(0));

    // find()
    auto it_found = fd.find(1);
    ASSERT_NE(it_found, fd.end());
    EXPECT_EQ(it_found->first, 1);
    EXPECT_EQ(it_found->second, "one");

    auto it_not_found = fd.find(10);
    EXPECT_EQ(it_not_found, fd.end());
}

TEST(FrozenDictTest, Iteration) {
    FD_StringInt fd = {{"gamma", 30}, {"alpha", 10}, {"beta", 20}};
    // Internal data is sorted by key: alpha, beta, gamma

    std::vector<std::pair<std::string, int>> expected_order = {
        {"alpha", 10}, {"beta", 20}, {"gamma", 30}
    };

    size_t i = 0;
    for (const auto& pair : fd) { // Uses fd.begin(), fd.end()
        ASSERT_LT(i, expected_order.size());
        EXPECT_EQ(pair.first, expected_order[i].first);
        EXPECT_EQ(pair.second, expected_order[i].second);
        i++;
    }
    EXPECT_EQ(i, expected_order.size());
}

TEST(FrozenDictTest, ComparisonOperators) {
    FD_StringInt fd1 = {{"a", 1}, {"b", 2}};
    FD_StringInt fd2 = {{"b", 2}, {"a", 1}}; // Same elements, different initializer order
    FD_StringInt fd3 = {{"a", 1}, {"c", 3}};
    FD_StringInt fd4 = {{"a", 1}};
    FD_StringInt fd_empty1;
    FD_StringInt fd_empty2;

    EXPECT_TRUE(fd1 == fd2);
    EXPECT_FALSE(fd1 != fd2);

    EXPECT_FALSE(fd1 == fd3);
    EXPECT_TRUE(fd1 != fd3);

    EXPECT_FALSE(fd1 == fd4);
    EXPECT_TRUE(fd1 != fd4);

    EXPECT_TRUE(fd_empty1 == fd_empty2);
    EXPECT_FALSE(fd1 == fd_empty1);
}

TEST(FrozenDictTest, StdHashAndUnorderedMap) {
    using FD_KeyType = cpp_collections::FrozenDict<std::string, int>;
    std::unordered_map<FD_KeyType, std::string> map_with_fd_keys;

    FD_KeyType fd1 = {{"key1", 10}, {"key2", 20}};
    FD_KeyType fd2 = {{"key2", 20}, {"key1", 10}}; // Same content as fd1
    FD_KeyType fd3 = {{"another", 30}};

    map_with_fd_keys[fd1] = "Data for fd1";

    EXPECT_TRUE(map_with_fd_keys.count(fd1));
    EXPECT_TRUE(map_with_fd_keys.count(fd2)); // fd2 should hash and compare equal to fd1
    EXPECT_EQ(map_with_fd_keys.at(fd1), "Data for fd1");
    EXPECT_EQ(map_with_fd_keys.at(fd2), "Data for fd1");

    map_with_fd_keys[fd3] = "Data for fd3";
    EXPECT_TRUE(map_with_fd_keys.count(fd3));
    EXPECT_EQ(map_with_fd_keys.at(fd3), "Data for fd3");

    EXPECT_EQ(map_with_fd_keys.size(), 2); // fd1 and fd3 are distinct keys

    // Test hash values directly
    std::hash<FD_KeyType> hasher;
    EXPECT_EQ(hasher(fd1), hasher(fd2));
    if (fd1.size() > 0 && fd3.size() > 0) { // Only expect neq if both are non-empty and different
         EXPECT_NE(hasher(fd1), hasher(fd3));
    }
}

TEST(FrozenDictTest, CustomTypes) {
    FD_TestKeyTestValue fd(
        {
            {TestKey(1, "one"), TestValue(1.1, "val_one")},
            {TestKey(2, "two"), TestValue(2.2, "val_two")}
        },
        std::less<TestKey>(),      // Explicitly pass comparator instance
        std::hash<TestKey>()       // Explicitly pass hasher instance
    );

    EXPECT_EQ(fd.size(), 2);
    EXPECT_TRUE(fd.contains(TestKey(1, "one")));
    EXPECT_EQ(fd.at(TestKey(2, "two")).val, 2.2);

    // Test hashing with custom types
    std::unordered_map<FD_TestKeyTestValue, int> map_custom_fd_keys;
    map_custom_fd_keys[fd] = 100;
    EXPECT_TRUE(map_custom_fd_keys.count(fd));
    EXPECT_EQ(map_custom_fd_keys.at(fd), 100);
}

TEST(FrozenDictTest, EmptyInputConstruction) {
    std::vector<std::pair<const std::string, int>> empty_vec;
    FD_StringInt fd_from_empty_vec(empty_vec.begin(), empty_vec.end());
    EXPECT_TRUE(fd_from_empty_vec.empty());
    EXPECT_EQ(fd_from_empty_vec.size(), 0);

    FD_StringInt fd_from_empty_init = {};
    EXPECT_TRUE(fd_from_empty_init.empty());
    EXPECT_EQ(fd_from_empty_init.size(), 0);
}

TEST(FrozenDictTest, GetAllocator) {
    std::allocator<std::pair<const std::string, int>> alloc;
    FD_StringInt fd(alloc);
    EXPECT_EQ(fd.get_allocator(), alloc);
}

TEST(FrozenDictTest, KeyComp) {
    // Using default std::less
    FD_StringInt fd1;
    EXPECT_NO_THROW(fd1.key_comp());

    // Using custom comparator
    auto custom_comp = [](const std::string& a, const std::string& b) {
        return a.length() < b.length(); // Sort by length
    };
    cpp_collections::FrozenDict<std::string, int, std::hash<std::string>, std::equal_to<std::string>, decltype(custom_comp)> fd2({}, custom_comp);

    FD_StringInt fd_comp_test = {{"bbb",1}, {"a",2}, {"cc",3}}; // Expected order with default comp: a, bbb, cc
    auto it = fd_comp_test.begin();
    EXPECT_EQ(it->first, "a"); ++it;
    EXPECT_EQ(it->first, "bbb"); ++it;
    EXPECT_EQ(it->first, "cc");
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#include "gtest/gtest.h"
#include "group_by_consecutive.hpp" // Adjust path as needed by CMake include directories
#include <vector>
#include <string>
#include <utility> // For std::pair

// Test fixture for common data types or setup if needed later, not strictly necessary for now
class GroupByConsecutiveTest : public ::testing::Test {
protected:
    // Per-test set-up logic, if any
    void SetUp() override {}

    // Per-test tear-down logic, if any
    void TearDown() override {}
};

// Test case: Empty input container
TEST_F(GroupByConsecutiveTest, HandlesEmptyInput) {
    std::vector<std::pair<char, int>> data = {};
    auto groups = utils::group_by_consecutive(data, [](const auto& p) { return p.first; });
    EXPECT_TRUE(groups.empty());

    std::vector<int> numbers = {};
    auto groups_numbers = utils::group_by_consecutive(numbers, [](int n){ return n; });
    EXPECT_TRUE(groups_numbers.empty());
}

// Test case: All items with the same key
TEST_F(GroupByConsecutiveTest, HandlesAllSameKey) {
    std::vector<std::pair<char, int>> data = {
        {'a', 1}, {'a', 2}, {'a', 3}
    };
    auto groups = utils::group_by_consecutive(data, [](const auto& p) { return p.first; });

    ASSERT_EQ(groups.size(), 1);
    EXPECT_EQ(groups[0].first, 'a');
    ASSERT_EQ(groups[0].second.size(), 3);
    EXPECT_EQ(groups[0].second[0].second, 1);
    EXPECT_EQ(groups[0].second[1].second, 2);
    EXPECT_EQ(groups[0].second[2].second, 3);
}

// Test case: All items with different keys
TEST_F(GroupByConsecutiveTest, HandlesAllDifferentKeys) {
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    auto groups = utils::group_by_consecutive(numbers.begin(), numbers.end(), [](int n) { return n; });

    ASSERT_EQ(groups.size(), 5);
    for (size_t i = 0; i < groups.size(); ++i) {
        EXPECT_EQ(groups[i].first, numbers[i]);
        ASSERT_EQ(groups[i].second.size(), 1);
        EXPECT_EQ(groups[i].second[0], numbers[i]);
    }
}

// Test case: Mixed alternating patterns (original example)
TEST_F(GroupByConsecutiveTest, HandlesMixedAlternatingPattern) {
    std::vector<std::pair<char, int>> data = {
        {'a', 1}, {'a', 2}, {'b', 3}, {'b', 4}, {'a', 5}
    };
    auto groups = utils::group_by_consecutive(data, [](const auto& p) { return p.first; });

    ASSERT_EQ(groups.size(), 3);

    EXPECT_EQ(groups[0].first, 'a');
    ASSERT_EQ(groups[0].second.size(), 2);
    EXPECT_EQ(groups[0].second[0].second, 1);
    EXPECT_EQ(groups[0].second[1].second, 2);

    EXPECT_EQ(groups[1].first, 'b');
    ASSERT_EQ(groups[1].second.size(), 2);
    EXPECT_EQ(groups[1].second[0].second, 3);
    EXPECT_EQ(groups[1].second[1].second, 4);

    EXPECT_EQ(groups[2].first, 'a');
    ASSERT_EQ(groups[2].second.size(), 1);
    EXPECT_EQ(groups[2].second[0].second, 5);
}

// Test case: Custom key function (struct with a member function pointer as key)
struct MyData {
    int id;
    std::string type;
    double val;

    const std::string& getType() const { return type; }
};

TEST_F(GroupByConsecutiveTest, HandlesCustomKeyFunctionStructMethod) {
    std::vector<MyData> items = {
        {1, "type1", 10.0}, {2, "type1", 12.0},
        {3, "type2", 20.0},
        {4, "type1", 15.0}, {5, "type1", 18.0}
    };

    // Using a lambda to call the member function
    auto groups = utils::group_by_consecutive(items, [](const MyData& md){ return md.getType(); });

    ASSERT_EQ(groups.size(), 3);

    EXPECT_EQ(groups[0].first, "type1");
    ASSERT_EQ(groups[0].second.size(), 2);
    EXPECT_EQ(groups[0].second[0].id, 1);
    EXPECT_EQ(groups[0].second[1].id, 2);

    EXPECT_EQ(groups[1].first, "type2");
    ASSERT_EQ(groups[1].second.size(), 1);
    EXPECT_EQ(groups[1].second[0].id, 3);

    EXPECT_EQ(groups[2].first, "type1");
    ASSERT_EQ(groups[2].second.size(), 2);
    EXPECT_EQ(groups[2].second[0].id, 4);
    EXPECT_EQ(groups[2].second[1].id, 5);
}

// Test case: Grouping strings by length
TEST_F(GroupByConsecutiveTest, HandlesGroupingStringsByLength) {
    std::vector<std::string> words = {"a", "b", "cc", "dd", "eee", "f", "gg"};
    auto groups = utils::group_by_consecutive(words, [](const std::string& s){ return s.length(); });

    ASSERT_EQ(groups.size(), 5);

    EXPECT_EQ(groups[0].first, 1); // "a", "b"
    ASSERT_EQ(groups[0].second.size(), 2);
    EXPECT_EQ(groups[0].second[0], "a");
    EXPECT_EQ(groups[0].second[1], "b");

    EXPECT_EQ(groups[1].first, 2); // "cc", "dd"
    ASSERT_EQ(groups[1].second.size(), 2);
    EXPECT_EQ(groups[1].second[0], "cc");
    EXPECT_EQ(groups[1].second[1], "dd");

    EXPECT_EQ(groups[2].first, 3); // "eee"
    ASSERT_EQ(groups[2].second.size(), 1);
    EXPECT_EQ(groups[2].second[0], "eee");

    EXPECT_EQ(groups[3].first, 1); // "f"
    ASSERT_EQ(groups[3].second.size(), 1);
    EXPECT_EQ(groups[3].second[0], "f");

    EXPECT_EQ(groups[4].first, 2); // "gg"
    ASSERT_EQ(groups[4].second.size(), 1);
    EXPECT_EQ(groups[4].second[0], "gg");
}


// Test with a stateful key function (using a lambda with captures)
TEST_F(GroupByConsecutiveTest, StatefulKeyFunction) {
    std::vector<int> data = {1, 2, 3, 10, 11, 12, 20, 21};
    // int group_size = 0; // Not needed for this version of the test
    // int group_id_counter = 0; // Not needed for this version of the test

    auto key_func_decade = [](int item) {
        return item / 10;
    };

    auto groups = utils::group_by_consecutive(data, key_func_decade);

    ASSERT_EQ(groups.size(), 3);

    EXPECT_EQ(groups[0].first, 0); // for 1, 2, 3
    ASSERT_EQ(groups[0].second.size(), 3);
    EXPECT_EQ(groups[0].second[0], 1);
    EXPECT_EQ(groups[0].second[1], 2);
    EXPECT_EQ(groups[0].second[2], 3);

    EXPECT_EQ(groups[1].first, 1); // for 10, 11, 12
    ASSERT_EQ(groups[1].second.size(), 3);
    EXPECT_EQ(groups[1].second[0], 10);
    EXPECT_EQ(groups[1].second[1], 11);
    EXPECT_EQ(groups[1].second[2], 12);

    EXPECT_EQ(groups[2].first, 2); // for 20, 21
    ASSERT_EQ(groups[2].second.size(), 2);
    EXPECT_EQ(groups[2].second[0], 20);
    EXPECT_EQ(groups[2].second[1], 21);
}

// Test for container-based overload
TEST_F(GroupByConsecutiveTest, ContainerOverload) {
    std::vector<std::pair<char, int>> data = {
        {'x', 1}, {'x', 2}, {'y', 3}, {'x', 4}
    };
    auto groups = utils::group_by_consecutive(data, [](const auto& p) { return p.first; });

    ASSERT_EQ(groups.size(), 3);

    EXPECT_EQ(groups[0].first, 'x');
    ASSERT_EQ(groups[0].second.size(), 2);
    EXPECT_EQ(groups[0].second[0].second, 1);
    EXPECT_EQ(groups[0].second[1].second, 2);

    EXPECT_EQ(groups[1].first, 'y');
    ASSERT_EQ(groups[1].second.size(), 1);
    EXPECT_EQ(groups[1].second[0].second, 3);

    EXPECT_EQ(groups[2].first, 'x');
    ASSERT_EQ(groups[2].second.size(), 1);
    EXPECT_EQ(groups[2].second[0].second, 4);
}


// It's good practice to also test with iterators explicitly if the container one is just a wrapper
TEST_F(GroupByConsecutiveTest, IteratorOverloadExplicit) {
    std::vector<std::pair<char, int>> data = {
        {'x', 1}, {'x', 2}, {'y', 3}, {'x', 4}
    };
    auto groups = utils::group_by_consecutive(data.begin(), data.end(), [](const auto& p) { return p.first; });

    ASSERT_EQ(groups.size(), 3);
    EXPECT_EQ(groups[0].first, 'x');
    ASSERT_EQ(groups[0].second.size(), 2);
    EXPECT_EQ(groups[1].first, 'y');
    ASSERT_EQ(groups[1].second.size(), 1);
    EXPECT_EQ(groups[2].first, 'x');
    ASSERT_EQ(groups[2].second.size(), 1);
}

// Consider a case where the key function returns a reference
struct ComplexKey {
    std::string key_val;
    bool operator==(const ComplexKey& other) const { return key_val == other.key_val; }
    ComplexKey(const std::string& kv) : key_val(kv) {}
    ComplexKey(const ComplexKey& other) : key_val(other.key_val) {}
    ComplexKey(ComplexKey&& other) noexcept : key_val(std::move(other.key_val)) {}
    ComplexKey& operator=(const ComplexKey& other) {
        key_val = other.key_val;
        return *this;
    }
    ComplexKey& operator=(ComplexKey&& other) noexcept {
        key_val = std::move(other.key_val);
        return *this;
    }
};

std::ostream& operator<<(std::ostream& os, const ComplexKey& ck) {
    return os << ck.key_val;
}

namespace std {
    template <> struct hash<ComplexKey> {
        std::size_t operator()(const ComplexKey& ck) const {
            return std::hash<std::string>()(ck.key_val);
        }
    };
}

TEST_F(GroupByConsecutiveTest, KeyFunctionReturnsReference) {
    std::vector<std::pair<ComplexKey, int>> data = {
        {ComplexKey("key1"), 1}, {ComplexKey("key1"), 2}, {ComplexKey("key2"), 3}
    };

    auto groups = utils::group_by_consecutive(data, [](const std::pair<ComplexKey, int>& p){
        return p.first;
    });

    ASSERT_EQ(groups.size(), 2);
    EXPECT_EQ(groups[0].first.key_val, "key1");
    ASSERT_EQ(groups[0].second.size(), 2);
    EXPECT_EQ(groups[0].second[0].second, 1);
    EXPECT_EQ(groups[0].second[1].second, 2);

    EXPECT_EQ(groups[1].first.key_val, "key2");
    ASSERT_EQ(groups[1].second.size(), 1);
    EXPECT_EQ(groups[1].second[0].second, 3);
}

// main function is not needed here, as GTest::gtest_main is linked,
// both for the aggregate 'run_tests' target and individual test executables.
// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }

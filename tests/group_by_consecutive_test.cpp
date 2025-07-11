#include "gtest/gtest.h"
#include "group_by_consecutive.h" // The header to test
#include <vector>
#include <string>
#include <list> // To test with other iterator types
#include <cmath> // For std::abs
#include <ostream> // For custom struct printing in GTest

// Helper to compare vectors of groups
template <typename Key, typename Value>
void ExpectGroupsEqual(const std::vector<cpp_collections::Group<Key, Value>>& actual,
                       const std::vector<cpp_collections::Group<Key, Value>>& expected) {
    ASSERT_EQ(actual.size(), expected.size()) << "Number of groups differ.";
    for (size_t i = 0; i < actual.size(); ++i) {
        EXPECT_EQ(actual[i].key, expected[i].key) << "Mismatch in key for group " << i;
        ASSERT_EQ(actual[i].items.size(), expected[i].items.size()) << "Mismatch in item count for group " << i << " with key " << actual[i].key;
        for(size_t j = 0; j < actual[i].items.size(); ++j) {
            EXPECT_EQ(actual[i].items[j], expected[i].items[j]) << "Mismatch in item " << j << " for group " << i << " with key " << actual[i].key;
        }
    }
}

// Test fixture for GroupByConsecutive tests
class GroupByConsecutiveTest : public ::testing::Test {
protected:
    // Simple struct for testing with objects
    struct TestObject {
        int id;
        std::string category;
        double value;

        bool operator==(const TestObject& other) const {
            return id == other.id && category == other.category && value == other.value;
        }
        // For GTest printing
        friend std::ostream& operator<<(std::ostream& os, const TestObject& obj) {
            return os << "{id:" << obj.id << ", cat:" << obj.category << ", val:" << obj.value << "}";
        }
    };

    // Key extractor for TestObject's category
    struct CategoryExtractor {
        const std::string& operator()(const TestObject& obj) const {
            return obj.category;
        }
    };

    // Predicate for TestObject's value proximity
    // True if o2 is "close" to o1 (previous element)
    struct ValueProximityPredicate {
        bool operator()(const TestObject& o1_prev, const TestObject& o2_curr) const {
            return std::abs(o1_prev.value - o2_curr.value) < 0.5;
        }
    };
};

// --- Tests for group_by_consecutive (Key Function Version) ---

TEST_F(GroupByConsecutiveTest, KeyFunc_EmptyRange) {
    std::vector<int> input = {};
    auto getKey = [](int x){ return x; };
    auto result = cpp_collections::group_by_consecutive(input.begin(), input.end(), getKey);
    std::vector<cpp_collections::Group<int, int>> expected = {};
    ExpectGroupsEqual(result, expected);
}

TEST_F(GroupByConsecutiveTest, KeyFunc_AllUniqueElements) {
    std::vector<int> input = {1, 2, 3, 4, 5};
    auto getKey = [](int x){ return x; };
    auto result = cpp_collections::group_by_consecutive(input.begin(), input.end(), getKey);
    std::vector<cpp_collections::Group<int, int>> expected = {
        {1, {1}}, {2, {2}}, {3, {3}}, {4, {4}}, {5, {5}}
    };
    ExpectGroupsEqual(result, expected);
}

TEST_F(GroupByConsecutiveTest, KeyFunc_AllSameElements) {
    std::vector<int> input = {7, 7, 7, 7};
    auto getKey = [](int x){ return x; };
    auto result = cpp_collections::group_by_consecutive(input.begin(), input.end(), getKey);
    std::vector<cpp_collections::Group<int, int>> expected = {
        {7, {7, 7, 7, 7}}
    };
    ExpectGroupsEqual(result, expected);
}

TEST_F(GroupByConsecutiveTest, KeyFunc_MixedGroupsIntegers) {
    std::vector<int> input = {1, 1, 2, 2, 2, 1, 3, 3, 2};
    auto getKey = [](int x){ return x; };
    auto result = cpp_collections::group_by_consecutive(input.begin(), input.end(), getKey);
    std::vector<cpp_collections::Group<int, int>> expected = {
        {1, {1, 1}},
        {2, {2, 2, 2}},
        {1, {1}},
        {3, {3, 3}},
        {2, {2}}
    };
    ExpectGroupsEqual(result, expected);
}

TEST_F(GroupByConsecutiveTest, KeyFunc_StringsByFirstChar) {
    std::vector<std::string> input = {"apple", "apricot", "banana", "blue", "berry", "cherry"};
    auto getKey = [](const std::string& s){ return s.empty() ? ' ' : s[0]; };
    auto result = cpp_collections::group_by_consecutive(input.begin(), input.end(), getKey);
    std::vector<cpp_collections::Group<char, std::string>> expected = {
        {'a', {"apple", "apricot"}},
        {'b', {"banana", "blue", "berry"}},
        {'c', {"cherry"}}
    };
    ExpectGroupsEqual(result, expected);
}

TEST_F(GroupByConsecutiveTest, KeyFunc_CustomObjectsByCategory) {
    std::vector<TestObject> input = {
        {1, "A", 1.0}, {2, "A", 1.1}, {3, "B", 2.0}, {4, "A", 1.2}
    };
    auto result = cpp_collections::group_by_consecutive(input.begin(), input.end(), CategoryExtractor());
    std::vector<cpp_collections::Group<std::string, TestObject>> expected = {
        {"A", {{1, "A", 1.0}, {2, "A", 1.1}}},
        {"B", {{3, "B", 2.0}}},
        {"A", {{4, "A", 1.2}}}
    };
    ExpectGroupsEqual(result, expected);
}

TEST_F(GroupByConsecutiveTest, KeyFunc_ListIterators) {
    std::list<int> input = {1, 1, 2, 3, 3, 3};
    auto getKey = [](int x){ return x; };
    auto result = cpp_collections::group_by_consecutive(input.begin(), input.end(), getKey);
    std::vector<cpp_collections::Group<int, int>> expected = {
        {1, {1, 1}},
        {2, {2}},
        {3, {3, 3, 3}}
    };
    ExpectGroupsEqual(result, expected);
}

TEST_F(GroupByConsecutiveTest, KeyFunc_SingleElementRange) {
    std::vector<std::string> input = {"hello"};
    auto getKey = [](const std::string& s){ return s.length(); };
    auto result = cpp_collections::group_by_consecutive(input.begin(), input.end(), getKey);
    std::vector<cpp_collections::Group<size_t, std::string>> expected = {
        {5, {"hello"}}
    };
    ExpectGroupsEqual(result, expected);
}

// --- Tests for group_by_consecutive_pred (Predicate Version) ---

TEST_F(GroupByConsecutiveTest, Pred_EmptyRange) {
    std::vector<int> input = {};
    auto pred = [](int, int){ return false; }; // Predicate won't be called
    auto result = cpp_collections::group_by_consecutive_pred(input.begin(), input.end(), pred);
    std::vector<cpp_collections::Group<int, int>> expected = {};
    ExpectGroupsEqual(result, expected);
}

TEST_F(GroupByConsecutiveTest, Pred_AllUniqueGroups) {
    std::vector<int> input = {1, 3, 5, 7}; // Difference is always 2
    auto pred = [](int prev, int curr){ return std::abs(curr - prev) <= 1; };
    auto result = cpp_collections::group_by_consecutive_pred(input.begin(), input.end(), pred);
    std::vector<cpp_collections::Group<int, int>> expected = {
        {1, {1}}, {3, {3}}, {5, {5}}, {7, {7}}
    };
    ExpectGroupsEqual(result, expected);
}

TEST_F(GroupByConsecutiveTest, Pred_AllInOneGroup) {
    std::vector<int> input = {2, 2, 2, 2};
    auto pred = [](int prev, int curr){ return prev == curr; };
    auto result = cpp_collections::group_by_consecutive_pred(input.begin(), input.end(), pred);
    std::vector<cpp_collections::Group<int, int>> expected = {
        {2, {2, 2, 2, 2}}
    };
    ExpectGroupsEqual(result, expected);
}

TEST_F(GroupByConsecutiveTest, Pred_MixedGroupsIntegers_Sequential) {
    std::vector<int> input = {1, 2, 3, 5, 6, 8, 9, 10, 12};
    auto pred = [](int prev, int curr){ return curr == prev + 1; };
    auto result = cpp_collections::group_by_consecutive_pred(input.begin(), input.end(), pred);
    std::vector<cpp_collections::Group<int, int>> expected = {
        {1, {1, 2, 3}},
        {5, {5, 6}},
        {8, {8, 9, 10}},
        {12, {12}}
    };
    ExpectGroupsEqual(result, expected);
}

TEST_F(GroupByConsecutiveTest, Pred_StringsSameLength) {
    std::vector<std::string> input = {"a", "b", "cat", "dog", "Sun", "moon", "stars", "x", "y"};
    auto pred = [](const std::string& s1, const std::string& s2){ return s1.length() == s2.length(); };
    auto result = cpp_collections::group_by_consecutive_pred(input.begin(), input.end(), pred);
    std::vector<cpp_collections::Group<std::string, std::string>> expected = {
        {"a", {"a", "b"}},
        {"cat", {"cat", "dog", "Sun"}},
        {"moon", {"moon"}},
        {"stars", {"stars"}},
        {"x", {"x", "y"}}
    };
    ExpectGroupsEqual(result, expected);
}

TEST_F(GroupByConsecutiveTest, Pred_CustomObjectsByValueProximity) {
    // Predicate logic: are_in_same_group(previous_element, current_element)
    // Group key for predicate version is the *first* element of that group.
    std::vector<TestObject> input = {
        {1, "V", 10.1}, {2, "V", 10.3}, // Group1: Key {1,V,10.1}. Item {2} vs {1} -> abs(10.1-10.3)=0.2 (<0.5) -> same group.
        {3, "V", 10.8},                // Item {3} vs {2} -> abs(10.3-10.8)=0.5 (not <0.5) -> new group. Group2: Key {3,V,10.8}
        {4, "W", 20.0}, {5, "W", 20.4}, {6, "X", 20.7}, // Item {4} vs {3} -> new group. Group3: Key {4,W,20.0}
                                       // Item {5} vs {4} -> abs(20.0-20.4)=0.4 (<0.5) -> same group.
                                       // Item {6} vs {5} -> abs(20.4-20.7)=0.3 (<0.5) -> same group.
        {7, "Y", 30.0}                 // Item {7} vs {6} -> new group. Group4: Key {7,Y,30.0}
    };

    auto result = cpp_collections::group_by_consecutive_pred(input.begin(), input.end(), ValueProximityPredicate());
    std::vector<cpp_collections::Group<TestObject, TestObject>> expected = {
        {{1, "V", 10.1}, {{1, "V", 10.1}, {2, "V", 10.3}}},
        {{3, "V", 10.8}, {{3, "V", 10.8}}},
        {{4, "W", 20.0}, {{4, "W", 20.0}, {5, "W", 20.4}, {6, "X", 20.7}}},
        {{7, "Y", 30.0}, {{7, "Y", 30.0}}}
    };
    ExpectGroupsEqual(result, expected);
}

TEST_F(GroupByConsecutiveTest, Pred_ListIterators) {
    std::list<int> input = {1, 2, 4, 5, 6, 8};
    auto pred = [](int prev, int curr){ return std::abs(curr - prev) <= 1; }; // Group if consecutive or same
    auto result = cpp_collections::group_by_consecutive_pred(input.begin(), input.end(), pred);
    std::vector<cpp_collections::Group<int, int>> expected = {
        {1, {1, 2}},
        {4, {4, 5, 6}},
        {8, {8}}
    };
    ExpectGroupsEqual(result, expected);
}

TEST_F(GroupByConsecutiveTest, Pred_SingleElementRange) {
    std::vector<int> input = {42};
    auto pred = [](int, int) -> bool {
        ADD_FAILURE() << "Predicate should not be called for single element range";
        return false; // Return a value; ADD_FAILURE records a non-fatal failure.
    };
    auto result = cpp_collections::group_by_consecutive_pred(input.begin(), input.end(), pred);
    std::vector<cpp_collections::Group<int, int>> expected = {
        {42, {42}}
    };
    ExpectGroupsEqual(result, expected);
}

TEST_F(GroupByConsecutiveTest, Pred_AllElementsGroupTogether) {
    std::vector<TestObject> input = {
        {1, "A", 1.0}, {2, "B", 1.1}, {3, "C", 1.2}, {4, "D", 1.3}
    };
    // Predicate always returns true, so all elements should be in one group
    auto pred_always_true = [](const TestObject&, const TestObject&){ return true; };
    auto result = cpp_collections::group_by_consecutive_pred(input.begin(), input.end(), pred_always_true);
    std::vector<cpp_collections::Group<TestObject, TestObject>> expected = {
        {{1, "A", 1.0}, {{1, "A", 1.0}, {2, "B", 1.1}, {3, "C", 1.2}, {4, "D", 1.3}}}
    };
    ExpectGroupsEqual(result, expected);
}

TEST_F(GroupByConsecutiveTest, Pred_NoElementsGroupTogether) {
    std::vector<TestObject> input = {
        {1, "A", 1.0}, {2, "B", 2.0}, {3, "C", 3.0}
    };
    // Predicate always returns false, so each element is its own group
    auto pred_always_false = [](const TestObject&, const TestObject&){ return false; };
    auto result = cpp_collections::group_by_consecutive_pred(input.begin(), input.end(), pred_always_false);
    std::vector<cpp_collections::Group<TestObject, TestObject>> expected = {
        {{1, "A", 1.0}, {{1, "A", 1.0}}},
        {{2, "B", 2.0}, {{2, "B", 2.0}}},
        {{3, "C", 3.0}, {{3, "C", 3.0}}}
    };
    ExpectGroupsEqual(result, expected);
}

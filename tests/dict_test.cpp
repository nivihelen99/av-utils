#include "gtest/gtest.h"
#include "dict.h" // Assuming dict.h is in include/ and test is in tests/
                // and compiler is run from root with -Iinclude
#include <string>
#include <vector>
#include <utility> // For std::pair
#include <stdexcept> // For std::out_of_range
#include <algorithm> // For std::equal, std::is_permutation
#include <map> // For comparison in some tests


// Helper to compare item vectors (order matters)
template<typename K, typename V>
void ExpectItemsEqual(const std::vector<std::pair<K, V>>& expected,
                      const std::vector<std::pair<K, V>>& actual) {
    ASSERT_EQ(expected.size(), actual.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(expected[i].first, actual[i].first) << "Mismatch at index " << i << " for key";
        EXPECT_EQ(expected[i].second, actual[i].second) << "Mismatch at index " << i << " for value";
    }
}

// Test fixture for common setup
class DictTest : public ::testing::Test {
protected:
    pydict::dict<std::string, int> d_string_int;
    pydict::dict<int, std::string> d_int_string;
};

// 1. Constructors
TEST_F(DictTest, DefaultConstructor) {
    pydict::dict<std::string, int> d;
    EXPECT_TRUE(d.empty());
    EXPECT_EQ(d.size(), 0);
    EXPECT_EQ(d.begin(), d.end());

    pydict::dict<int, std::string> d2;
    EXPECT_TRUE(d2.empty());
    EXPECT_EQ(d2.size(), 0);
    EXPECT_EQ(d2.begin(), d2.end());
}

TEST_F(DictTest, InitializerListConstructor) {
    pydict::dict<std::string, int> d = {{"apple", 1}, {"banana", 2}, {"cherry", 3}};
    EXPECT_FALSE(d.empty());
    EXPECT_EQ(d.size(), 3);
    EXPECT_EQ(d.at("apple"), 1);
    EXPECT_EQ(d.at("banana"), 2);
    EXPECT_EQ(d.at("cherry"), 3);

    // Check order
    auto it = d.begin();
    EXPECT_EQ(it->first(), "apple"); ++it;
    EXPECT_EQ(it->first(), "banana"); ++it;
    EXPECT_EQ(it->first(), "cherry"); ++it;
    EXPECT_EQ(it, d.end());

    // With duplicates, last one wins (consistent with std::map insert behavior, though dict might differ)
    // Current dict implementation: first one wins due to how insert checks existence.
    // Let's test current behavior.
    pydict::dict<std::string, int> d_dup = {{"apple", 1}, {"banana", 2}, {"apple", 100}};
    EXPECT_EQ(d_dup.size(), 2);
    EXPECT_EQ(d_dup.at("apple"), 1); // First "apple" was inserted
    EXPECT_EQ(d_dup.at("banana"), 2);
}

TEST_F(DictTest, CopyConstructor) {
    pydict::dict<std::string, int> d1 = {{"one", 1}, {"two", 2}};
    pydict::dict<std::string, int> d2 = d1;

    EXPECT_EQ(d1.size(), d2.size());
    EXPECT_EQ(d2.at("one"), 1);
    EXPECT_EQ(d2.at("two"), 2);
    EXPECT_NE(&d1.at("one"), &d2.at("one")); // Ensure deep copy for values if they were complex

    // Ensure order is preserved
    ExpectItemsEqual(d1.items(), d2.items());

    // Modify d1, d2 should not change
    d1["three"] = 3;
    EXPECT_EQ(d1.size(), 3);
    EXPECT_EQ(d2.size(), 2);
    EXPECT_FALSE(d2.contains("three"));
}

TEST_F(DictTest, MoveConstructor) {
    pydict::dict<std::string, int> d1 = {{"one", 1}, {"two", 2}};
    pydict::dict<std::string, int> d2 = std::move(d1);

    EXPECT_EQ(d2.size(), 2);
    EXPECT_EQ(d2.at("one"), 1);
    EXPECT_EQ(d2.at("two"), 2);

    // Test that d1 is in a valid but unspecified state (likely empty)
    // The standard guarantees d1 is valid. Our dict should ensure it's empty or clear.
    // Default move constructor for std::unordered_map and std::vector should leave them empty.
    EXPECT_TRUE(d1.empty() || d1.size() == 0); // Check if d1 is empty or size is 0
}

TEST_F(DictTest, RangeConstructor) {
    std::vector<std::pair<const std::string, int>> source_vec = {{"a", 10}, {"b", 20}, {"c", 30}};
    pydict::dict<std::string, int> d(source_vec.begin(), source_vec.end());

    EXPECT_EQ(d.size(), 3);
    EXPECT_EQ(d.at("a"), 10);
    EXPECT_EQ(d.at("b"), 20);
    EXPECT_EQ(d.at("c"), 30);

    auto it = d.begin();
    EXPECT_EQ(it->first(), "a"); ++it;
    EXPECT_EQ(it->first(), "b"); ++it;
    EXPECT_EQ(it->first(), "c"); ++it;
    EXPECT_EQ(it, d.end());

    std::map<int, std::string> source_map = {{1, "x"}, {2, "y"}};
    pydict::dict<int, std::string> d_map(source_map.begin(), source_map.end());
    EXPECT_EQ(d_map.size(), 2);
    EXPECT_EQ(d_map.at(1), "x");
    EXPECT_EQ(d_map.at(2), "y");
}

// 2. Assignment Operators
TEST_F(DictTest, CopyAssignment) {
    pydict::dict<std::string, int> d1 = {{"one", 1}, {"two", 2}};
    pydict::dict<std::string, int> d2;
    d2 = d1;

    EXPECT_EQ(d1.size(), d2.size());
    EXPECT_EQ(d2.at("one"), 1);
    EXPECT_EQ(d2.at("two"), 2);
    ExpectItemsEqual(d1.items(), d2.items());

    // Modify d1, d2 should not change
    d1["three"] = 3;
    EXPECT_FALSE(d2.contains("three"));

    // Self-assignment
    d2 = d2;
    EXPECT_EQ(d2.size(), 2);
    EXPECT_EQ(d2.at("one"), 1);
}

TEST_F(DictTest, MoveAssignment) {
    pydict::dict<std::string, int> d1 = {{"one", 1}, {"two", 2}};
    pydict::dict<std::string, int> d2;
    d2 = std::move(d1);

    EXPECT_EQ(d2.size(), 2);
    EXPECT_EQ(d2.at("one"), 1);
    EXPECT_EQ(d2.at("two"), 2);
    EXPECT_TRUE(d1.empty() || d1.size() == 0);

    pydict::dict<std::string, int> d3 = {{"x", 100}};
    d3 = std::move(d2);
    EXPECT_EQ(d3.size(), 2);
    EXPECT_EQ(d3.at("one"), 1);
    EXPECT_TRUE(d2.empty() || d2.size() == 0);
}

TEST_F(DictTest, InitializerListAssignment) {
    pydict::dict<std::string, int> d = {{"a", 1}, {"b", 2}};
    d = {{"x", 10}, {"y", 20}, {"z", 30}};

    EXPECT_EQ(d.size(), 3);
    EXPECT_FALSE(d.contains("a"));
    EXPECT_EQ(d.at("x"), 10);
    EXPECT_EQ(d.at("y"), 20);
    EXPECT_EQ(d.at("z"), 30);

    auto it = d.begin();
    EXPECT_EQ(it->first(), "x"); ++it;
    EXPECT_EQ(it->first(), "y"); ++it;
    EXPECT_EQ(it->first(), "z"); ++it;
    EXPECT_EQ(it, d.end());
}

// 3. Element Access
TEST_F(DictTest, OperatorSquareBrackets) {
    d_string_int["apple"] = 10;
    EXPECT_EQ(d_string_int.size(), 1);
    EXPECT_EQ(d_string_int["apple"], 10);

    d_string_int["banana"] = 20;
    EXPECT_EQ(d_string_int.size(), 2);
    EXPECT_EQ(d_string_int["banana"], 20);

    d_string_int["apple"] = 15; // Update existing
    EXPECT_EQ(d_string_int.size(), 2);
    EXPECT_EQ(d_string_int["apple"], 15);

    // Check order
    auto items = d_string_int.items();
    std::vector<std::pair<std::string, int>> expected_items = {{"apple", 15}, {"banana", 20}};
    ExpectItemsEqual(expected_items, items);

    // Access with rvalue key
    pydict::dict<std::string, int> d;
    d[std::string("mango")] = 30;
    EXPECT_EQ(d.at("mango"), 30);
    EXPECT_EQ(d[std::string("mango")], 30);
}

TEST_F(DictTest, AtMethod) {
    d_string_int["key1"] = 100;
    d_string_int["key2"] = 200;

    EXPECT_EQ(d_string_int.at("key1"), 100);
    EXPECT_EQ(d_string_int.at("key2"), 200);

    const auto& cd = d_string_int;
    EXPECT_EQ(cd.at("key1"), 100);

    EXPECT_THROW(d_string_int.at("non_existent_key"), std::out_of_range);
    EXPECT_THROW(cd.at("non_existent_key"), std::out_of_range);
}

TEST_F(DictTest, GetMethod) {
    d_string_int["hello"] = 5;
    const auto& cd = d_string_int;

    EXPECT_EQ(cd.get("hello"), 5);
    EXPECT_EQ(cd.get("world", 10), 10); // Default value for non-existent key
    EXPECT_EQ(cd.get("world"), 0);     // Default constructed Value (int defaults to 0)

    d_int_string[1] = "one";
    const auto& cd_is = d_int_string;
    EXPECT_EQ(cd_is.get(1), "one");
    EXPECT_EQ(cd_is.get(2, "default"), "default");
    EXPECT_EQ(cd_is.get(2), ""); // Default constructed std::string
}

TEST_F(DictTest, GetOptionalMethod) {
    d_string_int["opt_key"] = 77;
    const auto& cd = d_string_int;

    auto val = cd.get_optional("opt_key");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), 77);

    auto no_val = cd.get_optional("no_such_key");
    EXPECT_FALSE(no_val.has_value());
}

// 4. Insertion Methods
TEST_F(DictTest, InsertLValue) {
    std::pair<const std::string, int> item1 = {"alpha", 1};
    auto result1 = d_string_int.insert(item1);
    EXPECT_TRUE(result1.second); // Inserted
    EXPECT_EQ(result1.first->first(), "alpha");
    EXPECT_EQ(result1.first->second(), 1);
    EXPECT_EQ(d_string_int.size(), 1);
    EXPECT_EQ(d_string_int.at("alpha"), 1);

    auto result2 = d_string_int.insert(item1); // Duplicate
    EXPECT_FALSE(result2.second); // Not inserted
    EXPECT_EQ(result2.first->first(), "alpha");
    EXPECT_EQ(result2.first->second(), 1);
    EXPECT_EQ(d_string_int.size(), 1);

    std::pair<const std::string, int> item2 = {"beta", 2};
    d_string_int.insert(item2);
    EXPECT_EQ(d_string_int.size(), 2);
    EXPECT_EQ(d_string_int.at("beta"), 2);

    // Check order
    ExpectItemsEqual({{"alpha", 1}, {"beta", 2}}, d_string_int.items());
}

TEST_F(DictTest, InsertRValue) {
    auto result1 = d_string_int.insert({"gamma", 3});
    EXPECT_TRUE(result1.second);
    EXPECT_EQ(result1.first->first(), "gamma");
    EXPECT_EQ(result1.first->second(), 3);
    EXPECT_EQ(d_string_int.size(), 1);
    EXPECT_EQ(d_string_int.at("gamma"), 3);

    auto result2 = d_string_int.insert({"gamma", 30}); // Duplicate
    EXPECT_FALSE(result2.second);
    EXPECT_EQ(result1.first->first(), "gamma");
    EXPECT_EQ(result1.first->second(), 3); // Original value
    EXPECT_EQ(d_string_int.size(), 1);
}

TEST_F(DictTest, Emplace) {
    auto result1 = d_string_int.emplace("delta", 4);
    EXPECT_TRUE(result1.second);
    EXPECT_EQ(result1.first->first(), "delta");
    EXPECT_EQ(result1.first->second(), 4);
    EXPECT_EQ(d_string_int.size(), 1);
    EXPECT_EQ(d_string_int.at("delta"), 4);

    auto result2 = d_string_int.emplace("delta", 40); // Duplicate
    EXPECT_FALSE(result2.second);
    EXPECT_EQ(result2.first->first(), "delta");
    EXPECT_EQ(result2.first->second(), 4); // Original value
    EXPECT_EQ(d_string_int.size(), 1);

    // Emplace with explicit pair
    auto result3 = d_string_int.emplace(std::make_pair("epsilon", 5));
     EXPECT_TRUE(result3.second);
    EXPECT_EQ(result3.first->first(), "epsilon");
    EXPECT_EQ(result3.first->second(), 5);
    EXPECT_EQ(d_string_int.size(), 2);
}

TEST_F(DictTest, SetDefault) {
    pydict::dict<std::string, int>::mapped_type& val1 = d_string_int.setdefault("zeta", 6);
    EXPECT_EQ(val1, 6);
    EXPECT_EQ(d_string_int.at("zeta"), 6);
    EXPECT_EQ(d_string_int.size(), 1);

    pydict::dict<std::string, int>::mapped_type& val2 = d_string_int.setdefault("zeta", 60); // Key exists
    EXPECT_EQ(val2, 6); // Original value
    EXPECT_EQ(d_string_int.at("zeta"), 6);
    EXPECT_EQ(d_string_int.size(), 1);
    EXPECT_EQ(&val1, &val2); // Should be reference to the same value

    d_string_int.setdefault("eta"); // Default construct int (0)
    EXPECT_EQ(d_string_int.at("eta"), 0);
    EXPECT_EQ(d_string_int.size(), 2);

    ExpectItemsEqual({{"zeta", 6}, {"eta", 0}}, d_string_int.items());
}


// 5. Deletion Methods
TEST_F(DictTest, EraseKey) {
    d_string_int = {{"a", 1}, {"b", 2}, {"c", 3}};
    EXPECT_EQ(d_string_int.erase("b"), 1); // Erased
    EXPECT_EQ(d_string_int.size(), 2);
    EXPECT_FALSE(d_string_int.contains("b"));
    EXPECT_EQ(d_string_int.erase("non_existent"), 0); // Not erased
    EXPECT_EQ(d_string_int.size(), 2);

    // Check order after erase (swap-and-pop means 'c' might move to 'b''s old slot in insertion_order_)
    // Original: a, b, c. Erase b. insertion_order_: a, c.
    ExpectItemsEqual({{"a", 1}, {"c", 3}}, d_string_int.items());

    d_string_int.erase("a");
    ExpectItemsEqual({{"c", 3}}, d_string_int.items());
    d_string_int.erase("c");
    EXPECT_TRUE(d_string_int.empty());
}

TEST_F(DictTest, PopKey) {
    d_string_int = {{"a", 10}, {"b", 20}, {"c", 30}};
    EXPECT_EQ(d_string_int.pop("b"), 20);
    EXPECT_EQ(d_string_int.size(), 2);
    EXPECT_FALSE(d_string_int.contains("b"));
    EXPECT_THROW(d_string_int.pop("non_existent"), std::out_of_range);
    ExpectItemsEqual({{"a", 10}, {"c", 30}}, d_string_int.items());
}

TEST_F(DictTest, PopKeyWithDefault) {
    d_string_int = {{"a", 10}, {"b", 20}};
    EXPECT_EQ(d_string_int.pop("a", 0), 10);
    EXPECT_EQ(d_string_int.size(), 1);
    EXPECT_EQ(d_string_int.pop("non_existent", 99), 99);
    EXPECT_EQ(d_string_int.size(), 1); // Size unchanged for non-existent key
    ExpectItemsEqual({{"b", 20}}, d_string_int.items());
}

TEST_F(DictTest, PopItem) {
    d_string_int = {{"first", 1}, {"second", 2}, {"third", 3}};

    auto item1 = d_string_int.popitem();
    EXPECT_EQ(item1.first, "third");
    EXPECT_EQ(item1.second, 3);
    EXPECT_EQ(d_string_int.size(), 2);
    EXPECT_FALSE(d_string_int.contains("third"));
    ExpectItemsEqual({{"first", 1}, {"second", 2}}, d_string_int.items());

    auto item2 = d_string_int.popitem();
    EXPECT_EQ(item2.first, "second");
    EXPECT_EQ(item2.second, 2);
    EXPECT_EQ(d_string_int.size(), 1);
    ExpectItemsEqual({{"first", 1}}, d_string_int.items());

    auto item3 = d_string_int.popitem();
    EXPECT_EQ(item3.first, "first");
    EXPECT_EQ(item3.second, 1);
    EXPECT_TRUE(d_string_int.empty());

    EXPECT_THROW(d_string_int.popitem(), std::out_of_range);
}

TEST_F(DictTest, PopItemEmpty) {
    EXPECT_THROW(d_string_int.popitem(), std::out_of_range);
}


TEST_F(DictTest, Clear) {
    d_string_int = {{"x", 1}, {"y", 2}};
    EXPECT_FALSE(d_string_int.empty());
    d_string_int.clear();
    EXPECT_TRUE(d_string_int.empty());
    EXPECT_EQ(d_string_int.size(), 0);
    EXPECT_EQ(d_string_int.begin(), d_string_int.end());
    EXPECT_FALSE(d_string_int.contains("x")); // Check if elements are gone
}

// 6. Iterators and Order Preservation
TEST_F(DictTest, IterationOrder) {
    d_string_int["one"] = 1;
    d_string_int["two"] = 2;
    d_string_int["three"] = 3;

    std::vector<std::pair<std::string, int>> expected_items = {{"one", 1}, {"two", 2}, {"three", 3}};
    std::vector<std::pair<std::string, int>> actual_items;
    for (const auto& item : d_string_int) {
        actual_items.push_back({item.first(), item.second()});
    }
    ExpectItemsEqual(expected_items, actual_items);

    // Const iteration
    const auto& cd = d_string_int;
    actual_items.clear();
    for (const auto& item : cd) {
        actual_items.push_back({item.first(), item.second()});
    }
    ExpectItemsEqual(expected_items, actual_items);
}

TEST_F(DictTest, IteratorModification) {
    d_string_int = {{"a", 10}, {"b", 20}};
    for (auto it = d_string_int.begin(); it != d_string_int.end(); ++it) {
        if (it->first() == "a") {
            it->second() = 100; // Modify value through iterator
        }
    }
    EXPECT_EQ(d_string_int.at("a"), 100);
    EXPECT_EQ(d_string_int.at("b"), 20);
}

TEST_F(DictTest, BeginEndCBeginCEnd) {
    d_string_int = {{"k1", 1}};
    EXPECT_NE(d_string_int.begin(), d_string_int.end());
    EXPECT_NE(d_string_int.cbegin(), d_string_int.cend());

    auto it = d_string_int.begin();
    EXPECT_EQ(it->first(), "k1");
    ++it;
    EXPECT_EQ(it, d_string_int.end());

    const auto& cd = d_string_int;
    auto cit = cd.begin();
    EXPECT_EQ(cit->first(), "k1");
    ++cit;
    EXPECT_EQ(cit, cd.end());

    EXPECT_EQ(d_string_int.cbegin(), cd.begin());
    EXPECT_EQ(d_string_int.cend(), cd.end());

    pydict::dict<int,int> empty_d;
    EXPECT_EQ(empty_d.begin(), empty_d.end());
    EXPECT_EQ(empty_d.cbegin(), empty_d.cend());

}

// 7. Capacity and Size
TEST_F(DictTest, EmptyAndSize) {
    EXPECT_TRUE(d_string_int.empty());
    EXPECT_EQ(d_string_int.size(), 0);

    d_string_int["item1"] = 1;
    EXPECT_FALSE(d_string_int.empty());
    EXPECT_EQ(d_string_int.size(), 1);

    d_string_int["item2"] = 2;
    EXPECT_FALSE(d_string_int.empty());
    EXPECT_EQ(d_string_int.size(), 2);

    d_string_int.erase("item1");
    EXPECT_FALSE(d_string_int.empty());
    EXPECT_EQ(d_string_int.size(), 1);

    d_string_int.pop("item2");
    EXPECT_TRUE(d_string_int.empty());
    EXPECT_EQ(d_string_int.size(), 0);
}

// 8. Lookup Methods
TEST_F(DictTest, Find) {
    d_string_int["key1"] = 10;
    d_string_int["key2"] = 20;

    auto it_found = d_string_int.find("key1");
    ASSERT_NE(it_found, d_string_int.end());
    EXPECT_EQ(it_found->first(), "key1");
    EXPECT_EQ(it_found->second(), 10);

    auto it_not_found = d_string_int.find("non_existent_key");
    EXPECT_EQ(it_not_found, d_string_int.end());

    const auto& cd = d_string_int;
    auto cit_found = cd.find("key2");
    ASSERT_NE(cit_found, cd.end());
    EXPECT_EQ(cit_found->first(), "key2");
    EXPECT_EQ(cit_found->second(), 20);

    auto cit_not_found = cd.find("non_existent_key");
    EXPECT_EQ(cit_not_found, cd.end());
}

TEST_F(DictTest, Count) {
    d_string_int["k1"] = 1;
    d_string_int["k2"] = 2;

    EXPECT_EQ(d_string_int.count("k1"), 1);
    EXPECT_EQ(d_string_int.count("k3"), 0); // Non-existent
}

TEST_F(DictTest, Contains) {
    d_string_int["present"] = 0;
    EXPECT_TRUE(d_string_int.contains("present"));
    EXPECT_FALSE(d_string_int.contains("absent"));
}

// 9. Python-like Methods
TEST_F(DictTest, Keys) {
    d_string_int = {{"apple", 1}, {"banana", 2}, {"cherry", 3}};
    std::vector<std::string> k = d_string_int.keys();
    std::vector<std::string> expected_keys = {"apple", "banana", "cherry"};
    ASSERT_EQ(k.size(), expected_keys.size());
    for(size_t i=0; i < k.size(); ++i) {
        EXPECT_EQ(k[i], expected_keys[i]);
    }
}

TEST_F(DictTest, Values) {
    d_string_int = {{"apple", 10}, {"banana", 20}, {"cherry", 30}};
    std::vector<int> v = d_string_int.values();
    std::vector<int> expected_values = {10, 20, 30};
     ASSERT_EQ(v.size(), expected_values.size());
    for(size_t i=0; i < v.size(); ++i) {
        EXPECT_EQ(v[i], expected_values[i]);
    }
}

TEST_F(DictTest, Items) {
    d_string_int = {{"apple", 100}, {"banana", 200}, {"cherry", 300}};
    auto items_vec = d_string_int.items();
    std::vector<std::pair<std::string, int>> expected_items_vec = {
        {"apple", 100}, {"banana", 200}, {"cherry", 300}
    };
    ExpectItemsEqual(expected_items_vec, items_vec);
}

TEST_F(DictTest, UpdateWithDict) {
    d_string_int = {{"a", 1}, {"b", 2}};
    pydict::dict<std::string, int> other_dict = {{"b", 20}, {"c", 3}, {"d", 4}};
    d_string_int.update(other_dict);

    EXPECT_EQ(d_string_int.size(), 4);
    EXPECT_EQ(d_string_int.at("a"), 1);    // Original
    EXPECT_EQ(d_string_int.at("b"), 20);   // Updated
    EXPECT_EQ(d_string_int.at("c"), 3);    // Added
    EXPECT_EQ(d_string_int.at("d"), 4);    // Added

    // Check order: original items first, then new items from 'other_dict' in their order
    std::vector<std::pair<std::string, int>> expected_items = {
        {"a", 1}, {"b", 20}, {"c", 3}, {"d", 4}
    };
    // The order of updated elements depends on if update re-inserts or just updates value.
    // Current implementation of update uses operator[], which updates in place or adds to end.
    // "b" was existing, so its order is maintained. "c" and "d" are new, added to end in other_dict's order.
    // So, if "b" was originally at index i, it remains at index i.
    // Let's re-verify update logic: `(*this)[key] = value;`
    // If key exists, value is updated, order is preserved.
    // If key is new, it's added to the end.
    // So, "a" and "b" keep their original relative order. "c" and "d" from other_dict are added at the end.
    // Initial: a, b
    // Other: b, c, d
    // Update b: a, b (value 20)
    // Insert c: a, b, c
    // Insert d: a, b, c, d
    ExpectItemsEqual(expected_items, d_string_int.items());
}

TEST_F(DictTest, UpdateWithInitializerList) {
    d_string_int = {{"x", 10}, {"y", 20}};
    d_string_int.update({{"y", 200}, {"z", 300}});

    EXPECT_EQ(d_string_int.size(), 3);
    EXPECT_EQ(d_string_int.at("x"), 10);
    EXPECT_EQ(d_string_int.at("y"), 200);
    EXPECT_EQ(d_string_int.at("z"), 300);

    std::vector<std::pair<std::string, int>> expected_items = {
        {"x", 10}, {"y", 200}, {"z", 300}
    };
    ExpectItemsEqual(expected_items, d_string_int.items());
}

// 10. Comparison Operators
TEST_F(DictTest, EqualityOperators) {
    pydict::dict<std::string, int> d1 = {{"a", 1}, {"b", 2}};
    pydict::dict<std::string, int> d2 = {{"a", 1}, {"b", 2}};
    pydict::dict<std::string, int> d3 = {{"b", 2}, {"a", 1}}; // Different order
    pydict::dict<std::string, int> d4 = {{"a", 1}, {"b", 20}}; // Different value
    pydict::dict<std::string, int> d5 = {{"a", 1}}; // Different size

    EXPECT_TRUE(d1 == d2);
    EXPECT_FALSE(d1 != d2);

    // Equality depends on content and insertion order.
    // Standard dict comparison (Python) considers order for items(), but map comparison doesn't.
    // The implemented == checks size, then iterates through insertion_order_ of LHS,
    // checking key existence and value equality in RHS. This implicitly checks order IF keys are same.
    // If d1 = {a:1, b:2} and d3 = {b:2, a:1}
    // d1.insertion_order_ = [a, b]
    // d3.storage_ has a and b.
    // 1. Check key 'a': d3 has 'a', values match.
    // 2. Check key 'b': d3 has 'b', values match.
    // This means d1 == d3 is true if all keys in d1 are in d3 with same values, AND same size.
    // Order of insertion_order_ is the defining factor for equality.
    EXPECT_FALSE(d1 == d3); // Order matters
    EXPECT_TRUE(d1 != d3);

    EXPECT_FALSE(d1 == d4);
    EXPECT_TRUE(d1 != d4);

    EXPECT_FALSE(d1 == d5);
    EXPECT_TRUE(d1 != d5);

    pydict::dict<std::string, int> empty1, empty2;
    EXPECT_TRUE(empty1 == empty2);
}

// 11. Edge Cases
TEST_F(DictTest, EmptyDictOperations) {
    EXPECT_TRUE(d_string_int.empty());
    EXPECT_EQ(d_string_int.size(), 0);
    EXPECT_EQ(d_string_int.begin(), d_string_int.end());
    EXPECT_THROW(d_string_int.at("any"), std::out_of_range);
    EXPECT_EQ(d_string_int.get("any", 100), 100);
    EXPECT_FALSE(d_string_int.get_optional("any").has_value());
    EXPECT_EQ(d_string_int.erase("any"), 0);
    EXPECT_THROW(d_string_int.pop("any"), std::out_of_range);
    EXPECT_EQ(d_string_int.pop("any", 100), 100);
    EXPECT_THROW(d_string_int.popitem(), std::out_of_range);
    EXPECT_TRUE(d_string_int.keys().empty());
    EXPECT_TRUE(d_string_int.values().empty());
    EXPECT_TRUE(d_string_int.items().empty());
    EXPECT_FALSE(d_string_int.contains("any"));
    EXPECT_EQ(d_string_int.find("any"), d_string_int.end());
}

TEST_F(DictTest, SingleElementDict) {
    d_string_int["one"] = 1;
    EXPECT_EQ(d_string_int.size(), 1);
    EXPECT_EQ(d_string_int.at("one"), 1);
    auto item = d_string_int.popitem();
    EXPECT_EQ(item.first, "one");
    EXPECT_EQ(item.second, 1);
    EXPECT_TRUE(d_string_int.empty());
}

TEST_F(DictTest, DuplicateKeyHandlingInConstructorsAndInsert) {
    // Initializer list constructor (already tested, first one wins)
    pydict::dict<std::string, int> d_init = {{"apple", 1}, {"banana", 2}, {"apple", 100}};
    EXPECT_EQ(d_init.size(), 2);
    EXPECT_EQ(d_init.at("apple"), 1);

    // Range constructor
    std::vector<std::pair<const std::string, int>> source_vec_dup = {{"a", 10}, {"b", 20}, {"a", 100}};
    pydict::dict<std::string, int> d_range(source_vec_dup.begin(), source_vec_dup.end());
    EXPECT_EQ(d_range.size(), 2);
    EXPECT_EQ(d_range.at("a"), 10); // First "a" encountered was inserted

    // Insert method (already tested, insert returns false if key exists)
    d_string_int.clear();
    d_string_int.insert({"key", 1});
    auto result = d_string_int.insert({"key", 2});
    EXPECT_FALSE(result.second);
    EXPECT_EQ(d_string_int.at("key"), 1);
    EXPECT_EQ(d_string_int.size(), 1);

    // operator[] overwrites
    d_string_int["key"] = 3;
    EXPECT_EQ(d_string_int.at("key"), 3);
    EXPECT_EQ(d_string_int.size(), 1);
}


// 12. Type Variations
TEST_F(DictTest, IntKeysStringValues) {
    d_int_string[10] = "ten";
    d_int_string[20] = "twenty";
    EXPECT_EQ(d_int_string.size(), 2);
    EXPECT_EQ(d_int_string.at(10), "ten");
    EXPECT_EQ(d_int_string[20], "twenty");

    auto items = d_int_string.items();
    std::vector<std::pair<int, std::string>> expected = {{10, "ten"}, {20, "twenty"}};
    ExpectItemsEqual(expected, items);

    d_int_string.erase(10);
    EXPECT_FALSE(d_int_string.contains(10));
    EXPECT_EQ(d_int_string.size(), 1);
}

// Simple struct for value type
struct MyStruct {
    int id;
    std::string name;
    bool operator==(const MyStruct& other) const {
        return id == other.id && name == other.name;
    }
    // Default constructor for some dict methods like get()
    MyStruct(int i=0, std::string n="") : id(i), name(std::move(n)) {}
};

// For GTest printing
std::ostream& operator<<(std::ostream& os, const MyStruct& s) {
    return os << "MyStruct{id=" << s.id << ", name=\"" << s.name << "\"}";
}


TEST_F(DictTest, CustomValueType) {
    pydict::dict<std::string, MyStruct> d_custom;
    d_custom["user1"] = MyStruct{1, "Alice"};
    d_custom.insert({"user2", MyStruct{2, "Bob"}});

    EXPECT_EQ(d_custom.size(), 2);
    EXPECT_EQ(d_custom.at("user1").id, 1);
    EXPECT_EQ(d_custom.at("user1").name, "Alice");

    MyStruct s_bob = d_custom.pop("user2");
    EXPECT_EQ(s_bob.id, 2);
    EXPECT_EQ(s_bob.name, "Bob");
    EXPECT_EQ(d_custom.size(), 1);

    EXPECT_EQ(d_custom.get("non_existent_user", MyStruct{0, "Default"}).name, "Default");
    // Test get without default value (requires MyStruct to be default constructible)
    EXPECT_EQ(d_custom.get("non_existent_user2").id, 0);
}

// 13. noexcept correctness is mostly a compile-time/design check.
// We can verify move operations are efficient (though not directly noexcept).
TEST_F(DictTest, MoveOperationsEfficiency) {
    pydict::dict<std::string, int> d1;
    for (int i = 0; i < 1000; ++i) {
        d1[std::to_string(i)] = i;
    }
    // size_t original_cap_storage = d1.storage_.bucket_count(); // Example, not a direct capacity measure
    // size_t original_cap_order = d1.insertion_order_.capacity();


    pydict::dict<std::string, int> d2 = std::move(d1);
    // If moved efficiently, d1 should be empty or small, and d2 has taken over resources.
    // Checking capacity of underlying containers can be a hint.
    // This is not a strict test for noexcept but for the efficiency aspect of move.
    EXPECT_TRUE(d1.empty() || d1.size() == 0); // d1 should be left in a valid, likely empty state
    EXPECT_EQ(d2.size(), 1000);
    // It's hard to assert specific capacities after move without more specific guarantees from dict.
    // But we expect d2's capacities to be similar to d1's original.
    // EXPECT_GE(d2.storage_.bucket_count(), original_cap_storage); // or similar
    // EXPECT_GE(d2.insertion_order_.capacity(), original_cap_order);


    pydict::dict<std::string, int> d3;
    d3 = std::move(d2);
    EXPECT_TRUE(d2.empty() || d2.size() == 0);
    EXPECT_EQ(d3.size(), 1000);
}

// Test for swap
TEST_F(DictTest, SwapFunction) {
    pydict::dict<std::string, int> d1 = {{"a", 1}, {"b", 2}};
    pydict::dict<std::string, int> d2 = {{"x", 10}, {"y", 20}, {"z", 30}};

    std::vector<std::pair<std::string, int>> items1_orig = d1.items();
    std::vector<std::pair<std::string, int>> items2_orig = d2.items();

    swap(d1, d2); // Calling the free swap function

    EXPECT_EQ(d1.size(), items2_orig.size());
    ExpectItemsEqual(items2_orig, d1.items());

    EXPECT_EQ(d2.size(), items1_orig.size());
    ExpectItemsEqual(items1_orig, d2.items());

    // Swap with self
    swap(d1, d1);
    ExpectItemsEqual(items2_orig, d1.items());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

// Ensure Value is defined for setdefault test.
// If Value is not default constructible, setdefault without a value will fail to compile.
// In our case, int and std::string are default constructible. MyStruct is also made so.
// The setdefault test uses d_string_int which has Value=int.
// The test with MyStruct also explicitly provides a default or uses its default constructor.

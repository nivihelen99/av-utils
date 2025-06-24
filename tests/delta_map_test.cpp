
#include "gtest/gtest.h"
#include "delta_map.h"

#include <string>
#include <map>
#include <unordered_map>
#include <vector>

// Test fixture for DeltaMap with std::map<std::string, int>
class DeltaMapStringIntTest : public ::testing::Test {
protected:
    std::map<std::string, int> empty_map_ = {};

    std::map<std::string, int> map1_ = {
        {"a", 1},
        {"b", 2},
        {"c", 3}
    };

    std::map<std::string, int> map2_ = {
        {"b", 2},       // Unchanged
        {"c", 30},      // Changed
        {"d", 4}        // Added
                        // "a" is Removed
    };
};

TEST_F(DeltaMapStringIntTest, AllChanges) {
    deltamap::DeltaMap delta(map1_, map2_);

    std::map<std::string, int> expected_added = {{"d", 4}};
    std::map<std::string, int> expected_removed = {{"a", 1}};
    std::map<std::string, int> expected_changed = {{"c", 30}};
    std::map<std::string, int> expected_unchanged = {{"b", 2}};

    EXPECT_EQ(delta.added(), expected_added);
    EXPECT_EQ(delta.removed(), expected_removed);
    EXPECT_EQ(delta.changed(), expected_changed);
    EXPECT_EQ(delta.unchanged(), expected_unchanged);

    EXPECT_FALSE(delta.empty());
    EXPECT_EQ(delta.size(), 3); // 1 added, 1 removed, 1 changed

    // Test individual key queries
    EXPECT_TRUE(delta.was_added("d"));
    EXPECT_FALSE(delta.was_added("a"));

    EXPECT_TRUE(delta.was_removed("a"));
    EXPECT_FALSE(delta.was_removed("d"));

    EXPECT_TRUE(delta.was_changed("c"));
    EXPECT_FALSE(delta.was_changed("b"));

    EXPECT_TRUE(delta.was_unchanged("b"));
    EXPECT_FALSE(delta.was_unchanged("c"));

    EXPECT_FALSE(delta.was_added("non_existent_key"));
    EXPECT_FALSE(delta.was_removed("non_existent_key"));
    EXPECT_FALSE(delta.was_changed("non_existent_key"));
    EXPECT_FALSE(delta.was_unchanged("non_existent_key"));
}

TEST_F(DeltaMapStringIntTest, NoChanges) {
    deltamap::DeltaMap delta(map1_, map1_);

    EXPECT_TRUE(delta.added().empty());
    EXPECT_TRUE(delta.removed().empty());
    EXPECT_TRUE(delta.changed().empty());
    EXPECT_EQ(delta.unchanged(), map1_);

    EXPECT_TRUE(delta.empty());
    EXPECT_EQ(delta.size(), 0);
}

TEST_F(DeltaMapStringIntTest, AllAdded) {
    deltamap::DeltaMap delta(empty_map_, map1_);

    EXPECT_EQ(delta.added(), map1_);
    EXPECT_TRUE(delta.removed().empty());
    EXPECT_TRUE(delta.changed().empty());
    EXPECT_TRUE(delta.unchanged().empty());

    EXPECT_FALSE(delta.empty());
    EXPECT_EQ(delta.size(), map1_.size());

    EXPECT_TRUE(delta.was_added("a"));
    EXPECT_TRUE(delta.was_added("b"));
    EXPECT_TRUE(delta.was_added("c"));
}

TEST_F(DeltaMapStringIntTest, AllRemoved) {
    deltamap::DeltaMap delta(map1_, empty_map_);

    EXPECT_TRUE(delta.added().empty());
    EXPECT_EQ(delta.removed(), map1_);
    EXPECT_TRUE(delta.changed().empty());
    EXPECT_TRUE(delta.unchanged().empty());

    EXPECT_FALSE(delta.empty());
    EXPECT_EQ(delta.size(), map1_.size());

    EXPECT_TRUE(delta.was_removed("a"));
    EXPECT_TRUE(delta.was_removed("b"));
    EXPECT_TRUE(delta.was_removed("c"));
}

TEST_F(DeltaMapStringIntTest, AllChanged) {
    std::map<std::string, int> map1_changed_vals = {
        {"a", 10},
        {"b", 20},
        {"c", 30}
    };
    deltamap::DeltaMap delta(map1_, map1_changed_vals);

    EXPECT_TRUE(delta.added().empty());
    EXPECT_TRUE(delta.removed().empty());
    EXPECT_EQ(delta.changed(), map1_changed_vals);
    EXPECT_TRUE(delta.unchanged().empty());

    EXPECT_FALSE(delta.empty());
    EXPECT_EQ(delta.size(), map1_.size());

    EXPECT_TRUE(delta.was_changed("a"));
    EXPECT_TRUE(delta.was_changed("b"));
    EXPECT_TRUE(delta.was_changed("c"));
}

TEST_F(DeltaMapStringIntTest, ApplyTo) {
    deltamap::DeltaMap delta(map1_, map2_);
    std::map<std::string, int> result = delta.apply_to(map1_);
    EXPECT_EQ(result, map2_);

    // Apply to an empty map
    std::map<std::string, int> result_from_empty = delta.apply_to({});
    // This won't be map2_, as apply_to assumes base_map is the "old" state.
    // It will remove "a" (no-op on empty), add "d", change "c" (adds "c" with new value)
    std::map<std::string, int> expected_from_empty = {{"c", 30}, {"d", 4}};
    EXPECT_EQ(result_from_empty, expected_from_empty);
}

TEST_F(DeltaMapStringIntTest, Invert) {
    deltamap::DeltaMap delta12(map1_, map2_);
    deltamap::DeltaMap delta21 = delta12.invert(map1_, map2_); // Equivalent to DeltaMap(map2_, map1_)

    std::map<std::string, int> expected_added_inverted = {{"a", 1}};
    std::map<std::string, int> expected_removed_inverted = {{"d", 4}};
    std::map<std::string, int> expected_changed_inverted = {{"c", 3}}; // Value from map1_
    std::map<std::string, int> expected_unchanged_inverted = {{"b", 2}};

    EXPECT_EQ(delta21.added(), expected_added_inverted);
    EXPECT_EQ(delta21.removed(), expected_removed_inverted);
    EXPECT_EQ(delta21.changed(), expected_changed_inverted);
    EXPECT_EQ(delta21.unchanged(), expected_unchanged_inverted);

    // Applying inverted delta to map2_ should result in map1_
    std::map<std::string, int> result = delta21.apply_to(map2_);
    EXPECT_EQ(result, map1_);
}

// Test deduction guide for std::map
TEST(DeltaMapDeductionTest, StdMap) {
    std::map<std::string, int> m1 = {{"a", 1}};
    std::map<std::string, int> m2 = {{"a", 2}};
    deltamap::DeltaMap delta(m1, m2); // Uses deduction guide
    EXPECT_TRUE(delta.was_changed("a"));
}

// Test with integer keys
TEST(DeltaMapIntKeyTest, Basic) {
    std::map<int, std::string> old_m = {{1, "one"}, {2, "two"}};
    std::map<int, std::string> new_m = {{2, "deux"}, {3, "three"}};

    deltamap::DeltaMap delta(old_m, new_m);

    EXPECT_TRUE(delta.was_removed(1));
    EXPECT_TRUE(delta.was_changed(2));
    EXPECT_TRUE(delta.was_added(3));
    EXPECT_EQ(delta.changed().at(2), "deux");
}

// Test fixture for DeltaMap with std::unordered_map<std::string, int>
class DeltaMapUnorderedStringIntTest : public ::testing::Test {
protected:
    std::unordered_map<std::string, int> empty_map_ = {};

    std::unordered_map<std::string, int> map1_ = {
        {"a", 1},
        {"b", 2},
        {"c", 3}
    };

    std::unordered_map<std::string, int> map2_ = {
        {"b", 2},       // Unchanged
        {"c", 30},      // Changed
        {"d", 4}        // Added
                        // "a" is Removed
    };
};

TEST_F(DeltaMapUnorderedStringIntTest, AllChanges) {
    deltamap::DeltaMap delta(map1_, map2_);

    std::unordered_map<std::string, int> expected_added = {{"d", 4}};
    std::unordered_map<std::string, int> expected_removed = {{"a", 1}};
    std::unordered_map<std::string, int> expected_changed = {{"c", 30}};
    std::unordered_map<std::string, int> expected_unchanged = {{"b", 2}};

    EXPECT_EQ(delta.added(), expected_added);
    EXPECT_EQ(delta.removed(), expected_removed);
    EXPECT_EQ(delta.changed(), expected_changed);
    EXPECT_EQ(delta.unchanged(), expected_unchanged);

    EXPECT_FALSE(delta.empty());
    EXPECT_EQ(delta.size(), 3); // 1 added, 1 removed, 1 changed

    EXPECT_TRUE(delta.was_added("d"));
    EXPECT_TRUE(delta.was_removed("a"));
    EXPECT_TRUE(delta.was_changed("c"));
    EXPECT_TRUE(delta.was_unchanged("b"));
}

TEST_F(DeltaMapUnorderedStringIntTest, NoChanges) {
    deltamap::DeltaMap delta(map1_, map1_);

    EXPECT_TRUE(delta.added().empty());
    EXPECT_TRUE(delta.removed().empty());
    EXPECT_TRUE(delta.changed().empty());
    EXPECT_EQ(delta.unchanged(), map1_);

    EXPECT_TRUE(delta.empty());
    EXPECT_EQ(delta.size(), 0);
}

// Test deduction guide for std::unordered_map
TEST(DeltaMapDeductionTest, StdUnorderedMap) {
    std::unordered_map<std::string, int> m1 = {{"a", 1}};
    std::unordered_map<std::string, int> m2 = {{"a", 2}};
    // Explicitly specify MapType for unordered_map if not relying on C++17 deduction guides for template arguments
    deltamap::DeltaMap<std::string, int, std::unordered_map<std::string, int>> delta(m1, m2);
    EXPECT_TRUE(delta.was_changed("a"));
}

// --- Tests for Custom Types and Comparators ---

struct MyCustomValue {
    std::string data;
    int version;
    bool critical; // This field will be ignored by custom comparator

    // Default equality, considers all fields
    bool operator==(const MyCustomValue& other) const {
        return data == other.data && version == other.version && critical == other.critical;
    }
};

// For printing MyCustomValue in GTest failure messages
std::ostream& operator<<(std::ostream& os, const MyCustomValue& val) {
    os << "{data: " << val.data << ", version: " << val.version << ", critical: " << std::boolalpha << val.critical << "}";
    return os;
}

class DeltaMapCustomTypeTest : public ::testing::Test {
protected:
    using CustomMap = std::map<std::string, MyCustomValue>;

    CustomMap map_old_ = {
        {"key1", {"alpha", 1, true}},
        {"key2", {"beta", 1, false}},
        {"key3", {"gamma", 1, true}}
    };

    CustomMap map_new_ = {
        {"key1", {"alpha", 1, true}},       // Unchanged by default equality
        {"key2", {"beta", 2, false}},       // Changed (version)
        {"key4", {"delta", 1, false}}       // Added
                                            // key3 is Removed
    };

    CustomMap map_new_critical_changed_ = { // map_new_ but with 'critical' flag different for key1
        {"key1", {"alpha", 1, false}},      // Changed by default, Unchanged by custom
        {"key2", {"beta", 2, false}},
        {"key4", {"delta", 1, false}}
    };
};

TEST_F(DeltaMapCustomTypeTest, DefaultComparator) {
    deltamap::DeltaMap delta(map_old_, map_new_);

    EXPECT_EQ(delta.added().size(), 1);
    EXPECT_TRUE(delta.was_added("key4"));
    EXPECT_EQ(delta.added().at("key4").data, "delta");

    EXPECT_EQ(delta.removed().size(), 1);
    EXPECT_TRUE(delta.was_removed("key3"));

    EXPECT_EQ(delta.changed().size(), 1);
    EXPECT_TRUE(delta.was_changed("key2"));
    EXPECT_EQ(delta.changed().at("key2").version, 2);

    EXPECT_EQ(delta.unchanged().size(), 1);
    EXPECT_TRUE(delta.was_unchanged("key1"));
}

TEST_F(DeltaMapCustomTypeTest, CustomComparatorLambda) {
    auto custom_equal = [](const MyCustomValue& v1, const MyCustomValue& v2) {
        // Only compare data and version, ignore 'critical' field
        return v1.data == v2.data && v1.version == v2.version;
    };

    // Using map_new_critical_changed_ which has key1.critical modified
    // With custom_equal, key1 should now be UNCHANGED.
    // With default equality, key1 would be CHANGED.
    deltamap::DeltaMap<std::string, MyCustomValue, CustomMap, decltype(custom_equal)>
        delta(map_old_, map_new_critical_changed_, custom_equal);

    EXPECT_EQ(delta.added().size(), 1);
    EXPECT_TRUE(delta.was_added("key4"));
    EXPECT_EQ(delta.added().at("key4").data, "delta");

    EXPECT_EQ(delta.removed().size(), 1);
    EXPECT_TRUE(delta.was_removed("key3"));

    EXPECT_EQ(delta.changed().size(), 1); // key2 (version changed)
    EXPECT_TRUE(delta.was_changed("key2"));
    EXPECT_EQ(delta.changed().at("key2").version, 2);

    EXPECT_EQ(delta.unchanged().size(), 1); // key1 (data, version same, critical differs but ignored)
    EXPECT_TRUE(delta.was_unchanged("key1"));
    EXPECT_EQ(delta.unchanged().at("key1").data, "alpha"); // Value from new map
}

// Test deduction guide with custom comparator
TEST(DeltaMapDeductionTest, CustomComparator) {
    std::map<std::string, MyCustomValue> m1 = {{"a", {"data", 1, true}}};
    std::map<std::string, MyCustomValue> m2 = {{"a", {"data", 1, false}}};

    auto custom_equal = [](const MyCustomValue& v1, const MyCustomValue& v2) {
        return v1.data == v2.data && v1.version == v2.version;
    };

    // This should use the deduction guide for map and the provided comparator
    deltamap::DeltaMap delta(m1, m2, custom_equal);
    EXPECT_TRUE(delta.was_unchanged("a")); // Unchanged because custom_equal ignores 'critical'

    deltamap::DeltaMap delta_default_eq(m1, m2); // Uses default MyCustomValue::operator==
    EXPECT_TRUE(delta_default_eq.was_changed("a")); // Changed because 'critical' differs
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

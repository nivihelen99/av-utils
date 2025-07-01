#include "gtest/gtest.h"
#include "value_versioned_map.h"
#include <string>
#include <iostream> // For debugging, if needed
#include <algorithm> // Required for std::find

// Define a simple struct for testing custom version types
struct SemanticVersion {
    int major;
    int minor;
    int patch;

    SemanticVersion(int maj, int min, int p) : major(maj), minor(min), patch(p) {}

    bool operator<(const SemanticVersion& other) const {
        if (major != other.major) return major < other.major;
        if (minor != other.minor) return minor < other.minor;
        return patch < other.patch;
    }

    bool operator==(const SemanticVersion& other) const {
        return major == other.major && minor == other.minor && patch == other.patch;
    }
    // Required for GTest to print it in case of errors
    friend std::ostream& operator<<(std::ostream& os, const SemanticVersion& sv) {
        return os << sv.major << "." << sv.minor << "." << sv.patch;
    }
};

// Test fixture for ValueVersionedMap tests
class ValueVersionedMapTest : public ::testing::Test {
protected:
    cpp_collections::ValueVersionedMap<std::string, std::string> map_str_str_uint;
    cpp_collections::ValueVersionedMap<int, double, int> map_int_double_int;
    cpp_collections::ValueVersionedMap<std::string, int, SemanticVersion> map_str_int_semver;
};

// Test basic put and get_latest operations
TEST_F(ValueVersionedMapTest, PutAndGetLatest) {
    map_str_str_uint.put("key1", "value1_v1", 1);
    ASSERT_TRUE(map_str_str_uint.get_latest("key1").has_value());
    EXPECT_EQ(map_str_str_uint.get_latest("key1").value().get(), "value1_v1");

    map_str_str_uint.put("key1", "value1_v2", 2);
    ASSERT_TRUE(map_str_str_uint.get_latest("key1").has_value());
    EXPECT_EQ(map_str_str_uint.get_latest("key1").value().get(), "value1_v2");

    map_str_str_uint.put("key2", "value2_v1", 1);
    ASSERT_TRUE(map_str_str_uint.get_latest("key2").has_value());
    EXPECT_EQ(map_str_str_uint.get_latest("key2").value().get(), "value2_v1");

    EXPECT_FALSE(map_str_str_uint.get_latest("non_existent_key").has_value());
}

// Test get operation for specific versions
TEST_F(ValueVersionedMapTest, GetAtVersion) {
    map_str_str_uint.put("key1", "value_v10", 10);
    map_str_str_uint.put("key1", "value_v20", 20);
    map_str_str_uint.put("key1", "value_v30", 30);

    ASSERT_TRUE(map_str_str_uint.get("key1", 10).has_value());
    EXPECT_EQ(map_str_str_uint.get("key1", 10).value().get(), "value_v10");

    ASSERT_TRUE(map_str_str_uint.get("key1", 15).has_value()); // Should get v10
    EXPECT_EQ(map_str_str_uint.get("key1", 15).value().get(), "value_v10");

    ASSERT_TRUE(map_str_str_uint.get("key1", 20).has_value());
    EXPECT_EQ(map_str_str_uint.get("key1", 20).value().get(), "value_v20");

    ASSERT_TRUE(map_str_str_uint.get("key1", 29).has_value()); // Should get v20
    EXPECT_EQ(map_str_str_uint.get("key1", 29).value().get(), "value_v20");

    ASSERT_TRUE(map_str_str_uint.get("key1", 30).has_value());
    EXPECT_EQ(map_str_str_uint.get("key1", 30).value().get(), "value_v30");

    ASSERT_TRUE(map_str_str_uint.get("key1", 100).has_value()); // Should get v30 (latest)
    EXPECT_EQ(map_str_str_uint.get("key1", 100).value().get(), "value_v30");

    EXPECT_FALSE(map_str_str_uint.get("key1", 5).has_value()); // Version too early
    EXPECT_FALSE(map_str_str_uint.get("non_existent_key", 10).has_value());
}

// Test get_exact operation
TEST_F(ValueVersionedMapTest, GetExactVersion) {
    map_str_str_uint.put("key1", "value_v10", 10);
    map_str_str_uint.put("key1", "value_v20", 20);

    ASSERT_TRUE(map_str_str_uint.get_exact("key1", 10).has_value());
    EXPECT_EQ(map_str_str_uint.get_exact("key1", 10).value().get(), "value_v10");

    EXPECT_FALSE(map_str_str_uint.get_exact("key1", 15).has_value()); // No exact match

    ASSERT_TRUE(map_str_str_uint.get_exact("key1", 20).has_value());
    EXPECT_EQ(map_str_str_uint.get_exact("key1", 20).value().get(), "value_v20");

    EXPECT_FALSE(map_str_str_uint.get_exact("non_existent_key", 10).has_value());
}

// Test remove_version operation
TEST_F(ValueVersionedMapTest, RemoveVersion) {
    map_str_str_uint.put("key1", "v1", 1);
    map_str_str_uint.put("key1", "v2", 2);
    map_str_str_uint.put("key1", "v3", 3);
    map_str_str_uint.put("key2", "v_other", 1);


    EXPECT_TRUE(map_str_str_uint.remove_version("key1", 2));
    EXPECT_FALSE(map_str_str_uint.get_exact("key1", 2).has_value());
    ASSERT_TRUE(map_str_str_uint.get_latest("key1").has_value());
    EXPECT_EQ(map_str_str_uint.get_latest("key1").value().get(), "v3"); // v3 is now latest
    ASSERT_TRUE(map_str_str_uint.get("key1", 2).has_value()); // get at version 2 should now yield v1
    EXPECT_EQ(map_str_str_uint.get("key1", 2).value().get(), "v1");


    EXPECT_FALSE(map_str_str_uint.remove_version("key1", 10)); // Non-existent version
    EXPECT_FALSE(map_str_str_uint.remove_version("non_existent_key", 1));

    // Remove last version for a key
    map_str_str_uint.put("key_single", "single_val", 100);
    EXPECT_TRUE(map_str_str_uint.remove_version("key_single", 100));
    EXPECT_FALSE(map_str_str_uint.contains_key("key_single")); // Key should be removed
    EXPECT_FALSE(map_str_str_uint.get_latest("key_single").has_value());
}

// Test remove_key operation
TEST_F(ValueVersionedMapTest, RemoveKey) {
    map_str_str_uint.put("key1", "v1", 1);
    map_str_str_uint.put("key1", "v2", 2);
    map_str_str_uint.put("key2", "v_other", 1);

    EXPECT_TRUE(map_str_str_uint.remove_key("key1"));
    EXPECT_FALSE(map_str_str_uint.contains_key("key1"));
    EXPECT_FALSE(map_str_str_uint.get_latest("key1").has_value());
    EXPECT_TRUE(map_str_str_uint.contains_key("key2")); // Other keys unaffected

    EXPECT_FALSE(map_str_str_uint.remove_key("non_existent_key"));
}

// Test contains_key and contains_version
TEST_F(ValueVersionedMapTest, ContainsOperations) {
    map_str_str_uint.put("key1", "v1", 1);
    map_str_str_uint.put("key1", "v2", 2);

    EXPECT_TRUE(map_str_str_uint.contains_key("key1"));
    EXPECT_FALSE(map_str_str_uint.contains_key("key_unknown"));

    EXPECT_TRUE(map_str_str_uint.contains_version("key1", 1));
    EXPECT_TRUE(map_str_str_uint.contains_version("key1", 2));
    EXPECT_FALSE(map_str_str_uint.contains_version("key1", 3));
    EXPECT_FALSE(map_str_str_uint.contains_version("key_unknown", 1));
}

// Test empty, size, total_versions, clear
TEST_F(ValueVersionedMapTest, CapacityOperations) {
    EXPECT_TRUE(map_str_str_uint.empty());
    EXPECT_EQ(map_str_str_uint.size(), 0);
    EXPECT_EQ(map_str_str_uint.total_versions(), 0);

    map_str_str_uint.put("key1", "v1", 1);
    EXPECT_FALSE(map_str_str_uint.empty());
    EXPECT_EQ(map_str_str_uint.size(), 1);
    EXPECT_EQ(map_str_str_uint.total_versions(), 1);

    map_str_str_uint.put("key1", "v2", 2); // Same key, new version
    EXPECT_EQ(map_str_str_uint.size(), 1);
    EXPECT_EQ(map_str_str_uint.total_versions(), 2);

    map_str_str_uint.put("key2", "v_other", 10); // New key
    EXPECT_EQ(map_str_str_uint.size(), 2);
    EXPECT_EQ(map_str_str_uint.total_versions(), 3);

    map_str_str_uint.clear();
    EXPECT_TRUE(map_str_str_uint.empty());
    EXPECT_EQ(map_str_str_uint.size(), 0);
    EXPECT_EQ(map_str_str_uint.total_versions(), 0);
}

// Test keys and versions methods
TEST_F(ValueVersionedMapTest, KeysAndVersionsListers) {
    map_str_str_uint.put("apple", "red", 1);
    map_str_str_uint.put("banana", "yellow", 1);
    map_str_str_uint.put("apple", "green", 2);

    auto current_keys = map_str_str_uint.keys();
    ASSERT_EQ(current_keys.size(), 2);
    // Order is not guaranteed by unordered_map for keys
    EXPECT_TRUE(std::find(current_keys.begin(), current_keys.end(), "apple") != current_keys.end());
    EXPECT_TRUE(std::find(current_keys.begin(), current_keys.end(), "banana") != current_keys.end());

    auto apple_versions_opt = map_str_str_uint.versions("apple");
    ASSERT_TRUE(apple_versions_opt.has_value());
    auto apple_versions = apple_versions_opt.value();
    ASSERT_EQ(apple_versions.size(), 2);
    EXPECT_EQ(apple_versions[0], 1); // Versions are sorted (from std::map)
    EXPECT_EQ(apple_versions[1], 2);

    auto banana_versions_opt = map_str_str_uint.versions("banana");
    ASSERT_TRUE(banana_versions_opt.has_value());
    auto banana_versions = banana_versions_opt.value();
    ASSERT_EQ(banana_versions.size(), 1);
    EXPECT_EQ(banana_versions[0], 1);

    EXPECT_FALSE(map_str_str_uint.versions("cherry").has_value());
}

// Test get_all_versions
TEST_F(ValueVersionedMapTest, GetAllVersions) {
    map_str_str_uint.put("key1", "v10", 10);
    map_str_str_uint.put("key1", "v20", 20);

    auto all_versions_opt = map_str_str_uint.get_all_versions("key1");
    ASSERT_TRUE(all_versions_opt.has_value());
    const auto& version_map = all_versions_opt.value().get();

    ASSERT_EQ(version_map.size(), 2);
    EXPECT_EQ(version_map.at(10), "v10");
    EXPECT_EQ(version_map.at(20), "v20");

    EXPECT_FALSE(map_str_str_uint.get_all_versions("non_existent_key").has_value());
}

// Test with custom version type (SemanticVersion)
TEST_F(ValueVersionedMapTest, CustomVersionType) {
    map_str_int_semver.put("feature_A", 1, {1, 0, 0});
    map_str_int_semver.put("feature_A", 2, {1, 1, 0}); // Updated value for 1.1.0
    map_str_int_semver.put("feature_A", 3, {2, 0, 0}); // Updated value for 2.0.0
    map_str_int_semver.put("feature_B", 100, {1, 0, 5});

    ASSERT_TRUE(map_str_int_semver.get_latest("feature_A").has_value());
    EXPECT_EQ(map_str_int_semver.get_latest("feature_A").value().get(), 3);

    ASSERT_TRUE(map_str_int_semver.get("feature_A", {1, 0, 0}).has_value());
    EXPECT_EQ(map_str_int_semver.get("feature_A", {1, 0, 0}).value().get(), 1);

    ASSERT_TRUE(map_str_int_semver.get("feature_A", {1, 0, 5}).has_value()); // Should get 1.0.0
    EXPECT_EQ(map_str_int_semver.get("feature_A", {1, 0, 5}).value().get(), 1);

    ASSERT_TRUE(map_str_int_semver.get("feature_A", {1, 1, 0}).has_value());
    EXPECT_EQ(map_str_int_semver.get("feature_A", {1, 1, 0}).value().get(), 2);

    ASSERT_TRUE(map_str_int_semver.get("feature_A", {1, 5, 0}).has_value()); // Should get 1.1.0
    EXPECT_EQ(map_str_int_semver.get("feature_A", {1, 5, 0}).value().get(), 2);

    ASSERT_TRUE(map_str_int_semver.get_exact("feature_A", {1,1,0}).has_value());
    EXPECT_EQ(map_str_int_semver.get_exact("feature_A", {1,1,0}).value().get(), 2);
    EXPECT_FALSE(map_str_int_semver.get_exact("feature_A", {1,0,5}).has_value());


    EXPECT_FALSE(map_str_int_semver.get("feature_A", {0, 9, 0}).has_value()); // Too early
}

// Test iterators (basic check for begin != end)
TEST_F(ValueVersionedMapTest, Iterators) {
    EXPECT_EQ(map_str_str_uint.begin(), map_str_str_uint.end());
    EXPECT_EQ(map_str_str_uint.cbegin(), map_str_str_uint.cend());

    map_str_str_uint.put("key1", "v1", 1);
    EXPECT_NE(map_str_str_uint.begin(), map_str_str_uint.end());
    EXPECT_NE(map_str_str_uint.cbegin(), map_str_str_uint.cend());

    auto it = map_str_str_uint.begin();
    EXPECT_EQ(it->first, "key1");
    ASSERT_EQ(it->second.size(), 1);
    EXPECT_EQ(it->second.at(1), "v1");

    map_str_str_uint.put("key2", "v_other", 10);
    int count = 0;
    for (const auto& pair : map_str_str_uint) {
        count++;
        if (pair.first == "key1") {
            EXPECT_EQ(pair.second.at(1), "v1");
        } else if (pair.first == "key2") {
            EXPECT_EQ(pair.second.at(10), "v_other");
        }
    }
    EXPECT_EQ(count, 2);
}

// Test swap operation
TEST_F(ValueVersionedMapTest, SwapOperation) {
    map_str_str_uint.put("key1", "val1_map1", 1);
    map_str_str_uint.put("key1", "val2_map1", 2);
    map_str_str_uint.put("key2", "val3_map1", 3);

    cpp_collections::ValueVersionedMap<std::string, std::string> map2;
    map2.put("keyA", "valA_map2", 10);
    map2.put("keyB", "valB_map2", 20);

    map_str_str_uint.swap(map2);

    EXPECT_EQ(map_str_str_uint.size(), 2);
    EXPECT_TRUE(map_str_str_uint.contains_key("keyA"));
    EXPECT_TRUE(map_str_str_uint.contains_key("keyB"));
    EXPECT_FALSE(map_str_str_uint.contains_key("key1"));
    ASSERT_TRUE(map_str_str_uint.get_latest("keyA").has_value());
    EXPECT_EQ(map_str_str_uint.get_latest("keyA").value().get(), "valA_map2");


    EXPECT_EQ(map2.size(), 2); // Original map_str_str_uint had 2 unique keys
    EXPECT_TRUE(map2.contains_key("key1"));
    EXPECT_TRUE(map2.contains_key("key2"));
    EXPECT_FALSE(map2.contains_key("keyA"));
    ASSERT_TRUE(map2.get_latest("key1").has_value());
    EXPECT_EQ(map2.get_latest("key1").value().get(), "val2_map1");
    ASSERT_TRUE(map2.get("key1",1).has_value());
    EXPECT_EQ(map2.get("key1",1).value().get(), "val1_map1");
}

// Test equality operators
TEST_F(ValueVersionedMapTest, EqualityOperators) {
    cpp_collections::ValueVersionedMap<std::string, std::string> map1;
    map1.put("k1", "v1", 1);
    map1.put("k1", "v2", 2);
    map1.put("k2", "v3", 1);

    cpp_collections::ValueVersionedMap<std::string, std::string> map2;
    map2.put("k1", "v1", 1);
    map2.put("k1", "v2", 2);
    map2.put("k2", "v3", 1);

    cpp_collections::ValueVersionedMap<std::string, std::string> map3;
    map3.put("k1", "v1", 1);
    map3.put("k1", "v_changed", 2); // Different value
    map3.put("k2", "v3", 1);

    cpp_collections::ValueVersionedMap<std::string, std::string> map4;
    map4.put("k1", "v1", 1);
    map4.put("k1", "v2", 2);
    // k2 missing

    cpp_collections::ValueVersionedMap<std::string, std::string> map5;
    map5.put("k1", "v1", 1);
    map5.put("k1", "v2", 2);
    map5.put("k2", "v3", 1);
    map5.put("k3", "v4", 1); // Extra key

    EXPECT_TRUE(map1 == map2);
    EXPECT_FALSE(map1 != map2);

    EXPECT_FALSE(map1 == map3);
    EXPECT_TRUE(map1 != map3);

    EXPECT_FALSE(map1 == map4);
    EXPECT_TRUE(map1 != map4);

    EXPECT_FALSE(map1 == map5);
    EXPECT_TRUE(map1 != map5);

    // Test empty maps
    cpp_collections::ValueVersionedMap<std::string, std::string> empty1, empty2;
    EXPECT_TRUE(empty1 == empty2);
}

// Test modifying value through non-const get
TEST_F(ValueVersionedMapTest, ModifyViaNonConstGet) {
    map_int_double_int.put(10, 1.0, 100);
    map_int_double_int.put(10, 2.0, 200);

    auto val_ref_opt = map_int_double_int.get_latest(10);
    ASSERT_TRUE(val_ref_opt.has_value());
    val_ref_opt.value().get() = 2.5; // Modify the latest value

    ASSERT_TRUE(map_int_double_int.get_latest(10).has_value());
    EXPECT_DOUBLE_EQ(map_int_double_int.get_latest(10).value().get(), 2.5);
    // Check if the original version (200) was modified
    ASSERT_TRUE(map_int_double_int.get_exact(10, 200).has_value());
    EXPECT_DOUBLE_EQ(map_int_double_int.get_exact(10, 200).value().get(), 2.5);


    auto val_at_ver_opt = map_int_double_int.get(10, 150); // Should get version 100
    ASSERT_TRUE(val_at_ver_opt.has_value());
    val_at_ver_opt.value().get() = 1.5; // Modify value at version 100

    ASSERT_TRUE(map_int_double_int.get_exact(10, 100).has_value());
    EXPECT_DOUBLE_EQ(map_int_double_int.get_exact(10, 100).value().get(), 1.5);

    // Ensure other versions are not affected by modifying version 100
    ASSERT_TRUE(map_int_double_int.get_exact(10, 200).has_value());
    EXPECT_DOUBLE_EQ(map_int_double_int.get_exact(10, 200).value().get(), 2.5); // Still 2.5
}

// Test case where upper_bound is begin() in get()
TEST_F(ValueVersionedMapTest, GetVersionSmallerThanAllExisting) {
    map_str_str_uint.put("key1", "value_v100", 100);
    map_str_str_uint.put("key1", "value_v200", 200);

    // Request a version smaller than any existing version for "key1"
    auto result = map_str_str_uint.get("key1", 50);
    EXPECT_FALSE(result.has_value());

    // Request a version smaller than any existing version for a non-existent key
    auto result_ne = map_str_str_uint.get("non_existent_key", 50);
    EXPECT_FALSE(result_ne.has_value());
}

// Test with rvalue keys and values for put
TEST_F(ValueVersionedMapTest, PutRvalues) {
    std::string k = "rkey";
    std::string v = "rval";
    map_str_str_uint.put(std::move(k), std::move(v), 1);

    ASSERT_TRUE(map_str_str_uint.contains_key("rkey"));
    ASSERT_TRUE(map_str_str_uint.get_latest("rkey").has_value());
    EXPECT_EQ(map_str_str_uint.get_latest("rkey").value().get(), "rval");
    // k and v are in moved-from state, their content is unspecified.
}

// Test what happens when all versions of a key are removed one by one
TEST_F(ValueVersionedMapTest, RemoveAllVersionsOneByOne) {
    map_str_str_uint.put("key1", "v1", 1);
    map_str_str_uint.put("key1", "v2", 2);
    map_str_str_uint.put("key1", "v3", 3);

    EXPECT_EQ(map_str_str_uint.size(), 1);
    EXPECT_EQ(map_str_str_uint.total_versions(), 3);

    ASSERT_TRUE(map_str_str_uint.remove_version("key1", 1));
    EXPECT_TRUE(map_str_str_uint.contains_key("key1"));
    EXPECT_EQ(map_str_str_uint.total_versions(), 2);

    ASSERT_TRUE(map_str_str_uint.remove_version("key1", 3)); // Remove latest
    EXPECT_TRUE(map_str_str_uint.contains_key("key1"));
    EXPECT_EQ(map_str_str_uint.total_versions(), 1);
    ASSERT_TRUE(map_str_str_uint.get_latest("key1").has_value());
    EXPECT_EQ(map_str_str_uint.get_latest("key1").value().get(), "v2");


    ASSERT_TRUE(map_str_str_uint.remove_version("key1", 2)); // Remove the last one
    EXPECT_FALSE(map_str_str_uint.contains_key("key1")); // Key should be gone
    EXPECT_EQ(map_str_str_uint.size(), 0);
    EXPECT_EQ(map_str_str_uint.total_versions(), 0);
}
// TODO: Add more tests for edge cases, constructors, allocators if necessary.

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

// Note: Ensure value_versioned_map.h is in the include path for compilation.
// Link with GTest libraries.
/*
Example CMake linkage:
target_link_libraries(your_test_executable PRIVATE GTest::gtest GTest::gtest_main YourLibraryTarget)
*/

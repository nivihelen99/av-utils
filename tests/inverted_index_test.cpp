#include "gtest/gtest.h"
#include "inverted_index.h" // Adjust path as necessary based on include directories
#include <string>
#include <vector>
#include <algorithm> // For std::sort for comparing vectors if needed

// Basic test fixture
class InvertedIndexTest : public ::testing::Test {
protected:
    InvertedIndex<std::string, int> index;
    InvertedIndex<int, std::string> doc_tags_index; // For more semantic examples
};

TEST_F(InvertedIndexTest, InitialState) {
    EXPECT_TRUE(index.empty());
    EXPECT_TRUE(index.values_for("nonexistent_key").empty());
    EXPECT_TRUE(index.keys_for(12345).empty());
    EXPECT_FALSE(index.contains("key1", 1));
}

TEST_F(InvertedIndexTest, AddSingleEntry) {
    index.add("key1", 100);
    EXPECT_FALSE(index.empty());
    EXPECT_TRUE(index.contains("key1", 100));
    EXPECT_FALSE(index.contains("key1", 200));
    EXPECT_FALSE(index.contains("key2", 100));

    const auto& values = index.values_for("key1");
    ASSERT_EQ(values.size(), 1);
    EXPECT_NE(values.find(100), values.end());

    const auto& keys = index.keys_for(100);
    ASSERT_EQ(keys.size(), 1);
    EXPECT_NE(keys.find("key1"), keys.end());
}

TEST_F(InvertedIndexTest, AddMultipleValuesForKey) {
    index.add("doc1", 1); // tag 1
    index.add("doc1", 2); // tag 2
    index.add("doc1", 3); // tag 3

    EXPECT_TRUE(index.contains("doc1", 1));
    EXPECT_TRUE(index.contains("doc1", 2));
    EXPECT_TRUE(index.contains("doc1", 3));
    EXPECT_FALSE(index.contains("doc1", 4));

    const auto& values = index.values_for("doc1");
    ASSERT_EQ(values.size(), 3);
    EXPECT_NE(values.find(1), values.end());
    EXPECT_NE(values.find(2), values.end());
    EXPECT_NE(values.find(3), values.end());

    // Check reverse mapping
    const auto& keys_for_tag1 = index.keys_for(1);
    ASSERT_EQ(keys_for_tag1.size(), 1);
    EXPECT_NE(keys_for_tag1.find("doc1"), keys_for_tag1.end());

    const auto& keys_for_tag2 = index.keys_for(2);
    ASSERT_EQ(keys_for_tag2.size(), 1);
    EXPECT_NE(keys_for_tag2.find("doc1"), keys_for_tag2.end());
}

TEST_F(InvertedIndexTest, AddMultipleKeysForValue) {
    index.add("doc1", 10);
    index.add("doc2", 10);
    index.add("doc3", 10);

    EXPECT_TRUE(index.contains("doc1", 10));
    EXPECT_TRUE(index.contains("doc2", 10));
    EXPECT_TRUE(index.contains("doc3", 10));
    EXPECT_FALSE(index.contains("doc4", 10));

    const auto& keys = index.keys_for(10);
    ASSERT_EQ(keys.size(), 3);
    EXPECT_NE(keys.find("doc1"), keys.end());
    EXPECT_NE(keys.find("doc2"), keys.end());
    EXPECT_NE(keys.find("doc3"), keys.end());

    // Check forward mapping
    const auto& values_for_doc1 = index.values_for("doc1");
    ASSERT_EQ(values_for_doc1.size(), 1);
    EXPECT_NE(values_for_doc1.find(10), values_for_doc1.end());

    const auto& values_for_doc2 = index.values_for("doc2");
    ASSERT_EQ(values_for_doc2.size(), 1);
    EXPECT_NE(values_for_doc2.find(10), values_for_doc2.end());
}

TEST_F(InvertedIndexTest, AddComplexScenario) {
    // Document tagging example
    // doc1: cpp, programming, high-performance
    // doc2: cpp, search, library
    // doc3: java, programming, enterprise
    doc_tags_index.add(1, "cpp");
    doc_tags_index.add(1, "programming");
    doc_tags_index.add(1, "high-performance");

    doc_tags_index.add(2, "cpp");
    doc_tags_index.add(2, "search");
    doc_tags_index.add(2, "library");

    doc_tags_index.add(3, "java");
    doc_tags_index.add(3, "programming");
    doc_tags_index.add(3, "enterprise");

    // --- Verification ---
    // Values for doc1
    const auto& tags_for_doc1 = doc_tags_index.values_for(1);
    ASSERT_EQ(tags_for_doc1.size(), 3);
    EXPECT_NE(tags_for_doc1.find("cpp"), tags_for_doc1.end());
    EXPECT_NE(tags_for_doc1.find("programming"), tags_for_doc1.end());
    EXPECT_NE(tags_for_doc1.find("high-performance"), tags_for_doc1.end());

    // Values for doc2
    const auto& tags_for_doc2 = doc_tags_index.values_for(2);
    ASSERT_EQ(tags_for_doc2.size(), 3);
    EXPECT_NE(tags_for_doc2.find("cpp"), tags_for_doc2.end());
    EXPECT_NE(tags_for_doc2.find("search"), tags_for_doc2.end());
    EXPECT_NE(tags_for_doc2.find("library"), tags_for_doc2.end());

    // Keys for "cpp"
    const auto& docs_for_cpp = doc_tags_index.keys_for("cpp");
    ASSERT_EQ(docs_for_cpp.size(), 2);
    EXPECT_NE(docs_for_cpp.find(1), docs_for_cpp.end());
    EXPECT_NE(docs_for_cpp.find(2), docs_for_cpp.end());

    // Keys for "programming"
    const auto& docs_for_programming = doc_tags_index.keys_for("programming");
    ASSERT_EQ(docs_for_programming.size(), 2);
    EXPECT_NE(docs_for_programming.find(1), docs_for_programming.end());
    EXPECT_NE(docs_for_programming.find(3), docs_for_programming.end());

    // Keys for "library"
    const auto& docs_for_library = doc_tags_index.keys_for("library");
    ASSERT_EQ(docs_for_library.size(), 1);
    EXPECT_NE(docs_for_library.find(2), docs_for_library.end());

    // Contains checks
    EXPECT_TRUE(doc_tags_index.contains(1, "cpp"));
    EXPECT_TRUE(doc_tags_index.contains(2, "cpp"));
    EXPECT_FALSE(doc_tags_index.contains(3, "cpp"));
    EXPECT_TRUE(doc_tags_index.contains(1, "programming"));
    EXPECT_FALSE(doc_tags_index.contains(2, "programming")); // doc2 does not have "programming" directly, it has "search", "library"
    EXPECT_TRUE(doc_tags_index.contains(3, "programming"));

    // Check a non-existent mapping
    EXPECT_FALSE(doc_tags_index.contains(1, "nonexistent_tag"));
    EXPECT_TRUE(doc_tags_index.values_for(999).empty()); // Non-existent doc
    EXPECT_TRUE(doc_tags_index.keys_for("nonexistent_tag_key").empty()); // Non-existent tag
}

TEST_F(InvertedIndexTest, IdempotencyOfAdd) {
    index.add("key1", 100);
    index.add("key1", 100); // Add again
    index.add("key1", 100); // And again

    EXPECT_TRUE(index.contains("key1", 100));
    const auto& values = index.values_for("key1");
    ASSERT_EQ(values.size(), 1);
    EXPECT_NE(values.find(100), values.end());

    const auto& keys = index.keys_for(100);
    ASSERT_EQ(keys.size(), 1);
    EXPECT_NE(keys.find("key1"), keys.end());

    index.add("key1", 200);
    index.add("key1", 100); // Add original again after a new one

    const auto& values_updated = index.values_for("key1");
    ASSERT_EQ(values_updated.size(), 2);
    EXPECT_NE(values_updated.find(100), values_updated.end());
    EXPECT_NE(values_updated.find(200), values_updated.end());

    const auto& keys_for_100_updated = index.keys_for(100);
    ASSERT_EQ(keys_for_100_updated.size(), 1);
    EXPECT_NE(keys_for_100_updated.find("key1"), keys_for_100_updated.end());

    const auto& keys_for_200 = index.keys_for(200);
    ASSERT_EQ(keys_for_200.size(), 1);
    EXPECT_NE(keys_for_200.find("key1"), keys_for_200.end());
}

TEST_F(InvertedIndexTest, ValuesForNonExistentKey) {
    const auto& values = index.values_for("phantom_key");
    EXPECT_TRUE(values.empty());
}

TEST_F(InvertedIndexTest, KeysForNonExistentValue) {
    const auto& keys = index.keys_for(99999);
    EXPECT_TRUE(keys.empty());
}

// Test with custom hash and equality for Key and Value (using simple structs)
struct CustomKey {
    int id;
    std::string name;

    bool operator==(const CustomKey& other) const {
        return id == other.id && name == other.name;
    }
};

struct CustomKeyHash {
    std::size_t operator()(const CustomKey& k) const {
        return std::hash<int>()(k.id) ^ (std::hash<std::string>()(k.name) << 1);
    }
};

struct CustomValue {
    int value_id;

    bool operator==(const CustomValue& other) const {
        return value_id == other.value_id;
    }
};

struct CustomValueHash {
    std::size_t operator()(const CustomValue& v) const {
        return std::hash<int>()(v.value_id);
    }
};


TEST(InvertedIndexCustomTypeTest, AddAndRetrieveWithCustomTypes) {
    InvertedIndex<CustomKey, CustomValue, CustomKeyHash, std::equal_to<CustomKey>, CustomValueHash, std::equal_to<CustomValue>> custom_index;

    CustomKey k1 = {1, "one"};
    CustomValue v1 = {101};
    CustomKey k2 = {2, "two"};
    CustomValue v2 = {102};

    custom_index.add(k1, v1);
    custom_index.add(k1, v2);
    custom_index.add(k2, v1);

    EXPECT_TRUE(custom_index.contains(k1, v1));
    EXPECT_TRUE(custom_index.contains(k1, v2));
    EXPECT_TRUE(custom_index.contains(k2, v1));
    EXPECT_FALSE(custom_index.contains(k2, v2));

    const auto& values_for_k1 = custom_index.values_for(k1);
    ASSERT_EQ(values_for_k1.size(), 2);
    EXPECT_NE(values_for_k1.find(v1), values_for_k1.end());
    EXPECT_NE(values_for_k1.find(v2), values_for_k1.end());

    const auto& keys_for_v1 = custom_index.keys_for(v1);
    ASSERT_EQ(keys_for_v1.size(), 2);
    EXPECT_NE(keys_for_v1.find(k1), keys_for_v1.end());
    EXPECT_NE(keys_for_v1.find(k2), keys_for_v1.end());
}

// Test with default std::equal_to for custom types (if operator== is defined)
struct SimpleKey {
    int id;
    bool operator==(const SimpleKey& other) const { return id == other.id; }
};
namespace std {
    template <> struct hash<SimpleKey> {
        size_t operator()(const SimpleKey& k) const { return hash<int>()(k.id); }
    };
}

struct SimpleValue {
    std::string data;
    bool operator==(const SimpleValue& other) const { return data == other.data; }
};
namespace std {
    template <> struct hash<SimpleValue> {
        size_t operator()(const SimpleValue& v) const { return hash<std::string>()(v.data); }
    };
}

TEST(InvertedIndexCustomTypeStdHashTest, AddAndRetrieveWithStdHashableCustomTypes) {
    InvertedIndex<SimpleKey, SimpleValue> custom_index_std;

    SimpleKey sk1 = {1};
    SimpleValue sv1 = {"alpha"};
    SimpleKey sk2 = {2};
    SimpleValue sv2 = {"beta"};

    custom_index_std.add(sk1, sv1);
    custom_index_std.add(sk1, sv2);
    custom_index_std.add(sk2, sv1);

    EXPECT_TRUE(custom_index_std.contains(sk1, sv1));
    EXPECT_TRUE(custom_index_std.contains(sk1, sv2));
    EXPECT_TRUE(custom_index_std.contains(sk2, sv1));
    EXPECT_FALSE(custom_index_std.contains(sk2, sv2));

    const auto& values_for_sk1 = custom_index_std.values_for(sk1);
    ASSERT_EQ(values_for_sk1.size(), 2);
    EXPECT_NE(values_for_sk1.find(sv1), values_for_sk1.end());
    EXPECT_NE(values_for_sk1.find(sv2), values_for_sk1.end());

    const auto& keys_for_sv1 = custom_index_std.keys_for(sv1);
    ASSERT_EQ(keys_for_sv1.size(), 2);
    EXPECT_NE(keys_for_sv1.find(sk1), keys_for_sv1.end());
    EXPECT_NE(keys_for_sv1.find(sk2), keys_for_sv1.end());
}

// --- Tests for Remove Operations ---

TEST_F(InvertedIndexTest, RemoveSingleMapping) {
    index.add("key1", 100);
    index.add("key1", 200);
    index.add("key2", 100);

    ASSERT_TRUE(index.contains("key1", 100));
    ASSERT_TRUE(index.contains("key1", 200));
    ASSERT_TRUE(index.contains("key2", 100));
    ASSERT_EQ(index.values_for("key1").size(), 2);
    ASSERT_EQ(index.keys_for(100).size(), 2);

    index.remove("key1", 100);

    EXPECT_FALSE(index.contains("key1", 100));
    EXPECT_TRUE(index.contains("key1", 200)); // Should still be there
    EXPECT_TRUE(index.contains("key2", 100)); // Should still be there

    const auto& values_for_key1 = index.values_for("key1");
    ASSERT_EQ(values_for_key1.size(), 1);
    EXPECT_NE(values_for_key1.find(200), values_for_key1.end());

    const auto& keys_for_100 = index.keys_for(100);
    ASSERT_EQ(keys_for_100.size(), 1);
    EXPECT_NE(keys_for_100.find("key2"), keys_for_100.end());

    // Remove last value for a key, key should be removed from forward_map
    index.remove("key1", 200);
    EXPECT_FALSE(index.contains("key1", 200));
    EXPECT_TRUE(index.values_for("key1").empty());
    EXPECT_FALSE(index.key_count() == 2); // key1 should be gone, only key2 remains
    EXPECT_EQ(index.key_count(), 1);


    // Remove last key for a value, value should be removed from reverse_map
    index.add("key3", 300); // Add something new
    index.remove("key2", 100); // key2 was the last key for value 100
    EXPECT_FALSE(index.contains("key2", 100));
    EXPECT_TRUE(index.keys_for(100).empty());
}

TEST_F(InvertedIndexTest, RemoveNonExistentMapping) {
    index.add("key1", 100);
    ASSERT_TRUE(index.contains("key1", 100));
    ASSERT_EQ(index.values_for("key1").size(), 1);
    ASSERT_EQ(index.keys_for(100).size(), 1);

    // Try removing a value not mapped to key1
    index.remove("key1", 999);
    EXPECT_TRUE(index.contains("key1", 100)); // Original should still be there
    EXPECT_EQ(index.values_for("key1").size(), 1);

    // Try removing a key not in the index
    index.remove("nonexistent_key", 100);
    EXPECT_TRUE(index.contains("key1", 100));
    EXPECT_EQ(index.keys_for(100).size(), 1);

    // Try removing a value not in the index at all
    index.remove("key1", 777);
    EXPECT_TRUE(index.contains("key1", 100));
}

TEST_F(InvertedIndexTest, RemoveKey) {
    doc_tags_index.add(1, "cpp");
    doc_tags_index.add(1, "programming");
    doc_tags_index.add(2, "cpp");
    doc_tags_index.add(2, "search");
    doc_tags_index.add(3, "java");

    ASSERT_TRUE(doc_tags_index.contains(1, "cpp"));
    ASSERT_TRUE(doc_tags_index.contains(1, "programming"));
    ASSERT_TRUE(doc_tags_index.contains(2, "cpp"));
    ASSERT_EQ(doc_tags_index.keys_for("cpp").size(), 2);
    ASSERT_EQ(doc_tags_index.values_for(1).size(), 2);

    doc_tags_index.remove_key(1); // Remove all tags for doc 1

    EXPECT_FALSE(doc_tags_index.contains(1, "cpp"));
    EXPECT_FALSE(doc_tags_index.contains(1, "programming"));
    EXPECT_TRUE(doc_tags_index.values_for(1).empty()); // Doc 1 should have no tags

    // Check reverse map updates
    const auto& docs_for_cpp = doc_tags_index.keys_for("cpp");
    ASSERT_EQ(docs_for_cpp.size(), 1);
    EXPECT_NE(docs_for_cpp.find(2), docs_for_cpp.end()); // Only doc 2 should have "cpp"

    const auto& docs_for_programming = doc_tags_index.keys_for("programming");
    EXPECT_TRUE(docs_for_programming.empty()); // "programming" was only on doc 1

    // Check other keys are unaffected
    EXPECT_TRUE(doc_tags_index.contains(2, "cpp"));
    EXPECT_TRUE(doc_tags_index.contains(3, "java"));

    // Remove a key that doesn't exist
    doc_tags_index.remove_key(999); // Should not crash or change state
    EXPECT_TRUE(doc_tags_index.contains(2, "cpp"));
    EXPECT_EQ(doc_tags_index.keys_for("cpp").size(), 1);
}

TEST_F(InvertedIndexTest, RemoveValue) {
    doc_tags_index.add(1, "cpp");
    doc_tags_index.add(1, "programming");
    doc_tags_index.add(2, "cpp");
    doc_tags_index.add(2, "search");
    doc_tags_index.add(3, "cpp"); // Doc 3 also has "cpp"

    ASSERT_TRUE(doc_tags_index.contains(1, "cpp"));
    ASSERT_TRUE(doc_tags_index.contains(2, "cpp"));
    ASSERT_TRUE(doc_tags_index.contains(3, "cpp"));
    ASSERT_EQ(doc_tags_index.keys_for("cpp").size(), 3);
    ASSERT_EQ(doc_tags_index.values_for(1).count("cpp"), 1);

    doc_tags_index.remove_value("cpp"); // Remove "cpp" tag from all documents

    EXPECT_FALSE(doc_tags_index.contains(1, "cpp"));
    EXPECT_FALSE(doc_tags_index.contains(2, "cpp"));
    EXPECT_FALSE(doc_tags_index.contains(3, "cpp"));
    EXPECT_TRUE(doc_tags_index.keys_for("cpp").empty()); // No docs should have "cpp"

    // Check forward map updates
    EXPECT_FALSE(doc_tags_index.values_for(1).count("cpp"));
    EXPECT_TRUE(doc_tags_index.values_for(1).count("programming")); // This should remain

    EXPECT_FALSE(doc_tags_index.values_for(2).count("cpp"));
    EXPECT_TRUE(doc_tags_index.values_for(2).count("search")); // This should remain

    EXPECT_FALSE(doc_tags_index.values_for(3).count("cpp"));
    EXPECT_TRUE(doc_tags_index.values_for(3).empty()); // Doc 3 only had "cpp"

    // Remove a value that doesn't exist
    doc_tags_index.remove_value("nonexistent_tag"); // Should not crash
    EXPECT_TRUE(doc_tags_index.values_for(1).count("programming"));
}

// --- Tests for Clear and Empty ---
TEST_F(InvertedIndexTest, ClearAndEmpty) {
    EXPECT_TRUE(index.empty());
    index.add("key1", 1);
    index.add("key2", 2);
    EXPECT_FALSE(index.empty());
    ASSERT_EQ(index.key_count(), 2);

    index.clear();
    EXPECT_TRUE(index.empty());
    EXPECT_EQ(index.key_count(), 0);
    EXPECT_TRUE(index.values_for("key1").empty());
    EXPECT_TRUE(index.keys_for(1).empty());
    EXPECT_FALSE(index.contains("key1",1));

    // Clear an already empty index
    index.clear();
    EXPECT_TRUE(index.empty());
}

// --- Tests for Copy and Move Semantics ---
TEST_F(InvertedIndexTest, CopyConstructor) {
    index.add("doc1", 10);
    index.add("doc1", 20);
    index.add("doc2", 10);

    InvertedIndex<std::string, int> copied_index(index);

    // Check copied state
    EXPECT_FALSE(copied_index.empty());
    EXPECT_EQ(copied_index.key_count(), 2);
    EXPECT_TRUE(copied_index.contains("doc1", 10));
    EXPECT_TRUE(copied_index.contains("doc1", 20));
    EXPECT_TRUE(copied_index.contains("doc2", 10));
    ASSERT_EQ(copied_index.values_for("doc1").size(), 2);
    ASSERT_EQ(copied_index.keys_for(10).size(), 2);

    // Modify original, copied should not change
    index.add("doc3", 30);
    EXPECT_TRUE(index.contains("doc3", 30));
    EXPECT_FALSE(copied_index.contains("doc3", 30));
    EXPECT_EQ(copied_index.key_count(), 2);


    // Modify copied, original should not change
    copied_index.remove("doc1", 10);
    EXPECT_FALSE(copied_index.contains("doc1", 10));
    EXPECT_TRUE(index.contains("doc1", 10)); // Original still has it
}

TEST_F(InvertedIndexTest, CopyAssignmentOperator) {
    index.add("doc1", 10);
    index.add("doc2", 20);

    InvertedIndex<std::string, int> assigned_index;
    assigned_index.add("temp", 99); // Add some initial data

    assigned_index = index;

    // Check assigned state
    EXPECT_FALSE(assigned_index.empty());
    EXPECT_EQ(assigned_index.key_count(), 2);
    EXPECT_TRUE(assigned_index.contains("doc1", 10));
    EXPECT_TRUE(assigned_index.contains("doc2", 20));
    EXPECT_FALSE(assigned_index.contains("temp", 99)); // Original data gone

    // Modify original, assigned should not change
    index.add("doc3", 30);
    EXPECT_FALSE(assigned_index.contains("doc3", 30));

    // Self-assignment
    assigned_index = assigned_index; // Should not crash or corrupt
    EXPECT_TRUE(assigned_index.contains("doc1", 10));
    EXPECT_EQ(assigned_index.key_count(), 2);
}

TEST_F(InvertedIndexTest, MoveConstructor) {
    index.add("doc1", 10);
    index.add("doc1", 20);
    index.add("doc2", 10);

    // Need to get data before move to verify
    auto original_k1_values = index.values_for("doc1");
    ASSERT_EQ(original_k1_values.size(), 2);

    InvertedIndex<std::string, int> moved_index(std::move(index));

    // Check moved state
    EXPECT_FALSE(moved_index.empty());
    EXPECT_EQ(moved_index.key_count(), 2);
    EXPECT_TRUE(moved_index.contains("doc1", 10));
    EXPECT_TRUE(moved_index.contains("doc1", 20));
    EXPECT_TRUE(moved_index.contains("doc2", 10));
    ASSERT_EQ(moved_index.values_for("doc1").size(), 2);
    ASSERT_EQ(moved_index.keys_for(10).size(), 2);

    // Check original (source) state - should be valid but empty or unspecified
    // For std::unordered_map, moved-from state is valid but unspecified.
    // We expect it to be empty as per typical move semantics for containers.
    EXPECT_TRUE(index.empty()); // Assuming typical move leaves source empty
    EXPECT_EQ(index.key_count(), 0);
}

TEST_F(InvertedIndexTest, MoveAssignmentOperator) {
    index.add("doc1", 10);
    index.add("doc2", 20);

    InvertedIndex<std::string, int> moved_assigned_index;
    moved_assigned_index.add("temp", 99); // Initial data

    moved_assigned_index = std::move(index);

    // Check assigned state
    EXPECT_FALSE(moved_assigned_index.empty());
    EXPECT_EQ(moved_assigned_index.key_count(), 2);
    EXPECT_TRUE(moved_assigned_index.contains("doc1", 10));
    EXPECT_TRUE(moved_assigned_index.contains("doc2", 20));
    EXPECT_FALSE(moved_assigned_index.contains("temp", 99));

    // Check original (source) state
    EXPECT_TRUE(index.empty());
    EXPECT_EQ(index.key_count(), 0);

    // Self-move assignment
    // Note: Self-move assignment behavior can be tricky.
    // A robust implementation should handle it, typically resulting in the object remaining valid.
    // The standard doesn't strictly guarantee the object is unchanged but it must be in a valid state.
    // For our map-based implementation, moving members to themselves should be okay.
    InvertedIndex<std::string, int> self_move_test;
    self_move_test.add("self", 1);
    self_move_test = std::move(self_move_test);
    EXPECT_TRUE(self_move_test.contains("self", 1)); // Should ideally still be there or be validly empty
    // Depending on how std::unordered_map handles self-move, it might be empty or unchanged.
    // Let's assume it should remain valid. If it's empty, key_count would be 0.
    // If it's unchanged, key_count would be 1.
    // The important part is that it's in a valid state and doesn't crash.
    // A common pattern is for self-move to be a no-op if guarded by `if (this != &other)`.
}

// Re-verify idempotency of add (already partially tested)
TEST_F(InvertedIndexTest, AddIdempotencyStress) {
    index.add("k1", 1);
    index.add("k1", 1);
    index.add("k1", 2);
    index.add("k1", 2);
    index.add("k2", 1);
    index.add("k2", 1);

    EXPECT_EQ(index.values_for("k1").size(), 2);
    EXPECT_TRUE(index.values_for("k1").count(1));
    EXPECT_TRUE(index.values_for("k1").count(2));

    EXPECT_EQ(index.values_for("k2").size(), 1);
    EXPECT_TRUE(index.values_for("k2").count(1));

    EXPECT_EQ(index.keys_for(1).size(), 2);
    EXPECT_TRUE(index.keys_for(1).count("k1"));
    EXPECT_TRUE(index.keys_for(1).count("k2"));

    EXPECT_EQ(index.keys_for(2).size(), 1);
    EXPECT_TRUE(index.keys_for(2).count("k1"));
}

// Re-verify empty state queries (already partially tested in InitialState)
TEST_F(InvertedIndexTest, EmptyStateQueriesComprehensive) {
    EXPECT_TRUE(index.empty());
    EXPECT_EQ(index.key_count(), 0);

    const auto& vals = index.values_for("any");
    EXPECT_TRUE(vals.empty());
    // Test that the returned reference is to the static empty set
    const auto& vals2 = index.values_for("another");
    EXPECT_EQ(&vals, &index.EMPTY_VALUE_SET); // Check if it's THE static empty set
    EXPECT_EQ(&vals2, &index.EMPTY_VALUE_SET);


    const auto& keys = index.keys_for(0);
    EXPECT_TRUE(keys.empty());
    const auto& keys2 = index.keys_for(1);
    EXPECT_EQ(&keys, &index.EMPTY_KEY_SET);
    EXPECT_EQ(&keys2, &index.EMPTY_KEY_SET);


    EXPECT_FALSE(index.contains("any", 0));

    // Operations on empty should not fail
    index.remove("any", 0);
    index.remove_key("any");
    index.remove_value(0);
    EXPECT_TRUE(index.empty()); // Still empty

    index.clear(); // Clear an empty index
    EXPECT_TRUE(index.empty());
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

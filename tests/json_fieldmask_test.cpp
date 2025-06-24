#include "gtest/gtest.h"
#include "../include/json_fieldmask.h" // Adjust path as necessary
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace fieldmask;

// Test fixture for FieldMask tests
class FieldMaskTest : public ::testing::Test {
protected:
    FieldMask mask;
};

// Tests for FieldMask class
TEST_F(FieldMaskTest, AddPath) {
    mask.add_path("/a/b/c");
    ASSERT_TRUE(mask.contains("/a/b/c"));
    ASSERT_EQ(mask.get_paths().size(), 1);
}

TEST_F(FieldMaskTest, ContainsPath) {
    mask.add_path("/a/b");
    ASSERT_TRUE(mask.contains("/a/b"));
    ASSERT_FALSE(mask.contains("/a/x"));
}

TEST_F(FieldMaskTest, ContainsPrefix) {
    mask.add_path("/a/b/c");
    mask.add_path("/a/b/d");
    mask.add_path("/x/y");
    ASSERT_TRUE(mask.contains_prefix("/a/b"));
    ASSERT_TRUE(mask.contains_prefix("/a"));
    ASSERT_FALSE(mask.contains_prefix("/a/c"));
    ASSERT_TRUE(mask.contains_prefix("/x/y")); // Exact match is also a prefix
    ASSERT_FALSE(mask.contains_prefix("/z"));
}

TEST_F(FieldMaskTest, EmptyAndClear) {
    ASSERT_TRUE(mask.empty());
    mask.add_path("/test");
    ASSERT_FALSE(mask.empty());
    mask.clear();
    ASSERT_TRUE(mask.empty());
}

TEST_F(FieldMaskTest, Merge) {
    FieldMask other_mask;
    mask.add_path("/a");
    other_mask.add_path("/b");
    other_mask.add_path("/a"); // Overlap

    mask.merge(other_mask);
    ASSERT_TRUE(mask.contains("/a"));
    ASSERT_TRUE(mask.contains("/b"));
    ASSERT_EQ(mask.get_paths().size(), 2);
}

TEST_F(FieldMaskTest, ToString) {
    // Test empty mask
    ASSERT_EQ(mask.to_string(), "FieldMask{}");

    mask.add_path("/b/c");
    mask.add_path("/a"); // Paths are stored in std::set, so they will be ordered
    
    // The order in the set will be "/a", then "/b/c"
    std::string expected_str = "FieldMask{\"/a\", \"/b/c\"}";
    ASSERT_EQ(mask.to_string(), expected_str);
}

// Tests for path_utils namespace
TEST(PathUtilsTest, EscapeComponent) {
    ASSERT_EQ(path_utils::escape_component("foo"), "foo");
    ASSERT_EQ(path_utils::escape_component("foo/bar"), "foo~1bar");
    ASSERT_EQ(path_utils::escape_component("foo~bar"), "foo~0bar");
    ASSERT_EQ(path_utils::escape_component("foo~/bar"), "foo~0~1bar");
}

TEST(PathUtilsTest, BuildPath) {
    ASSERT_EQ(path_utils::build_path({}), "");
    ASSERT_EQ(path_utils::build_path({"a", "b", "c"}), "/a/b/c");
    ASSERT_EQ(path_utils::build_path({"foo/bar", "baz~qux"}), "/foo~1bar/baz~0qux");
    ASSERT_EQ(path_utils::build_path({"config", "interfaces", "0", "name"}), "/config/interfaces/0/name");
}

TEST(PathUtilsTest, SplitPath) {
    ASSERT_EQ(path_utils::split_path("").size(), 0);
    ASSERT_EQ(path_utils::split_path("/").size(), 0);
    
    std::vector<std::string> components1 = {"a", "b", "c"};
    ASSERT_EQ(path_utils::split_path("/a/b/c"), components1);
    
    std::vector<std::string> components2 = {"foo~1bar", "baz~0qux"}; // Note: split_path does not unescape
    ASSERT_EQ(path_utils::split_path("/foo~1bar/baz~0qux"), components2);
}

TEST(PathUtilsTest, GetParentPath) {
    ASSERT_EQ(path_utils::get_parent_path(""), "");
    ASSERT_EQ(path_utils::get_parent_path("/"), ""); // Or decide if "/" is its own parent, original code suggests ""
    ASSERT_EQ(path_utils::get_parent_path("/a"), "/");
    ASSERT_EQ(path_utils::get_parent_path("/a/b"), "/a");
    ASSERT_EQ(path_utils::get_parent_path("/a/b/c"), "/a/b");
}


// Tests for utility functions
class FieldMaskUtilsTest : public ::testing::Test {
protected:
    json json_a = {
        {"name", "Alice"},
        {"age", 30},
        {"address", {
            {"street", "123 Main St"},
            {"city", "Anytown"}
        }},
        {"hobbies", json::array({"reading", "hiking"})}
    };

    json json_b = {
        {"name", "Bob"}, // changed
        {"age", 30},     // same
        {"address", {
            {"street", "123 Main St"},
            {"city", "Otherville"} // changed
        }},
        {"hobbies", json::array({"reading", "cycling"})}, // changed
        {"occupation", "Engineer"} // added
    };
};

TEST_F(FieldMaskUtilsTest, DiffFieldsBasic) {
    FieldMask mask = diff_fields(json_a, json_b);
    
    ASSERT_TRUE(mask.contains("/name"));
    ASSERT_FALSE(mask.contains("/age")); // Should not be in mask
    ASSERT_TRUE(mask.contains("/address/city"));
    ASSERT_FALSE(mask.contains("/address/street")); // Should not be in mask
    ASSERT_TRUE(mask.contains("/hobbies/1")); // "hiking" vs "cycling"
    ASSERT_FALSE(mask.contains("/hobbies/0")); // "reading" is same
    ASSERT_TRUE(mask.contains("/occupation")); // Added field
    
    // Check for paths that shouldn't exist if only leaf nodes are reported for non-object/array types
    ASSERT_FALSE(mask.contains("/address")); // This would be true if parent path of changed child is added
                                             // The current implementation adds the most specific differing path.
}

TEST_F(FieldMaskUtilsTest, DiffFieldsTypeChange) {
    json j1 = {{"value", 10}};
    json j2 = {{"value", "10"}}; // Type changed from int to string
    FieldMask mask = diff_fields(j1, j2);
    ASSERT_TRUE(mask.contains("/value"));
}

TEST_F(FieldMaskUtilsTest, DiffFieldsArrayLengthChange) {
    json j1 = {{"arr", {1, 2}}};
    json j2 = {{"arr", {1, 2, 3}}}; // Element added
    FieldMask mask = diff_fields(j1, j2);
    ASSERT_TRUE(mask.contains("/arr/2")); // Path to the new element
    ASSERT_FALSE(mask.contains("/arr/0"));
    ASSERT_FALSE(mask.contains("/arr/1"));
}

TEST_F(FieldMaskUtilsTest, ApplyMaskedUpdate) {
    json target = json_a;
    json source_changes = { // Contains only the fields that should be updated from json_b
        {"name", "Bob"},
        {"address", {{"city", "Otherville"}}},
        {"hobbies", json::array({nullptr, "cycling"})}, // Source for array update needs to be careful
                                                       // if we want to patch an array element.
                                                       // Here, we are providing the value for hobbies/1
        {"occupation", "Engineer"}
    };

    FieldMask update_mask;
    update_mask.add_path("/name");
    update_mask.add_path("/address/city");
    update_mask.add_path("/hobbies/1");
    update_mask.add_path("/occupation");

    // It's better to apply from the full json_b using the mask
    apply_masked_update(target, json_b, update_mask);

    ASSERT_EQ(target["name"], "Bob");
    ASSERT_EQ(target["age"], 30); // Unchanged
    ASSERT_EQ(target["address"]["street"], "123 Main St"); // Unchanged
    ASSERT_EQ(target["address"]["city"], "Otherville");
    ASSERT_EQ(target["hobbies"][0], "reading"); // Unchanged
    ASSERT_EQ(target["hobbies"][1], "cycling");
    ASSERT_EQ(target["occupation"], "Engineer");
}


TEST_F(FieldMaskUtilsTest, ApplyMaskedUpdateCreatesPath) {
    json target = {{"user", {{"id", 1}}}};
    json source = {{"user", {{"profile", {{"status", "active"}}}}}};
    
    FieldMask mask;
    mask.add_path("/user/profile/status");
    
    apply_masked_update(target, source, mask);
    
    ASSERT_TRUE(target["user"].contains("profile"));
    ASSERT_TRUE(target["user"]["profile"].contains("status"));
    ASSERT_EQ(target["user"]["profile"]["status"], "active");
    ASSERT_EQ(target["user"]["id"], 1); // Original field still there
}


TEST_F(FieldMaskUtilsTest, ExtractByMask) {
    FieldMask extract_mask;
    extract_mask.add_path("/name");
    extract_mask.add_path("/address/city");
    extract_mask.add_path("/hobbies/0"); // Only the first hobby

    json extracted = extract_by_mask(json_a, extract_mask);

    ASSERT_TRUE(extracted.contains("name"));
    ASSERT_EQ(extracted["name"], "Alice");
    ASSERT_TRUE(extracted.contains("address"));
    ASSERT_TRUE(extracted["address"].contains("city"));
    ASSERT_EQ(extracted["address"]["city"], "Anytown");
    ASSERT_FALSE(extracted["address"].contains("street")); // Should not be present
    
    ASSERT_TRUE(extracted.contains("hobbies"));
    ASSERT_TRUE(extracted["hobbies"].is_array());
    // nlohmann/json creates objects for paths. If hobbies/0 is requested,
    // it creates hobbies as an object {"0": "reading"} unless specifically handled.
    // The current extract_by_mask implementation uses json_pointer which handles this correctly for arrays.
    ASSERT_EQ(extracted["hobbies"][0], "reading");

    ASSERT_FALSE(extracted.count("age"));
}

TEST_F(FieldMaskUtilsTest, PruneRedundantPaths) {
    FieldMask mask;
    mask.add_path("/a");
    mask.add_path("/a/b");
    mask.add_path("/a/b/c");
    mask.add_path("/x/y");
    mask.add_path("/x"); // Parent added after child

    FieldMask pruned = prune_redundant_paths(mask);
    
    // Paths are added to a set, so order isn't guaranteed for iteration
    // but the logic of prune_redundant_paths should ensure only minimal set.
    // The implementation iterates existing paths and adds to result if no parent is in result.
    // This means if "/a" is processed first, "/a/b" and "/a/b/c" would be redundant.
    // If "/x/y" is processed, then "/x" is processed, "/x/y" would be removed from result if we re-evaluate.
    // The current implementation of prune_redundant_paths is a bit naive.
    // A better prune would sort paths and then process.
    // Let's test current behavior: it adds a path if its parent isn't *already* in the result.
    // Given paths: "/a", "/a/b", "/a/b/c", "/x", "/x/y" (order in set)
    // 1. Path "/a": not redundant, add to result. result: {"/a"}
    // 2. Path "/a/b": parent "/a" is in result. redundant.
    // 3. Path "/a/b/c": parent "/a/b" not in result, parent "/a" is. redundant.
    // 4. Path "/x": not redundant, add to result. result: {"/a", "/x"}
    // 5. Path "/x/y": parent "/x" is in result. redundant.
    // Expected: {"/a", "/x"}

    // To make the test stable, let's refine the input order for the test based on typical set iteration (sorted)
    FieldMask test_mask_for_prune;
    test_mask_for_prune.add_path("/a");
    test_mask_for_prune.add_path("/a/b");
    test_mask_for_prune.add_path("/a/b/c");
    test_mask_for_prune.add_path("/x");
    test_mask_for_prune.add_path("/x/y");
    
    FieldMask pruned_actual = prune_redundant_paths(test_mask_for_prune);

    ASSERT_TRUE(pruned_actual.contains("/a"));
    ASSERT_TRUE(pruned_actual.contains("/x"));
    ASSERT_EQ(pruned_actual.get_paths().size(), 2); 
    // This assumes the prune implementation correctly handles cases where a parent might be added after a child
    // The current implementation iterates through the input mask's paths (which are sorted due to std::set)
    // and adds a path to the result if none of its parents are *already in the result*.
    // If "/a" is processed, it's added. Then "/a/b" is processed, parent "/a" is in result, so "/a/b" is skipped. Correct.
    // If "/x/y" is processed (and "/x" hasn't been yet), "/x/y" is added. Then if "/x" is processed, it's added.
    // The result would be {"/x", "/x/y"}. This is not ideal.
    // The provided `prune_redundant_paths` seems to check against `result.contains(parent)`.

    // Let's re-verify prune_redundant_paths logic from header:
    // For each path in input mask:
    //   is_redundant = false
    //   current_path = path
    //   WHILE current_path not empty:
    //     parent = get_parent_path(current_path)
    //     IF result.contains(parent): is_redundant = true; break
    //     current_path = parent
    //   IF NOT is_redundant: result.add_path(path)
    //
    // Paths in mask.get_paths() are sorted: "/a", "/a/b", "/a/b/c", "/x", "/x/y"
    // 1. path = "/a": parent chain empty. result.add("/a"). result: {"/a"}
    // 2. path = "/a/b": parent "/a". result.contains("/a") is true. redundant.
    // 3. path = "/a/b/c": parent "/a/b". result does not contain "/a/b". parent of "/a/b" is "/a". result.contains("/a") is true. redundant.
    // 4. path = "/x": parent chain empty. result.add("/x"). result: {"/a", "/x"}
    // 5. path = "/x/y": parent "/x". result.contains("/x") is true. redundant.
    // So the expected result IS {"/a", "/x"}. The test above should be correct.
}


TEST_F(FieldMaskUtilsTest, PruneRedundantPathsRoot) {
    FieldMask mask;
    mask.add_path("/"); 
    mask.add_path("/a");
    mask.add_path("/b/c");

    FieldMask pruned = prune_redundant_paths(mask);
    ASSERT_TRUE(pruned.contains("/"));
    ASSERT_EQ(pruned.get_paths().size(), 1);
}


TEST_F(FieldMaskUtilsTest, InvertMask) {
    // json_a and json_b from fixture
    // Diff: "/name", "/address/city", "/hobbies/1", "/occupation"
    // Same: "/age", "/address/street", "/hobbies/0"
    // The invert_mask function also considers the root path "/" and intermediate paths.

    FieldMask inverted = invert_mask(json_a, json_b);

    // Paths that should be in the inverted mask (i.e., fields that are the same or non-leaf paths not part of a diff)
    ASSERT_TRUE(inverted.contains("/")); // Root is always collected by collect_all_paths
    ASSERT_TRUE(inverted.contains("/age"));
    ASSERT_TRUE(inverted.contains("/address")); // Parent path
    ASSERT_TRUE(inverted.contains("/address/street"));
    ASSERT_TRUE(inverted.contains("/hobbies")); // Parent path
    ASSERT_TRUE(inverted.contains("/hobbies/0"));
    
    // Paths that should NOT be in the inverted mask (i.e., fields that are different)
    ASSERT_FALSE(inverted.contains("/name"));
    ASSERT_FALSE(inverted.contains("/address/city"));
    ASSERT_FALSE(inverted.contains("/hobbies/1"));
    ASSERT_FALSE(inverted.contains("/occupation")); // This was only in b, so it's a diff.
                                                 // all_paths will collect it from b. diff_fields will mark it.
                                                 // So it should not be in inverted.
}

TEST_F(FieldMaskUtilsTest, DiffIdenticalObjects) {
    FieldMask mask = diff_fields(json_a, json_a);
    ASSERT_TRUE(mask.empty());
}

TEST_F(FieldMaskUtilsTest, DiffCompletelyDifferentObjects) {
    json j1 = {{"a", 1}};
    json j2 = {{"b", 2}};
    FieldMask mask = diff_fields(j1, j2);
    // Expects /a (in j1, not j2) and /b (in j2, not j1)
    ASSERT_TRUE(mask.contains("/a"));
    ASSERT_TRUE(mask.contains("/b"));
    ASSERT_EQ(mask.get_paths().size(), 2);
}

TEST_F(FieldMaskUtilsTest, DiffWithNull) {
    json j1 = {{"key", nullptr}};
    json j2 = {{"key", "value"}};
    json j3 = {{"key", nullptr}};

    FieldMask mask12 = diff_fields(j1, j2);
    ASSERT_TRUE(mask12.contains("/key"));
    ASSERT_EQ(mask12.get_paths().size(), 1);

    FieldMask mask13 = diff_fields(j1, j3);
    ASSERT_TRUE(mask13.empty());
}

TEST_F(FieldMaskUtilsTest, DiffEmptyRootPath) {
    // Test diffing primitive types at root
    json j_num1 = 1;
    json j_num2 = 2;
    FieldMask mask_num = diff_fields(j_num1, j_num2);
    ASSERT_TRUE(mask_num.contains("/")); // Root path for primitive diff
    ASSERT_EQ(mask_num.get_paths().size(), 1);
    
    json j_str1 = "hello";
    json j_str2 = "world";
    FieldMask mask_str = diff_fields(j_str1, j_str2);
    ASSERT_TRUE(mask_str.contains("/"));
    ASSERT_EQ(mask_str.get_paths().size(), 1);

    json j_obj1 = {{"a",1}};
    json j_obj2 = {{"a",1}}; // Same objects
    FieldMask mask_obj_same = diff_fields(j_obj1, j_obj2);
    ASSERT_TRUE(mask_obj_same.empty()); // No diffs

    // Diffing identical primitive types
    json j_num_same1 = 100;
    json j_num_same2 = 100;
    FieldMask mask_num_same = diff_fields(j_num_same1, j_num_same2);
    ASSERT_TRUE(mask_num_same.empty());
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

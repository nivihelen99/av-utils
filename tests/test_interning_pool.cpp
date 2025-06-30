#include "interning_pool.hpp" // Adjust path as necessary
#include "gtest/gtest.h"
#include <string>
#include <vector>
#include <set> // For testing uniqueness of handles

// Test fixture for InterningPool tests
class InterningPoolTest : public ::testing::Test {
protected:
    // You can put setup code here if needed, e.g., creating a pool instance
    // For most tests, a local pool instance within the TEST_F block is fine.
};

TEST_F(InterningPoolTest, EmptyPool) {
    cpp_collections::InterningPool<std::string> pool;
    ASSERT_TRUE(pool.empty());
    ASSERT_EQ(pool.size(), 0);
}

TEST_F(InterningPoolTest, InternNewStrings) {
    cpp_collections::InterningPool<std::string> pool;
    const std::string s1 = "hello";
    const std::string s2 = "world";

    const std::string* h1 = pool.intern(s1);
    ASSERT_EQ(pool.size(), 1);
    ASSERT_TRUE(pool.contains(s1));
    ASSERT_NE(h1, nullptr);
    ASSERT_EQ(*h1, s1);

    const std::string* h2 = pool.intern(s2);
    ASSERT_EQ(pool.size(), 2);
    ASSERT_TRUE(pool.contains(s2));
    ASSERT_NE(h2, nullptr);
    ASSERT_EQ(*h2, s2);
    ASSERT_NE(h1, h2); // Handles to different strings should be different
}

TEST_F(InterningPoolTest, InternDuplicateStrings) {
    cpp_collections::InterningPool<std::string> pool;
    const std::string s1 = "duplicate";
    const std::string s_other = "another";

    const std::string* h1 = pool.intern(s1);
    ASSERT_EQ(pool.size(), 1);
    ASSERT_NE(h1, nullptr);
    ASSERT_EQ(*h1, s1);

    const std::string* h2 = pool.intern("duplicate"); // Intern string literal
    ASSERT_EQ(pool.size(), 1); // Size should not change
    ASSERT_EQ(h1, h2);         // Handles should be the same
    ASSERT_NE(h2, nullptr);
    ASSERT_EQ(*h2, s1);

    const std::string* h3 = pool.intern(s_other);
    ASSERT_EQ(pool.size(), 2);
    ASSERT_NE(h1, h3);

    const std::string* h4 = pool.intern(std::string("duplicate")); // Intern rvalue
    ASSERT_EQ(pool.size(), 2);
    ASSERT_EQ(h1, h4);
    ASSERT_NE(h4, nullptr);
    ASSERT_EQ(*h4, s1);
}

TEST_F(InterningPoolTest, InternEmptyString) {
    cpp_collections::InterningPool<std::string> pool;
    const std::string empty_str = "";

    const std::string* h1 = pool.intern(empty_str);
    ASSERT_EQ(pool.size(), 1);
    ASSERT_TRUE(pool.contains(""));
    ASSERT_NE(h1, nullptr);
    ASSERT_TRUE(h1->empty());

    const std::string* h2 = pool.intern("");
    ASSERT_EQ(pool.size(), 1);
    ASSERT_EQ(h1, h2);
}

TEST_F(InterningPoolTest, InternRvalueStrings) {
    cpp_collections::InterningPool<std::string> pool;

    const std::string* h1 = pool.intern(std::string("temporary1"));
    ASSERT_EQ(pool.size(), 1);
    ASSERT_TRUE(pool.contains("temporary1"));
    ASSERT_NE(h1, nullptr);
    ASSERT_EQ(*h1, "temporary1");

    const std::string* h2 = pool.intern(std::string("temporary2"));
    ASSERT_EQ(pool.size(), 2);
    ASSERT_TRUE(pool.contains("temporary2"));
    ASSERT_NE(h2, nullptr);
    ASSERT_EQ(*h2, "temporary2");
    ASSERT_NE(h1, h2);

    const std::string* h3 = pool.intern(std::string("temporary1")); // Duplicate rvalue
    ASSERT_EQ(pool.size(), 2); // Size should not change
    ASSERT_EQ(h1, h3);         // Handles should be the same
}

TEST_F(InterningPoolTest, ContainsMethod) {
    cpp_collections::InterningPool<std::string> pool;
    ASSERT_FALSE(pool.contains("test"));

    pool.intern("test");
    ASSERT_TRUE(pool.contains("test"));
    ASSERT_TRUE(pool.contains(std::string("test")));
    ASSERT_FALSE(pool.contains("non_existent"));
}

TEST_F(InterningPoolTest, ClearPool) {
    cpp_collections::InterningPool<std::string> pool;
    pool.intern("one");
    pool.intern("two");
    ASSERT_EQ(pool.size(), 2);
    ASSERT_FALSE(pool.empty());

    // const std::string* h_one_before_clear = pool.intern("one"); // Already interned

    pool.clear();
    ASSERT_TRUE(pool.empty());
    ASSERT_EQ(pool.size(), 0);
    ASSERT_FALSE(pool.contains("one"));
    ASSERT_FALSE(pool.contains("two"));

    // Test interning after clear
    const std::string* h_one_after_clear = pool.intern("one");
    ASSERT_EQ(pool.size(), 1);
    ASSERT_TRUE(pool.contains("one"));
    ASSERT_NE(h_one_after_clear, nullptr);
    ASSERT_EQ(*h_one_after_clear, "one");
}

// Custom struct for testing
struct MyStruct {
    int id;
    std::string name;

    bool operator==(const MyStruct& other) const {
        return id == other.id && name == other.name;
    }
};

// Hash specialization for MyStruct in std namespace (or provide as template arg)
namespace std {
    template <> struct hash<MyStruct> {
        std::size_t operator()(const MyStruct& s) const {
            std::size_t h1 = std::hash<int>{}(s.id);
            std::size_t h2 = std::hash<std::string>{}(s.name);
            return h1 ^ (h2 << 1); // Combine hashes
        }
    };
}


TEST_F(InterningPoolTest, InternDifferentTypes) {
    cpp_collections::InterningPool<int> int_pool;
    const int val1 = 123;
    const int val2 = 456;

    const int* h_i1 = int_pool.intern(val1);
    ASSERT_EQ(int_pool.size(), 1);
    ASSERT_TRUE(int_pool.contains(val1));
    ASSERT_NE(h_i1, nullptr);
    ASSERT_EQ(*h_i1, val1);

    const int* h_i2 = int_pool.intern(val2);
    ASSERT_EQ(int_pool.size(), 2);
    ASSERT_TRUE(int_pool.contains(val2));
    ASSERT_NE(h_i2, nullptr);
    ASSERT_EQ(*h_i2, val2);
    ASSERT_NE(h_i1, h_i2);

    const int* h_i3 = int_pool.intern(123); // Rvalue int
    ASSERT_EQ(int_pool.size(), 2);
    ASSERT_EQ(h_i1, h_i3);

    // Test with MyStruct
    // Using default std::hash<MyStruct> because we specialized it.
    cpp_collections::InterningPool<MyStruct> struct_pool;
    MyStruct struct1 = {1, "Alice"};
    MyStruct struct2 = {2, "Bob"};
    MyStruct struct1_dup = {1, "Alice"};

    const MyStruct* h_s1 = struct_pool.intern(struct1);
    ASSERT_EQ(struct_pool.size(), 1);
    ASSERT_TRUE(struct_pool.contains(struct1));
    ASSERT_NE(h_s1, nullptr);
    ASSERT_EQ(h_s1->id, 1);
    ASSERT_EQ(h_s1->name, "Alice");

    const MyStruct* h_s2 = struct_pool.intern(struct2);
    ASSERT_EQ(struct_pool.size(), 2);
    ASSERT_NE(h_s1, h_s2);

    const MyStruct* h_s3 = struct_pool.intern(struct1_dup);
    ASSERT_EQ(struct_pool.size(), 2);
    ASSERT_EQ(h_s1, h_s3);

    const MyStruct* h_s4 = struct_pool.intern({1, "Alice"}); // Rvalue MyStruct
    ASSERT_EQ(struct_pool.size(), 2);
    ASSERT_EQ(h_s1, h_s4);
}

TEST_F(InterningPoolTest, HandleStabilityAndValues) {
    cpp_collections::InterningPool<std::string> pool;
    std::vector<const std::string*> handles;
    std::vector<std::string> original_values;
    const int num_items = 100;

    for (int i = 0; i < num_items; ++i) {
        std::string val = "string_" + std::to_string(i);
        original_values.push_back(val);
        handles.push_back(pool.intern(val));
    }
    ASSERT_EQ(pool.size(), num_items);

    // Intern some duplicates
    handles.push_back(pool.intern("string_0"));
    original_values.push_back("string_0"); // For comparison
    handles.push_back(pool.intern("string_" + std::to_string(num_items / 2)));
    original_values.push_back("string_" + std::to_string(num_items / 2));

    ASSERT_EQ(pool.size(), num_items); // Size should remain the same due to duplicates

    // Verify all handles point to the correct values and are unique for unique strings
    std::set<const std::string*> unique_handles_set;
    for (size_t i = 0; i < handles.size(); ++i) {
        ASSERT_NE(handles[i], nullptr);
        ASSERT_EQ(*handles[i], original_values[i]); // Check value
        if (i < num_items) { // Only original unique strings for this set
             unique_handles_set.insert(handles[i]);
        }
    }
    ASSERT_EQ(unique_handles_set.size(), num_items); // Ensure unique strings got unique handles

    // Check specific duplicate handles
    ASSERT_EQ(handles[0], handles[num_items]); // "string_0"
    ASSERT_EQ(handles[num_items/2], handles[num_items+1]); // "string_num_items/2"
}

// main function for GTest can be omitted if linking with GTest::gtest_main
// Or included if preferred:
// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }

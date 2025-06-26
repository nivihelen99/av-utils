#include "FrozenSet.h"
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <unordered_map> // For testing hashing
#include <set> // For comparison data

// Test fixture for FrozenSet tests
class FrozenSetTest : public ::testing::Test {
protected:
    // Per-test set-up and tear-down logic if needed
};

// Test default constructor and basic properties
TEST_F(FrozenSetTest, DefaultConstructor) {
    cpp_collections::FrozenSet<int> fs_int;
    EXPECT_TRUE(fs_int.empty());
    EXPECT_EQ(fs_int.size(), 0);
    EXPECT_EQ(fs_int.begin(), fs_int.end());

    cpp_collections::FrozenSet<std::string> fs_str;
    EXPECT_TRUE(fs_str.empty());
    EXPECT_EQ(fs_str.size(), 0);
}

// Test construction from initializer list
TEST_F(FrozenSetTest, InitializerListConstructor) {
    cpp_collections::FrozenSet<int> fs1 = {3, 1, 4, 1, 5, 9, 2, 6};
    EXPECT_FALSE(fs1.empty());
    EXPECT_EQ(fs1.size(), 7); // Duplicates (1) should be removed

    std::vector<int> expected_elements = {1, 2, 3, 4, 5, 6, 9};
    size_t i = 0;
    for (int val : fs1) {
        EXPECT_EQ(val, expected_elements[i++]);
    }

    EXPECT_TRUE(fs1.contains(1));
    EXPECT_TRUE(fs1.contains(9));
    EXPECT_FALSE(fs1.contains(0));
    EXPECT_FALSE(fs1.contains(7));

    cpp_collections::FrozenSet<int> fs_empty = {};
    EXPECT_TRUE(fs_empty.empty());
    EXPECT_EQ(fs_empty.size(), 0);
}

// Test construction from iterators
TEST_F(FrozenSetTest, IteratorConstructor) {
    std::vector<std::string> data = {"apple", "banana", "cherry", "apple", "date"};
    cpp_collections::FrozenSet<std::string> fs(data.begin(), data.end());

    EXPECT_EQ(fs.size(), 4); // "apple" duplicate removed
    EXPECT_TRUE(fs.contains("apple"));
    EXPECT_TRUE(fs.contains("banana"));
    EXPECT_TRUE(fs.contains("cherry"));
    EXPECT_TRUE(fs.contains("date"));
    EXPECT_FALSE(fs.contains("fig"));

    std::vector<std::string> expected_elements = {"apple", "banana", "cherry", "date"};
    std::sort(expected_elements.begin(), expected_elements.end()); // Ensure sorted for comparison

    size_t i = 0;
    for (const auto& s : fs) {
        EXPECT_EQ(s, expected_elements[i++]);
    }

    std::vector<int> empty_vec;
    cpp_collections::FrozenSet<int> fs_empty(empty_vec.begin(), empty_vec.end());
    EXPECT_TRUE(fs_empty.empty());
}

// Test lookup methods: count, contains, find
TEST_F(FrozenSetTest, LookupMethods) {
    cpp_collections::FrozenSet<int> fs = {10, 20, 30, 40, 50};

    // contains
    EXPECT_TRUE(fs.contains(10));
    EXPECT_TRUE(fs.contains(30));
    EXPECT_TRUE(fs.contains(50));
    EXPECT_FALSE(fs.contains(0));
    EXPECT_FALSE(fs.contains(25));
    EXPECT_FALSE(fs.contains(60));

    // count
    EXPECT_EQ(fs.count(10), 1);
    EXPECT_EQ(fs.count(30), 1);
    EXPECT_EQ(fs.count(50), 1);
    EXPECT_EQ(fs.count(0), 0);
    EXPECT_EQ(fs.count(25), 0);
    EXPECT_EQ(fs.count(60), 0);

    // find
    EXPECT_NE(fs.find(10), fs.end());
    EXPECT_EQ(*fs.find(10), 10);
    EXPECT_NE(fs.find(30), fs.end());
    EXPECT_EQ(*fs.find(30), 30);
    EXPECT_NE(fs.find(50), fs.end());
    EXPECT_EQ(*fs.find(50), 50);

    EXPECT_EQ(fs.find(0), fs.end());
    EXPECT_EQ(fs.find(25), fs.end());
    EXPECT_EQ(fs.find(60), fs.end());
}

// Test iteration
TEST_F(FrozenSetTest, Iteration) {
    cpp_collections::FrozenSet<int> fs = {5, 1, 3, 2, 4};
    std::vector<int> expected = {1, 2, 3, 4, 5};
    std::vector<int> actual;
    for (int val : fs) {
        actual.push_back(val);
    }
    EXPECT_EQ(actual, expected);

    // const iteration
    const auto& cfs = fs;
    actual.clear();
    for (int val : cfs) {
        actual.push_back(val);
    }
    EXPECT_EQ(actual, expected);

    // cbegin/cend
    actual.clear();
    for (auto it = fs.cbegin(); it != fs.cend(); ++it) {
        actual.push_back(*it);
    }
    EXPECT_EQ(actual, expected);
}

// Test comparison operators
TEST_F(FrozenSetTest, ComparisonOperators) {
    cpp_collections::FrozenSet<int> fs1 = {1, 2, 3};
    cpp_collections::FrozenSet<int> fs2 = {1, 2, 3};
    cpp_collections::FrozenSet<int> fs3 = {1, 2, 4};
    cpp_collections::FrozenSet<int> fs4 = {1, 2};
    cpp_collections::FrozenSet<int> fs_empty1;
    cpp_collections::FrozenSet<int> fs_empty2;

    // == and !=
    EXPECT_TRUE(fs1 == fs2);
    EXPECT_FALSE(fs1 != fs2);
    EXPECT_FALSE(fs1 == fs3);
    EXPECT_TRUE(fs1 != fs3);
    EXPECT_FALSE(fs1 == fs4);
    EXPECT_TRUE(fs1 != fs4);
    EXPECT_TRUE(fs_empty1 == fs_empty2);
    EXPECT_FALSE(fs_empty1 != fs_empty2);
    EXPECT_FALSE(fs1 == fs_empty1);

    // <, <=, >, >=
    EXPECT_FALSE(fs1 < fs2); // Equal
    EXPECT_TRUE(fs1 <= fs2); // Equal
    EXPECT_FALSE(fs1 > fs2); // Equal
    EXPECT_TRUE(fs1 >= fs2); // Equal

    EXPECT_TRUE(fs1 < fs3);  // {1,2,3} < {1,2,4}
    EXPECT_TRUE(fs1 <= fs3);
    EXPECT_FALSE(fs1 > fs3);
    EXPECT_FALSE(fs1 >= fs3);

    EXPECT_FALSE(fs3 < fs1);
    EXPECT_FALSE(fs3 <= fs1);
    EXPECT_TRUE(fs3 > fs1);
    EXPECT_TRUE(fs3 >= fs1);

    EXPECT_TRUE(fs4 < fs1); // {1,2} < {1,2,3} (shorter sequence)
    EXPECT_TRUE(fs4 <= fs1);
    EXPECT_FALSE(fs4 > fs1);
    EXPECT_FALSE(fs4 >= fs1);

    EXPECT_TRUE(fs_empty1 < fs1);
    EXPECT_FALSE(fs1 < fs_empty1);
}

// Test hashing and usage in unordered_map
TEST_F(FrozenSetTest, Hashing) {
    cpp_collections::FrozenSet<std::string> fs1 = {"hello", "world"};
    cpp_collections::FrozenSet<std::string> fs2 = {"world", "hello"}; // Same elements, different order in list
    cpp_collections::FrozenSet<std::string> fs3 = {"hello", "c++"};
    cpp_collections::FrozenSet<std::string> fs_empty;

    std::hash<cpp_collections::FrozenSet<std::string>> hasher;

    EXPECT_EQ(hasher(fs1), hasher(fs2)); // Should be equal as content is same and sorted
    EXPECT_NE(hasher(fs1), hasher(fs3));
    EXPECT_NE(hasher(fs1), hasher(fs_empty));

    std::unordered_map<cpp_collections::FrozenSet<std::string>, int> map_fs_to_int;
    map_fs_to_int[fs1] = 100;
    map_fs_to_int[fs3] = 200;
    map_fs_to_int[fs_empty] = 0;

    EXPECT_EQ(map_fs_to_int.count(fs1), 1);
    EXPECT_EQ(map_fs_to_int.at(fs1), 100);
    EXPECT_EQ(map_fs_to_int.count(fs2), 1); // fs2 is equivalent to fs1
    EXPECT_EQ(map_fs_to_int.at(fs2), 100);
    EXPECT_EQ(map_fs_to_int.count(fs3), 1);
    EXPECT_EQ(map_fs_to_int.at(fs3), 200);
    EXPECT_EQ(map_fs_to_int.count(fs_empty), 1);
    EXPECT_EQ(map_fs_to_int.at(fs_empty), 0);

    cpp_collections::FrozenSet<std::string> fs_not_in_map = {"test"};
    EXPECT_EQ(map_fs_to_int.count(fs_not_in_map), 0);
}

// Test with custom comparator
struct CaseInsensitiveCompare {
    bool operator()(const std::string& a, const std::string& b) const {
        return std::lexicographical_compare(
            a.begin(), a.end(),
            b.begin(), b.end(),
            [](char c1, char c2) {
                return std::tolower(c1) < std::tolower(c2);
            }
        );
    }
};

// Custom equality for testing with CaseInsensitiveCompare
struct CaseInsensitiveEqual {
     bool operator()(const std::string& a, const std::string& b) const {
        if (a.length() != b.length()) return false;
        return std::equal(a.begin(), a.end(), b.begin(),
                          [](char c1, char c2){ return std::tolower(c1) == std::tolower(c2); });
    }
};


TEST_F(FrozenSetTest, CustomComparator) {
    cpp_collections::FrozenSet<std::string, CaseInsensitiveCompare> fs = {"Apple", "banana", "APPLE", "Cherry"};
    EXPECT_EQ(fs.size(), 3); // "Apple" and "APPLE" are duplicates with case-insensitive compare

    // Order should be Apple (or APPLE), banana, Cherry (case might vary based on sort stability)
    // Let's check contents
    EXPECT_TRUE(fs.contains("apple"));
    EXPECT_TRUE(fs.contains("APPLE")); // Both should work due to comparator
    EXPECT_TRUE(fs.contains("Banana"));
    EXPECT_TRUE(fs.contains("cherry"));
    EXPECT_FALSE(fs.contains("Date"));

    // Test find with custom comparator
    auto it_apple = fs.find("apple");
    ASSERT_NE(it_apple, fs.end());
    EXPECT_TRUE(CaseInsensitiveEqual()(*it_apple, "Apple")); // Check if one of the "apple"s is found

    auto it_APPLE = fs.find("APPLE");
    ASSERT_NE(it_APPLE, fs.end());
    EXPECT_TRUE(CaseInsensitiveEqual()(*it_APPLE, "Apple"));

    // Ensure internal order is consistent with comparator
    std::vector<std::string> actual_elements;
    for(const auto& s : fs) actual_elements.push_back(s);

    // Expected can be tricky due to sort stability. We know "apple" comes before "banana"
    // and "banana" before "cherry" case-insensitively.
    // The specific original casing ("Apple" vs "APPLE") kept depends on std::sort stability
    // and std::unique behavior with equivalent elements.
    // Let's just check the elements are present in a case-insensitive way and sorted.
    ASSERT_EQ(actual_elements.size(), 3);
    EXPECT_TRUE(CaseInsensitiveEqual()(actual_elements[0], "Apple")); // or "APPLE"
    EXPECT_TRUE(CaseInsensitiveEqual()(actual_elements[1], "banana"));
    EXPECT_TRUE(CaseInsensitiveEqual()(actual_elements[2], "Cherry"));

    // Test hashing with custom comparator
    // Note: std::hash<FrozenSet> uses std::hash<Key>. If Key is std::string,
    // it uses case-sensitive hash. This means fs_ci1 and fs_ci2 below might have different hashes
    // if their *stored representations* are different ("Apple" vs "apple"), even if they
    // compare equal with CaseInsensitiveCompare. This is a known subtlety.
    // For hash to be consistent with custom comparison, the std::hash<Key> would also need
    // to be consistent with that comparison (e.g. hash(tolower(str))).
    // The current std::hash<FrozenSet> will use the *actual stored values*.
    cpp_collections::FrozenSet<std::string, CaseInsensitiveCompare> fs_ci1 = {"Apple", "Banana"};
    cpp_collections::FrozenSet<std::string, CaseInsensitiveCompare> fs_ci2 = {"apple", "banana"};
    // fs_ci1 and fs_ci2 should compare equal using operator== for FrozenSet,
    // because operator== uses the FrozenSet's comparator implicitly through its sorted vector storage.
    // Let's verify this first.
    // The internal data_ vector for fs_ci1 might be {"Apple", "Banana"}
    // The internal data_ vector for fs_ci2 might be {"apple", "banana"}
    // If CaseInsensitiveCompare is used for std::sort, then "Apple" and "apple" are equivalent.
    // std::unique will keep the first one. So fs_ci1 from {"Apple", "Banana"} will store {"Apple", "Banana"}
    // fs_ci2 from {"apple", "banana"} will store {"apple", "banana"}
    // These will NOT be equal with operator== if their internal vectors are different.
    // This is a critical point: operator== for FrozenSet is defined as `lhs.data_ == rhs.data_`.
    // This implies that for two FrozenSets to be equal, their comparators must have produced
    // identical canonical representations in their `data_` vectors.
    // If fs_ci1 stores "Apple" and fs_ci2 stores "apple", they are NOT equal by FrozenSet::operator==.
    // This is actually the desired behavior for a generic FrozenSet. If you want case-insensitive
    // equality to also mean they are the *same* FrozenSet, the elements themselves would need to be
    // canonicalized (e.g., all lowercase) before insertion or the comparator for equality needs to be more complex.

    // Let's test this understanding:
    cpp_collections::FrozenSet<std::string, CaseInsensitiveCompare> fs_test1 = {"KEY", "value"}; // stores "KEY", "value" (assuming this order by CICompare)
    cpp_collections::FrozenSet<std::string, CaseInsensitiveCompare> fs_test2 = {"key", "VALUE"}; // stores "key", "VALUE"

    // They *contain* the same elements case-insensitively:
    EXPECT_TRUE(fs_test1.contains("key"));
    EXPECT_TRUE(fs_test1.contains("VALUE"));
    EXPECT_TRUE(fs_test2.contains("KEY"));
    EXPECT_TRUE(fs_test2.contains("value"));

    // But are they equal? Depends on what `std::unique` kept.
    // If CaseInsensitiveCompare sorts "KEY" before "value", and "key" before "VALUE"
    // and "KEY" is equivalent to "key", "value" to "VALUE".
    // {"KEY", "value"} -> sorts to {"KEY", "value"} (original order preserved for equivalent) -> unique keeps {"KEY", "value"}
    // {"key", "VALUE"} -> sorts to {"key", "VALUE"} -> unique keeps {"key", "VALUE"}
    // Then fs_test1.data_ is {"KEY", "value"} and fs_test2.data_ is {"key", "VALUE"}. These vectors are not equal.
    EXPECT_FALSE(fs_test1 == fs_test2); // This is expected.

    // If we construct them to have the same canonical representation:
    cpp_collections::FrozenSet<std::string, CaseInsensitiveCompare> fs_canon1 = {"apple", "banana"};
    cpp_collections::FrozenSet<std::string, CaseInsensitiveCompare> fs_canon2 = {"Apple", "Banana"};
    // fs_canon1.data_ will be {"apple", "banana"}
    // fs_canon2.data_ will be {"Apple", "Banana"} because unique keeps the first encountered.
    // So they are still not equal by data_ comparison.

    // The only way they'd be equal is if the input to the constructor, after sorting by CaseInsensitiveCompare,
    // results in the *exact same sequence of elements being kept by std::unique*.
    // Example:
    cpp_collections::FrozenSet<std::string, CaseInsensitiveCompare> fs_eq1 = {"apple", "Banana", "apple"}; // -> unique keeps "apple", "Banana"
    cpp_collections::FrozenSet<std::string, CaseInsensitiveCompare> fs_eq2 = {"Apple", "banana", "APPLE"}; // -> unique keeps "Apple", "banana"
    // These are NOT equal. fs_eq1.data_ is {"apple", "Banana"}, fs_eq2.data_ is {"Apple", "banana"}
    // (assuming "apple" comes before "Banana" with CICompare)

    // This shows that std::hash<FrozenSet> will correctly produce different hashes for fs_eq1 and fs_eq2,
    // which is consistent with their inequality.
    // The definition of equality for FrozenSet is strict: same elements in same order with same comparator logic.
    // The custom comparator influences sorting and uniqueness, but equality is on the final stored representation.

    std::unordered_map<cpp_collections::FrozenSet<std::string, CaseInsensitiveCompare>, int> map_fs_custom_comp;
    cpp_collections::FrozenSet<std::string, CaseInsensitiveCompare> key1 = {"Case", "Test"}; // Stored as e.g. {"Case", "Test"}
    cpp_collections::FrozenSet<std::string, CaseInsensitiveCompare> key2 = {"case", "test"}; // Stored as e.g. {"case", "test"}

    map_fs_custom_comp[key1] = 1;
    map_fs_custom_comp[key2] = 2;

    EXPECT_EQ(map_fs_custom_comp.at(key1), 1);
    EXPECT_EQ(map_fs_custom_comp.at(key2), 2); // key1 and key2 are different keys
    EXPECT_NE(std::hash<decltype(key1)>()(key1), std::hash<decltype(key2)>()(key2));
}


// Test copy and move semantics
TEST_F(FrozenSetTest, CopyAndMove) {
    cpp_collections::FrozenSet<int> fs1 = {1, 2, 3};

    // Copy constructor
    cpp_collections::FrozenSet<int> fs2 = fs1;
    EXPECT_EQ(fs1, fs2);
    EXPECT_EQ(fs2.size(), 3);
    EXPECT_TRUE(fs2.contains(2));

    // Copy assignment
    cpp_collections::FrozenSet<int> fs3;
    fs3 = fs1;
    EXPECT_EQ(fs1, fs3);
    EXPECT_EQ(fs3.size(), 3);
    EXPECT_TRUE(fs3.contains(3));

    // Move constructor
    cpp_collections::FrozenSet<int> fs4 = std::move(fs2); // fs2 is now in a valid but unspecified state
    EXPECT_EQ(fs1, fs4); // fs4 should have taken fs2's (which was fs1's) content
    EXPECT_EQ(fs4.size(), 3);
    EXPECT_TRUE(fs4.contains(1));
    // EXPECT_TRUE(fs2.empty()); // Not guaranteed, but likely for vector move

    // Move assignment
    cpp_collections::FrozenSet<int> fs5;
    fs5 = std::move(fs3); // fs3 is now in a valid but unspecified state
    EXPECT_EQ(fs1, fs5);
    EXPECT_EQ(fs5.size(), 3);
    EXPECT_TRUE(fs5.contains(2));
    // EXPECT_TRUE(fs3.empty()); // Not guaranteed
}

// Test FrozenSet with non-pod types
struct MyNonPodType {
    std::string s;
    int id;

    MyNonPodType(std::string s_val, int id_val) : s(std::move(s_val)), id(id_val) {}

    // Need comparison for FrozenSet
    bool operator<(const MyNonPodType& other) const {
        if (s < other.s) return true;
        if (s > other.s) return false;
        return id < other.id;
    }
    // Need equality for some tests, though FrozenSet uses < for equivalence
    bool operator==(const MyNonPodType& other) const {
        return s == other.s && id == other.id;
    }
};

// Hash specialization for MyNonPodType
namespace std {
    template<> struct hash<MyNonPodType> {
        size_t operator()(const MyNonPodType& m) const {
            return std::hash<std::string>()(m.s) ^ (std::hash<int>()(m.id) << 1);
        }
    };
}


TEST_F(FrozenSetTest, NonPodType) {
    MyNonPodType mp1("obj1", 10);
    MyNonPodType mp2("obj2", 20);
    MyNonPodType mp3("obj1", 5); // Different id, same string as mp1 but smaller
    MyNonPodType mp1_dup("obj1", 10);

    cpp_collections::FrozenSet<MyNonPodType> fs = {mp1, mp2, mp3, mp1_dup};
    EXPECT_EQ(fs.size(), 3); // mp1 and mp1_dup are duplicates

    EXPECT_TRUE(fs.contains(mp1));
    EXPECT_TRUE(fs.contains(MyNonPodType("obj2", 20)));
    EXPECT_TRUE(fs.contains(MyNonPodType("obj1", 5)));
    EXPECT_FALSE(fs.contains(MyNonPodType("obj3", 30)));

    // Check order: mp3 ("obj1", 5), mp1 ("obj1", 10), mp2 ("obj2", 20)
    auto it = fs.begin();
    EXPECT_EQ(it->s, "obj1"); EXPECT_EQ(it->id, 5); // mp3
    ++it;
    EXPECT_EQ(it->s, "obj1"); EXPECT_EQ(it->id, 10); // mp1
    ++it;
    EXPECT_EQ(it->s, "obj2"); EXPECT_EQ(it->id, 20); // mp2
    ++it;
    EXPECT_EQ(it, fs.end());

    // Test hashing with non-POD
    std::unordered_map<cpp_collections::FrozenSet<MyNonPodType>, std::string> map_fs_non_pod;
    map_fs_non_pod[fs] = "Set1";
    EXPECT_EQ(map_fs_non_pod.at(fs), "Set1");

    cpp_collections::FrozenSet<MyNonPodType> fs_other = {mp2, mp3}; // Different set
    map_fs_non_pod[fs_other] = "Set2";
    EXPECT_EQ(map_fs_non_pod.at(fs_other), "Set2");
}

// Test allocator support (basic check - not exhaustive)
TEST_F(FrozenSetTest, AllocatorSupport) {
    std::allocator<int> alloc;
    cpp_collections::FrozenSet<int, std::less<int>, std::allocator<int>> fs(alloc);
    EXPECT_TRUE(fs.empty());
    EXPECT_EQ(fs.get_allocator(), alloc);

    cpp_collections::FrozenSet<int> fs2({1,2,3}, std::less<int>(), alloc);
    EXPECT_EQ(fs2.size(), 3);
    EXPECT_EQ(fs2.get_allocator(), alloc);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#include "gtest/gtest.h"
#include "SkipList.h" // Assuming SkipList.h is in the include path
#include <string>
#include <vector>
#include <algorithm> // For std::sort, std::is_sorted
#include <set>     // For comparing results

// Basic test fixture for SkipList
class SkipListTest : public ::testing::Test {
protected:
    cpp_utils::SkipList<int> sl_int;
    cpp_utils::SkipList<std::string> sl_str;
};

TEST_F(SkipListTest, Construction) {
    EXPECT_EQ(sl_int.size(), 0);
    EXPECT_TRUE(sl_int.empty());
    EXPECT_EQ(sl_int.currentListLevel(), 0); // Initial level should be 0

    cpp_utils::SkipList<int, std::greater<int>> sl_int_greater;
    EXPECT_EQ(sl_int_greater.size(), 0);
    EXPECT_TRUE(sl_int_greater.empty());
}

TEST_F(SkipListTest, InsertBasic) {
    EXPECT_TRUE(sl_int.insert(10));
    EXPECT_EQ(sl_int.size(), 1);
    EXPECT_FALSE(sl_int.empty());
    EXPECT_TRUE(sl_int.contains(10));

    EXPECT_TRUE(sl_int.insert(5));
    EXPECT_EQ(sl_int.size(), 2);
    EXPECT_TRUE(sl_int.contains(5));
    EXPECT_TRUE(sl_int.contains(10));

    EXPECT_TRUE(sl_int.insert(15));
    EXPECT_EQ(sl_int.size(), 3);
    EXPECT_TRUE(sl_int.contains(15));
    EXPECT_TRUE(sl_int.contains(5));
    EXPECT_TRUE(sl_int.contains(10));

    // Test contains for non-existing elements
    EXPECT_FALSE(sl_int.contains(1));
    EXPECT_FALSE(sl_int.contains(100));
}

TEST_F(SkipListTest, InsertDuplicates) {
    EXPECT_TRUE(sl_int.insert(10));
    EXPECT_EQ(sl_int.size(), 1);

    EXPECT_FALSE(sl_int.insert(10)); // Insert duplicate
    EXPECT_EQ(sl_int.size(), 1);     // Size should not change
    EXPECT_TRUE(sl_int.contains(10));
}

TEST_F(SkipListTest, InsertStrings) {
    EXPECT_TRUE(sl_str.insert("hello"));
    EXPECT_EQ(sl_str.size(), 1);
    EXPECT_TRUE(sl_str.contains("hello"));

    EXPECT_TRUE(sl_str.insert("world"));
    EXPECT_EQ(sl_str.size(), 2);
    EXPECT_TRUE(sl_str.contains("world"));

    EXPECT_TRUE(sl_str.insert("apple"));
    EXPECT_EQ(sl_str.size(), 3);
    EXPECT_TRUE(sl_str.contains("apple"));

    EXPECT_FALSE(sl_str.insert("hello")); // Duplicate
    EXPECT_EQ(sl_str.size(), 3);

    EXPECT_FALSE(sl_str.contains("banana"));
}

TEST_F(SkipListTest, Clear) {
    sl_int.insert(10);
    sl_int.insert(20);
    sl_int.insert(5);
    ASSERT_EQ(sl_int.size(), 3);
    ASSERT_FALSE(sl_int.empty());

    sl_int.clear();
    EXPECT_EQ(sl_int.size(), 0);
    EXPECT_TRUE(sl_int.empty());
    EXPECT_FALSE(sl_int.contains(10));
    EXPECT_FALSE(sl_int.contains(20));
    EXPECT_FALSE(sl_int.contains(5));
    EXPECT_EQ(sl_int.currentListLevel(), 0); // Level should reset

    // Test clear on already empty list
    sl_int.clear();
    EXPECT_EQ(sl_int.size(), 0);
    EXPECT_TRUE(sl_int.empty());
}

TEST_F(SkipListTest, EraseBasic) {
    sl_int.insert(10);
    sl_int.insert(5);
    sl_int.insert(15);
    sl_int.insert(3);
    sl_int.insert(12);
    ASSERT_EQ(sl_int.size(), 5);

    // Erase existing element
    EXPECT_TRUE(sl_int.erase(10));
    EXPECT_EQ(sl_int.size(), 4);
    EXPECT_FALSE(sl_int.contains(10));
    EXPECT_TRUE(sl_int.contains(5));
    EXPECT_TRUE(sl_int.contains(15));

    // Erase another existing element
    EXPECT_TRUE(sl_int.erase(3));
    EXPECT_EQ(sl_int.size(), 3);
    EXPECT_FALSE(sl_int.contains(3));
    EXPECT_TRUE(sl_int.contains(12));

    // Erase non-existing element
    EXPECT_FALSE(sl_int.erase(100));
    EXPECT_EQ(sl_int.size(), 3);

    // Erase head-like element (smallest)
    EXPECT_TRUE(sl_int.erase(5));
    EXPECT_EQ(sl_int.size(), 2);
    EXPECT_FALSE(sl_int.contains(5));
    EXPECT_TRUE(sl_int.contains(15));
    EXPECT_TRUE(sl_int.contains(12));

    // Erase tail-like element (largest)
    EXPECT_TRUE(sl_int.erase(15));
    EXPECT_EQ(sl_int.size(), 1);
    EXPECT_FALSE(sl_int.contains(15));
    EXPECT_TRUE(sl_int.contains(12));

    // Erase last element
    EXPECT_TRUE(sl_int.erase(12));
    EXPECT_EQ(sl_int.size(), 0);
    EXPECT_TRUE(sl_int.empty());
    EXPECT_FALSE(sl_int.contains(12));

    // Erase from empty list
    EXPECT_FALSE(sl_int.erase(10));
}


TEST_F(SkipListTest, InsertManyElements) {
    const int num_elements = 1000;
    std::set<int> reference_set;

    for (int i = 0; i < num_elements; ++i) {
        int val = rand() % (num_elements * 2); // Insert some duplicates, some unique
        bool inserted_in_set = reference_set.insert(val).second;
        bool inserted_in_sl = sl_int.insert(val);
        // If it was new to the set, it should be new to the skip list
        // If it was a duplicate for the set, it should be a duplicate for skip list
        EXPECT_EQ(inserted_in_sl, inserted_in_set);
        EXPECT_EQ(sl_int.size(), reference_set.size());
    }

    EXPECT_EQ(sl_int.size(), reference_set.size());
    for (int val : reference_set) {
        EXPECT_TRUE(sl_int.contains(val)) << "Value " << val << " should be in SkipList";
    }
    for (int i = 0; i < num_elements * 2 + 100; ++i) { // Check some values not in set
        if (reference_set.find(i) == reference_set.end()) {
            EXPECT_FALSE(sl_int.contains(i)) << "Value " << i << " should NOT be in SkipList";
        }
    }
}

TEST_F(SkipListTest, EraseManyElements) {
    const int num_elements = 1000;
    std::vector<int> elements;
    std::set<int> reference_set;

    for (int i = 0; i < num_elements; ++i) {
        int val = rand() % (num_elements * 2);
        if (reference_set.find(val) == reference_set.end()) {
            elements.push_back(val);
            reference_set.insert(val);
            sl_int.insert(val);
        }
    }
    ASSERT_EQ(sl_int.size(), reference_set.size());

    // Shuffle elements to erase in random order
    std::random_shuffle(elements.begin(), elements.end());

    for (int val_to_erase : elements) {
        ASSERT_TRUE(reference_set.count(val_to_erase)) << "Value " << val_to_erase << " should be in reference set before erase.";
        ASSERT_TRUE(sl_int.contains(val_to_erase)) << "Value " << val_to_erase << " should be in SkipList before erase.";

        EXPECT_TRUE(sl_int.erase(val_to_erase));
        reference_set.erase(val_to_erase);

        EXPECT_EQ(sl_int.size(), reference_set.size());
        EXPECT_FALSE(sl_int.contains(val_to_erase)) << "Value " << val_to_erase << " should NOT be in SkipList after erase.";

        // Try erasing again, should fail
        EXPECT_FALSE(sl_int.erase(val_to_erase));
    }

    EXPECT_TRUE(sl_int.empty());
    EXPECT_EQ(sl_int.size(), 0);
}

TEST_F(SkipListTest, InsertAndEraseMixed) {
    const int operations = 2000;
    const int range = 500;
    std::set<int> reference_set;

    for(int i = 0; i < operations; ++i) {
        int val = rand() % range;
        bool do_insert = (rand() % 2 == 0);

        if (do_insert) {
            bool inserted_in_set = reference_set.insert(val).second;
            bool inserted_in_sl = sl_int.insert(val);
            EXPECT_EQ(inserted_in_sl, inserted_in_set);
        } else {
            if (!reference_set.empty()) { // Only erase if set is not empty
                 // To make it more interesting, pick an element that's actually in the set sometimes
                if (rand() % 3 == 0 && !reference_set.empty()) {
                    val = *reference_set.begin(); // Erase smallest
                } // else erase the random val which might or might not be there

                bool erased_from_set = (reference_set.erase(val) == 1);
                bool erased_from_sl = sl_int.erase(val);
                EXPECT_EQ(erased_from_sl, erased_from_set);
            }
        }
        ASSERT_EQ(sl_int.size(), reference_set.size());
        if (!sl_int.empty() && !reference_set.empty()) {
             ASSERT_TRUE(sl_int.contains(*reference_set.begin()));
             // Could add more checks here for random elements
        }
    }

    // Final check: ensure all elements in reference_set are in sl_int
    for (int val_in_set : reference_set) {
        EXPECT_TRUE(sl_int.contains(val_in_set));
    }
    // And check that elements not in reference_set are not in sl_int
    for (int i=0; i < range + 50; ++i) {
        if(reference_set.find(i) == reference_set.end()){
            EXPECT_FALSE(sl_int.contains(i));
        }
    }
}

TEST_F(SkipListTest, CustomComparatorGreater) {
    cpp_utils::SkipList<int, std::greater<int>> sl_greater;

    sl_greater.insert(10);
    sl_greater.insert(5);  // Should come before 10
    sl_greater.insert(15); // Should come after 10

    EXPECT_TRUE(sl_greater.contains(5));
    EXPECT_TRUE(sl_greater.contains(10));
    EXPECT_TRUE(sl_greater.contains(15));
    EXPECT_EQ(sl_greater.size(), 3);

    // findNode is internal, but we can infer order from erase and sequential inserts
    // For a std::greater comparator, 15 should be "smaller" than 10, 5 "smaller" than 10
    // When we iterate (if we had iterators), it would be 15, 10, 5.

    // Erase smallest according to std::greater (which is the largest numerically)
    EXPECT_TRUE(sl_greater.erase(15));
    EXPECT_FALSE(sl_greater.contains(15));
    EXPECT_EQ(sl_greater.size(), 2);

    // Erase largest according to std::greater (which is the smallest numerically)
    EXPECT_TRUE(sl_greater.erase(5));
    EXPECT_FALSE(sl_greater.contains(5));
    EXPECT_EQ(sl_greater.size(), 1);

    EXPECT_TRUE(sl_greater.contains(10));
    sl_greater.clear();
    EXPECT_TRUE(sl_greater.empty());
}

// A simple struct for testing
struct Point {
    int x, y;
    // Needed for std::less (default comparator) or provide custom one
    bool operator<(const Point& other) const {
        if (x != other.x) return x < other.x;
        return y < other.y;
    }
    // Needed for std::hash if used in unordered_map by SkipList (not directly, but good practice)
    // and for equality checks if not using !< && !>
     bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

// Custom comparator for Point, comparing by y then x
struct ComparePointYX {
    bool operator()(const Point& a, const Point& b) const {
        if (a.y != b.y) return a.y < b.y;
        return a.x < b.x;
    }
};


namespace std {
    // Required for some test assertions if Point is used directly with EXPECT_EQ on Point itself
    // or with containers of Points. For SkipList, this isn't directly used by the list logic
    // unless we are comparing nodes directly (which we are not in these tests).
    // It's more for printing Points in GTest messages if an EXPECT fails.
    ostream& operator<<(ostream& os, const Point& p) {
        return os << "(" << p.x << ", " << p.y << ")";
    }
}


TEST_F(SkipListTest, CustomTypeAndComparator) {
    cpp_utils::SkipList<Point, ComparePointYX> sl_point_custom;

    Point p1 = {1, 5};
    Point p2 = {3, 2}; // Will come before p1 due to y-value
    Point p3 = {0, 5}; // Will come before p1 (same y, smaller x)

    EXPECT_TRUE(sl_point_custom.insert(p1)); // {1,5}
    EXPECT_TRUE(sl_point_custom.insert(p2)); // {3,2}
    EXPECT_TRUE(sl_point_custom.insert(p3)); // {0,5}
    EXPECT_EQ(sl_point_custom.size(), 3);

    EXPECT_TRUE(sl_point_custom.contains(p1));
    EXPECT_TRUE(sl_point_custom.contains(p2));
    EXPECT_TRUE(sl_point_custom.contains(p3));

    // Test duplicate insertion
    EXPECT_FALSE(sl_point_custom.insert(p1));
    EXPECT_EQ(sl_point_custom.size(), 3);

    // Erase elements
    EXPECT_TRUE(sl_point_custom.erase(p2)); // Erase {3,2}
    EXPECT_FALSE(sl_point_custom.contains(p2));
    EXPECT_EQ(sl_point_custom.size(), 2);

    Point p_non_existent = {10,10};
    EXPECT_FALSE(sl_point_custom.contains(p_non_existent));
    EXPECT_FALSE(sl_point_custom.erase(p_non_existent));
}

TEST_F(SkipListTest, MoveOnlyType) {
    // Simple move-only type
    struct MoveOnly {
        int id;
        std::unique_ptr<int> ptr;

        MoveOnly() : id(-1), ptr(nullptr) {} // Default constructor
        MoveOnly(int i) : id(i), ptr(std::make_unique<int>(i)) {}
        MoveOnly(MoveOnly&& other) noexcept : id(other.id), ptr(std::move(other.ptr)) {}
        MoveOnly& operator=(MoveOnly&& other) noexcept {
            if (this != &other) {
                id = other.id;
                ptr = std::move(other.ptr);
            }
            return *this;
        }
        // Non-copyable
        MoveOnly(const MoveOnly&) = delete;
        MoveOnly& operator=(const MoveOnly&) = delete;

        bool operator<(const MoveOnly& other) const { return id < other.id; }
        // Required for find/contains if using the default !< && !> for equality
        bool operator==(const MoveOnly& other) const { return id == other.id; }
    };

    // Need a way to compare for equality if not providing full operator==
    // The SkipList uses !compare(a,b) && !compare(b,a) for equality.
    // So, only operator< is strictly needed by SkipList with std::less.

    cpp_utils::SkipList<MoveOnly> sl_move_only;

    EXPECT_TRUE(sl_move_only.insert(MoveOnly(10)));
    EXPECT_EQ(sl_move_only.size(), 1);
    // We can't use sl_move_only.contains(MoveOnly(10)) directly because MoveOnly(10) is an rvalue
    // and contains takes const T&. We need an lvalue.
    // Or, the SkipList::contains could be templated or take T.
    // For now, let's test by inserting and then trying to insert again.

    MoveOnly m5(5);
    EXPECT_TRUE(sl_move_only.insert(std::move(m5)));
    EXPECT_EQ(sl_move_only.size(), 2);
    // m5 is now in a moved-from state.

    MoveOnly m10_again(10);
    EXPECT_FALSE(sl_move_only.insert(std::move(m10_again))); // Duplicate
    EXPECT_EQ(sl_move_only.size(), 2);

    // To test contains and erase, we need a key that can be passed as const T&
    // Our current findNode/contains takes `const T& value`.
    // This is problematic for MoveOnly types if we only have rvalues.
    // A common solution is for `find` and `erase` to accept a `const Key&` where Key might be different from T (e.g. for maps)
    // or be templated `template<typename K> bool contains(const K& key) const;`
    // For now, this test will be limited. We can test erase by its effect on size if we know what's in.

    // To test erase, we'd ideally need a way to construct a "key" version of MoveOnly
    // or have find/erase work with types convertible to T or comparable with T.
    // The current SkipList erase(const T& value) will try to copy `value` if T is not implicitly convertible
    // or if no overload matches.
    // For a move-only type, this is tricky.
    // Let's assume we can't easily call contains/erase with a new MoveOnly instance.
    // This highlights a potential design improvement for wider applicability.

    // For now, we'll just clear it.
    sl_move_only.clear();
    EXPECT_TRUE(sl_move_only.empty());
}


// Test with a specific MaxLevel and P to ensure template parameters work
TEST(SkipListSpecificParamsTest, ConstructionWithParams) {
    cpp_utils::SkipList<int, std::less<int>, 5, 0.25> sl_custom_params;
    EXPECT_EQ(sl_custom_params.size(), 0);
    EXPECT_TRUE(sl_custom_params.empty());
    EXPECT_TRUE(sl_custom_params.insert(100));
    EXPECT_TRUE(sl_custom_params.contains(100));
    EXPECT_EQ(sl_custom_params.MaxLevel, 5); // Check static constexpr member
    EXPECT_DOUBLE_EQ(sl_custom_params.P, 0.25); // Check static constexpr member
}

// More rigorous test for level distribution (probabilistic, so hard to assert deterministically)
TEST_F(SkipListTest, LevelGrowth) {
    // With P=0.5, MaxLevel=16 (default)
    // We expect levels to grow logarithmically with N.
    // This is a weak test, just checks that levels don't stay 0.
    for (int i = 0; i < 100; ++i) {
        sl_int.insert(i);
    }
    EXPECT_GT(sl_int.currentListLevel(), 0) << "With 100 elements, level should likely be > 0";

    int current_max_level_seen = sl_int.currentListLevel();

    // Add more elements
    for (int i = 100; i < 500; ++i) {
        sl_int.insert(i);
    }
    EXPECT_GE(sl_int.currentListLevel(), current_max_level_seen) << "Level should not decrease with more insertions";
    // It's possible, though unlikely with P=0.5, for level to not increase much over a small range of N.
    // A very robust test would require statistical analysis of node levels.
}

TEST_F(SkipListTest, EraseAdjustsLevel) {
    // Insert elements to raise the level
    sl_int.insert(10); // Level could be >0
    sl_int.insert(20);
    sl_int.insert(30);

    // Force a high level node (difficult to do deterministically without friend access or internal hooks)
    // So, we rely on random chance or many insertions.
    // For this test, let's insert many and hope some high-level nodes are created.
    for (int i = 0; i < 200; ++i) {
        sl_int.insert(i * 100 + i); // Spaced out values
    }
    int initial_level = sl_int.currentListLevel();
    ASSERT_GT(initial_level, 0); // Should have some levels by now

    // Now, erase all elements. The level should drop back to 0.
    std::vector<int> all_elements;
    // This requires iterators or a way to get all elements.
    // For now, let's just clear and check.
    // If we had iterators:
    // for (int val : sl_int) { all_elements.push_back(val); }
    // for (int val : all_elements) { sl_int.erase(val); }
    // EXPECT_EQ(sl_int.currentListLevel(), 0);
    // EXPECT_TRUE(sl_int.empty());

    // Without iterators, we can clear.
    sl_int.clear();
    EXPECT_EQ(sl_int.currentListLevel(), 0);
    EXPECT_TRUE(sl_int.empty());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

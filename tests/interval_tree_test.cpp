#include "gtest/gtest.h"
#include "interval_tree.h"
#include <string>
#include <vector>
#include <algorithm> // For std::sort

using namespace interval_tree;

// Helper to compare vectors of intervals, ignoring order and value for some tests.
template <typename T>
bool compare_interval_vectors_ignore_order(std::vector<Interval<T>> vec1, std::vector<Interval<T>> vec2) {
    if (vec1.size() != vec2.size()) return false;
    // Sort based on start, then end. For values, rely on T's operator==.
    auto sort_key = [](const Interval<T>& iv) { return std::make_tuple(iv.start, iv.end, iv.value); };
    std::sort(vec1.begin(), vec1.end(), [&](const Interval<T>& a, const Interval<T>& b){
        return sort_key(a) < sort_key(b);
    });
    std::sort(vec2.begin(), vec2.end(), [&](const Interval<T>& a, const Interval<T>& b){
        return sort_key(a) < sort_key(b);
    });
    return vec1 == vec2;
}


struct TestVal {
    int id;
    std::string data;

    TestVal(int i = 0, std::string d = "") : id(i), data(std::move(d)) {}

    bool operator==(const TestVal& other) const {
        return id == other.id && data == other.data;
    }
     // Required for sorting in compare_interval_vectors_ignore_order
    bool operator<(const TestVal& other) const {
        if (id != other.id) return id < other.id;
        return data < other.data;
    }
};


// Test fixture for IntervalTree
class IntervalTreeTest : public ::testing::Test {
protected:
    IntervalTree<int> tree_int_;
    IntervalTree<std::string> tree_str_;
    IntervalTree<TestVal> tree_custom_;
    // IntervalTree<std::unique_ptr<int>> tree_uptr; // Cannot be easily queried by value
};

// Tests for Interval struct
TEST_F(IntervalTreeTest, IntervalStruct) {
    Interval<int> iv1(10, 20, 100);
    EXPECT_EQ(iv1.start, 10);
    EXPECT_EQ(iv1.end, 20);
    EXPECT_EQ(iv1.value, 100);

    EXPECT_TRUE(iv1.overlaps(15));
    EXPECT_TRUE(iv1.overlaps(10));
    EXPECT_FALSE(iv1.overlaps(20)); // end is exclusive
    EXPECT_FALSE(iv1.overlaps(5));
    EXPECT_FALSE(iv1.overlaps(25));

    EXPECT_TRUE(iv1.overlaps(12, 18)); // Fully contained
    EXPECT_TRUE(iv1.overlaps(5, 15));  // Overlaps start
    EXPECT_TRUE(iv1.overlaps(15, 25)); // Overlaps end
    EXPECT_TRUE(iv1.overlaps(5, 25));  // Contains
    EXPECT_FALSE(iv1.overlaps(1, 5));
    EXPECT_FALSE(iv1.overlaps(25, 30));
    EXPECT_FALSE(iv1.overlaps(20, 25)); // Adjacent, but end is exclusive

    Interval<int> iv2(10, 20, 100);
    Interval<int> iv3(10, 21, 100);
    Interval<int> iv4(10, 20, 101);
    EXPECT_TRUE(iv1 == iv2);
    EXPECT_FALSE(iv1 == iv3);
    EXPECT_FALSE(iv1 == iv4);

    ASSERT_DEATH({ Interval<int> iv_invalid(20, 10, 0); }, "");
}

// Basic tests for IntervalTree
TEST_F(IntervalTreeTest, EmptyTree) {
    EXPECT_TRUE(tree_int_.empty());
    EXPECT_EQ(tree_int_.size(), 0);
    auto result = tree_int_.query(10);
    EXPECT_TRUE(result.empty());
    auto result_range = tree_int_.query(10, 20);
    EXPECT_TRUE(result_range.empty());
    auto all_result = tree_int_.all();
    EXPECT_TRUE(all_result.empty());
}

TEST_F(IntervalTreeTest, InsertAndSize) {
    tree_int_.insert(10, 20, 1);
    EXPECT_FALSE(tree_int_.empty());
    EXPECT_EQ(tree_int_.size(), 1);

    tree_int_.insert(15, 25, 2);
    EXPECT_EQ(tree_int_.size(), 2);

    // Identical interval (start, end, value) should not increase size if we assume unique intervals are enforced by user or value equality.
    // However, the current tree allows duplicate intervals if values are different or even if values are same.
    // The problem states "Intervals are stored by {start, end, value}".
    // Let's assume the tree is an AVL tree of these Interval structs, sorted by start then end.
    // It does not enforce uniqueness of (start,end,value) triples beyond what AVL insertion would do.
    // If we insert the exact same interval, it might go to right based on current logic.
    tree_int_.insert(10, 20, 1); // duplicate
    EXPECT_EQ(tree_int_.size(), 3);

    tree_int_.insert(10, 20, 3); // same range, different value
    EXPECT_EQ(tree_int_.size(), 4);
}

TEST_F(IntervalTreeTest, ClearTree) {
    tree_int_.insert(10, 20, 1);
    tree_int_.insert(15, 25, 2);
    ASSERT_FALSE(tree_int_.empty());
    tree_int_.clear();
    EXPECT_TRUE(tree_int_.empty());
    EXPECT_EQ(tree_int_.size(), 0);
    auto result = tree_int_.query(15);
    EXPECT_TRUE(result.empty());
}

TEST_F(IntervalTreeTest, PointQueryBasic) {
    tree_int_.insert(10, 20, 101); // A
    tree_int_.insert(15, 25, 102); // B
    tree_int_.insert(30, 40, 103); // C
    tree_int_.insert(5, 12, 104);  // D (overlaps A at 10, 11)

    auto res1 = tree_int_.query(8); // Query point 8
    EXPECT_FALSE(res1.empty()) << "Query for point 8 should not be empty.";
    EXPECT_TRUE(compare_interval_vectors_ignore_order<int>(res1, {{5, 12, 104}})) << "Point 8 should find interval D.";

    auto res2 = tree_int_.query(11); // D, A
    EXPECT_TRUE(compare_interval_vectors_ignore_order<int>(res2, {{5,12,104}, {10,20,101}}));

    auto res3 = tree_int_.query(17); // A, B
    EXPECT_TRUE(compare_interval_vectors_ignore_order<int>(res3, {{10,20,101}, {15,25,102}}));

    auto res4 = tree_int_.query(22); // B
    EXPECT_TRUE(compare_interval_vectors_ignore_order<int>(res4, {{15,25,102}}));

    auto res5 = tree_int_.query(35); // C
    EXPECT_TRUE(compare_interval_vectors_ignore_order<int>(res5, {{30,40,103}}));

    auto res6 = tree_int_.query(10); // D, A (start is inclusive)
    EXPECT_TRUE(compare_interval_vectors_ignore_order<int>(res6, {{5,12,104}, {10,20,101}}));

    auto res7 = tree_int_.query(20); // B (A's end is exclusive, B's start is inclusive)
    EXPECT_TRUE(compare_interval_vectors_ignore_order<int>(res7, {{15,25,102}}));
}

TEST_F(IntervalTreeTest, RangeQueryBasic) {
    tree_str_.insert(10, 20, "Alpha");
    tree_str_.insert(15, 25, "Bravo");
    tree_str_.insert(30, 40, "Charlie");
    tree_str_.insert(5, 12, "Delta");

    auto res1 = tree_str_.query(1, 4); // No overlap
    EXPECT_TRUE(res1.empty());

    auto res2 = tree_str_.query(8, 11); // Delta, Alpha
    EXPECT_TRUE(compare_interval_vectors_ignore_order<std::string>(res2, {Interval<std::string>(5,12,"Delta"), Interval<std::string>(10,20,"Alpha")}));

    auto res3 = tree_str_.query(16, 19); // Alpha, Bravo
    EXPECT_TRUE(compare_interval_vectors_ignore_order<std::string>(res3, {Interval<std::string>(10,20,"Alpha"), Interval<std::string>(15,25,"Bravo")}));

    auto res4 = tree_str_.query(5, 45); // All
    EXPECT_TRUE(compare_interval_vectors_ignore_order<std::string>(res4, {
        Interval<std::string>(5,12,"Delta"), Interval<std::string>(10,20,"Alpha"),
        Interval<std::string>(15,25,"Bravo"), Interval<std::string>(30,40,"Charlie")
    }));

    auto res5 = tree_str_.query(12, 15); // Alpha (touches D's end, B's start)
     EXPECT_TRUE(compare_interval_vectors_ignore_order<std::string>(res5, {Interval<std::string>(10,20,"Alpha")}));
}

TEST_F(IntervalTreeTest, RemoveBasic) {
    tree_int_.insert(10, 20, 1);
    tree_int_.insert(15, 25, 2);
    tree_int_.insert(30, 40, 3);
    ASSERT_EQ(tree_int_.size(), 3);

    tree_int_.remove(15, 25, 2); // Remove B
    EXPECT_EQ(tree_int_.size(), 2);
    auto query_res_empty_at_20 = tree_int_.query(20); // Point 20 is exclusive for A=[10,20)
    EXPECT_TRUE(query_res_empty_at_20.empty()) << "Querying exclusive end point 20 should be empty after B is removed.";
    auto query_res_present_at_19 = tree_int_.query(19); // Point 19 is inclusive for A=[10,20)
    EXPECT_TRUE(compare_interval_vectors_ignore_order<int>(query_res_present_at_19, {{10,20,1}})) << "Querying point 19 within interval A failed after B removed.";

    tree_int_.remove(10, 20, 1); // Remove A
    EXPECT_EQ(tree_int_.size(), 1);
    auto query_res_after_A_removed_at_15 = tree_int_.query(15);
    EXPECT_TRUE(query_res_after_A_removed_at_15.empty()) << "Querying for A at point 15 after it was removed should be empty.";
    // Check that C is still present
    auto query_res_C_still_present_at_35 = tree_int_.query(35);
    EXPECT_TRUE(compare_interval_vectors_ignore_order<int>(query_res_C_still_present_at_35, {{30,40,3}})) << "Interval C not found at point 35 after removing A and B.";

    tree_int_.remove(30, 40, 3); // Remove C
    EXPECT_EQ(tree_int_.size(), 0);
    EXPECT_TRUE(tree_int_.empty());

    // Remove non-existent
    tree_int_.remove(100, 200, 100);
    EXPECT_EQ(tree_int_.size(), 0);
}

TEST_F(IntervalTreeTest, RemoveSpecificValue) {
    tree_custom_.insert(10, 20, TestVal(1, "A"));
    tree_custom_.insert(10, 20, TestVal(2, "B")); // Same range, different value
    tree_custom_.insert(10, 20, TestVal(1, "A")); // Exact duplicate
    ASSERT_EQ(tree_custom_.size(), 3);

    tree_custom_.remove(10, 20, TestVal(1, "A")); // Removes one instance of (1,A). Size becomes 2.
    EXPECT_EQ(tree_custom_.size(), 2);

    auto res_after_first_remove = tree_custom_.query(15);
    // Expected: one (1,A) and one (2,B) remaining
    int count1A_after_first_remove = 0;
    int count2B_after_first_remove = 0;
    for(const auto& iv : res_after_first_remove) {
        if(iv.value == TestVal(1,"A")) count1A_after_first_remove++;
        if(iv.value == TestVal(2,"B")) count2B_after_first_remove++;
    }
    EXPECT_EQ(count1A_after_first_remove, 1) << "Should be one TestVal(1,A) left.";
    EXPECT_EQ(count2B_after_first_remove, 1) << "Should be one TestVal(2,B) left.";
    EXPECT_EQ(res_after_first_remove.size(), 2);

    tree_custom_.remove(10, 20, TestVal(1, "A")); // Removes the second instance of (1,A). Size becomes 1.
    EXPECT_EQ(tree_custom_.size(), 1);
    auto res_after_second_remove = tree_custom_.query(15); // Should contain only (2,B)
    EXPECT_TRUE(compare_interval_vectors_ignore_order<TestVal>(res_after_second_remove, {{10,20,TestVal(2,"B")}}));
}


TEST_F(IntervalTreeTest, AllMethod) {
    tree_str_.insert(30, 40, "C");
    tree_str_.insert(10, 20, "A");
    tree_str_.insert(15, 25, "B");

    auto all_ivs = tree_str_.all();
    EXPECT_EQ(all_ivs.size(), 3);
    // Order from all() is in-order traversal (by start, then end)
    std::vector<Interval<std::string>> expected = {
        {10, 20, "A"}, {15, 25, "B"}, {30, 40, "C"}
    };
    // The current tree structure might not guarantee this specific order from collect_all_impl
    // if intervals with same start are inserted in different orders.
    // Let's sort both to be sure.
    auto sort_key = [](const Interval<std::string>& iv) { return std::make_tuple(iv.start, iv.end, iv.value); };
    std::sort(all_ivs.begin(), all_ivs.end(), [&](const auto& a, const auto& b){ return sort_key(a) < sort_key(b); });
    std::sort(expected.begin(), expected.end(), [&](const auto& a, const auto& b){ return sort_key(a) < sort_key(b); });
    EXPECT_EQ(all_ivs, expected);
}


TEST_F(IntervalTreeTest, AVLBalanceProperties) {
    // This test implicitly checks AVL balancing by inserting in various orders
    // and ensuring size and queries are correct. More rigorous AVL testing would
    // require inspecting node heights and balance factors directly.

    // Ascending order (lots of left rotations expected at root or near root)
    for (int i = 0; i < 100; ++i) {
        tree_int_.insert(i, i + 10, i);
    }
    EXPECT_EQ(tree_int_.size(), 100);
    auto res_asc = tree_int_.query(5,15); // Should find multiple
    EXPECT_FALSE(res_asc.empty());
    tree_int_.clear();

    // Descending order (lots of right rotations)
    for (int i = 100; i > 0; --i) {
        tree_int_.insert(i, i + 10, i);
    }
    EXPECT_EQ(tree_int_.size(), 100);
    auto res_desc = tree_int_.query(5,15);
    EXPECT_FALSE(res_desc.empty());
    tree_int_.clear();

    // Insert middle, then smaller, then larger (zig-zag)
    tree_int_.insert(50, 60, 50);
    for (int i = 0; i < 50; ++i) {
        tree_int_.insert(i, i + 5, i);          // Smaller
        tree_int_.insert(100 - i, 105 - i, 100 - i); // Larger
    }
    EXPECT_EQ(tree_int_.size(), 101); // 1 (middle) + 50*2
    auto res_zz = tree_int_.query(55); // Query near middle
    // Intervals overlapping 55: (50,60,50) and 5 from the second loop: (51,56,51) to (55,60,55)
    EXPECT_EQ(res_zz.size(), 6);
    // Optionally, verify contents if exact values are important beyond size
    // For now, correcting size is the main goal.
    // To check the (50,60,50) interval:
    bool found_middle = false;
    for(const auto& iv : res_zz) {
        if (iv.start == 50 && iv.end == 60 && iv.value == 50) {
            found_middle = true;
            break;
        }
    }
    EXPECT_TRUE(found_middle) << "The original middle interval (50,60,50) was not found when querying point 55.";
    tree_int_.clear();
}

// TEST_F(IntervalTreeTest, MoveSemanticsForValue) {
//     // This test is problematic because IntervalTree::query returns std::vector<Interval<T>>,
//     // which requires Interval<T> to be copyable. If T is std::unique_ptr, Interval<T> is not copyable.
//     // Supporting this would require query to return e.g. vector<const Interval<T>*>.
//     IntervalTree<std::unique_ptr<int>> tree_ptr;
//     tree_ptr.insert(10, 20, std::make_unique<int>(1));
//     tree_ptr.insert(15, 25, std::make_unique<int>(2));

//     auto res = tree_ptr.query(16); // This line would fail to compile
//     EXPECT_EQ(res.size(), 2);
// }

TEST_F(IntervalTreeTest, MaxEndNodeUpdates) {
    // This is hard to test without inspecting internal nodes.
    // Correctness of query functions somewhat implies max_end is working.
    // Example: if max_end was not updated, queries might terminate prematurely.
    tree_int_.insert(10, 20, 1); // root, max_end=20
    tree_int_.insert(5, 15, 2);  // left child, max_end=15. root's max_end should still be 20.
    tree_int_.insert(30, 40, 3); // right child. root's max_end should become 40.

    // If root's max_end was stuck at 20, this query might fail to find (30,40,3)
    auto res = tree_int_.query(35);
    ASSERT_EQ(res.size(), 1);
    EXPECT_EQ(res[0].value, 3);

    tree_int_.insert(0, 50, 4); // This could become new root or cause rotations. Max_end should be 50.
    res = tree_int_.query(45);
    ASSERT_EQ(res.size(), 1);
    EXPECT_EQ(res[0].value, 4);

    tree_int_.remove(0,50,4); // max_end should revert or be recalculated
    res = tree_int_.query(35);
    ASSERT_EQ(res.size(), 1);
    EXPECT_EQ(res[0].value, 3); // (30,40,3) should still be there
}

// Test move constructor and assignment for IntervalTree
TEST_F(IntervalTreeTest, IntervalTreeMoveSemantics) {
    IntervalTree<int> tree1;
    tree1.insert(10, 20, 1);
    tree1.insert(15, 25, 2);

    IntervalTree<int> tree2 = std::move(tree1);
    EXPECT_EQ(tree2.size(), 2);
    EXPECT_TRUE(tree1.empty()); // tree1 should be empty after move

    auto res2 = tree2.query(16);
    EXPECT_EQ(res2.size(), 2);

    IntervalTree<int> tree3;
    tree3.insert(100,110,10);
    tree3 = std::move(tree2);
    EXPECT_EQ(tree3.size(), 2);
    EXPECT_TRUE(tree2.empty()); // tree2 should be empty

    auto res3 = tree3.query(16);
    EXPECT_EQ(res3.size(), 2);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

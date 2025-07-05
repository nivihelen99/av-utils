#include "sparse_set.h" // Assuming sparse_set.h is in include directory accessible here
#include "gtest/gtest.h"
#include <vector>
#include <algorithm> // For std::sort, std::is_permutation
#include <set>       // For comparing contents

// Test fixture for SparseSet tests
class SparseSetTest : public ::testing::Test {
protected:
    // Per-test set-up and tear-down can be done here if needed
};

TEST_F(SparseSetTest, ConstructorAndBasicProperties) {
    cpp_collections::SparseSet<uint32_t, uint32_t> ss(100); // Max value 99
    EXPECT_TRUE(ss.empty());
    EXPECT_EQ(ss.size(), 0);
    EXPECT_EQ(ss.max_value_capacity(), 100);
    EXPECT_GE(ss.dense_capacity(), 0);

    cpp_collections::SparseSet<int, size_t> ss_int(50, 10); // Max value 49, initial dense 10
    EXPECT_TRUE(ss_int.empty());
    EXPECT_EQ(ss_int.size(), 0);
    EXPECT_EQ(ss_int.max_value_capacity(), 50);
    EXPECT_GE(ss_int.dense_capacity(), 10);

    cpp_collections::SparseSet<uint16_t> ss_zero_max(0);
    EXPECT_TRUE(ss_zero_max.empty());
    EXPECT_EQ(ss_zero_max.max_value_capacity(), 0);
    auto insert_res_zero = ss_zero_max.insert(0);
    EXPECT_FALSE(insert_res_zero.second);

    cpp_collections::SparseSet<uint16_t> ss_one_max(1);
    EXPECT_TRUE(ss_one_max.empty());
    EXPECT_EQ(ss_one_max.max_value_capacity(), 1);
    auto insert_res_one = ss_one_max.insert(0);
    EXPECT_TRUE(insert_res_one.second);
    EXPECT_EQ(ss_one_max.size(), 1);
    EXPECT_TRUE(ss_one_max.contains(0));
    auto insert_res_one_again = ss_one_max.insert(1);
    EXPECT_FALSE(insert_res_one_again.second);
}

TEST_F(SparseSetTest, InsertAndContains) {
    cpp_collections::SparseSet<uint32_t> ss(100);

    auto res1 = ss.insert(10);
    EXPECT_TRUE(res1.second);
    EXPECT_EQ(*res1.first, 10);
    EXPECT_TRUE(ss.contains(10));
    EXPECT_EQ(ss.size(), 1);

    auto res2 = ss.insert(20);
    EXPECT_TRUE(res2.second);
    EXPECT_EQ(*res2.first, 20);
    EXPECT_TRUE(ss.contains(20));
    EXPECT_EQ(ss.size(), 2);

    auto res3 = ss.insert(10);
    EXPECT_FALSE(res3.second);
    EXPECT_EQ(*res3.first, 10);
    EXPECT_TRUE(ss.contains(10));
    EXPECT_EQ(ss.size(), 2);

    auto res_boundary = ss.insert(99);
    EXPECT_TRUE(res_boundary.second);
    EXPECT_TRUE(ss.contains(99));
    EXPECT_EQ(ss.size(), 3);

    auto res_zero = ss.insert(0);
    EXPECT_TRUE(res_zero.second);
    EXPECT_TRUE(ss.contains(0));
    EXPECT_EQ(ss.size(), 4);

    EXPECT_FALSE(ss.contains(5));
    EXPECT_FALSE(ss.contains(100));

    auto res_oor = ss.insert(100);
    EXPECT_FALSE(res_oor.second);
    EXPECT_EQ(ss.size(), 4);
}

TEST_F(SparseSetTest, Erase) {
    cpp_collections::SparseSet<uint32_t> ss(100);
    ss.insert(10);
    ss.insert(20);
    ss.insert(30);
    ss.insert(0);
    ss.insert(99);
    ASSERT_EQ(ss.size(), 5);

    EXPECT_TRUE(ss.erase(20));
    EXPECT_FALSE(ss.contains(20));
    EXPECT_EQ(ss.size(), 4);

    EXPECT_TRUE(ss.erase(0));
    EXPECT_FALSE(ss.contains(0));
    EXPECT_EQ(ss.size(), 3);

    EXPECT_TRUE(ss.erase(99));
    EXPECT_FALSE(ss.contains(99));
    EXPECT_EQ(ss.size(), 2);

    EXPECT_FALSE(ss.erase(50));
    EXPECT_EQ(ss.size(), 2);

    EXPECT_FALSE(ss.erase(20));
    EXPECT_EQ(ss.size(), 2);

    EXPECT_FALSE(ss.erase(100));
    EXPECT_EQ(ss.size(), 2);

    EXPECT_TRUE(ss.erase(10));
    EXPECT_TRUE(ss.erase(30));
    EXPECT_TRUE(ss.empty());
    EXPECT_EQ(ss.size(), 0);

    EXPECT_FALSE(ss.erase(10));

    cpp_collections::SparseSet<uint32_t> ss2(20);
    ss2.insert(1);
    ss2.insert(5);
    ss2.insert(3);
    ASSERT_TRUE(ss2.erase(1));
    EXPECT_EQ(ss2.size(), 2);
    EXPECT_FALSE(ss2.contains(1));
    EXPECT_TRUE(ss2.contains(3));
    EXPECT_TRUE(ss2.contains(5));

    std::set<uint32_t> expected_after_erase_1 = {3, 5};
    std::set<uint32_t> actual_after_erase_1;
    for(uint32_t val : ss2) actual_after_erase_1.insert(val);
    EXPECT_EQ(actual_after_erase_1, expected_after_erase_1);
}

TEST_F(SparseSetTest, ClearAndSwap) {
    cpp_collections::SparseSet<uint32_t> ss1(100);
    ss1.insert(10);
    ss1.insert(20);
    ASSERT_EQ(ss1.size(), 2);

    ss1.clear();
    EXPECT_TRUE(ss1.empty());
    EXPECT_EQ(ss1.size(), 0);
    EXPECT_FALSE(ss1.contains(10));
    EXPECT_EQ(ss1.max_value_capacity(), 100);

    ss1.insert(5);
    EXPECT_TRUE(ss1.contains(5));
    EXPECT_EQ(ss1.size(), 1);

    cpp_collections::SparseSet<uint32_t> ss2(50);
    ss2.insert(30);
    ss2.insert(40);

    ss1.swap(ss2);

    EXPECT_EQ(ss1.size(), 2);
    EXPECT_TRUE(ss1.contains(30));
    EXPECT_TRUE(ss1.contains(40));
    EXPECT_FALSE(ss1.contains(5));
    EXPECT_EQ(ss1.max_value_capacity(), 50);

    EXPECT_EQ(ss2.size(), 1);
    EXPECT_TRUE(ss2.contains(5));
    EXPECT_FALSE(ss2.contains(30));
    EXPECT_EQ(ss2.max_value_capacity(), 100);

    cpp_collections::swap(ss1, ss2);

    EXPECT_EQ(ss2.size(), 2);
    EXPECT_TRUE(ss2.contains(30));
    EXPECT_EQ(ss1.size(), 1);
    EXPECT_TRUE(ss1.contains(5));
}

TEST_F(SparseSetTest, Iteration) {
    cpp_collections::SparseSet<uint32_t> ss(100);

    int count = 0;
    for (uint32_t val : ss) {
        (void)val;
        count++;
    }
    EXPECT_EQ(count, 0);
    EXPECT_EQ(ss.begin(), ss.end());

    ss.insert(10);
    ss.insert(1);
    ss.insert(50);
    ss.insert(5);

    std::set<uint32_t> expected_elements = {1, 5, 10, 50};
    std::set<uint32_t> actual_elements;
    for (uint32_t val : ss) {
        actual_elements.insert(val);
    }
    EXPECT_EQ(actual_elements, expected_elements);
    EXPECT_EQ(ss.size(), expected_elements.size());

    const auto& const_ss = ss;
    actual_elements.clear();
    for (uint32_t val : const_ss) {
        actual_elements.insert(val);
    }
    EXPECT_EQ(actual_elements, expected_elements);
    EXPECT_EQ(const_ss.cbegin(), const_ss.begin());
    EXPECT_EQ(const_ss.cend(), const_ss.end());


    std::vector<uint32_t> from_iter;
    for (auto it = ss.begin(); it != ss.end(); ++it) {
        from_iter.push_back(*it);
    }
    EXPECT_EQ(from_iter.size(), expected_elements.size());
    for(uint32_t val : from_iter) {
        EXPECT_TRUE(expected_elements.count(val));
    }

    auto it_found = ss.find(10);
    ASSERT_NE(it_found, ss.end());
    EXPECT_EQ(*it_found, 10);

    auto it_not_found = ss.find(101);
    EXPECT_EQ(it_not_found, ss.end());

    const auto it_const_found = const_ss.find(5);
    ASSERT_NE(it_const_found, const_ss.end());
    EXPECT_EQ(*it_const_found, 5);
}

TEST_F(SparseSetTest, DenseReallocation) {
    cpp_collections::SparseSet<uint32_t> ss(1000, 2);

    EXPECT_GE(ss.dense_capacity(), 2);

    ss.insert(10);
    ss.insert(20);
    size_t cap_before = ss.dense_capacity();
    ASSERT_EQ(ss.size(), 2);

    ss.insert(30);
    EXPECT_EQ(ss.size(), 3);
    EXPECT_TRUE(ss.contains(10));
    EXPECT_TRUE(ss.contains(20));
    EXPECT_TRUE(ss.contains(30));
    if (cap_before <= 2 && cap_before > 0) { // Only check growth if initial capacity was small and binding
      EXPECT_GT(ss.dense_capacity(), cap_before);
    }


    for (uint32_t i = 0; i < 50; ++i) {
        ss.insert(i + 100);
    }
    EXPECT_EQ(ss.size(), 3 + 50);
    EXPECT_TRUE(ss.contains(10));
    EXPECT_TRUE(ss.contains(120));

    cpp_collections::SparseSet<uint32_t> ss2(100);
    ss2.reserve_dense(10);
    EXPECT_GE(ss2.dense_capacity(), 10);
    for(int i=0; i<10; ++i) ss2.insert(i);
    EXPECT_EQ(ss2.size(),10);
    size_t cap_after_fill = ss2.dense_capacity();
    ss2.insert(10);
    if (cap_after_fill <=10 ) { // Check growth only if capacity was binding
        EXPECT_GT(ss2.dense_capacity(), cap_after_fill);
    }
}

TEST_F(SparseSetTest, ComparisonOperators) {
    cpp_collections::SparseSet<uint32_t> ss1(100), ss2(100), ss3(100), ss4(50);

    ss1.insert(10); ss1.insert(20);
    ss2.insert(20); ss2.insert(10);
    ss3.insert(10); ss3.insert(30);
    ss4.insert(10); ss4.insert(20);

    EXPECT_EQ(ss1, ss2);
    EXPECT_FALSE(ss1 != ss2);

    EXPECT_NE(ss1, ss3);
    EXPECT_FALSE(ss1 == ss3);

    EXPECT_EQ(ss1, ss4);

    ss2.clear();
    EXPECT_NE(ss1, ss2);
}

// It's good practice to have a main if not linking with gtest_main,
// but since tests/CMakeLists.txt links GTest::gtest_main, this is not needed.
// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }

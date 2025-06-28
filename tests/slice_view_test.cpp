#include "gtest/gtest.h"
#include "slice_view.h"
#include <vector>
#include <string>
#include <array>
#include <numeric> // for std::iota

// Helper function to compare slice content with a vector
template<typename T, typename SliceViewType>
void EXPECT_SLICE_EQ(const SliceViewType& slice_view, const std::vector<T>& expected) {
    EXPECT_EQ(slice_view.size(), expected.size());
    if (slice_view.size() != expected.size()) { // Guard further checks if sizes differ
        return;
    }
    for (size_t i = 0; i < slice_view.size(); ++i) {
        EXPECT_EQ(slice_view[i], expected[i]) << "Mismatch at index " << i;
    }

    // Test iterators
    std::vector<T> from_iter;
    for (const auto& elem : slice_view) {
        from_iter.push_back(elem);
    }
    EXPECT_EQ(from_iter, expected);

    if (!expected.empty()) {
        EXPECT_EQ(slice_view.front(), expected.front());
        EXPECT_EQ(slice_view.back(), expected.back());
    }
    EXPECT_EQ(slice_view.empty(), expected.empty());
}


TEST(SliceViewTest, EmptyContainer) {
    std::vector<int> vec_empty;
    auto s_empty = slice(vec_empty, 0, 0);
    EXPECT_SLICE_EQ<int>(s_empty, {});
    EXPECT_TRUE(s_empty.empty());
    EXPECT_EQ(s_empty.size(), 0);

    auto s_empty_neg_idx = slice(vec_empty, -1, -1);
    EXPECT_SLICE_EQ<int>(s_empty_neg_idx, {});

    std::string str_empty = "";
    auto s_str_empty = slice(str_empty, 0, 0);
    EXPECT_SLICE_EQ<char>(s_str_empty, {});
}

TEST(SliceViewTest, BasicSlicingVector) {
    std::vector<int> vec = {10, 20, 30, 40, 50, 60, 70};

    auto s1 = slice(vec, 0, 3); // {10, 20, 30}
    EXPECT_SLICE_EQ<int>(s1, {10, 20, 30});

    auto s2 = slice(vec, 2, 5); // {30, 40, 50}
    EXPECT_SLICE_EQ<int>(s2, {30, 40, 50});

    auto s3 = slice(vec, 2);    // {30, 40, 50, 60, 70}
    EXPECT_SLICE_EQ<int>(s3, {30, 40, 50, 60, 70});

    auto s_full = slice(vec); // {10, 20, 30, 40, 50, 60, 70}
    EXPECT_SLICE_EQ<int>(s_full, {10, 20, 30, 40, 50, 60, 70});
    EXPECT_EQ(s_full.data(), vec.data());
}

TEST(SliceViewTest, NegativeIndexSlicingVector) {
    std::vector<int> vec = {10, 20, 30, 40, 50, 60, 70};

    auto s1 = slice(vec, -2);   // {60, 70}
    EXPECT_SLICE_EQ<int>(s1, {60, 70});

    auto s2 = slice(vec, 0, -1); // {10, 20, 30, 40, 50, 60}
    EXPECT_SLICE_EQ<int>(s2, {10, 20, 30, 40, 50, 60});

    auto s3 = slice(vec, -5, -2); // {30, 40, 50}
    EXPECT_SLICE_EQ<int>(s3, {30, 40, 50});

    auto s4 = slice(vec, -1, -2); // Empty, start >= stop normalized
    EXPECT_SLICE_EQ<int>(s4, {});
}

TEST(SliceViewTest, StepSlicingVector) {
    std::vector<int> vec(10);
    std::iota(vec.begin(), vec.end(), 0); // 0, 1, 2, ..., 9

    auto s1 = slice(vec, 0, 10, 2); // {0, 2, 4, 6, 8}
    EXPECT_SLICE_EQ<int>(s1, {0, 2, 4, 6, 8});

    auto s2 = slice(vec, 1, 7, 3);  // {1, 4}
    EXPECT_SLICE_EQ<int>(s2, {1, 4});

    auto s3 = slice(vec, 0, 10, 1); // Equivalent to slice(vec)
    EXPECT_SLICE_EQ<int>(s3, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});

    auto s4 = slice(vec, 0, 1, 2); // {0}
    EXPECT_SLICE_EQ<int>(s4, {0});

    auto s5 = slice(vec, 8, 10, 2); // {8}
    EXPECT_SLICE_EQ<int>(s5, {8});

    auto s6 = slice(vec, 0, 10, 100); // {0}
    EXPECT_SLICE_EQ<int>(s6, {0});
}

TEST(SliceViewTest, ReverseSlicingVector) {
    std::vector<int> vec = {10, 20, 30, 40, 50, 60, 70};

    auto s1 = slice(vec, -1, -8, -1); // {70, 60, 50, 40, 30, 20, 10}
    EXPECT_SLICE_EQ<int>(s1, {70, 60, 50, 40, 30, 20, 10});
    EXPECT_EQ(s1.data(), &vec[6]);


    auto s2 = slice(vec, 6, -1, -1); // Same as above {70, ..., 10}
    EXPECT_SLICE_EQ<int>(s2, {70, 60, 50, 40, 30, 20, 10});


    auto s3 = slice(vec, 4, 1, -1);   // {50, 40, 30}
    EXPECT_SLICE_EQ<int>(s3, {50, 40, 30});
    EXPECT_EQ(s3.data(), &vec[4]);

    auto s4 = slice(vec, 0, 7, -1); // Empty, start <= stop normalized
    EXPECT_SLICE_EQ<int>(s4, {});

    auto s5 = slice(vec, 2, 0, -2); // {30, 10}
    EXPECT_SLICE_EQ<int>(s5, {30, 10});

    auto s6 = slice(vec, 6, 0, -3); // {70, 40, 10}
    EXPECT_SLICE_EQ<int>(s6, {70, 40, 10});
}

TEST(SliceViewTest, StringSlicing) {
    std::string str = "Hello, World!"; // size 13

    auto s1 = slice(str, 0, 5); // "Hello"
    EXPECT_SLICE_EQ<char>(s1, {'H', 'e', 'l', 'l', 'o'});

    auto s2 = slice(str, 7, 12); // "World"
    EXPECT_SLICE_EQ<char>(s2, {'W', 'o', 'r', 'l', 'd'});

    auto s3 = slice(str, -6);    // "World!"
    EXPECT_SLICE_EQ<char>(s3, {'W', 'o', 'r', 'l', 'd', '!'});

    auto s4 = slice(str, -1, -14, -1); // "!dlroW ,olleH" - Original string "Hello, World!" is 13 chars. Reversed is also 13.
    EXPECT_SLICE_EQ<char>(s4, {'!', 'd', 'l', 'r', 'o', 'W', ' ', ',', 'o', 'l', 'l', 'e', 'H'}); // Corrected to 13 chars. Removed extra space.

    auto s5 = slice(str, 0, str.size(), 2); // "Hlo ol!"
    EXPECT_SLICE_EQ<char>(s5, {'H', 'l', 'o', ' ', 'o', 'l', '!'});
}

TEST(SliceViewTest, ArraySlicing) {
    std::array<double, 6> arr = {1.1, 2.2, 3.3, 4.4, 5.5, 6.6};

    auto s1 = slice(arr, 2, -1); // {3.3, 4.4, 5.5}
    EXPECT_SLICE_EQ<double>(s1, {3.3, 4.4, 5.5});

    auto s2 = slice(arr, 0, arr.size(), 2); // {1.1, 3.3, 5.5}
    EXPECT_SLICE_EQ<double>(s2, {1.1, 3.3, 5.5});

    auto s_full = slice(arr);
    EXPECT_SLICE_EQ<double>(s_full, {1.1, 2.2, 3.3, 4.4, 5.5, 6.6});
    EXPECT_EQ(s_full.data(), arr.data());
}

TEST(SliceViewTest, ConstSlicing) {
    const std::vector<int> vec = {10, 20, 30, 40, 50};

    auto s1 = slice(vec, 1, 4); // {20, 30, 40}
    // Pass non-const version to EXPECT_SLICE_EQ's expected parameter
    EXPECT_SLICE_EQ<int>(s1, {20, 30, 40});
    EXPECT_EQ(s1.front(), 20);
    EXPECT_EQ(s1.back(), 40);
    EXPECT_EQ(s1[1], 30);

    // s1[0] = 100; // This should not compile if s1 is a slice of const int
    static_assert(std::is_const_v<std::remove_reference_t<decltype(s1[0])>>, "Slice of const container should yield const references");

    const std::string str = "test";
    auto s_str = slice(str, 0, 2);
    // Pass non-const version to EXPECT_SLICE_EQ's expected parameter
    EXPECT_SLICE_EQ<char>(s_str, {'t', 'e'});
    static_assert(std::is_const_v<std::remove_reference_t<decltype(s_str[0])>>, "Slice of const string should yield const char references");
}

TEST(SliceViewTest, MutableSlicing) {
    std::vector<int> vec = {1, 2, 3, 4, 5};

    auto s1 = slice(vec, 1, 4); // {2, 3, 4}
    for (auto& elem : s1) {
        elem *= 10;
    }
    EXPECT_SLICE_EQ<int>(s1, {20, 30, 40});
    EXPECT_SLICE_EQ<int>(vec, {1, 20, 30, 40, 5}); // Original vector modified

    auto s2 = slice(vec, 0, 5, 2); // {1, 30, 5} (after previous modification)
    for (auto& elem : s2) {
        elem += 1;
    }
    // s2 was {1, 30, 5} -> now elements are {2, 31, 6}
    EXPECT_SLICE_EQ<int>(s2, {2, 31, 6});
    // Original vec was {1, 20, 30, 40, 5}
    // Modified elements: vec[0]=1->2, vec[2]=30->31, vec[4]=5->6
    EXPECT_SLICE_EQ<int>(vec, {2, 20, 31, 40, 6});
}

TEST(SliceViewTest, EdgeCasesAndOutOfBounds) {
    std::vector<int> vec = {0, 1, 2, 3, 4};

    // Slice beyond end
    auto s1 = slice(vec, 3, 10); // {3, 4}
    EXPECT_SLICE_EQ<int>(s1, {3, 4});

    // Slice before start (normalized to 0)
    auto s2 = slice(vec, -10, 2); // {0, 1}
    EXPECT_SLICE_EQ<int>(s2, {0, 1});

    // Slice completely out of bounds
    auto s3 = slice(vec, 10, 20); // {}
    EXPECT_SLICE_EQ<int>(s3, {});
    EXPECT_TRUE(s3.empty());

    auto s4 = slice(vec, -20, -10); // {}
    EXPECT_SLICE_EQ<int>(s4, {});

    // Start >= Stop with positive step
    auto s5 = slice(vec, 3, 3); // {}
    EXPECT_SLICE_EQ<int>(s5, {});
    auto s6 = slice(vec, 3, 2); // {}
    EXPECT_SLICE_EQ<int>(s6, {});

    // Start <= Stop with negative step
    auto s7 = slice(vec, 3, 3, -1); // {}
    EXPECT_SLICE_EQ<int>(s7, {});
    auto s8 = slice(vec, 2, 3, -1); // {}
    EXPECT_SLICE_EQ<int>(s8, {});

    // Single element slice
    std::vector<int> single_vec = {42};
    auto s_single = slice(single_vec, 0, 1);
    EXPECT_SLICE_EQ<int>(s_single, {42});
    EXPECT_EQ(s_single.front(), 42);
    EXPECT_EQ(s_single.back(), 42);

    auto s_single_neg = slice(single_vec, -1);
    EXPECT_SLICE_EQ<int>(s_single_neg, {42});

    // Zero step (should produce empty slice with step 1)
    auto s_zero_step = slice(vec, 0, 5, 0);
    EXPECT_SLICE_EQ<int>(s_zero_step, {});
    // The SliceView constructor will default step to 1 if 0 is passed.
    // The slice() helper function for step 0 returns an empty slice with step 1.
}

TEST(SliceViewTest, IteratorFunctionality) {
    std::vector<int> vec = {0, 1, 2, 3, 4, 5, 6};
    auto s = slice(vec, 1, 6, 2); // {1, 3, 5}

    auto it = s.begin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 3);
    it++;
    EXPECT_EQ(*it, 5);
    ++it;
    EXPECT_EQ(it, s.end());

    auto it_rend = s.end();
    --it_rend;
    EXPECT_EQ(*it_rend, 5);
    it_rend--;
    EXPECT_EQ(*it_rend, 3);
    --it_rend;
    EXPECT_EQ(*it_rend, 1);
    EXPECT_EQ(it_rend, s.begin());

    // Random access
    auto it_start = s.begin();
    EXPECT_EQ(*(it_start + 1), 3);
    EXPECT_EQ(it_start[2], 5);

    auto it_end = s.end();
    EXPECT_EQ(*(it_end - 1), 5);

    EXPECT_EQ(s.end() - s.begin(), 3); // Difference
    EXPECT_EQ(s.begin() - s.end(), -3);

    // Comparison
    EXPECT_TRUE(s.begin() < s.end());
    EXPECT_TRUE(s.begin() <= s.end());
    EXPECT_TRUE(s.end() > s.begin());
    EXPECT_TRUE(s.end() >= s.begin());
    EXPECT_FALSE(s.begin() == s.end());
    EXPECT_TRUE(s.begin() != s.end());

    // Test cbegin/cend
    auto cit = s.cbegin();
     EXPECT_EQ(*cit, 1);
    ++cit;
    EXPECT_EQ(*cit, 3);
    cit++;
    EXPECT_EQ(*cit, 5);
    ++cit;
    EXPECT_EQ(cit, s.cend());
}

TEST(SliceViewTest, ReverseIteratorFunctionality) {
    std::vector<int> vec = {0, 1, 2, 3, 4, 5, 6};
    auto s_rev = slice(vec, 5, 0, -2); // {5, 3, 1}

    EXPECT_SLICE_EQ<int>(s_rev, {5, 3, 1});

    auto it = s_rev.begin();
    EXPECT_EQ(*it, 5);
    ++it;
    EXPECT_EQ(*it, 3);
    it++;
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(it, s_rev.end());

    auto it_rend = s_rev.end();
    --it_rend;
    EXPECT_EQ(*it_rend, 1);
    it_rend--;
    EXPECT_EQ(*it_rend, 3);
    --it_rend;
    EXPECT_EQ(*it_rend, 5);
    EXPECT_EQ(it_rend, s_rev.begin());

    // Random access
    auto it_start = s_rev.begin();
    EXPECT_EQ(*(it_start + 1), 3);
    EXPECT_EQ(it_start[2], 1);

    auto it_end = s_rev.end();
    EXPECT_EQ(*(it_end - 1), 1);

    EXPECT_EQ(s_rev.end() - s_rev.begin(), 3);
}


TEST(SliceViewTest, DirectConstructor) {
    std::vector<int> vec = {10, 20, 30, 40, 50};

    // Normal slice
    slice_util::SliceView<int> sv1(vec.data() + 1, 3, 1); // 20, 30, 40
    EXPECT_SLICE_EQ<int>(sv1, {20, 30, 40});

    // Stepped slice
    slice_util::SliceView<int> sv2(vec.data(), 3, 2); // 10, 30, 50
    EXPECT_SLICE_EQ<int>(sv2, {10, 30, 50});

    // Reversed slice
    slice_util::SliceView<int> sv3(vec.data() + 4, 3, -2); // 50, 30, 10
    EXPECT_SLICE_EQ<int>(sv3, {50, 30, 10});

    // Empty slice via constructor
    slice_util::SliceView<int> sv_empty(vec.data(), 0, 1);
    EXPECT_SLICE_EQ<int>(sv_empty, {});
    EXPECT_TRUE(sv_empty.empty());

    const std::vector<int> cvec = {10, 20, 30, 40, 50};
    slice_util::SliceView<const int> csv1(cvec.data() + 1, 3, 1);
    EXPECT_SLICE_EQ<int>(csv1, {20, 30, 40}); // Changed const int to int for the expected vector
    static_assert(std::is_const_v<std::remove_reference_t<decltype(csv1[0])>>, "SliceView<const T> should provide const access");
}

TEST(SliceViewTest, DataMethod) {
    std::vector<int> vec = {10, 20, 30, 40, 50};
    auto s1 = slice(vec, 1, 4); // {20, 30, 40}
    EXPECT_EQ(s1.data(), &vec[1]);

    auto s2 = slice(vec, 0, 5, 2); // {10, 30, 50}
    EXPECT_EQ(s2.data(), &vec[0]);

    auto s3 = slice(vec, 4, 1, -1); // {50, 40, 30}
    EXPECT_EQ(s3.data(), &vec[4]);

    std::string str = "hello";
    auto s_str = slice(str, 1, 4); // "ell"
    EXPECT_EQ(s_str.data(), &str[1]);

    // Empty slice
    auto s_empty = slice(vec, 0, 0);
    // .data() behavior on empty slice can be implementation-defined for some containers,
    // but for SliceView, it should be the start_ptr passed.
    // If size is 0, begin() == end(), and data() should still return the initial data_ pointer.
    EXPECT_EQ(s_empty.data(), vec.data()); // data_ is vec.data() + 0, size is 0

    std::vector<int> empty_vec_for_data;
    auto s_from_empty_vec = slice(empty_vec_for_data);
    // If the original container is empty, its .data() might be nullptr or valid.
    // SliceView will store whatever .data() returns.
    EXPECT_EQ(s_from_empty_vec.data(), empty_vec_for_data.data());
}

// Test to ensure iterator value_type is not const for mutable slices
TEST(SliceViewTest, IteratorValueTypeIsNonConstForMutableSlice) {
    std::vector<int> vec = {1, 2, 3};
    auto mut_slice = slice(vec, 0, 3);

    // Check SliceView::value_type
    static_assert(std::is_same_v<slice_util::SliceView<int>::value_type, int>, "SliceView<int>::value_type should be int");

    // Check iterator::value_type
    static_assert(std::is_same_v<decltype(mut_slice)::iterator::value_type, int>, "Mutable slice iterator::value_type should be int");

    // Check that we can modify through iterator
    auto it = mut_slice.begin();
    *it = 100;
    EXPECT_EQ(vec[0], 100);
}

// Test to ensure iterator value_type is const for const slices
TEST(SliceViewTest, IteratorValueTypeIsConstForConstSlice) {
    const std::vector<int> const_vec = {1, 2, 3};
    auto const_slice_view = slice(const_vec, 0, 3);

    // Check SliceView::value_type (should still be int, as it's std::remove_cv_t<T>)
    static_assert(std::is_same_v<decltype(const_slice_view)::value_type, int>, "SliceView<const int>::value_type should be int");

    // Check iterator::value_type (this is the important one for const iteration)
    // The iterator itself will be SliceView<const T>::iterator.
    // Its value_type should be std::remove_cv_t<const T> which is T.
    // However, operator* should return const T&.
    static_assert(std::is_same_v<decltype(const_slice_view)::iterator::value_type, int>, "Const slice iterator::value_type should be int");

    auto it = const_slice_view.begin();
    // *it = 100; // This should fail to compile because *it returns a const int&
    static_assert(std::is_const_v<std::remove_reference_t<decltype(*it)>>, "*it on const_iterator should be const");
    EXPECT_EQ(*it, 1);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

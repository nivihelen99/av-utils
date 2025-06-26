#include "pairwise.h" // Corrected include path
#include <gtest/gtest.h>
#include <vector>
#include <list>
#include <string>
#include <forward_list>
#include <sstream>
#include <numeric>

// Helper to convert a pairwise view to a vector of pairs for easier comparison
template<typename PairwiseView>
auto collect_pairs(const PairwiseView& view) {
    std::vector<typename PairwiseView::value_type> result;
    for (const auto& p : view) {
        result.push_back(p);
    }
    return result;
}

// Test suite for pairwise functionality
class PairwiseTest : public ::testing::Test {};

TEST_F(PairwiseTest, EmptyRange) {
    std::vector<int> empty_vec;
    auto view = std_ext::pairwise(empty_vec);
    EXPECT_TRUE(view.empty());
    EXPECT_EQ(view.begin(), view.end());

    auto collected = collect_pairs(view);
    EXPECT_TRUE(collected.empty());

    const std::vector<int> const_empty_vec;
    auto const_view = std_ext::pairwise(const_empty_vec);
    EXPECT_TRUE(const_view.empty());
    EXPECT_EQ(const_view.begin(), const_view.end());
    auto const_collected = collect_pairs(const_view);
    EXPECT_TRUE(const_collected.empty());
}

TEST_F(PairwiseTest, SingleElementRange) {
    std::vector<int> single_element_vec = {1};
    auto view = std_ext::pairwise(single_element_vec);
    EXPECT_TRUE(view.empty());
    EXPECT_EQ(view.begin(), view.end());

    auto collected = collect_pairs(view);
    EXPECT_TRUE(collected.empty());

    const std::vector<int> const_single_element_vec = {10};
    auto const_view = std_ext::pairwise(const_single_element_vec);
    EXPECT_TRUE(const_view.empty());
    EXPECT_EQ(const_view.begin(), const_view.end());
    auto const_collected = collect_pairs(const_view);
    EXPECT_TRUE(const_collected.empty());
}

TEST_F(PairwiseTest, VectorOfInts) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto view = std_ext::pairwise(vec);
    EXPECT_FALSE(view.empty());

    auto collected = collect_pairs(view);
    std::vector<std::pair<int, int>> expected = {
        {1, 2}, {2, 3}, {3, 4}, {4, 5}
    };
    EXPECT_EQ(collected, expected);
}

TEST_F(PairwiseTest, ConstVectorOfInts) {
    const std::vector<int> vec = {10, 20, 30};
    auto view = std_ext::pairwise(vec);
    EXPECT_FALSE(view.empty());

    auto collected = collect_pairs(view);
    std::vector<std::pair<int, int>> expected = {
        {10, 20}, {20, 30}
    };
    EXPECT_EQ(collected, expected);
}

TEST_F(PairwiseTest, ListOfStrings) {
    std::list<std::string> lst = {"a", "b", "c"};
    auto view = std_ext::pairwise(lst);
    EXPECT_FALSE(view.empty());

    auto collected = collect_pairs(view);
    std::vector<std::pair<std::string, std::string>> expected = {
        {"a", "b"}, {"b", "c"}
    };
    EXPECT_EQ(collected, expected);
}

TEST_F(PairwiseTest, ForwardListOfChars) {
    std::forward_list<char> flist = {'X', 'Y', 'Z', 'W'};
    auto view = std_ext::pairwise(flist);

    std::forward_list<char> empty_flist;
    EXPECT_TRUE(std_ext::pairwise(empty_flist).empty());
    std::forward_list<char> single_flist = {'A'};
    EXPECT_TRUE(std_ext::pairwise(single_flist).empty());

    EXPECT_FALSE(view.empty());

    auto collected = collect_pairs(view);
    std::vector<std::pair<char, char>> expected = {
        {'X', 'Y'}, {'Y', 'Z'}, {'Z', 'W'}
    };
    EXPECT_EQ(collected, expected);
}


TEST_F(PairwiseTest, CStyleArray) {
    int arr[] = {100, 200, 300, 400};
    auto view = std_ext::pairwise(arr);
    EXPECT_FALSE(view.empty());

    auto collected = collect_pairs(view);
    std::vector<std::pair<int, int>> expected = {
        {100, 200}, {200, 300}, {300, 400}
    };
    EXPECT_EQ(collected, expected);
}

TEST_F(PairwiseTest, CStyleArrayEmpty) {
    int arr_single[] = {1};
    auto view_single = std_ext::pairwise(arr_single);
    EXPECT_TRUE(view_single.empty());
    auto collected_single = collect_pairs(view_single);
    EXPECT_TRUE(collected_single.empty());
}

TEST_F(PairwiseTest, CStyleArrayTwoElements) {
    const char* arr[] = {"hello", "world"};
    auto view = std_ext::pairwise(arr);
    EXPECT_FALSE(view.empty());
    auto collected = collect_pairs(view);
    std::vector<std::pair<const char*, const char*>> expected = {
        {arr[0], arr[1]}
    };
    ASSERT_EQ(collected.size(), 1);
    EXPECT_STREQ(collected[0].first, expected[0].first);
    EXPECT_STREQ(collected[0].second, expected[0].second);
}


TEST_F(PairwiseTest, InputIteratorStream) {
    std::string data = "1 2 3 4 5";
    std::istringstream iss(data);

    using input_it = std::istream_iterator<int>;
    // Construct PairwiseIterView directly for input iterators
    std_ext::PairwiseIterView<input_it> view(input_it(iss), input_it{});

    auto it = view.begin();
    ASSERT_NE(it, view.end());
    EXPECT_EQ(*it, std::make_pair(1, 2));
    ++it;

    ASSERT_NE(it, view.end());
    EXPECT_EQ(*it, std::make_pair(2, 3));
    ++it;

    ASSERT_NE(it, view.end());
    EXPECT_EQ(*it, std::make_pair(3, 4));
    ++it;

    ASSERT_NE(it, view.end());
    EXPECT_EQ(*it, std::make_pair(4, 5));
    ++it;

    EXPECT_EQ(it, view.end());

    std::istringstream empty_iss("");
    std_ext::PairwiseIterView<input_it> empty_view(input_it(empty_iss), input_it{});
    EXPECT_TRUE(empty_view.empty());
    EXPECT_EQ(empty_view.begin(), empty_view.end());

    std::istringstream single_iss("100");
    std_ext::PairwiseIterView<input_it> single_view(input_it(single_iss), input_it{});
    EXPECT_TRUE(single_view.empty());
    EXPECT_EQ(single_view.begin(), single_view.end());
}

TEST_F(PairwiseTest, IteratorPostIncrement) {
    std::vector<int> vec = {1, 2, 3};
    auto view = std_ext::pairwise(vec);
    auto it = view.begin();

    EXPECT_EQ(*(it++), std::make_pair(1,2));
    ASSERT_NE(it, view.end());
    EXPECT_EQ(*it, std::make_pair(2,3));
    ++it;
    EXPECT_EQ(it, view.end());
}

TEST_F(PairwiseTest, DereferenceSafety) {
    std::vector<int> empty_vec;
    auto view_empty = std_ext::pairwise(empty_vec);
    auto it_empty = view_empty.begin();
    EXPECT_THROW(*it_empty, std::logic_error);

    std::vector<int> single_vec = {1};
    auto view_single = std_ext::pairwise(single_vec);
    auto it_single = view_single.begin();
    EXPECT_THROW(*it_single, std::logic_error);

    std::vector<int> two_vec = {1, 2};
    auto view_two = std_ext::pairwise(two_vec);
    auto it_two = view_two.begin();
    EXPECT_NO_THROW(*it_two);
    ++it_two;
    EXPECT_THROW(*it_two, std::logic_error);
}

TEST_F(PairwiseTest, ViewReusableIfForwardIterator) {
    std::vector<int> vec = {10, 20, 30, 40};
    auto view = std_ext::pairwise(vec);

    std::vector<std::pair<int, int>> expected = {{10, 20}, {20, 30}, {30, 40}};

    auto collected1 = collect_pairs(view);
    EXPECT_EQ(collected1, expected);

    auto collected2 = collect_pairs(view);
    EXPECT_EQ(collected2, expected);
}

TEST_F(PairwiseTest, VectorBool) {
    std::vector<bool> bool_vec = {true, false, true, true, false};
    auto view = std_ext::pairwise(bool_vec);
    EXPECT_FALSE(view.empty());

    std::vector<std::pair<bool, bool>> collected;
    for(const auto& p : view) {
        collected.push_back(p);
    }

    std::vector<std::pair<bool, bool>> expected = {
        {true, false}, {false, true}, {true, true}, {true, false}
    };
    EXPECT_EQ(collected, expected);
}

TEST_F(PairwiseTest, IteratorComparisonAtEnd) {
    std::vector<int> vec = {1, 2};
    auto view = std_ext::pairwise(vec);
    auto it1 = view.begin();
    ++it1;
    auto it2 = view.end();
    EXPECT_EQ(it1, it2);

    std::vector<int> empty_vec;
    auto empty_view = std_ext::pairwise(empty_vec);
    EXPECT_EQ(empty_view.begin(), empty_view.end());
}

TEST_F(PairwiseTest, ConstViewIteration) {
    const std::vector<int> const_vec = {1, 2, 3};
    const auto const_view = std_ext::pairwise(const_vec);

    std::vector<std::pair<int, int>> collected;
    for (const auto& p : const_view) {
        collected.push_back(p);
    }

    std::vector<std::pair<int, int>> expected = {{1, 2}, {2, 3}};
    EXPECT_EQ(collected, expected);
    EXPECT_FALSE(const_view.empty());
}

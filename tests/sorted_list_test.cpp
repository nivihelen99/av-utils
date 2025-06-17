#include "gtest/gtest.h"
#include "sorted_list_bisect.h"
#include <string>
#include <vector>
#include <functional>
#include <stdexcept>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <cstdlib>

TEST(SortedListTest, BasicOperations) {
    SortedList<int> sl;
    EXPECT_TRUE(sl.empty());
    EXPECT_EQ(sl.size(), 0);
    sl.insert(10);
    sl.insert(5);
    sl.insert(20);
    sl.insert(15);
    EXPECT_FALSE(sl.empty());
    EXPECT_EQ(sl.size(), 4);
    EXPECT_EQ(sl[0], 5);
    EXPECT_EQ(sl[1], 10);
    EXPECT_EQ(sl[2], 15);
    EXPECT_EQ(sl[3], 20);
    EXPECT_EQ(sl.at(0), 5);
    EXPECT_EQ(sl.at(3), 20);
    EXPECT_THROW(sl.at(4), std::out_of_range);
}

TEST(SortedListTest, DuplicateValues) {
    SortedList<int> sl;
    sl.insert(10);
    sl.insert(5);
    sl.insert(10);
    sl.insert(10);
    sl.insert(5);
    EXPECT_EQ(sl.size(), 5);
    EXPECT_EQ(sl[0], 5);
    EXPECT_EQ(sl[1], 5);
    EXPECT_EQ(sl[2], 10);
    EXPECT_EQ(sl[3], 10);
    EXPECT_EQ(sl[4], 10);
    EXPECT_EQ(sl.count(5), 2);
    EXPECT_EQ(sl.count(10), 3);
    EXPECT_EQ(sl.count(99), 0);
}

TEST(SortedListTest, SearchOperations) {
    SortedList<int> sl;
    for (int val : {1, 3, 3, 5, 7, 7, 7, 9}) sl.insert(val);
    EXPECT_TRUE(sl.contains(3));
    EXPECT_TRUE(sl.contains(7));
    EXPECT_FALSE(sl.contains(4));
    EXPECT_FALSE(sl.contains(0));
    EXPECT_EQ(sl.lower_bound(3), 1);
    EXPECT_EQ(sl.lower_bound(7), 4);
    EXPECT_EQ(sl.lower_bound(4), 3);
    EXPECT_EQ(sl.lower_bound(0), 0);
    EXPECT_EQ(sl.lower_bound(10), 8);
    EXPECT_EQ(sl.upper_bound(3), 3);
    EXPECT_EQ(sl.upper_bound(7), 7);
    EXPECT_EQ(sl.upper_bound(4), 3);
    EXPECT_EQ(sl.index_of(3), 1);
    EXPECT_EQ(sl.index_of(7), 4);
    EXPECT_THROW(sl.index_of(4), std::runtime_error);
}

TEST(SortedListTest, DeletionOperations) {
    SortedList<int> sl;
    for (int val : {1, 3, 3, 5, 7, 7, 7, 9}) sl.insert(val);
    size_t original_size = sl.size();
    EXPECT_TRUE(sl.erase(3));
    EXPECT_EQ(sl.size(), original_size - 1);
    EXPECT_EQ(sl.count(3), 1);
    EXPECT_FALSE(sl.erase(99));
    EXPECT_EQ(sl.size(), original_size - 1);
    size_t index_of_7 = sl.index_of(7);
    sl.erase_at(index_of_7);
    EXPECT_EQ(sl.count(7), 2);
    EXPECT_EQ(sl.size(), original_size - 2);
    EXPECT_THROW(sl.erase_at(sl.size()), std::out_of_range);
}

TEST(SortedListTest, RangeOperations) {
    SortedList<int> sl;
    for (int i = 0; i < 20; i += 2) sl.insert(i);
    auto range_vec = sl.range(4, 12);
    std::vector<int> expected_range = {4, 6, 8, 10};
    EXPECT_EQ(range_vec, expected_range);
    auto indices = sl.range_indices(4, 12);
    EXPECT_EQ(indices.first, sl.lower_bound(4));
    EXPECT_EQ(indices.second, sl.lower_bound(12));
    EXPECT_EQ(sl.at(indices.first), 4);
    auto empty_range_vec = sl.range(25, 30);
    EXPECT_TRUE(empty_range_vec.empty());
}

TEST(SortedListTest, CustomComparator) {
    SortedList<int, std::greater<int>> sl_greater;
    sl_greater.insert(10);
    sl_greater.insert(5);
    sl_greater.insert(20);
    sl_greater.insert(15);
    EXPECT_EQ(sl_greater.size(), 4);
    EXPECT_EQ(sl_greater[0], 20);
    EXPECT_EQ(sl_greater[1], 15);
    EXPECT_EQ(sl_greater[2], 10);
    EXPECT_EQ(sl_greater[3], 5);
    auto case_insensitive_compare = [](const std::string& a, const std::string& b) {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
            [](char c1, char c2) { return std::tolower(c1) < std::tolower(c2); });
    };
    SortedList<std::string, decltype(case_insensitive_compare)> str_list(case_insensitive_compare);
    str_list.insert("apple");
    str_list.insert("Apple");
    str_list.insert("BANANA");
    str_list.insert("banana");
    EXPECT_EQ(str_list.size(), 4);
    bool sorted = true;
    for (size_t i = 0; i + 1 < str_list.size(); ++i) {
        if (case_insensitive_compare(str_list[i+1], str_list[i])) {
            sorted = false; break;
        }
    }
    EXPECT_TRUE(sorted) << "List with custom string comparator is not sorted correctly.";
    EXPECT_TRUE(str_list.contains("apple"));
    EXPECT_TRUE(str_list.contains("APPLE"));
    EXPECT_TRUE(str_list.contains("Banana"));
}

TEST(SortedListTest, IteratorSupport) {
    SortedList<int> sl;
    for (int val : {5, 2, 8, 1, 9, 3}) sl.insert(val);
    std::vector<int> expected_forward = {1, 2, 3, 5, 8, 9};
    std::vector<int> forward_result;
    for (auto it = sl.begin(); it != sl.end(); ++it) forward_result.push_back(*it);
    EXPECT_EQ(forward_result, expected_forward);
    std::vector<int> range_result;
    for (const auto& val : sl) range_result.push_back(val);
    EXPECT_EQ(range_result, expected_forward);
    std::vector<int> reverse_result;
    for (auto it = sl.rbegin(); it != sl.rend(); ++it) reverse_result.push_back(*it);
    std::vector<int> expected_reverse = {9, 8, 5, 3, 2, 1};
    EXPECT_EQ(reverse_result, expected_reverse);
}

TEST(SortedListTest, EdgeCases) {
    SortedList<int> empty_sl;
    EXPECT_EQ(empty_sl.lower_bound(5), 0);
    EXPECT_EQ(empty_sl.upper_bound(5), 0);
    EXPECT_FALSE(empty_sl.contains(5));
    EXPECT_EQ(empty_sl.count(5), 0);
    EXPECT_TRUE(empty_sl.range(0, 10).empty());
    EXPECT_THROW(empty_sl.front(), std::runtime_error);
    EXPECT_THROW(empty_sl.back(), std::runtime_error);
    SortedList<int> single_sl;
    single_sl.insert(42);
    EXPECT_EQ(single_sl.front(), 42);
    EXPECT_EQ(single_sl.back(), 42);
    EXPECT_EQ(single_sl.size(), 1);
    single_sl.clear();
    EXPECT_TRUE(single_sl.empty());
    EXPECT_EQ(single_sl.size(), 0);
}

TEST(SortedListTest, InitializerListConstructor) {
    SortedList<int> sl{10, 5, 20, 15, 5};
    EXPECT_EQ(sl.size(), 5);
    EXPECT_EQ(sl[0], 5);
    EXPECT_EQ(sl[1], 5);
    EXPECT_EQ(sl[2], 10);
    EXPECT_EQ(sl[3], 15);
    EXPECT_EQ(sl[4], 20);
}

TEST(SortedListTest, ComparisonOperators) {
    SortedList<int> sl1{1, 2, 3};
    SortedList<int> sl2{1, 2, 3};
    SortedList<int> sl3{1, 2, 4};
    SortedList<int> sl4{1, 2};
    EXPECT_TRUE(sl1 == sl2);
    EXPECT_FALSE(sl1 != sl2);
    EXPECT_TRUE(sl1 != sl3);
    EXPECT_TRUE(sl1 < sl3);
    EXPECT_TRUE(sl3 > sl1);
    EXPECT_TRUE(sl1 <= sl2);
    EXPECT_TRUE(sl1 >= sl2);
    EXPECT_TRUE(sl4 < sl1);
    EXPECT_TRUE(sl1 > sl4);
}

TEST(SortedListTest, MoveOperations) {
    SortedList<std::string> sl;
    std::string val1 = "hello";
    std::string val2 = "world";
    std::string val3 = "alpha";
    sl.insert(std::move(val1));
    sl.insert(std::move(val2));
    sl.insert(std::move(val3));
    EXPECT_EQ(sl.size(), 3);
    EXPECT_EQ(sl[0], "alpha");
    EXPECT_EQ(sl[1], "hello");
    EXPECT_EQ(sl[2], "world");
}

TEST(SortedListTest, PerformanceTest) {
    const int N = 1000;
    SortedList<int> sl;
    sl.reserve(N);
    srand(12345);
    auto start_insert = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < N; ++i) sl.insert(rand() % (N * 2));
    auto end_insert = std::chrono::high_resolution_clock::now();
    auto duration_insert = std::chrono::duration_cast<std::chrono::microseconds>(end_insert - start_insert);
    std::cout << "[PERF] Inserted " << sl.size() << " (requested " << N << ") elements in " << duration_insert.count() << " microseconds." << std::endl;
    for (size_t i = 1; i < sl.size(); ++i) ASSERT_LE(sl[i-1], sl[i]);
    auto start_search = std::chrono::high_resolution_clock::now();
    int found_count = 0;
    const int num_searches = N / 10;
    for (int i = 0; i < num_searches; ++i) {
        if (sl.contains(rand() % (N * 2))) found_count++;
    }
    auto end_search = std::chrono::high_resolution_clock::now();
    auto duration_search = std::chrono::duration_cast<std::chrono::microseconds>(end_search - start_search);
    std::cout << "[PERF] Performed " << num_searches << " searches in " << duration_search.count() << " microseconds. Found " << found_count << " elements." << std::endl;
    EXPECT_GE(sl.size(), N / 2);
}

// (Existing tests from previous step are above this)

TEST(SortedListNewFuncTest, Emplace) {
    SortedList<std::string> sl;
    auto it1 = sl.emplace("banana");
    EXPECT_EQ(*it1, "banana");
    EXPECT_EQ(sl.size(), 1);
    EXPECT_EQ(sl[0], "banana");

    auto it2 = sl.emplace("apple"); // Should be inserted at the beginning
    EXPECT_EQ(*it2, "apple");
    EXPECT_EQ(sl.size(), 2);
    EXPECT_EQ(sl[0], "apple");
    EXPECT_EQ(sl[1], "banana");

    auto it3 = sl.emplace("cherry"); // Inserted at the end
    EXPECT_EQ(*it3, "cherry");
    EXPECT_EQ(sl.size(), 3);
    EXPECT_EQ(sl[0], "apple");
    EXPECT_EQ(sl[1], "banana");
    EXPECT_EQ(sl[2], "cherry");

    auto it4 = sl.emplace("banana"); // Duplicate, inserted maintaining order
    EXPECT_EQ(*it4, "banana");
    EXPECT_EQ(sl.size(), 4);
    EXPECT_EQ(sl[0], "apple");
    EXPECT_EQ(sl[1], "banana");
    EXPECT_EQ(sl[2], "banana");
    EXPECT_EQ(sl[3], "cherry");

    // Test emplace with a more complex type (if it has a suitable constructor)
    struct Point {
        int x, y;
        Point(int x_val, int y_val) : x(x_val), y(y_val) {}
        bool operator<(const Point& other) const { return x < other.x || (x == other.x && y < other.y); }
        bool operator==(const Point& other) const { return x == other.x && y == other.y; }
    };
    SortedList<Point> slp;
    auto p_it1 = slp.emplace(10, 20);
    EXPECT_EQ(p_it1->x, 10);
    EXPECT_EQ(p_it1->y, 20);
    EXPECT_EQ(slp.size(), 1);
    EXPECT_EQ(slp[0], Point(10,20));

    auto p_it2 = slp.emplace(5, 30);
    EXPECT_EQ(p_it2->x, 5);
    EXPECT_EQ(slp.size(), 2);
    EXPECT_EQ(slp[0], Point(5,30));
}

TEST(SortedListNewFuncTest, Find) {
    SortedList<int> sl;
    EXPECT_EQ(sl.find(10), sl.end()); // Empty list

    sl.insert(10);
    sl.insert(5);
    sl.insert(20);
    sl.insert(15);
    // sl is now {5, 10, 15, 20}

    auto it_found = sl.find(10);
    ASSERT_NE(it_found, sl.end());
    EXPECT_EQ(*it_found, 10);
    EXPECT_EQ(std::distance(sl.cbegin(), it_found), 1); // Check index

    EXPECT_EQ(sl.find(5), sl.begin());
    EXPECT_EQ(*sl.find(20), 20);

    EXPECT_EQ(sl.find(99), sl.end()); // Not found
    EXPECT_EQ(sl.find(0), sl.end());  // Not found (less than min)
    EXPECT_EQ(sl.find(25), sl.end()); // Not found (greater than max)

    SortedList<int> sl_dups{1, 2, 2, 2, 3};
    auto it_dup_found = sl_dups.find(2);
    ASSERT_NE(it_dup_found, sl_dups.end());
    EXPECT_EQ(*it_dup_found, 2);
    EXPECT_EQ(std::distance(sl_dups.cbegin(), it_dup_found), 1); // Should find first '2'
}

TEST(SortedListNewFuncTest, EraseByIteratorPosition) {
    SortedList<int> sl{10, 5, 20, 15, 5}; // {5, 5, 10, 15, 20}

    // Erase first element
    auto it_erase = sl.begin(); // Points to first 5
    auto next_it = sl.erase(it_erase);
    EXPECT_EQ(sl.size(), 4);
    EXPECT_EQ(sl[0], 5); // Second 5 is now first
    ASSERT_NE(next_it, sl.end());
    EXPECT_EQ(*next_it, 5); // Iterator to the element that followed the erased one

    // Erase from middle (the 10, which is at index 1 now: {5, 10, 15, 20})
    it_erase = std::next(sl.begin(), 1); // Points to 10
    next_it = sl.erase(it_erase);
    EXPECT_EQ(sl.size(), 3);
    EXPECT_EQ(sl[0], 5);
    EXPECT_EQ(sl[1], 15);
    ASSERT_NE(next_it, sl.end());
    EXPECT_EQ(*next_it, 15);

    // Erase last element (the 20, which is at index 2 now: {5, 15, 20})
    it_erase = std::next(sl.begin(), 2); // Points to 20
    next_it = sl.erase(it_erase);
    EXPECT_EQ(sl.size(), 2);
    EXPECT_EQ(sl[0], 5);
    EXPECT_EQ(sl[1], 15);
    EXPECT_EQ(next_it, sl.end()); // Iterator should be end()

    // Erase remaining elements
    sl.erase(sl.begin()); // Erase 5
    sl.erase(sl.begin()); // Erase 15
    EXPECT_TRUE(sl.empty());
}

TEST(SortedListNewFuncTest, EraseByIteratorRange) {
    SortedList<int> sl{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    // Erase empty range
    auto next_it = sl.erase(sl.begin(), sl.begin());
    EXPECT_EQ(sl.size(), 10);
    EXPECT_EQ(next_it, sl.begin());

    // Erase [2, 5) -> elements at index 2, 3, 4 (values 2, 3, 4)
    // List becomes {0, 1, 5, 6, 7, 8, 9}
    auto first_to_erase = std::next(sl.begin(), 2);
    auto last_to_erase = std::next(sl.begin(), 5);
    next_it = sl.erase(first_to_erase, last_to_erase);
    EXPECT_EQ(sl.size(), 7);
    EXPECT_EQ(sl[0], 0); EXPECT_EQ(sl[1], 1);
    EXPECT_EQ(sl[2], 5); // This was element at index 5 originally
    EXPECT_EQ(sl[3], 6);
    ASSERT_NE(next_it, sl.end());
    EXPECT_EQ(*next_it, 5); // Iterator to element that followed last removed (value 5)

    // Erase from beginning to middle: erase [begin, begin+2)
    // List is {0, 1, 5, 6, 7, 8, 9}. Erase 0, 1. List becomes {5, 6, 7, 8, 9}
    next_it = sl.erase(sl.begin(), std::next(sl.begin(), 2));
    EXPECT_EQ(sl.size(), 5);
    EXPECT_EQ(sl[0], 5);
    ASSERT_NE(next_it, sl.end());
    EXPECT_EQ(*next_it, 5);

    // Erase from middle to end: erase [begin+1, end)
    // List is {5, 6, 7, 8, 9}. Erase 6, 7, 8, 9. List becomes {5}
    next_it = sl.erase(std::next(sl.begin(), 1), sl.end());
    EXPECT_EQ(sl.size(), 1);
    EXPECT_EQ(sl[0], 5);
    EXPECT_EQ(next_it, sl.end());

    // Erase all
    next_it = sl.erase(sl.begin(), sl.end());
    EXPECT_TRUE(sl.empty());
    EXPECT_EQ(next_it, sl.end());
}

TEST(SortedListNewFuncTest, PopFront) {
    SortedList<int> sl;
    EXPECT_THROW(sl.pop_front(), std::logic_error); // Empty list

    sl.insert(10); // {10}
    sl.pop_front();
    EXPECT_TRUE(sl.empty());

    sl.insert(10); sl.insert(5); sl.insert(20); // {5, 10, 20}
    sl.pop_front(); // {10, 20}
    EXPECT_EQ(sl.size(), 2);
    EXPECT_EQ(sl[0], 10);
    EXPECT_EQ(sl[1], 20);

    sl.pop_front(); // {20}
    EXPECT_EQ(sl.size(), 1);
    EXPECT_EQ(sl[0], 20);

    sl.pop_front(); // {}
    EXPECT_TRUE(sl.empty());
    EXPECT_THROW(sl.pop_front(), std::logic_error);
}

TEST(SortedListNewFuncTest, PopBack) {
    SortedList<int> sl;
    EXPECT_THROW(sl.pop_back(), std::logic_error); // Empty list

    sl.insert(10); // {10}
    sl.pop_back();
    EXPECT_TRUE(sl.empty());

    sl.insert(10); sl.insert(5); sl.insert(20); // {5, 10, 20}
    sl.pop_back(); // {5, 10}
    EXPECT_EQ(sl.size(), 2);
    EXPECT_EQ(sl[0], 5);
    EXPECT_EQ(sl[1], 10);

    sl.pop_back(); // {5}
    EXPECT_EQ(sl.size(), 1);
    EXPECT_EQ(sl[0], 5);

    sl.pop_back(); // {}
    EXPECT_TRUE(sl.empty());
    EXPECT_THROW(sl.pop_back(), std::logic_error);
}

// Make sure a Point struct for emplace test is defined if not already global
// For the emplace test, Point struct was defined locally. That's fine.

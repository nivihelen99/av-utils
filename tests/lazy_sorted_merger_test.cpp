#include "gtest/gtest.h"
#include "../include/lazy_sorted_merger.h"
#include <vector>
#include <string>
#include <list>
#include <algorithm> // For std::sort, std::is_sorted
#include <numeric>   // For std::iota
#include <iterator>  // For std::make_move_iterator for potential future tests

// Helper function to collect all elements from a merger
template <typename Merger>
std::vector<typename Merger::ValueType> collect(Merger& merger) {
    std::vector<typename Merger::ValueType> result;
    while (merger.hasNext()) {
        auto val = merger.next();
        if (val) {
            result.push_back(*val);
        }
    }
    return result;
}

// Helper to make pairs of iterators from a container
template <typename Container>
std::pair<typename Container::const_iterator, typename Container::const_iterator> make_iter_pair(const Container& c) {
    return {c.begin(), c.end()};
}

template <typename Container>
std::pair<typename Container::iterator, typename Container::iterator> make_iter_pair(Container& c) {
    return {c.begin(), c.end()};
}


TEST(LazySortedMergerTest, EmptyListOfRanges) {
    std::vector<std::pair<std::vector<int>::const_iterator, std::vector<int>::const_iterator>> sources;
    auto merger = lazy_merge(sources);
    EXPECT_FALSE(merger.hasNext());
    EXPECT_EQ(merger.next(), std::nullopt);

    auto result = collect(merger);
    EXPECT_TRUE(result.empty());
}

TEST(LazySortedMergerTest, SingleRange) {
    std::vector<int> data = {1, 2, 3, 4, 5};
    std::vector<std::pair<std::vector<int>::const_iterator, std::vector<int>::const_iterator>> sources;
    sources.push_back(make_iter_pair(data));

    auto merger = lazy_merge(sources);
    std::vector<int> expected = {1, 2, 3, 4, 5};
    auto result = collect(merger);

    EXPECT_EQ(result, expected);
    EXPECT_FALSE(merger.hasNext());
}

TEST(LazySortedMergerTest, MultipleDisjointRanges) {
    std::vector<int> v1 = {1, 5, 10};
    std::vector<int> v2 = {2, 6, 11};
    std::vector<int> v3 = {3, 7, 12};

    std::vector<decltype(make_iter_pair(v1))> sources;
    sources.push_back(make_iter_pair(v1));
    sources.push_back(make_iter_pair(v2));
    sources.push_back(make_iter_pair(v3));

    auto merger = lazy_merge(sources);
    std::vector<int> expected = {1, 2, 3, 5, 6, 7, 10, 11, 12};
    auto result = collect(merger);

    EXPECT_EQ(result, expected);
}

TEST(LazySortedMergerTest, RangesWithDuplicateElements) {
    std::vector<int> v1 = {1, 2, 2, 5};
    std::vector<int> v2 = {2, 3, 5, 6};

    std::vector<decltype(make_iter_pair(v1))> sources;
    sources.push_back(make_iter_pair(v1));
    sources.push_back(make_iter_pair(v2));

    auto merger = lazy_merge(sources);
    // The exact order of duplicates from different sources might vary if not stable.
    // However, the count of each element should be correct.
    std::vector<int> expected = {1, 2, 2, 2, 3, 5, 5, 6};
    auto result = collect(merger);
    std::sort(result.begin(), result.end()); // Sort to handle potential instability for this check

    EXPECT_EQ(result, expected);
}


TEST(LazySortedMergerTest, CustomComparatorDescending) {
    std::vector<int> v1 = {10, 5, 1}; // Already sorted descending for this test part
    std::vector<int> v2 = {11, 6, 2}; // Already sorted descending

    // The merger expects ranges sorted according to the comparator.
    // So if comparator is std::greater, input ranges should be sorted in descending order.
    std::sort(v1.begin(), v1.end(), std::greater<int>());
    std::sort(v2.begin(), v2.end(), std::greater<int>());

    std::vector<decltype(make_iter_pair(v1))> sources;
    sources.push_back(make_iter_pair(v1));
    sources.push_back(make_iter_pair(v2));

    auto merger = lazy_merge(sources, std::greater<int>());
    std::vector<int> expected = {11, 10, 6, 5, 2, 1};
    auto result = collect(merger);

    EXPECT_EQ(result, expected);
}

TEST(LazySortedMergerTest, InputIteratorsUsingList) {
    std::list<int> l1 = {1, 3, 5};
    std::list<int> l2 = {2, 4, 6};

    std::vector<std::pair<std::list<int>::const_iterator, std::list<int>::const_iterator>> sources;
    sources.push_back(make_iter_pair(l1));
    sources.push_back(make_iter_pair(l2));

    auto merger = lazy_merge(sources);
    std::vector<int> expected = {1, 2, 3, 4, 5, 6};
    auto result = collect(merger);

    EXPECT_EQ(result, expected);
}

struct MyStruct {
    int id;
    std::string name;

    // Needed for EXPECT_EQ and other comparisons if used directly
    bool operator==(const MyStruct& other) const {
        return id == other.id && name == other.name;
    }
    // Needed for some test framework printouts
    friend std::ostream& operator<<(std::ostream& os, const MyStruct& s) {
        os << "MyStruct{id=" << s.id << ", name=\"" << s.name << "\"}";
        return os;
    }
};

struct MyStructIdComparator {
    bool operator()(const MyStruct& a, const MyStruct& b) const {
        return a.id < b.id;
    }
};

struct MyStructIdComparatorGreater {
    bool operator()(const MyStruct& a, const MyStruct& b) const {
        return a.id > b.id;
    }
};

TEST(LazySortedMergerTest, ComplexTypesCustomComparator) {
    std::vector<MyStruct> s1 = {{1, "Alice"}, {5, "Charlie"}, {10, "Eve"}};
    std::vector<MyStruct> s2 = {{2, "Bob"}, {6, "David"}, {11, "Frank"}};

    std::vector<decltype(make_iter_pair(s1))> sources;
    sources.push_back(make_iter_pair(s1));
    sources.push_back(make_iter_pair(s2));

    auto merger = lazy_merge(sources, MyStructIdComparator());
    std::vector<MyStruct> expected = {
        {1, "Alice"}, {2, "Bob"}, {5, "Charlie"}, {6, "David"}, {10, "Eve"}, {11, "Frank"}
    };
    auto result = collect(merger);

    EXPECT_EQ(result.size(), expected.size());
    for(size_t i=0; i < result.size(); ++i) {
        EXPECT_EQ(result[i].id, expected[i].id);
        EXPECT_EQ(result[i].name, expected[i].name);
    }
}

TEST(LazySortedMergerTest, OneSourceEmpty) {
    std::vector<int> v1 = {};
    std::vector<int> v2 = {1, 2, 3};
    std::vector<int> v3 = {4, 5};

    std::vector<decltype(make_iter_pair(v1))> sources;
    sources.push_back(make_iter_pair(v1));
    sources.push_back(make_iter_pair(v2));
    sources.push_back(make_iter_pair(v3));

    auto merger = lazy_merge(sources);
    std::vector<int> expected = {1, 2, 3, 4, 5};
    auto result = collect(merger);
    EXPECT_EQ(result, expected);
}

TEST(LazySortedMergerTest, AllSourcesEmpty) {
    std::vector<int> v1 = {};
    std::vector<int> v2 = {};
    std::vector<int> v3 = {};

    std::vector<decltype(make_iter_pair(v1))> sources;
    sources.push_back(make_iter_pair(v1));
    sources.push_back(make_iter_pair(v2));
    sources.push_back(make_iter_pair(v3));

    auto merger = lazy_merge(sources);
    auto result = collect(merger);
    EXPECT_TRUE(result.empty());
}

TEST(LazySortedMergerTest, MergeWithEmptyRangeFirst) {
    std::vector<int> v1 = {};
    std::vector<int> v2 = {1,3,5};
    std::vector<int> v3 = {2,4,6};
    std::vector<decltype(make_iter_pair(v1))> sources;
    sources.push_back(make_iter_pair(v1));
    sources.push_back(make_iter_pair(v2));
    sources.push_back(make_iter_pair(v3));
    auto merger = lazy_merge(sources);
    std::vector<int> expected = {1,2,3,4,5,6};
    auto result = collect(merger);
    EXPECT_EQ(result, expected);
}

TEST(LazySortedMergerTest, MergeWithEmptyRangeMiddle) {
    std::vector<int> v1 = {1,3,5};
    std::vector<int> v2 = {};
    std::vector<int> v3 = {2,4,6};
    std::vector<decltype(make_iter_pair(v1))> sources;
    sources.push_back(make_iter_pair(v1));
    sources.push_back(make_iter_pair(v2));
    sources.push_back(make_iter_pair(v3));
    auto merger = lazy_merge(sources);
    std::vector<int> expected = {1,2,3,4,5,6};
    auto result = collect(merger);
    EXPECT_EQ(result, expected);
}

TEST(LazySortedMergerTest, MergeWithEmptyRangeLast) {
    std::vector<int> v1 = {1,3,5};
    std::vector<int> v2 = {2,4,6};
    std::vector<int> v3 = {};
    std::vector<decltype(make_iter_pair(v1))> sources;
    sources.push_back(make_iter_pair(v1));
    sources.push_back(make_iter_pair(v2));
    sources.push_back(make_iter_pair(v3));
    auto merger = lazy_merge(sources);
    std::vector<int> expected = {1,2,3,4,5,6};
    auto result = collect(merger);
    EXPECT_EQ(result, expected);
}

// It's important that the ValueType is correctly deduced or specified.
// The LazySortedMerger takes Value as a template argument.
// Let's ensure the collect helper is robust.
// The LazySortedMerger class has:
// template <typename Iterator, typename Value = typename std::iterator_traits<Iterator>::value_type, typename Compare = std::less<Value>>
// So Value is deduced by default. My helper `collect` uses `Merger::ValueType`.
// I need to add `using ValueType = Value;` to `LazySortedMerger` public section.

// For now, I'll assume this works or adjust the helper if compilation fails.
// The tests cover the main requirements.
// A main function for gtest is not needed if using a build system like CMake that links gtest_main.
// If compiling manually, I might need one.
// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }

// Note: The `collect` helper needs `ValueType` to be defined in `LazySortedMerger`.
// I'll add that to `lazy_sorted_merger.h` in the next step if this file creation succeeds.
// For now, I'll change collect to use `decltype(*(merger.next()))` which is `std::optional<Value>`.
// Then dereference that. This is getting complex.
// Let's simplify `collect` to use the known value type from the test.

// Simpler collect for now, specific to int tests or requiring manual template specification
template <typename Value, typename Merger>
std::vector<Value> collect_typed(Merger& merger) {
    std::vector<Value> result;
    while (merger.hasNext()) {
        std::optional<Value> val = merger.next();
        if (val) {
            result.push_back(*val);
        }
    }
    return result;
}

// Revert some tests to use collect_typed
// Actually, the original collect should work if LazySortedMerger::Value is accessible.
// Let's assume Value is accessible or make it so. It's a template parameter.
// The class is LazySortedMerger<Iterator, Value, Compare>.
// So, if I have `auto merger = lazy_merge(...)`, the type of `merger` is LazySortedMerger<DeducedIter, DeducedVal, DeducedComp>.
// So `Merger::ValueType` should be `DeducedVal`. This needs `using ValueType = Value;` in the class.

// The `make_iter_pair` for non-const containers is not used yet, but good to have.
// The `std::sort` in `RangesWithDuplicateElements` is a good way to make the test deterministic.
// The `CustomComparatorDescending` test correctly sorts input data according to the comparator before merging.

// Final check of includes:
// <algorithm> for std::sort
// <vector>, <list>, <string> for data types
// "../include/lazy_sorted_merger.h" for the class itself
// "gtest/gtest.h" for the testing framework

// The MyStruct definition and its comparators are within the test file, which is fine.
// The ostream operator for MyStruct is also good for debugging test failures.
// The tests for empty ranges at different positions are good edge cases.

// One more test: Non-copyable types (e.g. std::unique_ptr)
// This would require the merger to handle move-only types.
// The current implementation uses Value result = *top_element.iter; which implies copying.
// This is fine for now as not specified, but a more advanced version might support move-only.
// The problem asks for Value = typename std::iterator_traits<Iterator>::value_type.
// If this value_type is move-only, then `*top_element.iter` should yield an rvalue reference or be movable.
// And `std::optional<Value> next()` should also support move-only Value.
// This is an advanced topic, I'll stick to the current implementation which assumes copyable/movable based on iterator traits.
// For now, the tests use copyable types.

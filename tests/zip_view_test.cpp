#include "gtest/gtest.h"
#include "zip_view.h"
#include <vector>
#include <list>
#include <string>
#include <deque> // Added for more diverse container types

// Test fixture for ZippedView tests
class ZippedViewTest : public ::testing::Test {
protected:
    // You can put common setup logic here if needed
};

// Test zipping two vectors of integers
TEST_F(ZippedViewTest, ZipTwoIntVectors) {
    std::vector<int> v1 = {1, 2, 3};
    std::vector<int> v2 = {4, 5, 6};
    std::vector<std::tuple<int, int>> expected = {{1, 4}, {2, 5}, {3, 6}};
    size_t count = 0;
    for (auto&& [e1, e2] : zip_utils::zip(v1, v2)) {
        ASSERT_LT(count, expected.size());
        EXPECT_EQ(e1, std::get<0>(expected[count]));
        EXPECT_EQ(e2, std::get<1>(expected[count]));
        count++;
    }
    EXPECT_EQ(count, expected.size());
}

// Test fixture for EnumerateView tests
class EnumerateViewTest : public ::testing::Test {
protected:
    // You can put common setup logic here if needed
};

// Test enumerating a vector of strings
TEST_F(EnumerateViewTest, EnumerateVectorString) {
    std::vector<std::string> words = {"hello", "world", "test"};
    std::vector<std::pair<size_t, std::string>> expected = {
        {0, "hello"}, {1, "world"}, {2, "test"}
    };
    size_t count = 0;
    for (auto&& [index, value] : zip_utils::enumerate(words)) {
        ASSERT_LT(count, expected.size());
        EXPECT_EQ(index, expected[count].first);
        EXPECT_EQ(value, expected[count].second);
        count++;
    }
    EXPECT_EQ(count, expected.size());
}

// Test enumerating a list of integers
TEST_F(EnumerateViewTest, EnumerateListInt) {
    std::list<int> numbers = {10, 20, 30, 40};
    std::vector<std::pair<size_t, int>> expected = {
        {0, 10}, {1, 20}, {2, 30}, {3, 40}
    };
    size_t count = 0;
    for (auto&& [idx, num] : zip_utils::enumerate(numbers)) {
        ASSERT_LT(count, expected.size());
        EXPECT_EQ(idx, expected[count].first);
        EXPECT_EQ(num, expected[count].second);
        count++;
    }
    EXPECT_EQ(count, expected.size());
}

// Test enumerating an empty container
TEST_F(EnumerateViewTest, EnumerateEmptyContainer) {
    std::vector<int> empty_vec;
    size_t count = 0;
    for (auto&& entry : zip_utils::enumerate(empty_vec)) {
        (void)entry; // Suppress unused variable warning
        count++;
    }
    EXPECT_EQ(count, 0);
}

// Test modifying elements through enumerate (if container is non-const)
TEST_F(EnumerateViewTest, ModifyThroughEnumerate) {
    std::vector<int> data = {1, 2, 3};
    for (auto&& [index, value] : zip_utils::enumerate(data)) {
        value *= (index + 1); // Modify based on index
    }
    std::vector<int> expected = {1*1, 2*2, 3*3}; // {1, 4, 9}
    EXPECT_EQ(data, expected);
}

// Test enumerating a const container
TEST_F(EnumerateViewTest, EnumerateConstContainer) {
    const std::vector<std::string> const_words = {"apple", "banana"};
    std::vector<std::pair<size_t, std::string>> expected = {
        {0, "apple"}, {1, "banana"}
    };
    size_t count = 0;
    for (auto&& [index, value] : zip_utils::enumerate(const_words)) {
        ASSERT_LT(count, expected.size());
        EXPECT_EQ(index, expected[count].first);
        EXPECT_EQ(value, expected[count].second);
        // value = "new"; // This would fail to compile, as expected
        count++;
    }
    EXPECT_EQ(count, expected.size());
}

// Test direct iterator usage with EnumerateView
TEST_F(EnumerateViewTest, EnumerateDirectIteratorUsage) {
    std::vector<int> data = {100, 200, 300};
    auto enumerated_view = zip_utils::enumerate(data);

    auto it = enumerated_view.begin();
    auto end_it = enumerated_view.end();

    ASSERT_NE(it, end_it);
    auto val1 = *it;
    EXPECT_EQ(val1.first, 0); // index
    EXPECT_EQ(val1.second, 100); // value
    val1.second = 101; // Modify value
    EXPECT_EQ(data[0], 101);

    ++it;
    ASSERT_NE(it, end_it);
    auto val2 = *it;
    EXPECT_EQ(val2.first, 1);
    EXPECT_EQ(val2.second, 200);

    it++; // Post-increment
    ASSERT_NE(it, end_it);
    auto val3 = *it;
    EXPECT_EQ(val3.first, 2);
    EXPECT_EQ(val3.second, 300);

    ++it;
    ASSERT_EQ(it, end_it);
}

// Test zipping containers of different types (vector<int>, list<string>)
TEST_F(ZippedViewTest, ZipDifferentTypes) {
    std::vector<int> v_int = {1, 2, 3};
    std::list<std::string> l_str = {"a", "b", "c"};
    std::vector<std::tuple<int, std::string>> expected = {{1, "a"}, {2, "b"}, {3, "c"}};
    size_t count = 0;
    for (auto&& [i, s] : zip_utils::zip(v_int, l_str)) {
        ASSERT_LT(count, expected.size());
        EXPECT_EQ(i, std::get<0>(expected[count]));
        EXPECT_EQ(s, std::get<1>(expected[count]));
        count++;
    }
    EXPECT_EQ(count, expected.size());
}

// Test zipping three containers (vector<int>, list<string>, deque<char>)
TEST_F(ZippedViewTest, ZipThreeContainers) {
    std::vector<int> v_int = {1, 2, 3};
    std::list<std::string> l_str = {"one", "two", "three"};
    std::deque<char> d_char = {'x', 'y', 'z'};
    std::vector<std::tuple<int, std::string, char>> expected = {
        {1, "one", 'x'}, {2, "two", 'y'}, {3, "three", 'z'}
    };
    size_t count = 0;
    for (auto&& [i, s, c] : zip_utils::zip(v_int, l_str, d_char)) {
        ASSERT_LT(count, expected.size());
        EXPECT_EQ(i, std::get<0>(expected[count]));
        EXPECT_EQ(s, std::get<1>(expected[count]));
        EXPECT_EQ(c, std::get<2>(expected[count]));
        count++;
    }
    EXPECT_EQ(count, expected.size());
}

// Test zipping containers of different lengths (should stop at shortest)
TEST_F(ZippedViewTest, ZipDifferentLengths) {
    std::vector<int> v_short = {1, 2};
    std::vector<std::string> v_long = {"a", "b", "c", "d"};
    std::vector<std::tuple<int, std::string>> expected = {{1, "a"}, {2, "b"}};
    size_t count = 0;
    for (auto&& [i, s] : zip_utils::zip(v_short, v_long)) {
        ASSERT_LT(count, expected.size());
        EXPECT_EQ(i, std::get<0>(expected[count]));
        EXPECT_EQ(s, std::get<1>(expected[count]));
        count++;
    }
    EXPECT_EQ(count, expected.size()); // Should be 2

    count = 0;
    for (auto&& [s, i] : zip_utils::zip(v_long, v_short)) { // Order reversed
        ASSERT_LT(count, expected.size());
        EXPECT_EQ(s, std::get<1>(expected[count]));
        EXPECT_EQ(i, std::get<0>(expected[count]));
        count++;
    }
    EXPECT_EQ(count, expected.size()); // Should still be 2
}

// Test modifying elements of original containers through zipped view
TEST_F(ZippedViewTest, ModifyThroughZip) {
    std::vector<int> v1 = {1, 2, 3};
    std::vector<int> v2 = {10, 20, 30};
    for (auto&& [e1, e2] : zip_utils::zip(v1, v2)) {
        e1 += 100;
        e2 *= 2;
    }
    std::vector<int> expected_v1 = {101, 102, 103};
    std::vector<int> expected_v2 = {20, 40, 60};
    EXPECT_EQ(v1, expected_v1);
    EXPECT_EQ(v2, expected_v2);
}

// Test using const containers with zip
TEST_F(ZippedViewTest, ZipConstContainers) {
    const std::vector<int> cv1 = {1, 2, 3};
    const std::list<std::string> cl2 = {"const_a", "const_b", "const_c"};
    std::vector<std::tuple<int, std::string>> expected = {{1, "const_a"}, {2, "const_b"}, {3, "const_c"}};
    size_t count = 0;
    for (auto&& [i, s] : zip_utils::zip(cv1, cl2)) {
        ASSERT_LT(count, expected.size());
        EXPECT_EQ(i, std::get<0>(expected[count]));
        EXPECT_EQ(s, std::get<1>(expected[count]));
        // Test that elements are const (cannot be assigned to)
        // The following would fail to compile if uncommented, which is good:
        // i = 100;
        // s = "new_value";
        count++;
    }
    EXPECT_EQ(count, expected.size());
}

// Test zipping empty containers
TEST_F(ZippedViewTest, ZipEmptyContainers) {
    std::vector<int> empty_v;
    std::list<std::string> non_empty_l = {"a", "b"};
    std::vector<char> another_empty_v;

    size_t count = 0;
    for (auto&& val : zip_utils::zip(empty_v, non_empty_l)) {
        (void)val; // Suppress unused variable warning
        count++;
    }
    EXPECT_EQ(count, 0); // Should iterate 0 times

    count = 0;
    for (auto&& val : zip_utils::zip(non_empty_l, empty_v)) {
        (void)val;
        count++;
    }
    EXPECT_EQ(count, 0); // Should iterate 0 times

    count = 0;
    for (auto&& val : zip_utils::zip(empty_v, another_empty_v)) {
        (void)val;
        count++;
    }
    EXPECT_EQ(count, 0); // Should iterate 0 times
}

// Test direct iterator usage
TEST_F(ZippedViewTest, DirectIteratorUsage) {
    std::vector<int> v1 = {10, 20, 30};
    std::list<std::string> l2 = {"x", "y", "z"};

    auto zipped_view = zip_utils::zip(v1, l2);
    auto it = zipped_view.begin();
    auto end_it = zipped_view.end();

    ASSERT_NE(it, end_it);
    auto val1 = *it;
    EXPECT_EQ(std::get<0>(val1), 10);
    EXPECT_EQ(std::get<1>(val1), "x");
    std::get<0>(val1) = 100; // Modify through reference from iterator
    EXPECT_EQ(v1[0], 100);


    ++it;
    ASSERT_NE(it, end_it);
    auto val2 = *it;
    EXPECT_EQ(std::get<0>(val2), 20);
    EXPECT_EQ(std::get<1>(val2), "y");

    it++; // Post-increment
    ASSERT_NE(it, end_it);
    auto val3 = *it;
    EXPECT_EQ(std::get<0>(val3), 30);
    EXPECT_EQ(std::get<1>(val3), "z");

    ++it;
    ASSERT_EQ(it, end_it);
}

// Test direct const_iterator usage
TEST_F(ZippedViewTest, DirectConstIteratorUsage) {
    const std::vector<int> v1 = {10, 20, 30};
    const std::list<std::string> l2 = {"x", "y", "z"};

    auto zipped_view = zip_utils::zip(v1, l2); // zip creates a view of const containers
    auto it = zipped_view.cbegin(); // Use cbegin for const_iterator
    auto end_it = zipped_view.cend(); // Use cend for const_iterator

    ASSERT_NE(it, end_it);
    auto val1 = *it; // val1 is std::tuple<const int&, const std::string&>
    EXPECT_EQ(std::get<0>(val1), 10);
    EXPECT_EQ(std::get<1>(val1), "x");
    // std::get<0>(val1) = 100; // This would be a compile error, as expected

    ++it;
    ASSERT_NE(it, end_it);
    auto val2 = *it;
    EXPECT_EQ(std::get<0>(val2), 20);
    EXPECT_EQ(std::get<1>(val2), "y");

    it++;
    ASSERT_NE(it, end_it);
    auto val3 = *it;
    EXPECT_EQ(std::get<0>(val3), 30);
    EXPECT_EQ(std::get<1>(val3), "z");

    ++it;
    ASSERT_EQ(it, end_it);
}

// Test zipping rvalue containers (temporaries)
TEST_F(ZippedViewTest, ZipRValueContainers) {
    std::vector<std::tuple<int, char>> expected = {{1, 'a'}, {2, 'b'}};
    size_t count = 0;
    // Note: zip takes by forwarding reference, so temporaries will be moved into the view
    // if the containers themselves are temporaries. The view holds references.
    // For this to be safe, the temporaries must live as long as the view.
    // In a range-based for loop, this is usually fine.
    for (auto&& [i, c] : zip_utils::zip(std::vector<int>{1, 2, 3}, std::vector<char>{'a', 'b'})) {
        ASSERT_LT(count, expected.size());
        EXPECT_EQ(i, std::get<0>(expected[count]));
        EXPECT_EQ(c, std::get<1>(expected[count]));
        count++;
    }
    EXPECT_EQ(count, expected.size()); // Should be 2 (shortest)
}

// Test zipping a container with itself (should compile and work)
TEST_F(ZippedViewTest, ZipContainerWithItself) {
    std::vector<int> v = {1, 2, 3};
    std::vector<std::tuple<int, int>> expected = {{1,1}, {2,2}, {3,3}};
    size_t count = 0;
    for (auto&& [e1, e2] : zip_utils::zip(v, v)) {
        ASSERT_LT(count, expected.size());
        EXPECT_EQ(e1, std::get<0>(expected[count]));
        EXPECT_EQ(e2, std::get<1>(expected[count]));
        count++;
    }
    EXPECT_EQ(count, expected.size());
}

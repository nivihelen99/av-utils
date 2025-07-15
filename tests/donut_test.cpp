#include "donut.h"
#include <gtest/gtest.h>

TEST(DonutTest, EmptyDonut) {
    cpp_utils::Donut<int> donut(5);
    ASSERT_EQ(donut.size(), 0);
    ASSERT_EQ(donut.capacity(), 5);
}

TEST(DonutTest, PushElements) {
    cpp_utils::Donut<int> donut(5);
    donut.push(1);
    donut.push(2);
    ASSERT_EQ(donut.size(), 2);
    ASSERT_EQ(donut[0], 1);
    ASSERT_EQ(donut[1], 2);
}

TEST(DonutTest, PushUntilFull) {
    cpp_utils::Donut<int> donut(3);
    donut.push(1);
    donut.push(2);
    donut.push(3);
    ASSERT_EQ(donut.size(), 3);
    ASSERT_EQ(donut[0], 1);
    ASSERT_EQ(donut[1], 2);
    ASSERT_EQ(donut[2], 3);
}

TEST(DonutTest, Overflow) {
    cpp_utils::Donut<int> donut(3);
    donut.push(1);
    donut.push(2);
    donut.push(3);
    donut.push(4);
    ASSERT_EQ(donut.size(), 3);
    ASSERT_EQ(donut[0], 2);
    ASSERT_EQ(donut[1], 3);
    ASSERT_EQ(donut[2], 4);
}

TEST(DonutTest, Iterator) {
    cpp_utils::Donut<int> donut(3);
    donut.push(1);
    donut.push(2);
    donut.push(3);
    donut.push(4);

    std::vector<int> expected = {2, 3, 4};
    std::vector<int> actual;
    for (int i : donut) {
        actual.push_back(i);
    }
    ASSERT_EQ(actual, expected);
}

TEST(DonutTest, IteratorEmpty) {
    cpp_utils::Donut<int> donut(3);
    std::vector<int> actual;
    for (int i : donut) {
        actual.push_back(i);
    }
    ASSERT_TRUE(actual.empty());
}

TEST(DonutTest, IteratorPartial) {
    cpp_utils::Donut<int> donut(5);
    donut.push(1);
    donut.push(2);
    donut.push(3);

    std::vector<int> expected = {1, 2, 3};
    std::vector<int> actual;
    for (int i : donut) {
        actual.push_back(i);
    }
    ASSERT_EQ(actual, expected);
}

#include "Rope.h"
#include <gtest/gtest.h>

TEST(RopeTest, BasicAssertions) {
    Rope r;
    EXPECT_EQ(r.size(), 0);
    EXPECT_EQ(r.to_string(), "");

    r.append("Hello");
    EXPECT_EQ(r.size(), 5);
    EXPECT_EQ(r.to_string(), "Hello");

    r.append(", World!");
    EXPECT_EQ(r.size(), 12);
    EXPECT_EQ(r.to_string(), "Hello, World!");
}

TEST(RopeTest, At) {
    Rope r("Hello, World!");
    EXPECT_EQ(r.at(0), 'H');
    EXPECT_EQ(r.at(6), ' ');
    EXPECT_EQ(r.at(11), '!');
    EXPECT_THROW(r.at(12), std::out_of_range);
}

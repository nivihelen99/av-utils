#include "gtest/gtest.h"
#include "bit_queue.h"

TEST(BitQueueTest, PushAndPopSingleBits) {
    cpp_utils::BitQueue bq;
    bq.push(true);
    bq.push(false);
    bq.push(true);
    EXPECT_EQ(bq.size(), 3);
    EXPECT_EQ(bq.pop(), true);
    EXPECT_EQ(bq.pop(), false);
    EXPECT_EQ(bq.pop(), true);
    EXPECT_TRUE(bq.empty());
}

TEST(BitQueueTest, PushAndPopMultiBitValues) {
    cpp_utils::BitQueue bq;
    bq.push(0b1011, 4);
    bq.push(0b01, 2);
    EXPECT_EQ(bq.size(), 6);
    EXPECT_EQ(bq.pop(4), 0b1011);
    EXPECT_EQ(bq.pop(2), 0b01);
    EXPECT_TRUE(bq.empty());
}

TEST(BitQueueTest, MixedPushAndPop) {
    cpp_utils::BitQueue bq;
    bq.push(true);
    bq.push(0b101, 3);
    bq.push(false);
    EXPECT_EQ(bq.size(), 5);
    EXPECT_EQ(bq.pop(), true);
    EXPECT_EQ(bq.pop(3), 0b101);
    EXPECT_EQ(bq.pop(), false);
    EXPECT_TRUE(bq.empty());
}

TEST(BitQueueTest, PopEmpty) {
    cpp_utils::BitQueue bq;
    EXPECT_THROW(bq.pop(), std::out_of_range);
}

TEST(BitQueueTest, PopTooManyBits) {
    cpp_utils::BitQueue bq;
    bq.push(0b101, 3);
    EXPECT_THROW(bq.pop(4), std::out_of_range);
}

TEST(BitQueueTest, Front) {
    cpp_utils::BitQueue bq;
    bq.push(true);
    bq.push(false);
    EXPECT_EQ(bq.front(), true);
    bq.pop();
    EXPECT_EQ(bq.front(), false);
}

TEST(BitQueueTest, FrontEmpty) {
    cpp_utils::BitQueue bq;
    EXPECT_THROW(bq.front(), std::out_of_range);
}

TEST(BitQueueTest, Clear) {
    cpp_utils::BitQueue bq;
    bq.push(0b101, 3);
    bq.clear();
    EXPECT_TRUE(bq.empty());
    EXPECT_EQ(bq.size(), 0);
}

TEST(BitQueueTest, LargeNumberOfBits) {
    cpp_utils::BitQueue bq;
    for (int i = 0; i < 1000; ++i) {
        bq.push(i % 2);
    }
    EXPECT_EQ(bq.size(), 1000);
    for (int i = 0; i < 1000; ++i) {
        EXPECT_EQ(bq.pop(), i % 2);
    }
    EXPECT_TRUE(bq.empty());
}

TEST(BitQueueTest, PushMoreThan64Bits) {
    cpp_utils::BitQueue bq;
    EXPECT_THROW(bq.push(0, 65), std::invalid_argument);
}

TEST(BitQueueTest, PopMoreThan64Bits) {
    cpp_utils::BitQueue bq;
    EXPECT_THROW(bq.pop(65), std::invalid_argument);
}

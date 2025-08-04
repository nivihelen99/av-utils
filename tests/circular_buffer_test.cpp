#include "gtest/gtest.h"
#include "circular_buffer.h"
#include <string>
#include <numeric>

TEST(CircularBufferTest, ConstructorAndCapacity) {
    CircularBuffer<int> buffer(5);
    EXPECT_EQ(buffer.capacity(), 5);
    EXPECT_EQ(buffer.size(), 0);
    EXPECT_TRUE(buffer.empty());
    EXPECT_FALSE(buffer.full());

    EXPECT_THROW(CircularBuffer<int>(0), std::invalid_argument);
}

TEST(CircularBufferTest, PushBack) {
    CircularBuffer<int> buffer(3);
    buffer.push_back(1);
    buffer.push_back(2);

    EXPECT_EQ(buffer.size(), 2);
    EXPECT_EQ(buffer.front(), 1);
    EXPECT_EQ(buffer.back(), 2);
    EXPECT_FALSE(buffer.full());

    buffer.push_back(3);
    EXPECT_EQ(buffer.size(), 3);
    EXPECT_EQ(buffer.front(), 1);
    EXPECT_EQ(buffer.back(), 3);
    EXPECT_TRUE(buffer.full());

    // Test overwrite
    buffer.push_back(4);
    EXPECT_EQ(buffer.size(), 3);
    EXPECT_EQ(buffer.front(), 2); // Oldest element (1) was overwritten
    EXPECT_EQ(buffer.back(), 4);
    EXPECT_TRUE(buffer.full());
}

TEST(CircularBufferTest, PushFront) {
    CircularBuffer<std::string> buffer(3);
    buffer.push_front("A");
    buffer.push_front("B");

    EXPECT_EQ(buffer.size(), 2);
    EXPECT_EQ(buffer.front(), "B");
    EXPECT_EQ(buffer.back(), "A");

    buffer.push_front("C");
    EXPECT_TRUE(buffer.full());
    EXPECT_EQ(buffer.front(), "C");
    EXPECT_EQ(buffer.back(), "A");

    // Test overwrite
    buffer.push_front("D");
    EXPECT_TRUE(buffer.full());
    EXPECT_EQ(buffer.front(), "D");
    EXPECT_EQ(buffer.back(), "B"); // Oldest element ("A") was overwritten at the back
}

TEST(CircularBufferTest, PopFront) {
    CircularBuffer<int> buffer(3);
    buffer.push_back(10);
    buffer.push_back(20);
    buffer.push_back(30);

    buffer.pop_front();
    EXPECT_EQ(buffer.size(), 2);
    EXPECT_EQ(buffer.front(), 20);
    EXPECT_EQ(buffer.back(), 30);

    buffer.pop_front();
    buffer.pop_front();
    EXPECT_TRUE(buffer.empty());

    EXPECT_THROW(buffer.pop_front(), std::runtime_error);
}

TEST(CircularBufferTest, PopBack) {
    CircularBuffer<int> buffer(3);
    buffer.push_back(10);
    buffer.push_back(20);
    buffer.push_back(30);

    buffer.pop_back();
    EXPECT_EQ(buffer.size(), 2);
    EXPECT_EQ(buffer.front(), 10);
    EXPECT_EQ(buffer.back(), 20);

    buffer.pop_back();
    buffer.pop_back();
    EXPECT_TRUE(buffer.empty());

    EXPECT_THROW(buffer.pop_back(), std::runtime_error);
}

TEST(CircularBufferTest, ElementAccess) {
    CircularBuffer<int> buffer(5);
    for (int i = 0; i < 5; ++i) {
        buffer.push_back(i * 10);
    }

    EXPECT_EQ(buffer[0], 0);
    EXPECT_EQ(buffer[2], 20);
    EXPECT_EQ(buffer[4], 40);
    EXPECT_THROW(buffer[5], std::out_of_range);

    const auto& c_buffer = buffer;
    EXPECT_EQ(c_buffer[1], 10);
    EXPECT_THROW(c_buffer[5], std::out_of_range);

    buffer.push_back(50); // Overwrites 0
    EXPECT_EQ(buffer.front(), 10);
    EXPECT_EQ(buffer[0], 10);
    EXPECT_EQ(buffer[4], 50);
}

TEST(CircularBufferTest, Clear) {
    CircularBuffer<int> buffer(5);
    buffer.push_back(1);
    buffer.push_back(2);
    buffer.clear();

    EXPECT_EQ(buffer.size(), 0);
    EXPECT_TRUE(buffer.empty());
    EXPECT_FALSE(buffer.full());
    EXPECT_THROW(buffer.front(), std::runtime_error);
}

TEST(CircularBufferTest, Rotation) {
    CircularBuffer<int> buffer(5);
    for (int i = 1; i <= 5; ++i) {
        buffer.push_back(i);
    }

    // Rotate right
    buffer.rotate(2);
    EXPECT_EQ(buffer[0], 4);
    EXPECT_EQ(buffer[1], 5);
    EXPECT_EQ(buffer[2], 1);
    EXPECT_EQ(buffer[3], 2);
    EXPECT_EQ(buffer[4], 3);

    // Rotate left
    buffer.rotate(-3);
    EXPECT_EQ(buffer[0], 2);
    EXPECT_EQ(buffer[1], 3);
    EXPECT_EQ(buffer[2], 4);
    EXPECT_EQ(buffer[3], 5);
    EXPECT_EQ(buffer[4], 1);

    // Rotate by 0
    buffer.rotate(0);
    EXPECT_EQ(buffer[0], 2);
    EXPECT_EQ(buffer[4], 1);

    // Rotate large number
    buffer.rotate(7); // Same as rotate(2)
    EXPECT_EQ(buffer[0], 5);
    EXPECT_EQ(buffer[1], 1);
    EXPECT_EQ(buffer[2], 2);
}

TEST(CircularBufferTest, Iterator) {
    CircularBuffer<int> buffer(4);
    buffer.push_back(10);
    buffer.push_back(20);
    buffer.push_back(30);

    // Forward iterator
    auto it = buffer.begin();
    EXPECT_EQ(*it, 10);
    ++it;
    EXPECT_EQ(*it, 20);
    it++;
    EXPECT_EQ(*it, 30);
    ++it;
    EXPECT_EQ(it, buffer.end());

    // Sum using std::accumulate
    int sum = std::accumulate(buffer.begin(), buffer.end(), 0);
    EXPECT_EQ(sum, 60);

    // Test with overwrite
    buffer.push_back(40);
    buffer.push_back(50); // Overwrites 10
    sum = std::accumulate(buffer.begin(), buffer.end(), 0);
    EXPECT_EQ(sum, 20 + 30 + 40 + 50);
    EXPECT_EQ(*buffer.begin(), 20);
}

TEST(CircularBufferTest, ConstIterator) {
    CircularBuffer<int> buffer(3);
    buffer.push_back(1);
    buffer.push_back(2);
    buffer.push_back(3);
    buffer.push_back(4); // Overwrites 1

    const CircularBuffer<int>& c_buffer = buffer;
    auto it = c_buffer.cbegin();
    EXPECT_EQ(*it, 2);
    ++it;
    EXPECT_EQ(*it, 3);
    ++it;
    EXPECT_EQ(*it, 4);
    ++it;
    EXPECT_EQ(it, c_buffer.cend());
}

TEST(CircularBufferTest, ReverseIterator) {
    CircularBuffer<int> buffer(5);
    for (int i = 0; i < 5; ++i) {
        buffer.push_back(i);
    }
    buffer.push_back(5); // Overwrite 0 -> buffer is 1,2,3,4,5

    auto it = buffer.rbegin();
    EXPECT_EQ(*it, 5);
    ++it;
    EXPECT_EQ(*it, 4);
    ++it;
    EXPECT_EQ(*it, 3);
    ++it;
    EXPECT_EQ(*it, 2);
    ++it;
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(it, buffer.rend());
}

TEST(CircularBufferTest, RandomAccessIterator) {
    CircularBuffer<int> buffer(5);
    for (int i=1; i<=5; ++i) buffer.push_back(i);

    auto it = buffer.begin();
    EXPECT_EQ(*(it + 3), 4);
    it += 2;
    EXPECT_EQ(*it, 3);
    auto it2 = it - 1;
    EXPECT_EQ(*it2, 2);

    EXPECT_GT(it, it2);
    EXPECT_LT(it2, it);
    EXPECT_EQ(it - it2, 1);
}

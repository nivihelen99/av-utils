#include "gtest/gtest.h"
#include "heap_queue.h" // Adjust path if heap_queue.h is in root
#include <string>
#include <vector>
#include <functional> // For std::greater, std::less

// Basic test for default constructor (min-heap of ints)
TEST(HeapQueueTest, DefaultConstructorAndBasicOps) {
    HeapQueue<int> pq;
    ASSERT_TRUE(pq.empty());
    ASSERT_EQ(pq.size(), 0);

    pq.push(10);
    ASSERT_FALSE(pq.empty());
    ASSERT_EQ(pq.size(), 1);
    ASSERT_EQ(pq.top(), 10);

    pq.push(5);
    ASSERT_EQ(pq.size(), 2);
    ASSERT_EQ(pq.top(), 5); // Min-heap, 5 should be top

    pq.push(15);
    ASSERT_EQ(pq.size(), 3);
    ASSERT_EQ(pq.top(), 5);

    ASSERT_EQ(pq.pop(), 5);
    ASSERT_EQ(pq.size(), 2);
    ASSERT_EQ(pq.top(), 10);

    ASSERT_EQ(pq.pop(), 10);
    ASSERT_EQ(pq.size(), 1);
    ASSERT_EQ(pq.top(), 15);

    ASSERT_EQ(pq.pop(), 15);
    ASSERT_TRUE(pq.empty());
    ASSERT_EQ(pq.size(), 0);

    // Test pop or top on empty heap
    ASSERT_THROW(pq.pop(), std::out_of_range);
    ASSERT_THROW(pq.top(), std::out_of_range);
}

TEST(HeapQueueTest, ClearHeap) {
    HeapQueue<int> pq;
    pq.push(10);
    pq.push(5);
    ASSERT_FALSE(pq.empty());
    pq.clear();
    ASSERT_TRUE(pq.empty());
    ASSERT_EQ(pq.size(), 0);
    // Check if push works after clear
    pq.push(20);
    ASSERT_EQ(pq.top(), 20);
}

TEST(HeapQueueTest, HeapifyInts) {
    HeapQueue<int> pq;
    std::vector<int> data = {30, 10, 50, 20, 40};
    pq.heapify(data); // Copy version

    ASSERT_EQ(pq.size(), 5);
    ASSERT_EQ(pq.pop(), 10);
    ASSERT_EQ(pq.pop(), 20);
    ASSERT_EQ(pq.pop(), 30);
    ASSERT_EQ(pq.pop(), 40);
    ASSERT_EQ(pq.pop(), 50);
    ASSERT_TRUE(pq.empty());

    // Move version
    std::vector<int> data_to_move = {7, 3, 9, 1, 5};
    pq.heapify(std::move(data_to_move));
    ASSERT_TRUE(data_to_move.empty()); // Check if moved from
    ASSERT_EQ(pq.size(), 5);
    ASSERT_EQ(pq.pop(), 1);
    ASSERT_EQ(pq.pop(), 3);
    ASSERT_EQ(pq.pop(), 5);
    ASSERT_EQ(pq.pop(), 7);
    ASSERT_EQ(pq.pop(), 9);
    ASSERT_TRUE(pq.empty());
}

// Custom struct for testing
struct TestEvent {
    int id;
    std::string payload;
    int priority; // Lower value means higher priority for min-heap

    bool operator==(const TestEvent& other) const {
        return id == other.id && payload == other.payload && priority == other.priority;
    }
};

// Key extractor for TestEvent's priority
auto test_event_priority_key = [](const TestEvent& ev) {
    return ev.priority;
};

TEST(HeapQueueTest, StructWithKeyFnMinHeap) {
    HeapQueue<TestEvent, decltype(test_event_priority_key), std::less<int>>
        pq(test_event_priority_key, std::less<int>());

    pq.push({1, "Event A", 10});
    pq.push({2, "Event B", 5});  // Higher priority
    pq.push({3, "Event C", 12});
    pq.push({4, "Event D", 5});  // Same priority as B

    ASSERT_EQ(pq.size(), 4);
    TestEvent top_event = pq.pop();
    ASSERT_EQ(top_event.priority, 5);
    // Could be Event B or D, order for equal keys is not guaranteed

    top_event = pq.pop();
    ASSERT_EQ(top_event.priority, 5);

    ASSERT_EQ(pq.pop().priority, 10);
    ASSERT_EQ(pq.pop().priority, 12);
    ASSERT_TRUE(pq.empty());
}

TEST(HeapQueueTest, StructWithKeyFnMaxHeap) {
    HeapQueue<TestEvent, decltype(test_event_priority_key), std::greater<int>>
        pq(test_event_priority_key, std::greater<int>());

    pq.push({1, "Event A", 10});
    pq.push({2, "Event B", 5});  // Lower priority for max-heap
    pq.push({3, "Event C", 12}); // Higher priority

    ASSERT_EQ(pq.size(), 3);
    ASSERT_EQ(pq.pop().priority, 12);
    ASSERT_EQ(pq.pop().priority, 10);
    ASSERT_EQ(pq.pop().priority, 5);
    ASSERT_TRUE(pq.empty());
}

TEST(HeapQueueTest, UpdateTop) {
    HeapQueue<int> pq;
    pq.push(100);
    pq.push(200);
    pq.push(50); // Top is 50

    ASSERT_EQ(pq.top(), 50);
    int old_top = pq.update_top(150); // Replace 50 with 150
    ASSERT_EQ(old_top, 50);
    ASSERT_EQ(pq.top(), 100); // New top should be 100

    ASSERT_EQ(pq.pop(), 100);
    ASSERT_EQ(pq.pop(), 150);
    ASSERT_EQ(pq.pop(), 200);
    ASSERT_TRUE(pq.empty());

    // Update top on single element heap
    pq.push(10);
    old_top = pq.update_top(5);
    ASSERT_EQ(old_top, 10);
    ASSERT_EQ(pq.top(), 5);
    ASSERT_EQ(pq.pop(), 5);

    // Update top on empty heap should throw
    ASSERT_THROW(pq.update_top(99), std::out_of_range);
}

TEST(HeapQueueTest, AsVector) {
    HeapQueue<int> pq;
    pq.push(10);
    pq.push(5);
    pq.push(15);
    // Internal order is implementation defined, but elements are there
    const std::vector<int>& internal_data = pq.as_vector();
    ASSERT_EQ(internal_data.size(), 3);
    // Check if elements exist, not their order
    bool found5 = false, found10 = false, found15 = false;
    for (int val : internal_data) {
        if (val == 5) found5 = true;
        if (val == 10) found10 = true;
        if (val == 15) found15 = true;
    }
    ASSERT_TRUE(found5 && found10 && found15);
}

// Main function to run tests (not strictly needed if CMake handles it)
// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }

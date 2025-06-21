#include "gtest/gtest.h"
#include "round_robin_queue.h"
#include <string>
#include <vector>
#include <memory> // For std::shared_ptr
#include <numeric> // For std::iota

// Test fixture for RoundRobinQueue tests
class RoundRobinQueueTest : public ::testing::Test {
protected:
    RoundRobinQueue<int> rr_int;
    RoundRobinQueue<std::string> rr_str;

    struct CustomStruct {
        int id;
        std::string data;

        bool operator==(const CustomStruct& other) const {
            return id == other.id && data == other.data;
        }
    };
    RoundRobinQueue<CustomStruct> rr_custom;
};

// Test basic operations: empty, size, enqueue
TEST_F(RoundRobinQueueTest, BasicOperations) {
    EXPECT_TRUE(rr_int.empty());
    EXPECT_EQ(rr_int.size(), 0);

    rr_int.enqueue(10);
    rr_int.enqueue(20);
    rr_int.enqueue(30);

    EXPECT_FALSE(rr_int.empty());
    EXPECT_EQ(rr_int.size(), 3);
}

// Test round-robin access behavior
TEST_F(RoundRobinQueueTest, RoundRobinAccess) {
    rr_str.enqueue("A");
    rr_str.enqueue("B");
    rr_str.enqueue("C");

    EXPECT_EQ(rr_str.next(), "A");
    EXPECT_EQ(rr_str.next(), "B");
    EXPECT_EQ(rr_str.next(), "C");
    EXPECT_EQ(rr_str.next(), "A"); // Wrap around
    EXPECT_EQ(rr_str.next(), "B");
}

// Test peek functionality
TEST_F(RoundRobinQueueTest, Peek) {
    rr_int.enqueue(100);
    rr_int.enqueue(200);

    EXPECT_EQ(rr_int.peek(), 100);
    EXPECT_EQ(rr_int.peek(), 100); // Peek should not advance

    EXPECT_EQ(rr_int.next(), 100); // Next advances
    EXPECT_EQ(rr_int.peek(), 200); // Peek now shows the new current
}

// Test skip functionality
TEST_F(RoundRobinQueueTest, Skip) {
    RoundRobinQueue<char> rr_char;
    rr_char.enqueue('X');
    rr_char.enqueue('Y');
    rr_char.enqueue('Z');

    EXPECT_EQ(rr_char.peek(), 'X');
    rr_char.skip(); // Remove 'X'
    EXPECT_EQ(rr_char.size(), 2);
    EXPECT_EQ(rr_char.peek(), 'Y'); // Current should be 'Y'

    EXPECT_EQ(rr_char.next(), 'Y'); // Advance current to 'Z'
    EXPECT_EQ(rr_char.peek(), 'Z');
    rr_char.skip(); // Remove 'Z'
    EXPECT_EQ(rr_char.size(), 1);
    EXPECT_EQ(rr_char.peek(), 'Y'); // Current should be 'Y' (the only element left)

    rr_char.skip(); // Remove 'Y'
    EXPECT_TRUE(rr_char.empty());
    EXPECT_EQ(rr_char.size(), 0);
}

// Test skip on last element
TEST_F(RoundRobinQueueTest, SkipLastElement) {
    rr_int.enqueue(1);
    rr_int.enqueue(2);
    rr_int.enqueue(3);

    rr_int.next(); // Current is 2
    rr_int.next(); // Current is 3
    EXPECT_EQ(rr_int.peek(), 3);
    rr_int.skip(); // Remove 3
    EXPECT_EQ(rr_int.size(), 2);
    EXPECT_EQ(rr_int.peek(), 1); // Current should wrap to 1
}


// Test reset functionality
TEST_F(RoundRobinQueueTest, Reset) {
    rr_int.enqueue(1);
    rr_int.enqueue(2);
    rr_int.enqueue(3);

    rr_int.next(); // Current: 1 (pos 0), next element is 2
    rr_int.next(); // Current: 2 (pos 1), next element is 3
    EXPECT_EQ(rr_int.peek(), 3);

    rr_int.reset();
    EXPECT_EQ(rr_int.peek(), 1);
    EXPECT_EQ(rr_int.current_position(), 0);
}

// Test clear functionality
TEST_F(RoundRobinQueueTest, Clear) {
    rr_str.enqueue("test1");
    rr_str.enqueue("test2");
    EXPECT_EQ(rr_str.size(), 2);

    rr_str.clear();
    EXPECT_TRUE(rr_str.empty());
    EXPECT_EQ(rr_str.size(), 0);
    EXPECT_EQ(rr_str.current_position(), 0); // Current should be reset
}

// Test insert_front functionality
TEST_F(RoundRobinQueueTest, InsertFront) {
    rr_int.enqueue(2); // Q: [2], current=0
    rr_int.enqueue(3); // Q: [2, 3], current=0

    // Current is 0 (points to 2)
    rr_int.insert_front(1); // Q: [1, 2, 3], current should become 1 (still points to 2)
    EXPECT_EQ(rr_int.size(), 3);
    EXPECT_EQ(rr_int.peek(), 2); // Should still point to original '2'

    EXPECT_EQ(rr_int.next(), 2); // Returns 2, current points to 3
    EXPECT_EQ(rr_int.next(), 3); // Returns 3, current points to 1
    EXPECT_EQ(rr_int.next(), 1); // Returns 1, current points to 2

    // Test insert_front when queue is empty
    RoundRobinQueue<int> rr_empty;
    rr_empty.insert_front(10);
    EXPECT_EQ(rr_empty.size(), 1);
    EXPECT_EQ(rr_empty.peek(), 10);
    EXPECT_EQ(rr_empty.current_position(), 0);

    // Test insert_front when current is not 0
    rr_int.clear();
    rr_int.enqueue(10);
    rr_int.enqueue(20);
    rr_int.enqueue(30); // [10, 20, 30], current=0
    rr_int.next();      // current=1 (points to 20)
    rr_int.insert_front(5); // [5, 10, 20, 30], current should become 2 (still points to 20)
    EXPECT_EQ(rr_int.size(), 4);
    EXPECT_EQ(rr_int.peek(), 20);
    EXPECT_EQ(rr_int.next(), 20);
    EXPECT_EQ(rr_int.next(), 30);
    EXPECT_EQ(rr_int.next(), 5);
    EXPECT_EQ(rr_int.next(), 10);
}


// Test for_each functionality
TEST_F(RoundRobinQueueTest, ForEach) {
    rr_int.enqueue(10);
    rr_int.enqueue(20);
    rr_int.enqueue(30);

    rr_int.next(); // current points to 20

    std::vector<int> visited;
    rr_int.for_each([&visited](const int& val) {
        visited.push_back(val);
    });

    ASSERT_EQ(visited.size(), 3);
    EXPECT_EQ(visited[0], 20); // Starts from current
    EXPECT_EQ(visited[1], 30);
    EXPECT_EQ(visited[2], 10); // Wraps around

    // Test for_each on empty queue
    RoundRobinQueue<int> empty_rr;
    std::vector<int> visited_empty;
    empty_rr.for_each([&visited_empty](const int& val) {
        visited_empty.push_back(val);
    });
    EXPECT_TRUE(visited_empty.empty());
}

// Test exception handling for empty queue operations
TEST_F(RoundRobinQueueTest, ExceptionsOnEmpty) {
    EXPECT_THROW(rr_int.peek(), std::runtime_error);
    EXPECT_THROW(rr_int.next(), std::runtime_error);
    EXPECT_THROW(rr_int.skip(), std::runtime_error);
    // rotate, remove, contains should not throw on empty, but return gracefully/false
    EXPECT_NO_THROW(rr_int.rotate(1));
    EXPECT_FALSE(rr_int.remove(10));
    EXPECT_FALSE(rr_int.contains(10));
}

// Test with std::shared_ptr
TEST_F(RoundRobinQueueTest, SmartPointers) {
    RoundRobinQueue<std::shared_ptr<int>> rr_ptr;
    rr_ptr.enqueue(std::make_shared<int>(42));
    rr_ptr.enqueue(std::make_shared<int>(84));

    auto ptr1 = rr_ptr.next();
    ASSERT_TRUE(ptr1);
    EXPECT_EQ(*ptr1, 42);

    auto ptr2 = rr_ptr.peek();
    ASSERT_TRUE(ptr2);
    EXPECT_EQ(*ptr2, 84);
}

// Test constructor with iterators
TEST_F(RoundRobinQueueTest, IteratorConstructor) {
    std::vector<int> initial_data = {1, 2, 3, 4, 5};
    RoundRobinQueue<int> rr_from_iter(initial_data.begin(), initial_data.end());

    EXPECT_EQ(rr_from_iter.size(), 5);
    EXPECT_FALSE(rr_from_iter.empty());
    EXPECT_EQ(rr_from_iter.peek(), 1);
    EXPECT_EQ(rr_from_iter.next(), 1);
    EXPECT_EQ(rr_from_iter.next(), 2);
    EXPECT_EQ(rr_from_iter.next(), 3);
    EXPECT_EQ(rr_from_iter.next(), 4);
    EXPECT_EQ(rr_from_iter.next(), 5);
    EXPECT_EQ(rr_from_iter.next(), 1); // Wrap
}

// Test rotate method
TEST_F(RoundRobinQueueTest, Rotate) {
    rr_int.enqueue(1);
    rr_int.enqueue(2);
    rr_int.enqueue(3);
    rr_int.enqueue(4);
    // Queue: [1, 2, 3, 4], current = 0 (points to 1)

    EXPECT_EQ(rr_int.peek(), 1);

    rr_int.rotate(1); // current = (0 + 1) % 4 = 1 (points to 2)
    EXPECT_EQ(rr_int.peek(), 2);
    EXPECT_EQ(rr_int.current_position(), 1);

    rr_int.rotate(2); // current = (1 + 2) % 4 = 3 (points to 4)
    EXPECT_EQ(rr_int.peek(), 4);
    EXPECT_EQ(rr_int.current_position(), 3);

    rr_int.rotate(-1); // current = (3 - 1 + 4) % 4 = 2 (points to 3)
    EXPECT_EQ(rr_int.peek(), 3);
    EXPECT_EQ(rr_int.current_position(), 2);

    rr_int.rotate(-3); // current = (2 - 3 + 4) % 4 = 3 (points to 4)
    EXPECT_EQ(rr_int.peek(), 4);
    EXPECT_EQ(rr_int.current_position(), 3);

    rr_int.rotate(4); // current = (3 + 4) % 4 = 3 (points to 4)
    EXPECT_EQ(rr_int.peek(), 4);
    EXPECT_EQ(rr_int.current_position(), 3);

    rr_int.rotate(0); // current = (3 + 0) % 4 = 3 (points to 4)
    EXPECT_EQ(rr_int.peek(), 4);
    EXPECT_EQ(rr_int.current_position(), 3);

    // Rotate on empty queue
    RoundRobinQueue<int> empty_rr;
    empty_rr.rotate(5); // Should not throw, no effect
    EXPECT_TRUE(empty_rr.empty());
}

// Test remove method
TEST_F(RoundRobinQueueTest, Remove) {
    rr_str.enqueue("A");
    rr_str.enqueue("B");
    rr_str.enqueue("C");
    rr_str.enqueue("B"); // Duplicate
    rr_str.enqueue("D");
    // Queue: [A, B, C, B, D], current = 0 (points to A)

    // Remove element not present
    EXPECT_FALSE(rr_str.remove("X"));
    EXPECT_EQ(rr_str.size(), 5);

    // Remove first "B" (at index 1)
    // Current is 0 (A). Removing B (idx 1) doesn't affect current index.
    EXPECT_EQ(rr_str.peek(), "A");
    EXPECT_TRUE(rr_str.remove("B")); // Removes first "B"
    // Queue: [A, C, B, D], current = 0 (points to A)
    EXPECT_EQ(rr_str.size(), 4);
    EXPECT_EQ(rr_str.peek(), "A");
    EXPECT_EQ(rr_str.next(), "A"); // current -> C
    EXPECT_EQ(rr_str.next(), "C"); // current -> B
    EXPECT_EQ(rr_str.next(), "B"); // current -> D
    EXPECT_EQ(rr_str.next(), "D"); // current -> A

    // Reset and test removing element before current
    rr_str.clear();
    rr_str.enqueue("A");
    rr_str.enqueue("B");
    rr_str.enqueue("C");
    rr_str.enqueue("D"); // [A, B, C, D], current = 0
    rr_str.next();       // current = 1 (points to B)
    rr_str.next();       // current = 2 (points to C)
    EXPECT_EQ(rr_str.peek(), "C");
    EXPECT_TRUE(rr_str.remove("A")); // Remove "A" (index 0)
    // Queue: [B, C, D], current should be 1 (still points to C)
    EXPECT_EQ(rr_str.size(), 3);
    EXPECT_EQ(rr_str.peek(), "C");
    EXPECT_EQ(rr_str.current_position(), 1); // C is now at index 1
    EXPECT_EQ(rr_str.next(), "C"); // current -> D
    EXPECT_EQ(rr_str.next(), "D"); // current -> B
    EXPECT_EQ(rr_str.next(), "B"); // current -> C

    // Test removing current element
    rr_str.clear();
    rr_str.enqueue("A");
    rr_str.enqueue("B");
    rr_str.enqueue("C"); // [A, B, C], current = 0
    rr_str.next();       // current = 1 (points to B)
    EXPECT_EQ(rr_str.peek(), "B");
    EXPECT_TRUE(rr_str.remove("B")); // Remove "B" (current element)
    // Queue: [A, C], current should be 1 (points to C)
    EXPECT_EQ(rr_str.size(), 2);
    EXPECT_EQ(rr_str.peek(), "C");
    EXPECT_EQ(rr_str.current_position(), 1);
    EXPECT_EQ(rr_str.next(), "C"); // current -> A
    EXPECT_EQ(rr_str.next(), "A"); // current -> C

    // Test removing current element when it's the last one and current needs to wrap
    rr_str.clear();
    rr_str.enqueue("A");
    rr_str.enqueue("B"); // [A, B], current = 0
    rr_str.next();       // current = 1 (points to B)
    EXPECT_EQ(rr_str.peek(), "B");
    EXPECT_TRUE(rr_str.remove("B")); // Remove "B"
    // Queue: [A], current should be 0 (points to A)
    EXPECT_EQ(rr_str.size(), 1);
    EXPECT_EQ(rr_str.peek(), "A");
    EXPECT_EQ(rr_str.current_position(), 0);

    // Test removing the only element
    rr_str.clear();
    rr_str.enqueue("Z"); // [Z], current = 0
    EXPECT_TRUE(rr_str.remove("Z"));
    EXPECT_TRUE(rr_str.empty());
    EXPECT_EQ(rr_str.current_position(), 0);

    // Remove from empty queue
    EXPECT_FALSE(rr_str.remove("X"));
}

// Test contains method
TEST_F(RoundRobinQueueTest, Contains) {
    rr_str.enqueue("apple");
    rr_str.enqueue("banana");
    rr_str.enqueue("cherry");

    EXPECT_TRUE(rr_str.contains("apple"));
    EXPECT_TRUE(rr_str.contains("banana"));
    EXPECT_TRUE(rr_str.contains("cherry"));
    EXPECT_FALSE(rr_str.contains("grape"));

    // Test on empty queue
    RoundRobinQueue<int> empty_rr;
    EXPECT_FALSE(empty_rr.contains(1));
}

// Test with custom struct
TEST_F(RoundRobinQueueTest, CustomStructOperations) {
    CustomStruct s1 = {1, "one"};
    CustomStruct s2 = {2, "two"};
    CustomStruct s3 = {3, "three"};

    rr_custom.enqueue(s1);
    rr_custom.enqueue(s2);

    EXPECT_EQ(rr_custom.size(), 2);
    EXPECT_EQ(rr_custom.peek(), s1);
    EXPECT_EQ(rr_custom.next(), s1);
    EXPECT_EQ(rr_custom.peek(), s2);

    rr_custom.insert_front(s3); // Q: [s3, s1, s2]. current was 1 (pointing to s2). After insert_front, current becomes 2 (still pointing to s2).
    EXPECT_EQ(rr_custom.size(), 3);
    EXPECT_EQ(rr_custom.peek(), s2); // Current element should still be s2
    EXPECT_TRUE(rr_custom.contains(s3));
    EXPECT_TRUE(rr_custom.contains(s1));
    EXPECT_FALSE(rr_custom.contains({4, "four"}));

    // Current is 2, pointing to s2. Queue is [s3, s1, s2].
    // remove(s1): s1 is at index 1.
    // After removing s1: Q: [s3, s2].
    // removed_idx (1) < current (2), so current becomes 2-1 = 1.
    // Current is 1, pointing to s2 in [s3, s2].
    EXPECT_TRUE(rr_custom.remove(s1));
    EXPECT_EQ(rr_custom.size(), 2);
    EXPECT_EQ(rr_custom.peek(), s2); // Current should still point to s2
}

// Test remove when current becomes invalid and needs to wrap or reset
TEST_F(RoundRobinQueueTest, RemoveAdjustsCurrentComplex) {
    rr_int.enqueue(1);
    rr_int.enqueue(2);
    rr_int.enqueue(3);
    rr_int.enqueue(4);
    rr_int.enqueue(5);
    // [1, 2, 3, 4, 5], current = 0 (1)

    rr_int.next(); // current = 1 (2)
    rr_int.next(); // current = 2 (3)
    rr_int.next(); // current = 3 (4)
    EXPECT_EQ(rr_int.peek(), 4); // Current is 3, points to 4

    // Remove element 5 (at index 4, after current)
    EXPECT_TRUE(rr_int.remove(5));
    // [1, 2, 3, 4], current = 3 (4) - no change to current index
    EXPECT_EQ(rr_int.size(), 4);
    EXPECT_EQ(rr_int.peek(), 4);
    EXPECT_EQ(rr_int.current_position(), 3);

    // Remove element 4 (at index 3, which is current)
    EXPECT_TRUE(rr_int.remove(4));
    // [1, 2, 3], current was 3, now points to element at index 3.
    // Since queue size is 3, current becomes 3 % 3 = 0. (points to 1)
    EXPECT_EQ(rr_int.size(), 3);
    EXPECT_EQ(rr_int.peek(), 1);
    EXPECT_EQ(rr_int.current_position(), 0);


    rr_int.clear();
    rr_int.enqueue(1);
    rr_int.enqueue(2);
    rr_int.enqueue(3);
    // [1,2,3] current = 0 (1)
    rr_int.next(); // current = 1 (2)
    rr_int.next(); // current = 2 (3)
    EXPECT_EQ(rr_int.peek(), 3); // Current is 2, points to 3. This is the last element by index.
    EXPECT_TRUE(rr_int.remove(3)); // Remove current element which is last by index
    // [1,2], current was 2. After removing element at index 2, new size is 2.
    // current should become 0. (points to 1)
    EXPECT_EQ(rr_int.size(), 2);
    EXPECT_EQ(rr_int.peek(), 1);
    EXPECT_EQ(rr_int.current_position(), 0);


    // Test removing an element that causes current to decrement
    rr_int.clear();
    rr_int.enqueue(10);
    rr_int.enqueue(20);
    rr_int.enqueue(30);
    rr_int.enqueue(40); // [10, 20, 30, 40], current=0
    rr_int.next();      // current=1 (20)
    rr_int.next();      // current=2 (30)
    EXPECT_EQ(rr_int.peek(), 30); // current is 2
    EXPECT_TRUE(rr_int.remove(10)); // remove element at index 0
    // [20, 30, 40], current should be 1 (still pointing to 30)
    EXPECT_EQ(rr_int.size(), 3);
    EXPECT_EQ(rr_int.peek(), 30);
    EXPECT_EQ(rr_int.current_position(), 1); // 30 is now at index 1
}

// Test skip behavior at boundaries
TEST_F(RoundRobinQueueTest, SkipAtBoundaries) {
    rr_int.enqueue(1);
    rr_int.enqueue(2);
    rr_int.enqueue(3); // [1, 2, 3], current = 0 (1)

    // Skip first element
    rr_int.skip(); // Removes 1. Current becomes 0 (2).
    // [2, 3]
    EXPECT_EQ(rr_int.size(), 2);
    EXPECT_EQ(rr_int.peek(), 2);
    EXPECT_EQ(rr_int.current_position(), 0);

    rr_int.next(); // Current becomes 1 (3).
    // [2, 3]
    EXPECT_EQ(rr_int.peek(), 3);
    EXPECT_EQ(rr_int.current_position(), 1);

    // Skip last element (when current points to it)
    rr_int.skip(); // Removes 3. Current becomes 0 (2).
    // [2]
    EXPECT_EQ(rr_int.size(), 1);
    EXPECT_EQ(rr_int.peek(), 2);
    EXPECT_EQ(rr_int.current_position(), 0);

    // Skip the only remaining element
    rr_int.skip(); // Removes 2. Current becomes 0.
    EXPECT_TRUE(rr_int.empty());
    EXPECT_EQ(rr_int.current_position(), 0);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

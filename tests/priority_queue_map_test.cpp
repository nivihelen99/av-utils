#include "gtest/gtest.h"
#include "PriorityQueueMap.h" // Assuming it's in the include path

#include <string>
#include <vector>

// Basic test fixture
class PriorityQueueMapTest : public ::testing::Test {
protected:
    cpp_utils::PriorityQueueMap<int, std::string, int> pq_int_string_int; // Min-heap by default
    cpp_utils::PriorityQueueMap<std::string, double, double, std::less<double>> pq_string_double_double_max; // Max-heap
};

TEST_F(PriorityQueueMapTest, IsEmptyInitially) {
    EXPECT_TRUE(pq_int_string_int.empty());
    EXPECT_EQ(0, pq_int_string_int.size());
}

TEST_F(PriorityQueueMapTest, PushAndTop) {
    pq_int_string_int.push(1, "one", 10);
    EXPECT_FALSE(pq_int_string_int.empty());
    EXPECT_EQ(1, pq_int_string_int.size());
    EXPECT_EQ(1, pq_int_string_int.top_key());
    EXPECT_EQ(10, pq_int_string_int.top_priority());
    EXPECT_EQ("one", pq_int_string_int.get_value(1));

    pq_int_string_int.push(2, "two", 5); // Lower priority, should be new top
    EXPECT_EQ(2, pq_int_string_int.size());
    EXPECT_EQ(2, pq_int_string_int.top_key());
    EXPECT_EQ(5, pq_int_string_int.top_priority());
    EXPECT_EQ("two", pq_int_string_int.get_value(2));

    pq_int_string_int.push(3, "three", 12); // Higher priority
    EXPECT_EQ(3, pq_int_string_int.size());
    EXPECT_EQ(2, pq_int_string_int.top_key()); // Top should still be 2
    EXPECT_EQ(5, pq_int_string_int.top_priority());
    EXPECT_EQ("three", pq_int_string_int.get_value(3));
}

TEST_F(PriorityQueueMapTest, Pop) {
    pq_int_string_int.push(1, "one", 10);
    pq_int_string_int.push(2, "two", 5);
    pq_int_string_int.push(3, "three", 12);

    EXPECT_EQ(5, pq_int_string_int.pop()); // Pops key 2
    EXPECT_EQ(1, pq_int_string_int.top_key());
    EXPECT_EQ(10, pq_int_string_int.top_priority());
    EXPECT_EQ(2, pq_int_string_int.size());
    EXPECT_FALSE(pq_int_string_int.contains(2));

    EXPECT_EQ(10, pq_int_string_int.pop()); // Pops key 1
    EXPECT_EQ(3, pq_int_string_int.top_key());
    EXPECT_EQ(12, pq_int_string_int.top_priority());
    EXPECT_EQ(1, pq_int_string_int.size());
    EXPECT_FALSE(pq_int_string_int.contains(1));

    EXPECT_EQ(12, pq_int_string_int.pop()); // Pops key 3
    EXPECT_TRUE(pq_int_string_int.empty());
    EXPECT_EQ(0, pq_int_string_int.size());
    EXPECT_FALSE(pq_int_string_int.contains(3));

    EXPECT_THROW(pq_int_string_int.pop(), std::out_of_range);
    EXPECT_THROW(pq_int_string_int.top_key(), std::out_of_range);
}

TEST_F(PriorityQueueMapTest, PushExistingKeyUpdates) {
    pq_int_string_int.push(1, "one_v1", 10);
    EXPECT_EQ("one_v1", pq_int_string_int.get_value(1));
    EXPECT_EQ(10, pq_int_string_int.top_priority());

    pq_int_string_int.push(1, "one_v2", 5); // Update value and priority
    EXPECT_EQ(1, pq_int_string_int.size());
    EXPECT_EQ("one_v2", pq_int_string_int.get_value(1));
    EXPECT_EQ(5, pq_int_string_int.top_priority()); // Priority updated, should be top

    pq_int_string_int.push(2, "two", 20);
    pq_int_string_int.push(1, "one_v3", 30); // Update again, no longer top
    EXPECT_EQ(2, pq_int_string_int.size());
    EXPECT_EQ("one_v3", pq_int_string_int.get_value(1));
    EXPECT_EQ(2, pq_int_string_int.top_key());
    EXPECT_EQ(20, pq_int_string_int.top_priority());
}

TEST_F(PriorityQueueMapTest, UpdatePriority) {
    pq_int_string_int.push(1, "one", 10);
    pq_int_string_int.push(2, "two", 20);
    pq_int_string_int.push(3, "three", 5); // Top: 3 (5)

    EXPECT_EQ(3, pq_int_string_int.top_key());

    // Decrease priority of 1 (make it better for min-heap)
    pq_int_string_int.update_priority(1, 2); // 1 (2), 3 (5), 2 (20)
    EXPECT_EQ(1, pq_int_string_int.top_key());
    EXPECT_EQ(2, pq_int_string_int.top_priority());

    // Increase priority of 1 (make it worse for min-heap)
    pq_int_string_int.update_priority(1, 25); // 3 (5), 2 (20), 1 (25)
    EXPECT_EQ(3, pq_int_string_int.top_key());
    EXPECT_EQ(5, pq_int_string_int.top_priority());

    // Update priority of current top
    pq_int_string_int.update_priority(3, 30); // 2 (20), 1 (25), 3 (30)
    EXPECT_EQ(2, pq_int_string_int.top_key());
    EXPECT_EQ(20, pq_int_string_int.top_priority());

    // Update to same priority
    pq_int_string_int.update_priority(2, 20);
    EXPECT_EQ(2, pq_int_string_int.top_key());
    EXPECT_EQ(20, pq_int_string_int.top_priority());


    EXPECT_THROW(pq_int_string_int.update_priority(100, 1), std::out_of_range); // Key not found
}

TEST_F(PriorityQueueMapTest, Remove) {
    pq_int_string_int.push(1, "one", 10);
    pq_int_string_int.push(2, "two", 5);   // Top
    pq_int_string_int.push(3, "three", 12);
    pq_int_string_int.push(4, "four", 3);  // New Top
    pq_int_string_int.push(5, "five", 8);
    // Heap state (key:priority): 4:3, 2:5, 3:12, 1:10, 5:8 (approx, depends on insertion details)
    // Actual heap: (4,3), (5,8), (3,12), (1,10), (2,5) -> after sifting from (2,5) at (1)
    // (4,3)
    // (2,5) (3,12)
    // (1,10) (5,8)
    // After 4:3 -> (4,3), (2,5), (3,12), (1,10), (5,8)
    // After 5:8 -> (4,3), (5,8), (3,12), (1,10), (2,5) -> key_to_idx: 4:0, 5:1, 3:2, 1:3, 2:4
    // Pushed 1(10), 2(5), 3(12), 4(3), 5(8)
    // Initial:
    // (1,10)
    // Push 2(5): (2,5), (1,10)
    // Push 3(12): (2,5), (1,10), (3,12)
    // Push 4(3): (4,3), (1,10), (3,12), (2,5)  -- 2(5) was root's child, 1(10) was other child
    // Push 5(8): (4,3), (5,8), (3,12), (2,5), (1,10)
    // Order of keys if popped: 4, 2, 5, 1, 3
    // Priorities:             3, 5, 8, 10,12

    EXPECT_EQ(5, pq_int_string_int.size());
    EXPECT_EQ(4, pq_int_string_int.top_key());

    // Remove top (4, priority 3)
    pq_int_string_int.remove(4); // (2,5), (5,8), (3,12), (1,10)
    EXPECT_EQ(4, pq_int_string_int.size());
    EXPECT_FALSE(pq_int_string_int.contains(4));
    EXPECT_EQ(2, pq_int_string_int.top_key()); // New top should be 2 (priority 5)
    EXPECT_EQ(5, pq_int_string_int.top_priority());

    // Remove a leaf (e.g. 1, priority 10 - assuming its a leaf or easily becomes one)
    // Current: (2,5), (5,8), (3,12), (1,10)
    pq_int_string_int.remove(1); // (2,5), (5,8), (3,12)
    EXPECT_EQ(3, pq_int_string_int.size());
    EXPECT_FALSE(pq_int_string_int.contains(1));
    EXPECT_EQ(2, pq_int_string_int.top_key()); // Top still 2

    // Remove a middle element (e.g. 5, priority 8)
    // Current: (2,5), (5,8), (3,12)
    pq_int_string_int.remove(5); // (2,5), (3,12)
    EXPECT_EQ(2, pq_int_string_int.size());
    EXPECT_FALSE(pq_int_string_int.contains(5));
    EXPECT_EQ(2, pq_int_string_int.top_key());

    pq_int_string_int.remove(2); // (3,12)
    EXPECT_EQ(1, pq_int_string_int.size());
    EXPECT_FALSE(pq_int_string_int.contains(2));
    EXPECT_EQ(3, pq_int_string_int.top_key());

    pq_int_string_int.remove(3);
    EXPECT_TRUE(pq_int_string_int.empty());
    EXPECT_FALSE(pq_int_string_int.contains(3));

    EXPECT_THROW(pq_int_string_int.remove(100), std::out_of_range); // Key not found
}

TEST_F(PriorityQueueMapTest, Contains) {
    EXPECT_FALSE(pq_int_string_int.contains(1));
    pq_int_string_int.push(1, "one", 10);
    EXPECT_TRUE(pq_int_string_int.contains(1));
    pq_int_string_int.push(2, "two", 5);
    EXPECT_TRUE(pq_int_string_int.contains(2));
    pq_int_string_int.pop(); // Removes 2
    EXPECT_TRUE(pq_int_string_int.contains(1));
    EXPECT_FALSE(pq_int_string_int.contains(2));
}

TEST_F(PriorityQueueMapTest, GetValue) {
    pq_int_string_int.push(1, "apple", 10);
    pq_int_string_int.push(2, "banana", 5);
    EXPECT_EQ("apple", pq_int_string_int.get_value(1));
    EXPECT_EQ("banana", pq_int_string_int.get_value(2));

    pq_int_string_int.push(1, "apricot", 12); // Update value
    EXPECT_EQ("apricot", pq_int_string_int.get_value(1));

    const auto& const_pq = pq_int_string_int;
    EXPECT_EQ("apricot", const_pq.get_value(1));

    EXPECT_THROW(pq_int_string_int.get_value(3), std::out_of_range);
    EXPECT_THROW(const_pq.get_value(3), std::out_of_range);
}

TEST_F(PriorityQueueMapTest, MaxHeapOperations) {
    pq_string_double_double_max.push("taskA", 10.0, 10.0); // Key, Value, Priority
    pq_string_double_double_max.push("taskB", 20.0, 20.0);
    pq_string_double_double_max.push("taskC", 5.0, 5.0);

    EXPECT_EQ("taskB", pq_string_double_double_max.top_key());
    EXPECT_EQ(20.0, pq_string_double_double_max.top_priority());

    EXPECT_EQ(20.0, pq_string_double_double_max.pop()); // Pops taskB

    EXPECT_EQ("taskA", pq_string_double_double_max.top_key());
    EXPECT_EQ(10.0, pq_string_double_double_max.top_priority());

    pq_string_double_double_max.update_priority("taskC", 25.0); // Increase priority of C
    EXPECT_EQ("taskC", pq_string_double_double_max.top_key());
    EXPECT_EQ(25.0, pq_string_double_double_max.top_priority());
}

TEST_F(PriorityQueueMapTest, StressTestLike) {
    const int num_elements = 1000;
    for (int i = 0; i < num_elements; ++i) {
        pq_int_string_int.push(i, "val_" + std::to_string(i), num_elements - i); // Higher i = lower priority val
    }
    EXPECT_EQ(num_elements, pq_int_string_int.size());
    EXPECT_EQ(num_elements - 1, pq_int_string_int.top_key()); // Key n-1 has priority 1 (highest)
    EXPECT_EQ(1, pq_int_string_int.top_priority());

    for (int i = 0; i < num_elements / 2; ++i) {
        int key_to_update = i * 2; // Update even keys
        // Make their priority worse (larger value for min-heap)
        pq_int_string_int.update_priority(key_to_update, (num_elements - key_to_update) + num_elements);
    }

    // Check order after updates and pops
    int last_priority = -1;
    int count = 0;
    while(!pq_int_string_int.empty()) {
        int current_priority = pq_int_string_int.top_priority();
        if (last_priority != -1) {
            EXPECT_LE(last_priority, current_priority);
        }
        last_priority = pq_int_string_int.pop();
        count++;
    }
    EXPECT_EQ(num_elements, count);
}

TEST_F(PriorityQueueMapTest, RValuePush) {
    pq_int_string_int.push(100, std::string("hundred"), 100);
    EXPECT_EQ("hundred", pq_int_string_int.get_value(100));
    EXPECT_EQ(100, pq_int_string_int.top_key());

    pq_int_string_int.push(100, std::string("hundred_v2"), 50); // Update with rvalues
    EXPECT_EQ("hundred_v2", pq_int_string_int.get_value(100));
    EXPECT_EQ(50, pq_int_string_int.top_priority());

    pq_int_string_int.push(200, std::string("two_hundred"), 200);
    EXPECT_EQ(100, pq_int_string_int.top_key()); // 100 (50) should still be top
}

TEST_F(PriorityQueueMapTest, RemoveComplexScenario) {
    // Build a specific heap structure
    // (0,0) (1,1) (2,2) (3,3) (4,4) (5,5) (6,6)
    //             0(0)
    //       /             \
    //     1(1)            2(2)
    //    /   \           /   \
    //  3(3)  4(4)      5(5)  6(6)
    for(int i=0; i<=6; ++i) {
        pq_int_string_int.push(i, "v"+std::to_string(i), i);
    }
    EXPECT_EQ(0, pq_int_string_int.top_key());

    // Remove root: 0
    // Last element is 6(6). It moves to root.
    // Sift down 6(6): should swap with 1(1).
    // Heap becomes: 1(1) at root. 6(6) becomes child of 1(1).
    //             1(1)
    //       /             \
    //     3(3)            2(2)   <-- Error in manual trace, 6(6) swaps with 1(1), then 6(6) sifts down again
    //    /   \           /   \
    //  6(6)  4(4)      5(5)
    // Expected after removing 0: top is 1.
    pq_int_string_int.remove(0);
    EXPECT_EQ(6, pq_int_string_int.size());
    EXPECT_FALSE(pq_int_string_int.contains(0));
    EXPECT_EQ(1, pq_int_string_int.top_key());
    EXPECT_EQ(1, pq_int_string_int.top_priority());

    // Current top is 1. Remove 3 (a leaf child of 1)
    //             1(1)
    //       /             \
    //     3(3)            2(2)
    //    /   \           /
    //  6(6)  4(4)      5(5)
    // Remove 3(3). Last is 5(5). 5(5) moves to 3's spot.
    // Sift up/down 5(5) at 3's old spot. Parent 1(1) is smaller. Children none. Stays.
    // Top is 1.
    pq_int_string_int.remove(3);
    EXPECT_EQ(5, pq_int_string_int.size());
    EXPECT_FALSE(pq_int_string_int.contains(3));
    EXPECT_EQ(1, pq_int_string_int.top_key());
    EXPECT_EQ(1, pq_int_string_int.top_priority());

    // Pop all elements and check order
    std::vector<int> expected_keys_after_removals = {1, 2, 4, 5, 6};
    std::vector<int> popped_keys;
    while(!pq_int_string_int.empty()){
        popped_keys.push_back(pq_int_string_int.top_key());
        pq_int_string_int.pop();
    }
    EXPECT_EQ(expected_keys_after_removals, popped_keys);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

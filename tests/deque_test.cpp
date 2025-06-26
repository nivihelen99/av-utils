#include "deque.h" // Adjust path as needed based on CMake setup
#include <gtest/gtest.h>
#include <string>
#include <deque> // For std::deque in stress test
#include <vector>
#include <stdexcept>
#include <algorithm> // For std::sort, std::is_sorted

// Using namespace ankerl for convenience in test file
using namespace ankerl;

// Test fixture for Deque tests
class DequeTest : public ::testing::Test {
protected:
    Deque<int> d_int;
    Deque<std::string> d_str;
};

// Test default constructor
TEST_F(DequeTest, DefaultConstructor) {
    EXPECT_TRUE(d_int.empty());
    EXPECT_EQ(d_int.size(), 0);
    // EXPECT_GE(d_int.capacity(), 0); // Internal detail, but good to be aware
}

// Test constructor with count and value
TEST_F(DequeTest, ConstructorWithValue) {
    Deque<int> d(5, 10);
    EXPECT_FALSE(d.empty());
    EXPECT_EQ(d.size(), 5);
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(d[i], 10);
    }
}

// Test constructor with initializer list
TEST_F(DequeTest, ConstructorWithInitializerList) {
    Deque<int> d = {1, 2, 3, 4, 5};
    EXPECT_FALSE(d.empty());
    EXPECT_EQ(d.size(), 5);
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(d[i], i + 1);
    }
    Deque<int> empty_d = {};
    EXPECT_TRUE(empty_d.empty());
}

// Test push_back
TEST_F(DequeTest, PushBack) {
    d_int.push_back(1);
    EXPECT_EQ(d_int.size(), 1);
    EXPECT_EQ(d_int.back(), 1);
    EXPECT_EQ(d_int.front(), 1);

    d_int.push_back(2);
    EXPECT_EQ(d_int.size(), 2);
    EXPECT_EQ(d_int.back(), 2);
    EXPECT_EQ(d_int.front(), 1);

    // Test resizing
    for (int i = 3; i <= 20; ++i) {
        d_int.push_back(i);
    }
    EXPECT_EQ(d_int.size(), 20);
    EXPECT_EQ(d_int.back(), 20);
    EXPECT_EQ(d_int.front(), 1);
    for(int i=0; i < 20; ++i) {
        EXPECT_EQ(d_int[i], i+1);
    }
}

// Test push_front
TEST_F(DequeTest, PushFront) {
    d_int.push_front(1);
    EXPECT_EQ(d_int.size(), 1);
    EXPECT_EQ(d_int.front(), 1);
    EXPECT_EQ(d_int.back(), 1);

    d_int.push_front(2);
    EXPECT_EQ(d_int.size(), 2);
    EXPECT_EQ(d_int.front(), 2);
    EXPECT_EQ(d_int.back(), 1);

    // Test resizing
    for (int i = 3; i <= 20; ++i) {
        d_int.push_front(i);
    }
    EXPECT_EQ(d_int.size(), 20);
    EXPECT_EQ(d_int.front(), 20);
    EXPECT_EQ(d_int.back(), 1);
    // Elements are 20, 19, ..., 3, 2, 1
    for(int i=0; i < 20; ++i) {
        EXPECT_EQ(d_int[i], 20-i);
    }
}

// Test pop_back
TEST_F(DequeTest, PopBack) {
    d_int.push_back(1);
    d_int.push_back(2);
    d_int.push_back(3);

    d_int.pop_back();
    EXPECT_EQ(d_int.size(), 2);
    EXPECT_EQ(d_int.back(), 2);
    EXPECT_EQ(d_int.front(), 1);

    d_int.pop_back();
    EXPECT_EQ(d_int.size(), 1);
    EXPECT_EQ(d_int.back(), 1);
    EXPECT_EQ(d_int.front(), 1);

    d_int.pop_back();
    EXPECT_EQ(d_int.size(), 0);
    EXPECT_TRUE(d_int.empty());

    EXPECT_THROW(d_int.pop_back(), std::out_of_range);
}

// Test pop_front
TEST_F(DequeTest, PopFront) {
    d_int.push_back(1);
    d_int.push_back(2);
    d_int.push_back(3);

    d_int.pop_front();
    EXPECT_EQ(d_int.size(), 2);
    EXPECT_EQ(d_int.front(), 2);
    EXPECT_EQ(d_int.back(), 3);

    d_int.pop_front();
    EXPECT_EQ(d_int.size(), 1);
    EXPECT_EQ(d_int.front(), 3);
    EXPECT_EQ(d_int.back(), 3);

    d_int.pop_front();
    EXPECT_EQ(d_int.size(), 0);
    EXPECT_TRUE(d_int.empty());

    EXPECT_THROW(d_int.pop_front(), std::out_of_range);
}

// Test front and back access
TEST_F(DequeTest, FrontBackAccess) {
    EXPECT_THROW(d_int.front(), std::out_of_range);
    EXPECT_THROW(d_int.back(), std::out_of_range);

    d_int.push_back(10);
    EXPECT_EQ(d_int.front(), 10);
    EXPECT_EQ(d_int.back(), 10);

    d_int.push_back(20);
    EXPECT_EQ(d_int.front(), 10);
    EXPECT_EQ(d_int.back(), 20);

    d_int.push_front(5);
    EXPECT_EQ(d_int.front(), 5);
    EXPECT_EQ(d_int.back(), 20);

    const Deque<int>& const_d_int = d_int;
    EXPECT_EQ(const_d_int.front(), 5);
    EXPECT_EQ(const_d_int.back(), 20);
}

// Test operator[] and at()
TEST_F(DequeTest, OperatorAndAt) {
    d_int.push_back(10); // 10
    d_int.push_back(20); // 10 20
    d_int.push_front(5); // 5 10 20
    d_int.push_back(30); // 5 10 20 30
    d_int.push_front(0); // 0 5 10 20 30

    EXPECT_EQ(d_int[0], 0);
    EXPECT_EQ(d_int[1], 5);
    EXPECT_EQ(d_int[2], 10);
    EXPECT_EQ(d_int[3], 20);
    EXPECT_EQ(d_int[4], 30);

    EXPECT_EQ(d_int.at(0), 0);
    EXPECT_EQ(d_int.at(4), 30);
    EXPECT_THROW(d_int.at(5), std::out_of_range);
    EXPECT_THROW(d_int.at(100), std::out_of_range);

    Deque<int> empty_d;
    EXPECT_THROW(empty_d.at(0), std::out_of_range);


    const Deque<int>& const_d_int = d_int;
    EXPECT_EQ(const_d_int[0], 0);
    EXPECT_EQ(const_d_int.at(1), 5);
    EXPECT_THROW(const_d_int.at(5), std::out_of_range);
}

// Test clear
TEST_F(DequeTest, Clear) {
    d_int.push_back(1);
    d_int.push_back(2);
    d_int.clear();
    EXPECT_TRUE(d_int.empty());
    EXPECT_EQ(d_int.size(), 0);
    // Check if it can be reused
    d_int.push_back(3);
    EXPECT_EQ(d_int.size(), 1);
    EXPECT_EQ(d_int.front(), 3);
}

// Test copy constructor
TEST_F(DequeTest, CopyConstructor) {
    d_int.push_back(1);
    d_int.push_front(0);
    d_int.push_back(2); // 0 1 2

    Deque<int> d_copy(d_int);
    EXPECT_EQ(d_copy.size(), 3);
    EXPECT_EQ(d_copy[0], 0);
    EXPECT_EQ(d_copy[1], 1);
    EXPECT_EQ(d_copy[2], 2);

    // Ensure original is not affected
    d_copy.pop_front();
    EXPECT_EQ(d_int.size(), 3);
    EXPECT_EQ(d_int[0], 0);
}

// Test copy assignment operator
TEST_F(DequeTest, CopyAssignmentOperator) {
    d_int.push_back(1);
    d_int.push_front(0);
    d_int.push_back(2); // 0 1 2

    Deque<int> d_assigned;
    d_assigned.push_back(100); // Should be overwritten
    d_assigned = d_int;

    EXPECT_EQ(d_assigned.size(), 3);
    EXPECT_EQ(d_assigned[0], 0);
    EXPECT_EQ(d_assigned[1], 1);
    EXPECT_EQ(d_assigned[2], 2);

    // Ensure original is not affected
    d_assigned.pop_back();
    EXPECT_EQ(d_int.size(), 3);
    EXPECT_EQ(d_int[2], 2);

    // Self-assignment
    d_int = d_int;
    EXPECT_EQ(d_int.size(), 3);
    EXPECT_EQ(d_int[0], 0);

}

// Test move constructor
TEST_F(DequeTest, MoveConstructor) {
    d_int.push_back(1);
    d_int.push_front(0);
    d_int.push_back(2); // 0 1 2

    Deque<int> d_moved(std::move(d_int));
    EXPECT_EQ(d_moved.size(), 3);
    EXPECT_EQ(d_moved[0], 0);
    EXPECT_EQ(d_moved[1], 1);
    EXPECT_EQ(d_moved[2], 2);

    // Original d_int should be in a valid but unspecified state (likely empty)
    // EXPECT_TRUE(d_int.empty()); // This is a common state, but not strictly guaranteed by move
    EXPECT_EQ(d_int.size(), 0); // More robust check after our move implementation
}

// Test move assignment operator
TEST_F(DequeTest, MoveAssignmentOperator) {
    d_int.push_back(1);
    d_int.push_front(0);
    d_int.push_back(2); // 0 1 2

    Deque<int> d_assigned_move;
    d_assigned_move.push_back(100);
    d_assigned_move = std::move(d_int);

    EXPECT_EQ(d_assigned_move.size(), 3);
    EXPECT_EQ(d_assigned_move[0], 0);
    EXPECT_EQ(d_assigned_move[1], 1);
    EXPECT_EQ(d_assigned_move[2], 2);

    EXPECT_EQ(d_int.size(), 0);

    // Self-assignment (move)
    // Deque<int> temp_self_move = {5,6,7};
    // temp_self_move = std::move(temp_self_move); // This is tricky to test for correctness beyond not crashing
    // EXPECT_EQ(temp_self_move.size(), 3); // The standard doesn't guarantee much for self-move assignment.
                                        // Our implementation should handle it without issues.
}


// Test iterators
TEST_F(DequeTest, Iterators) {
    d_int.push_back(10);
    d_int.push_back(20);
    d_int.push_front(5); // 5, 10, 20

    std::vector<int> expected = {5, 10, 20};
    size_t i = 0;
    for (Deque<int>::iterator it = d_int.begin(); it != d_int.end(); ++it) {
        EXPECT_EQ(*it, expected[i++]);
    }
    EXPECT_EQ(i, expected.size());

    // Test const iterators
    const Deque<int>& const_d_int = d_int;
    i = 0;
    for (Deque<int>::const_iterator it = const_d_int.cbegin(); it != const_d_int.cend(); ++it) {
        EXPECT_EQ(*it, expected[i++]);
    }
    EXPECT_EQ(i, expected.size());

    // Test range-based for loop
    i = 0;
    for (int val : d_int) {
        EXPECT_EQ(val, expected[i++]);
    }
    EXPECT_EQ(i, expected.size());

    // Test empty deque iterators
    Deque<int> empty_d;
    EXPECT_EQ(empty_d.begin(), empty_d.end());
    EXPECT_EQ(empty_d.cbegin(), empty_d.cend());
}

TEST_F(DequeTest, IteratorOperations) {
    for (int i = 0; i < 5; ++i) d_int.push_back(i); // 0 1 2 3 4

    auto it = d_int.begin();
    EXPECT_EQ(*it, 0);
    ++it;
    EXPECT_EQ(*it, 1);
    it++;
    EXPECT_EQ(*it, 2);
    --it;
    EXPECT_EQ(*it, 1);
    it--;
    EXPECT_EQ(*it, 0);

    auto it2 = d_int.begin() + 2;
    EXPECT_EQ(*it2, 2);
    it2 = it2 - 1;
    EXPECT_EQ(*it2, 1);

    it = d_int.begin();
    it += 3;
    EXPECT_EQ(*it, 3);
    it -= 2;
    EXPECT_EQ(*it, 1);

    EXPECT_EQ(*(d_int.begin() + 4), 4);
    EXPECT_EQ(d_int.end() - d_int.begin(), 5);
    EXPECT_TRUE(d_int.begin() < d_int.end());
    EXPECT_TRUE(d_int.begin() + 2 < d_int.end());
    EXPECT_TRUE(d_int.end() > d_int.begin());

    EXPECT_EQ(d_int.begin()[2], 2);
}


// Test mixed operations (stress test like)
TEST_F(DequeTest, MixedOperationsStress) {
    std::deque<int> std_deque; // Standard deque for comparison
    Deque<int> my_deque;

    const int num_operations = 1000;
    srand(time(0));

    for (int i = 0; i < num_operations; ++i) {
        int op = rand() % 6; // 0: push_back, 1: push_front, 2: pop_back, 3: pop_front, 4: access, 5: size/empty

        if (op == 0) { // push_back
            int val = rand();
            std_deque.push_back(val);
            my_deque.push_back(val);
        } else if (op == 1) { // push_front
            int val = rand();
            std_deque.push_front(val);
            my_deque.push_front(val);
        } else if (op == 2) { // pop_back
            if (!std_deque.empty()) {
                ASSERT_FALSE(my_deque.empty());
                EXPECT_EQ(std_deque.back(), my_deque.back());
                std_deque.pop_back();
                my_deque.pop_back();
            } else {
                ASSERT_TRUE(my_deque.empty());
            }
        } else if (op == 3) { // pop_front
            if (!std_deque.empty()) {
                ASSERT_FALSE(my_deque.empty());
                EXPECT_EQ(std_deque.front(), my_deque.front());
                std_deque.pop_front();
                my_deque.pop_front();
            } else {
                ASSERT_TRUE(my_deque.empty());
            }
        } else if (op == 4) { // access
             if (!std_deque.empty()) {
                ASSERT_FALSE(my_deque.empty());
                EXPECT_EQ(std_deque.front(), my_deque.front());
                EXPECT_EQ(std_deque.back(), my_deque.back());
                size_t rand_idx = rand() % std_deque.size();
                EXPECT_EQ(std_deque.at(rand_idx), my_deque.at(rand_idx));
                EXPECT_EQ(std_deque[rand_idx], my_deque[rand_idx]);
            } else {
                ASSERT_TRUE(my_deque.empty());
            }
        } else if (op == 5) { // size/empty
            EXPECT_EQ(std_deque.empty(), my_deque.empty());
            EXPECT_EQ(std_deque.size(), my_deque.size());
        }

        // Verify contents periodically or at the end
        if (i % 100 == 0 || i == num_operations -1) {
            ASSERT_EQ(std_deque.size(), my_deque.size());
            if (!std_deque.empty()){
                 for(size_t k=0; k<std_deque.size(); ++k) {
                    EXPECT_EQ(std_deque[k], my_deque[k]) << "Mismatch at index " << k << " during operation " << i;
                }
            }
        }
    }
}

// Test with std::string
TEST_F(DequeTest, StringOperations) {
    d_str.push_back("hello");
    d_str.push_front("world"); // world hello
    EXPECT_EQ(d_str.size(), 2);
    EXPECT_EQ(d_str.front(), "world");
    EXPECT_EQ(d_str.back(), "hello");

    d_str.pop_back(); // world
    EXPECT_EQ(d_str.front(), "world");
    d_str.pop_front(); // empty
    EXPECT_TRUE(d_str.empty());

    Deque<std::string> d_str_init = {"a", "b", "c"};
    EXPECT_EQ(d_str_init.size(), 3);
    EXPECT_EQ(d_str_init[1], "b");
}

// Test that iterators are invalidated correctly (conceptual, hard to test directly without knowing internals)
// This test mainly checks that operations that *should* invalidate iterators don't cause crashes
// when iterators are used *after* such operations (if they were not re-obtained).
// For a robust deque, iterators are generally invalidated by any modification.
TEST_F(DequeTest, IteratorInvalidationConceptual) {
    d_int.push_back(1);
    d_int.push_back(2);
    d_int.push_back(3);

    auto it_begin = d_int.begin();
    auto it_middle = d_int.begin() + 1;
    auto it_end = d_int.end();

    EXPECT_EQ(*it_begin, 1);
    EXPECT_EQ(*it_middle, 2);

    // Operations that might invalidate iterators
    d_int.push_back(4); // Likely invalidates all iterators if resize occurs
    // It's unsafe to use old it_begin, it_middle, it_end here without reassigning
    // This test doesn't assert their old values, but checks that subsequent operations work.

    it_begin = d_int.begin(); // Re-obtain iterator
    EXPECT_EQ(*it_begin, 1);

    d_int.push_front(0); // Likely invalidates
    it_begin = d_int.begin(); // Re-obtain
    EXPECT_EQ(*it_begin, 0);

    d_int.pop_front(); // Likely invalidates
    it_begin = d_int.begin(); // Re-obtain
    EXPECT_EQ(*it_begin, 1);

    d_int.pop_back(); // Likely invalidates
    it_begin = d_int.begin(); // Re-obtain
    it_middle = d_int.begin() +1;
    // d_int should be {1, 2}
    EXPECT_EQ(*it_begin, 1);
    EXPECT_EQ(*it_middle, 2);

    d_int.clear(); // Invalidates
    it_begin = d_int.begin();
    it_end = d_int.end();
    EXPECT_EQ(it_begin, it_end); // Should be empty
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

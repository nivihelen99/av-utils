#include "round_robin_queue.h"
#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <memory>

// Test utilities
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "FAIL: " << message << std::endl; \
            return false; \
        } \
    } while(0)

#define RUN_TEST(test_func) \
    do { \
        std::cout << "Running " << #test_func << "... "; \
        if (test_func()) { \
            std::cout << "PASS" << std::endl; \
        } else { \
            std::cout << "FAIL" << std::endl; \
            return 1; \
        } \
    } while(0)

// Test basic functionality
bool test_basic_operations() {
    RoundRobinQueue<int> rr;
    
    // Test empty queue
    TEST_ASSERT(rr.empty(), "New queue should be empty");
    TEST_ASSERT(rr.size() == 0, "New queue should have size 0");
    
    // Test enqueue
    rr.enqueue(10);
    rr.enqueue(20);
    rr.enqueue(30);
    
    TEST_ASSERT(!rr.empty(), "Queue should not be empty after enqueue");
    TEST_ASSERT(rr.size() == 3, "Queue should have size 3");
    
    return true;
}

// Test round-robin behavior
bool test_round_robin_access() {
    RoundRobinQueue<std::string> rr;
    rr.enqueue("A");
    rr.enqueue("B");
    rr.enqueue("C");
    
    // Test circular access
    TEST_ASSERT(rr.next() == "A", "First next() should return A");
    TEST_ASSERT(rr.next() == "B", "Second next() should return B");
    TEST_ASSERT(rr.next() == "C", "Third next() should return C");
    TEST_ASSERT(rr.next() == "A", "Fourth next() should wrap to A");
    TEST_ASSERT(rr.next() == "B", "Fifth next() should return B");
    
    return true;
}

// Test peek functionality
bool test_peek() {
    RoundRobinQueue<int> rr;
    rr.enqueue(100);
    rr.enqueue(200);
    
    // Test peek doesn't advance
    TEST_ASSERT(rr.peek() == 100, "Peek should return first element");
    TEST_ASSERT(rr.peek() == 100, "Multiple peeks should return same element");
    
    // Test peek vs next
    TEST_ASSERT(rr.next() == 100, "Next should return same as peek");
    TEST_ASSERT(rr.peek() == 200, "Peek should now return second element");
    
    return true;
}

// Test skip functionality
bool test_skip() {
    RoundRobinQueue<char> rr;
    rr.enqueue('X');
    rr.enqueue('Y');
    rr.enqueue('Z');
    
    // Skip first element
    TEST_ASSERT(rr.peek() == 'X', "Should start at X");
    rr.skip();
    TEST_ASSERT(rr.size() == 2, "Size should be 2 after skip");
    TEST_ASSERT(rr.peek() == 'Y', "Should now be at Y");
    
    // Skip in middle of cycle
    TEST_ASSERT(rr.next() == 'Y', "Next should return Y");
    TEST_ASSERT(rr.peek() == 'Z', "Should now be at Z");
    rr.skip();
    TEST_ASSERT(rr.size() == 1, "Size should be 1 after second skip");
    TEST_ASSERT(rr.peek() == 'Y', "Should wrap back to Y");
    
    return true;
}

// Test reset functionality
bool test_reset() {
    RoundRobinQueue<int> rr;
    rr.enqueue(1);
    rr.enqueue(2);
    rr.enqueue(3);
    
    // Advance pointer
    rr.next(); // 1
    rr.next(); // 2
    TEST_ASSERT(rr.peek() == 3, "Should be at position 2 (value 3)");
    
    // Reset and verify
    rr.reset();
    TEST_ASSERT(rr.peek() == 1, "After reset should be at position 0 (value 1)");
    
    return true;
}

// Test clear functionality
bool test_clear() {
    RoundRobinQueue<std::string> rr;
    rr.enqueue("test1");
    rr.enqueue("test2");
    
    TEST_ASSERT(rr.size() == 2, "Should have 2 elements before clear");
    
    rr.clear();
    TEST_ASSERT(rr.empty(), "Should be empty after clear");
    TEST_ASSERT(rr.size() == 0, "Size should be 0 after clear");
    
    return true;
}

// Test insert_front functionality
bool test_insert_front() {
    RoundRobinQueue<int> rr;
    rr.enqueue(2);
    rr.enqueue(3);
    
    rr.insert_front(1);
    TEST_ASSERT(rr.size() == 3, "Size should be 3 after insert_front");
    TEST_ASSERT(rr.peek() == 1, "Peek should return the front-inserted element");
    
    // Test round-robin order
    TEST_ASSERT(rr.next() == 1, "First next() should return 1");
    TEST_ASSERT(rr.next() == 2, "Second next() should return 2");
    TEST_ASSERT(rr.next() == 3, "Third next() should return 3");
    
    return true;
}

// Test for_each functionality
bool test_for_each() {
    RoundRobinQueue<int> rr;
    rr.enqueue(10);
    rr.enqueue(20);
    rr.enqueue(30);
    
    // Advance to middle
    rr.next(); // Skip 10
    
    std::vector<int> visited;
    rr.for_each([&visited](const int& val) {
        visited.push_back(val);
    });
    
    // Should visit in round-robin order starting from current position
    TEST_ASSERT(visited.size() == 3, "Should visit all 3 elements");
    TEST_ASSERT(visited[0] == 20, "First visit should be 20 (current position)");
    TEST_ASSERT(visited[1] == 30, "Second visit should be 30");
    TEST_ASSERT(visited[2] == 10, "Third visit should be 10 (wrapped around)");
    
    return true;
}

// Test exception handling
bool test_exceptions() {
    RoundRobinQueue<int> rr;
    
    bool caught_peek = false;
    try {
        rr.peek();
    } catch (const std::runtime_error&) {
        caught_peek = true;
    }
    TEST_ASSERT(caught_peek, "peek() on empty queue should throw");
    
    bool caught_next = false;
    try {
        rr.next();
    } catch (const std::runtime_error&) {
        caught_next = true;
    }
    TEST_ASSERT(caught_next, "next() on empty queue should throw");
    
    bool caught_skip = false;
    try {
        rr.skip();
    } catch (const std::runtime_error&) {
        caught_skip = true;
    }
    TEST_ASSERT(caught_skip, "skip() on empty queue should throw");
    
    return true;
}

// Test with smart pointers
bool test_smart_pointers() {
    RoundRobinQueue<std::shared_ptr<int>> rr;
    
    rr.enqueue(std::make_shared<int>(42));
    rr.enqueue(std::make_shared<int>(84));
    
    auto ptr1 = rr.next();
    TEST_ASSERT(*ptr1 == 42, "Smart pointer should contain correct value");
    
    auto ptr2 = rr.peek();
    TEST_ASSERT(*ptr2 == 84, "Second smart pointer should contain correct value");
    
    return true;
}

// Performance test for large datasets
bool test_performance() {
    RoundRobinQueue<int> rr;
    const int N = 100000;
    
    // Enqueue many elements
    for (int i = 0; i < N; ++i) {
        rr.enqueue(i);
    }
    
    TEST_ASSERT(rr.size() == N, "Should have correct size after bulk enqueue");
    
    // Access elements many times
    int sum = 0;
    for (int i = 0; i < N * 3; ++i) {
        sum += rr.next();
    }
    
    // Verify we got expected pattern (each number appears 3 times)
    int expected_sum = 3 * (N * (N - 1) / 2);
    TEST_ASSERT(sum == expected_sum, "Sum should match expected pattern");
    
    return true;
}

// Practical use case test: Load balancing
bool test_load_balancer() {
    struct Server {
        std::string name;
        int load = 0;
        Server(const std::string& n) : name(n) {}
    };
    
    RoundRobinQueue<Server> servers;
    servers.enqueue(Server("Server1"));
    servers.enqueue(Server("Server2"));
    servers.enqueue(Server("Server3"));
    
    // Simulate 15 requests
    for (int i = 0; i < 15; ++i) {
        Server& server = servers.next();
        server.load++;
    }
    
    // Each server should have 5 requests
    servers.for_each([](const Server& s) {
        assert(s.load == 5); // Each server should have equal load
    });
    
    return true;
}

int main() {
    std::cout << "=== RoundRobinQueue Test Suite ===" << std::endl;
    
    RUN_TEST(test_basic_operations);
    RUN_TEST(test_round_robin_access);
    RUN_TEST(test_peek);
    RUN_TEST(test_skip);
    RUN_TEST(test_reset);
    RUN_TEST(test_clear);
    RUN_TEST(test_insert_front);
    RUN_TEST(test_for_each);
    RUN_TEST(test_exceptions);
    RUN_TEST(test_smart_pointers);
    RUN_TEST(test_performance);
    RUN_TEST(test_load_balancer);
    
    std::cout << "\n=== All Tests Passed! ===" << std::endl;
    
    // Demonstrate practical usage
    std::cout << "\n=== Practical Usage Example ===" << std::endl;
    
    RoundRobinQueue<std::string> tasks;
    tasks.enqueue("Process emails");
    tasks.enqueue("Update database");
    tasks.enqueue("Generate reports");
    tasks.enqueue("Backup files");
    
    std::cout << "Task scheduler (round-robin):" << std::endl;
    for (int i = 0; i < 10; ++i) {
        std::cout << "Cycle " << i + 1 << ": " << tasks.next() << std::endl;
    }
    
    return 0;
}

#include "unique_queue.h"
#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <chrono>
#include <memory>

// Simple test framework
#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion failed: " << #condition << " at line " << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while(0)

#define TEST(name) \
    void test_##name(); \
    void test_##name()

// Basic functionality tests
TEST(basic_push_pop) {
    UniqueQueue<int> q;
    
    // Test empty queue
    ASSERT(q.empty());
    ASSERT(q.size() == 0);
    
    // Test push
    ASSERT(q.push(1) == true);  // First insertion
    ASSERT(q.push(1) == false); // Duplicate
    ASSERT(q.push(2) == true);  // New element
    
    ASSERT(!q.empty());
    ASSERT(q.size() == 2);
    ASSERT(q.contains(1));
    ASSERT(q.contains(2));
    ASSERT(!q.contains(3));
    
    // Test FIFO order
    ASSERT(q.front() == 1);
    ASSERT(q.pop() == 1);
    ASSERT(q.front() == 2);
    ASSERT(q.pop() == 2);
    
    ASSERT(q.empty());
    ASSERT(q.size() == 0);
}

TEST(try_pop) {
    UniqueQueue<std::string> q;
    
    // Empty queue
    auto result = q.try_pop();
    ASSERT(!result.has_value());
    
    // Non-empty queue
    q.push("hello");
    result = q.try_pop();
    ASSERT(result.has_value());
    ASSERT(result.value() == "hello");
    
    // Empty again
    result = q.try_pop();
    ASSERT(!result.has_value());
}

TEST(move_semantics) {
    UniqueQueue<std::unique_ptr<int>> q;
    
    auto ptr1 = std::make_unique<int>(42);
    auto ptr2 = std::make_unique<int>(42); // Same value, different object
    
    // Move unique_ptr into queue
    ASSERT(q.push(std::move(ptr1)) == true);
    ASSERT(ptr1 == nullptr); // Should be moved
    
    // Try to push another with same value
    ASSERT(q.push(std::move(ptr2)) == false); // Duplicate value
    ASSERT(ptr2 != nullptr); // Should not be moved since it's a duplicate
    
    ASSERT(q.size() == 1);
    auto retrieved = q.pop();
    ASSERT(*retrieved == 42);
}

TEST(remove_operation) {
    UniqueQueue<int> q;
    
    // Add elements
    q.push(1);
    q.push(2);
    q.push(3);
    q.push(4);
    
    // Remove middle element
    ASSERT(q.remove(3) == true);
    ASSERT(q.remove(3) == false); // Already removed
    ASSERT(q.size() == 3);
    ASSERT(!q.contains(3));
    
    // Verify order is maintained
    ASSERT(q.pop() == 1);
    ASSERT(q.pop() == 2);
    ASSERT(q.pop() == 4);
}

TEST(clear_operation) {
    UniqueQueue<int> q;
    
    q.push(1);
    q.push(2);
    q.push(3);
    
    ASSERT(q.size() == 3);
    q.clear();
    ASSERT(q.empty());
    ASSERT(q.size() == 0);
    
    // Should be able to reuse after clear
    ASSERT(q.push(1) == true);
    ASSERT(q.size() == 1);
}

TEST(iterator_support) {
    UniqueQueue<int> q;
    
    std::vector<int> expected = {1, 2, 3, 4};
    for (int i : expected) {
        q.push(i);
    }
    
    // Test range-based for loop
    std::vector<int> actual;
    for (const auto& item : q) {
        actual.push_back(item);
    }
    
    ASSERT(actual == expected);
}

TEST(copy_and_assignment) {
    UniqueQueue<int> q1;
    q1.push(1);
    q1.push(2);
    q1.push(3);
    
    // Copy constructor
    UniqueQueue<int> q2(q1);
    ASSERT(q2.size() == 3);
    ASSERT(q2.contains(1));
    ASSERT(q2.contains(2));
    ASSERT(q2.contains(3));
    
    // Assignment operator
    UniqueQueue<int> q3;
    q3 = q1;
    ASSERT(q3.size() == 3);
    
    // Verify independence
    q1.pop();
    ASSERT(q1.size() == 2);
    ASSERT(q2.size() == 3);
    ASSERT(q3.size() == 3);
}

TEST(custom_hash_equal) {
    // Custom string hash that ignores case
    struct CaseInsensitiveHash {
        std::size_t operator()(const std::string& s) const {
            std::string lower_s = s;
            std::transform(lower_s.begin(), lower_s.end(), lower_s.begin(), ::tolower);
            return std::hash<std::string>{}(lower_s);
        }
    };
    
    struct CaseInsensitiveEqual {
        bool operator()(const std::string& lhs, const std::string& rhs) const {
            return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                [](char a, char b) { return ::tolower(a) == ::tolower(b); });
        }
    };
    
    UniqueQueue<std::string, CaseInsensitiveHash, CaseInsensitiveEqual> q;
    
    ASSERT(q.push("Hello") == true);
    ASSERT(q.push("HELLO") == false); // Should be treated as duplicate
    ASSERT(q.push("hello") == false); // Should be treated as duplicate
    ASSERT(q.push("World") == true);
    
    ASSERT(q.size() == 2);
}

// Usage examples from the requirements
void example_task_queue() {
    std::cout << "\n=== Task Queue Example ===\n";
    
    using TaskID = int;
    UniqueQueue<TaskID> worklist;
    
    worklist.push(42);     // ok
    worklist.push(42);     // skipped (duplicate)
    worklist.push(43);     // ok
    
    std::cout << "Tasks to process:\n";
    while (!worklist.empty()) {
        std::cout << "Processing task " << worklist.pop() << "\n";
    }
}

void example_graph_traversal() {
    std::cout << "\n=== Graph Traversal Example ===\n";
    
    // Simulate a simple graph with integer node IDs
    struct Graph {
        std::unordered_map<int, std::vector<int>> adjacency_list = {
            {1, {2, 3}},
            {2, {1, 4}},
            {3, {1, 4}},
            {4, {2, 3}}
        };
        
        std::vector<int> get_neighbors(int node) const {
            auto it = adjacency_list.find(node);
            return (it != adjacency_list.end()) ? it->second : std::vector<int>{};
        }
    };
    
    Graph graph;
    UniqueQueue<int> to_visit;
    std::vector<int> visited_order;
    
    to_visit.push(1); // Start from node 1
    
    while (!to_visit.empty()) {
        int current = to_visit.pop();
        visited_order.push_back(current);
        
        std::cout << "Visiting node " << current << "\n";
        
        for (int neighbor : graph.get_neighbors(current)) {
            to_visit.push(neighbor); // Duplicates automatically ignored
        }
    }
    
    std::cout << "Visited nodes in order: ";
    for (int node : visited_order) {
        std::cout << node << " ";
    }
    std::cout << "\n";
}

void example_message_deduplication() {
    std::cout << "\n=== Message Deduplication Example ===\n";
    
    struct Message {
        int id;
        std::string content;
        
        bool operator==(const Message& other) const {
            return id == other.id; // Deduplicate by ID
        }
    };
    
    struct MessageHash {
        std::size_t operator()(const Message& msg) const {
            return std::hash<int>{}(msg.id);
        }
    };
    
    UniqueQueue<Message, MessageHash> message_queue;
    
    // Simulate receiving messages
    std::vector<Message> incoming = {
        {1, "Hello"},
        {2, "World"},
        {1, "Hello (duplicate)"}, // Same ID, should be skipped
        {3, "Test"},
        {2, "World (duplicate)"}  // Same ID, should be skipped
    };
    
    for (const auto& msg : incoming) {
        bool added = message_queue.push(msg);
        std::cout << "Message " << msg.id << " (" << msg.content << "): " 
                  << (added ? "Added" : "Skipped (duplicate)") << "\n";
    }
    
    std::cout << "\nProcessing unique messages:\n";
    while (!message_queue.empty()) {
        Message msg = message_queue.pop();
        std::cout << "Processing message " << msg.id << ": " << msg.content << "\n";
    }
}

void performance_test() {
    std::cout << "\n=== Performance Test ===\n";
    
    const int N = 100000;
    UniqueQueue<int> q;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Insert many elements with some duplicates
    for (int i = 0; i < N; ++i) {
        q.push(i % (N/2)); // Creates duplicates
    }
    
    auto mid = std::chrono::high_resolution_clock::now();
    
    // Pop all elements
    int count = 0;
    while (!q.empty()) {
        q.pop();
        ++count;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    
    auto insert_time = std::chrono::duration_cast<std::chrono::milliseconds>(mid - start);
    auto pop_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - mid);
    
    std::cout << "Inserted " << N << " elements (with duplicates) in " 
              << insert_time.count() << "ms\n";
    std::cout << "Popped " << count << " unique elements in " 
              << pop_time.count() << "ms\n";
    std::cout << "Duplicate rejection rate: " 
              << (100.0 * (N - count) / N) << "%\n";
}

int main() {
    std::cout << "Running UniqueQueue tests...\n";
    
    // Run all tests
    test_basic_push_pop();
    test_try_pop();
    test_move_semantics();
    test_remove_operation();
    test_clear_operation();
    test_iterator_support();
    test_copy_and_assignment();
    test_custom_hash_equal();
    
    std::cout << "All tests passed!\n";
    
    // Run examples
    example_task_queue();
    example_graph_traversal();
    example_message_deduplication();
    performance_test();
    
    std::cout << "\nAll examples completed successfully!\n";
    return 0;
}

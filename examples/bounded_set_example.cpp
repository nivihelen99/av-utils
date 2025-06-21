#include "bounded_set.h"
#include <iostream>
#include <string>
#include <cassert>

void test_basic_functionality() {
    std::cout << "Testing basic functionality...\n";
    
    BoundedSet<int> s(3);
    
    // Test initial state
    assert(s.size() == 0);
    assert(s.capacity() == 3);
    assert(s.empty());
    
    // Test insertion
    assert(s.insert(10) == true);   // [10]
    assert(s.insert(20) == true);   // [10, 20]
    assert(s.insert(30) == true);   // [10, 20, 30]
    
    assert(s.size() == 3);
    assert(!s.empty());
    
    // Test duplicate insertion
    assert(s.insert(20) == false); // No change: [10, 20, 30]
    assert(s.size() == 3);
    
    // Test eviction
    assert(s.insert(40) == true);   // [20, 30, 40] → evicted 10
    assert(s.size() == 3);
    
    // Test contains
    assert(!s.contains(10));  // evicted
    assert(s.contains(20));
    assert(s.contains(30));
    assert(s.contains(40));
    
    std::cout << "✓ Basic functionality tests passed\n";
}

void test_front_back_access() {
    std::cout << "Testing front/back access...\n";
    
    BoundedSet<int> s(3);
    s.insert(10);
    s.insert(20);
    s.insert(30);
    
    assert(s.front() == 10);  // oldest
    assert(s.back() == 30);   // newest
    
    s.insert(40);  // evicts 10
    assert(s.front() == 20);  // new oldest
    assert(s.back() == 40);   // newest
    
    std::cout << "✓ Front/back access tests passed\n";
}

void test_iteration() {
    std::cout << "Testing iteration...\n";
    
    BoundedSet<int> s(4);
    s.insert(10);
    s.insert(20);
    s.insert(30);
    s.insert(40);
    
    std::vector<int> expected = {10, 20, 30, 40};
    std::vector<int> actual;
    
    for (const auto& val : s) {
        actual.push_back(val);
    }
    
    assert(actual == expected);
    
    // Test as_vector
    auto vec = s.as_vector();
    assert(vec == expected);
    
    std::cout << "✓ Iteration tests passed\n";
}

void test_erase() {
    std::cout << "Testing erase functionality...\n";
    
    BoundedSet<int> s(3);
    s.insert(10);
    s.insert(20);
    s.insert(30);
    
    // Erase middle element
    assert(s.erase(20) == true);
    assert(s.size() == 2);
    assert(!s.contains(20));
    assert(s.contains(10));
    assert(s.contains(30));
    
    // Erase non-existent element
    assert(s.erase(99) == false);
    assert(s.size() == 2);
    
    // Erase all
    s.clear();
    assert(s.size() == 0);
    assert(s.empty());
    
    std::cout << "✓ Erase tests passed\n";
}

void test_capacity_changes() {
    std::cout << "Testing capacity changes...\n";
    
    BoundedSet<int> s(5);
    for (int i = 1; i <= 5; ++i) {
        s.insert(i);
    }
    assert(s.size() == 5);
    
    // Reduce capacity - should evict oldest elements
    s.reserve(3);
    assert(s.capacity() == 3);
    assert(s.size() == 3);
    assert(!s.contains(1));  // evicted
    assert(!s.contains(2));  // evicted
    assert(s.contains(3));
    assert(s.contains(4));
    assert(s.contains(5));
    
    // Increase capacity
    s.reserve(6);
    assert(s.capacity() == 6);
    assert(s.size() == 3);  // size unchanged
    
    std::cout << "✓ Capacity change tests passed\n";
}

void test_string_elements() {
    std::cout << "Testing with string elements...\n";
    
    BoundedSet<std::string> dns_cache(3);
    
    dns_cache.insert("google.com");
    dns_cache.insert("github.com");
    dns_cache.insert("stackoverflow.com");
    
    assert(dns_cache.contains("google.com"));
    
    dns_cache.insert("reddit.com");  // evicts google.com
    assert(!dns_cache.contains("google.com"));
    assert(dns_cache.contains("reddit.com"));
    
    std::cout << "✓ String element tests passed\n";
}

void test_use_case_examples() {
    std::cout << "Testing real-world use cases...\n";
    
    // Use case 1: Recent address tracking
    BoundedSet<std::string> recent_hosts(1024);
    recent_hosts.insert("10.0.0.1");
    recent_hosts.insert("10.0.0.2");
    assert(recent_hosts.contains("10.0.0.1"));
    
    // Use case 2: DNS Cache deduplication
    BoundedSet<std::string> recent_queries(500);
    auto query = "example.com";
    if (!recent_queries.contains(query)) {
        // send_query(query);  // Would send query in real code
        recent_queries.insert(query);
    }
    assert(recent_queries.contains(query));
    
    // Use case 3: Loop detection
    BoundedSet<int> recent_ids(100);
    int pkt_id = 12345;
    if (recent_ids.contains(pkt_id)) {
        // drop(pkt);  // Would drop packet in real code
        assert(false);  // Should not reach here in this test
    } else {
        recent_ids.insert(pkt_id);
    }
    assert(recent_ids.contains(pkt_id));
    
    std::cout << "✓ Use case examples passed\n";
}

void test_edge_cases() {
    std::cout << "Testing edge cases...\n";
    
    // Test capacity of 1
    BoundedSet<int> s1(1);
    s1.insert(10);
    assert(s1.size() == 1);
    s1.insert(20);  // evicts 10
    assert(s1.size() == 1);
    assert(!s1.contains(10));
    assert(s1.contains(20));
    
    // Test empty set operations
    BoundedSet<int> empty_set(5);
    assert(!empty_set.contains(1));
    assert(!empty_set.erase(1));
    
    try {
        empty_set.front();
        assert(false);  // Should throw
    } catch (const std::runtime_error&) {
        // Expected
    }
    
    try {
        empty_set.back();
        assert(false);  // Should throw
    } catch (const std::runtime_error&) {
        // Expected
    }
    
    std::cout << "✓ Edge case tests passed\n";
}

void print_set_state(const BoundedSet<int>& s, const std::string& label) {
    std::cout << label << ": [";
    bool first = true;
    for (const auto& val : s) {
        if (!first) std::cout << ", ";
        std::cout << val;
        first = false;
    }
    std::cout << "] (size: " << s.size() << "/" << s.capacity() << ")\n";
}

void demo_bounded_set() {
    std::cout << "\n=== BoundedSet Demo ===\n";
    
    BoundedSet<int> s(3);
    print_set_state(s, "Initial");
    
    s.insert(10);
    print_set_state(s, "After insert(10)");
    
    s.insert(20);
    print_set_state(s, "After insert(20)");
    
    s.insert(30);
    print_set_state(s, "After insert(30)");
    
    s.insert(40);  // This will evict 10
    print_set_state(s, "After insert(40) - evicted oldest");
    
    s.insert(20);  // Duplicate - no change
    print_set_state(s, "After insert(20) - duplicate");
    
    s.erase(30);
    print_set_state(s, "After erase(30)");
    
    std::cout << "Contains 10: " << (s.contains(10) ? "yes" : "no") << "\n";
    std::cout << "Contains 40: " << (s.contains(40) ? "yes" : "no") << "\n";
}

int main() {
    std::cout << "Running BoundedSet tests...\n\n";
    
    try {
        test_basic_functionality();
        test_front_back_access();
        test_iteration();
        test_erase();
        test_capacity_changes();
        test_string_elements();
        test_use_case_examples();
        test_edge_cases();
        
        demo_bounded_set();
        
        std::cout << "\n🎉 All tests passed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

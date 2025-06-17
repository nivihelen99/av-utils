#include "sorted_list_bisect.h"
#include <iostream>
#include <cassert>
#include <string>
#include <chrono>

// Test helper macros
#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion failed: " << #condition << " at line " << __LINE__ << std::endl; \
            std::abort(); \
        } \
    } while (0)

#define TEST_SECTION(name) \
    std::cout << "\n=== Testing " << name << " ===" << std::endl

void test_basic_operations() {
    TEST_SECTION("Basic Operations");
    
    SortedList<int> sl;
    
    // Test empty state
    ASSERT(sl.empty());
    ASSERT(sl.size() == 0);
    
    // Test insertion and ordering
    sl.insert(10);
    sl.insert(5);
    sl.insert(20);
    sl.insert(15);
    
    ASSERT(!sl.empty());
    ASSERT(sl.size() == 4);
    
    // Verify sorted order
    ASSERT(sl[0] == 5);
    ASSERT(sl[1] == 10);
    ASSERT(sl[2] == 15);
    ASSERT(sl[3] == 20);
    
    // Test at() method with bounds checking
    ASSERT(sl.at(0) == 5);
    ASSERT(sl.at(3) == 20);
    
    try {
        sl.at(4); // Should throw
        ASSERT(false); // Should not reach here
    } catch (const std::out_of_range&) {
        // Expected
    }
    
    std::cout << "Basic operations: PASSED" << std::endl;
}

void test_duplicates() {
    TEST_SECTION("Duplicate Values");
    
    SortedList<int> sl;
    sl.insert(10);
    sl.insert(5);
    sl.insert(10);
    sl.insert(10);
    sl.insert(5);
    
    ASSERT(sl.size() == 5);
    ASSERT(sl[0] == 5);
    ASSERT(sl[1] == 5);
    ASSERT(sl[2] == 10);
    ASSERT(sl[3] == 10);
    ASSERT(sl[4] == 10);
    
    // Test count
    ASSERT(sl.count(5) == 2);
    ASSERT(sl.count(10) == 3);
    ASSERT(sl.count(99) == 0);
    
    std::cout << "Duplicate values: PASSED" << std::endl;
}

void test_search_operations() {
    TEST_SECTION("Search Operations");
    
    SortedList<int> sl;
    for (int val : {1, 3, 3, 5, 7, 7, 7, 9}) {
        sl.insert(val);
    }
    
    // Test contains
    ASSERT(sl.contains(3));
    ASSERT(sl.contains(7));
    ASSERT(!sl.contains(4));
    ASSERT(!sl.contains(0));
    
    // Test lower_bound
    ASSERT(sl.lower_bound(3) == 1);  // First 3 is at index 1
    ASSERT(sl.lower_bound(7) == 3);  // First 7 is at index 3
    ASSERT(sl.lower_bound(4) == 3);  // Would be inserted at index 3
    ASSERT(sl.lower_bound(0) == 0);  // Would be inserted at beginning
    ASSERT(sl.lower_bound(10) == 8); // Would be inserted at end
    
    // Test upper_bound
    ASSERT(sl.upper_bound(3) == 3);  // After last 3
    ASSERT(sl.upper_bound(7) == 6);  // After last 7
    ASSERT(sl.upper_bound(4) == 3);  // Same as lower_bound for non-existing
    
    // Test index_of
    ASSERT(sl.index_of(3) == 1);
    ASSERT(sl.index_of(7) == 3);
    
    try {
        sl.index_of(4); // Should throw
        ASSERT(false);
    } catch (const std::runtime_error&) {
        // Expected
    }
    
    std::cout << "Search operations: PASSED" << std::endl;
}

void test_deletion() {
    TEST_SECTION("Deletion Operations");
    
    SortedList<int> sl;
    for (int val : {1, 3, 3, 5, 7, 7, 7, 9}) {
        sl.insert(val);
    }
    
    size_t original_size = sl.size();
    
    // Test erase by value
    ASSERT(sl.erase(3)); // Should remove first occurrence
    ASSERT(sl.size() == original_size - 1);
    ASSERT(sl.count(3) == 1); // One 3 should remain
    
    ASSERT(!sl.erase(99)); // Non-existing value
    ASSERT(sl.size() == original_size - 1);
    
    // Test erase_at
    size_t index_of_7 = sl.index_of(7);
    sl.erase_at(index_of_7);
    ASSERT(sl.count(7) == 2); // One 7 should be removed
    
    try {
        sl.erase_at(sl.size()); // Should throw
        ASSERT(false);
    } catch (const std::out_of_range&) {
        // Expected
    }
    
    std::cout << "Deletion operations: PASSED" << std::endl;
}

void test_range_operations() {
    TEST_SECTION("Range Operations");
    
    SortedList<int> sl;
    for (int i = 0; i < 20; i += 2) {
        sl.insert(i); // 0, 2, 4, 6, 8, 10, 12, 14, 16, 18
    }
    
    // Test range extraction
    auto range = sl.range(4, 12);
    std::vector<int> expected = {4, 6, 8, 10};
    ASSERT(range == expected);
    
    // Test range_indices
    auto indices = sl.range_indices(4, 12);
    ASSERT(indices.first == 2);  // Index of 4
    ASSERT(indices.second == 6); // Index where 12 would be
    
    // Empty range
    auto empty_range = sl.range(25, 30);
    ASSERT(empty_range.empty());
    
    std::cout << "Range operations: PASSED" << std::endl;
}

void test_custom_comparator() {
    TEST_SECTION("Custom Comparator");
    
    // Reverse order comparator
    SortedList<int, std::greater<int>> sl;
    
    sl.insert(10);
    sl.insert(5);
    sl.insert(20);
    sl.insert(15);
    
    // Should be in descending order
    ASSERT(sl[0] == 20);
    ASSERT(sl[1] == 15);
    ASSERT(sl[2] == 10);
    ASSERT(sl[3] == 5);
    
    // Test with strings (case-insensitive)
    auto case_insensitive = [](const std::string& a, const std::string& b) {
        return std::lexicographical_compare(
            a.begin(), a.end(),
            b.begin(), b.end(),
            [](char c1, char c2) {
                return std::tolower(c1) < std::tolower(c2);
            }
        );
    };
    
    SortedList<std::string, decltype(case_insensitive)> str_list(case_insensitive);
    str_list.insert("apple");
    str_list.insert("Apple");
    str_list.insert("BANANA");
    str_list.insert("banana");
    
    // Should be sorted case-insensitively
    ASSERT(str_list.size() == 4);
    // Exact order depends on stable sort, but apples should come before bananas
    ASSERT(str_list[0].substr(0, 5) == "apple" || str_list[0].substr(0, 5) == "Apple");
    ASSERT(str_list[2].substr(0, 6) == "BANANA" || str_list[2].substr(0, 6) == "banana");
    
    std::cout << "Custom comparator: PASSED" << std::endl;
}

void test_iterators() {
    TEST_SECTION("Iterator Support");
    
    SortedList<int> sl;
    for (int val : {5, 2, 8, 1, 9, 3}) {
        sl.insert(val);
    }
    
    // Test forward iteration
    std::vector<int> forward_result;
    for (auto it = sl.begin(); it != sl.end(); ++it) {
        forward_result.push_back(*it);
    }
    std::vector<int> expected_forward = {1, 2, 3, 5, 8, 9};
    ASSERT(forward_result == expected_forward);
    
    // Test range-based for loop
    std::vector<int> range_result;
    for (const auto& val : sl) {
        range_result.push_back(val);
    }
    ASSERT(range_result == expected_forward);
    
    // Test reverse iteration
    std::vector<int> reverse_result;
    for (auto it = sl.rbegin(); it != sl.rend(); ++it) {
        reverse_result.push_back(*it);
    }
    std::vector<int> expected_reverse = {9, 8, 5, 3, 2, 1};
    ASSERT(reverse_result == expected_reverse);
    
    std::cout << "Iterator support: PASSED" << std::endl;
}

void test_edge_cases() {
    TEST_SECTION("Edge Cases");
    
    // Test with empty list
    SortedList<int> empty_sl;
    ASSERT(empty_sl.lower_bound(5) == 0);
    ASSERT(empty_sl.upper_bound(5) == 0);
    ASSERT(!empty_sl.contains(5));
    ASSERT(empty_sl.count(5) == 0);
    ASSERT(empty_sl.range(0, 10).empty());
    
    try {
        empty_sl.front();
        ASSERT(false);
    } catch (const std::runtime_error&) {
        // Expected
    }
    
    try {
        empty_sl.back();
        ASSERT(false);
    } catch (const std::runtime_error&) {
        // Expected
    }
    
    // Test with single element
    SortedList<int> single_sl;
    single_sl.insert(42);
    ASSERT(single_sl.front() == 42);
    ASSERT(single_sl.back() == 42);
    ASSERT(single_sl.size() == 1);
    
    // Test clear
    single_sl.clear();
    ASSERT(single_sl.empty());
    ASSERT(single_sl.size() == 0);
    
    std::cout << "Edge cases: PASSED" << std::endl;
}

void test_initializer_list() {
    TEST_SECTION("Initializer List Constructor");
    
    SortedList<int> sl{10, 5, 20, 15, 5};
    
    ASSERT(sl.size() == 5);
    ASSERT(sl[0] == 5);
    ASSERT(sl[1] == 5);
    ASSERT(sl[2] == 10);
    ASSERT(sl[3] == 15);
    ASSERT(sl[4] == 20);
    
    std::cout << "Initializer list: PASSED" << std::endl;
}

void test_comparison_operators() {
    TEST_SECTION("Comparison Operators");
    
    SortedList<int> sl1{1, 2, 3};
    SortedList<int> sl2{1, 2, 3};
    SortedList<int> sl3{1, 2, 4};
    
    ASSERT(sl1 == sl2);
    ASSERT(!(sl1 != sl2));
    ASSERT(sl1 != sl3);
    ASSERT(sl1 < sl3);
    ASSERT(sl3 > sl1);
    ASSERT(sl1 <= sl2);
    ASSERT(sl1 >= sl2);
    
    std::cout << "Comparison operators: PASSED" << std::endl;
}

void test_move_operations() {
    TEST_SECTION("Move Operations");
    
    SortedList<std::string> sl;
    
    std::string val1 = "hello";
    std::string val2 = "world";
    
    sl.insert(std::move(val1));
    sl.insert(std::move(val2));
    
    ASSERT(sl.size() == 2);
    ASSERT(sl[0] == "hello");
    ASSERT(sl[1] == "world");
    
    // Original strings should be moved from (empty or unspecified state)
    // We can't assert their exact state, but they should be valid for destruction
    
    std::cout << "Move operations: PASSED" << std::endl;
}

void performance_test() {
    TEST_SECTION("Performance Test");
    
    const int N = 10000;
    SortedList<int> sl;
    
    // Reserve capacity to avoid multiple reallocations
    sl.reserve(N);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Insert random values
    for (int i = 0; i < N; ++i) {
        sl.insert(rand() % (N * 2));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Inserted " << N << " elements in " << duration.count() << " microseconds" << std::endl;
    std::cout << "Final size: " << sl.size() << std::endl;
    
    // Verify it's still sorted
    for (size_t i = 1; i < sl.size(); ++i) {
        ASSERT(sl[i-1] <= sl[i]);
    }
    
    // Test search performance
    start = std::chrono::high_resolution_clock::now();
    int found_count = 0;
    for (int i = 0; i < 1000; ++i) {
        if (sl.contains(rand() % (N * 2))) {
            found_count++;
        }
    }
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Performed 1000 searches in " << duration.count() << " microseconds" << std::endl;
    std::cout << "Found " << found_count << " elements" << std::endl;
    
    std::cout << "Performance test: COMPLETED" << std::endl;
}

int main() {
    std::cout << "Running SortedList<T> tests..." << std::endl;
    
    test_basic_operations();
    test_duplicates();
    test_search_operations();
    test_deletion();
    test_range_operations();
    test_custom_comparator();
    test_iterators();
    test_edge_cases();
    test_initializer_list();
    test_comparison_operators();
    test_move_operations();
    performance_test();
    
    std::cout << "\n🎉 All tests passed! SortedList<T> implementation is working correctly." << std::endl;
    
    // Example usage from requirements
    std::cout << "\n=== Example Usage ===" << std::endl;
    SortedList<int> sl;
    sl.insert(10);
    sl.insert(5);
    sl.insert(20);

    std::cout << sl[0] << ", " << sl[1] << ", " << sl[2] << std::endl;  // 5, 10, 20

    if (sl.contains(10)) {
        std::cout << "Found 10 at index " << sl.index_of(10) << std::endl;
    }

    auto subset = sl.range(6, 25);  // returns [10, 20]
    std::cout << "Subset [6, 25): ";
    for (const auto& val : subset) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
    
    return 0;
}

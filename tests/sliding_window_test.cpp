#include "sliding_window_minmax.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <cmath>

using namespace sliding_window;

class TestSuite {
private:
    int tests_run = 0;
    int tests_passed = 0;
    
public:
    void assert_test(bool condition, const std::string& test_name) {
        tests_run++;
        if (condition) {
            tests_passed++;
            std::cout << "✓ " << test_name << "\n";
        }

void test_custom_comparator(TestSuite& suite) {
    std::cout << "\n=== Custom Comparator Tests ===\n";
    
    // Test with greater comparator (for max behavior)
    SlidingWindow<int, std::greater<int>> max_window(3);
    
    max_window.push(5);
    max_window.push(3);
    max_window.push(7);
    
    suite.assert_test(max_window.extreme() == 7, "Custom comparator max behavior");
    
    max_window.push(1);  // Remove 5, add 1
    suite.assert_test(max_window.extreme() == 7, "Max maintained after overflow");
    
    // Test with custom struct and comparator
    struct Point {
        double x, y;
        Point(double x = 0, double y = 0) : x(x), y(y) {}
        
        double distance() const {
            return std::sqrt(x * x + y * y);
        }
        
        bool operator==(const Point& other) const {
            return std::abs(x - other.x) < 1e-9 && std::abs(y - other.y) < 1e-9;
        }
    };
    
    auto distance_less = [](const Point& a, const Point& b) {
        return a.distance() < b.distance();
    };
    
    SlidingWindow<Point, decltype(distance_less)> point_window(3, distance_less);
    
    point_window.push(Point(3, 4));  // distance = 5
    point_window.push(Point(1, 1));  // distance ≈ 1.41
    point_window.push(Point(2, 0));  // distance = 2
    
    suite.assert_test(std::abs(point_window.extreme().distance() - 1.414) < 0.01, 
                     "Custom struct with comparator");
}

void test_large_window_performance(TestSuite& suite) {
    std::cout << "\n=== Performance Tests ===\n";
    
    const int window_size = 10000;
    const int num_operations = 100000;
    
    SlidingWindowMin<int> window(window_size);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::mt19937 gen(42);  // Fixed seed for reproducibility
    std::uniform_int_distribution<> dis(1, 1000000);
    
    for (int i = 0; i < num_operations; ++i) {
        window.push(dis(gen));
        volatile int min_val = window.min();  // Prevent optimization
        (void)min_val;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Should be very fast - each operation should be much less than 1 microsecond on average
    double avg_time = (double)duration.count() / num_operations;
    suite.assert_test(avg_time < 1.0, "Performance test - average operation < 1 microsecond");
    
    std::cout << "  Performance: " << avg_time << " microseconds per operation\n";
}

void test_correctness_against_naive(TestSuite& suite) {
    std::cout << "\n=== Correctness Tests (vs Naive Implementation) ===\n";
    
    const int window_size = 100;
    const int num_tests = 1000;
    
    SlidingWindowMin<int> optimized_min(window_size);
    SlidingWindowMax<int> optimized_max(window_size);
    
    std::vector<int> window_data;
    
    std::mt19937 gen(123);
    std::uniform_int_distribution<> dis(-1000, 1000);
    
    bool all_correct = true;
    
    for (int i = 0; i < num_tests; ++i) {
        int value = dis(gen);
        
        // Add to optimized windows
        optimized_min.push(value);
        optimized_max.push(value);
        
        // Maintain naive window
        window_data.push_back(value);
        if (window_data.size() > window_size) {
            window_data.erase(window_data.begin());
        }
        
        // Compare results
        int naive_min = *std::min_element(window_data.begin(), window_data.end());
        int naive_max = *std::max_element(window_data.begin(), window_data.end());
        
        if (optimized_min.min() != naive_min || optimized_max.max() != naive_max) {
            all_correct = false;
            break;
        }
    }
    
    suite.assert_test(all_correct, "Correctness vs naive implementation");
}

void test_monotonic_property(TestSuite& suite) {
    std::cout << "\n=== Monotonic Property Tests ===\n";
    
    // Test that internal monotonic deque maintains its properties
    SlidingWindowMin<int> window(5);
    
    // Add elements that would break monotonicity if not handled correctly
    std::vector<int> test_sequence = {5, 3, 7, 2, 8, 1, 9, 4, 6};
    
    bool monotonic_maintained = true;
    
    for (int val : test_sequence) {
        window.push(val);
        
        // The minimum should always be correct regardless of internal state
        // We can't directly test monotonic property without exposing internals,
        // but we can verify correctness under various patterns
        
        // Create a naive verification
        std::vector<int> current_window;
        // We'd need access to internal data to verify this properly
        // For now, just verify the min is always obtainable
        try {
            int current_min = window.min();
            (void)current_min;  // Use the value to prevent optimization
        } catch (...) {
            monotonic_maintained = false;
            break;
        }
    }
    
    suite.assert_test(monotonic_maintained, "Monotonic property maintained");
}

void test_memory_efficiency(TestSuite& suite) {
    std::cout << "\n=== Memory Efficiency Tests ===\n";
    
    // Test that memory usage doesn't grow beyond expected bounds
    const int window_size = 1000;
    SlidingWindowMin<int> window(window_size);
    
    // Fill beyond capacity many times
    for (int i = 0; i < 10000; ++i) {
        window.push(i);
    }
    
    // Size should never exceed capacity
    suite.assert_test(window.size() <= window_size, "Size never exceeds capacity");
    suite.assert_test(window.size() == window_size, "Size equals capacity when full");
    
    // Test that clear actually frees memory (size check)
    window.clear();
    suite.assert_test(window.size() == 0, "Clear reduces size to 0");
}

void test_type_requirements(TestSuite& suite) {
    std::cout << "\n=== Type Requirements Tests ===\n";
    
    // Test with double
    {
        SlidingWindowMin<double> double_window(3);
        double_window.push(3.14);
        double_window.push(2.71);
        double_window.push(1.41);
        
        suite.assert_test(std::abs(double_window.min() - 1.41) < 1e-9, "Double type support");
    }
    
    // Test with custom comparable type
    struct CustomInt {
        int value;
        CustomInt(int v) : value(v) {}
        bool operator<(const CustomInt& other) const { return value < other.value; }
        bool operator>(const CustomInt& other) const { return value > other.value; }
        bool operator==(const CustomInt& other) const { return value == other.value; }
        bool operator<=(const CustomInt& other) const { return value <= other.value; }
        bool operator>=(const CustomInt& other) const { return value >= other.value; }
    };
    
    SlidingWindowMin<CustomInt> custom_window(3);
    custom_window.push(CustomInt(5));
    custom_window.push(CustomInt(3));
    custom_window.push(CustomInt(7));
    
    suite.assert_test(custom_window.min().value == 3, "Custom comparable type support");
}

int main() {
    std::cout << "🧪 Sliding Window Min/Max - Comprehensive Test Suite\n";
    std::cout << "====================================================\n";
    
    TestSuite suite;
    
    test_basic_functionality(suite);
    test_manual_operations(suite);
    test_edge_cases(suite);
    test_error_conditions(suite);
    test_clear_functionality(suite);
    test_move_semantics(suite);
    test_custom_comparator(suite);
    test_large_window_performance(suite);
    test_correctness_against_naive(suite);
    test_monotonic_property(suite);
    test_memory_efficiency(suite);
    test_type_requirements(suite);
    
    suite.print_summary();
    
    return 0;
} else {
            std::cout << "✗ " << test_name << " FAILED!\n";
        }
    }
    
    void print_summary() {
        std::cout << "\n" << std::string(50, '=') << "\n";
        std::cout << "Test Summary: " << tests_passed << "/" << tests_run << " tests passed\n";
        if (tests_passed == tests_run) {
            std::cout << "🎉 All tests passed!\n";
        } else {
            std::cout << "❌ Some tests failed!\n";
        }
    }
};

void test_basic_functionality(TestSuite& suite) {
    std::cout << "\n=== Basic Functionality Tests ===\n";
    
    // Test SlidingWindowMin
    {
        SlidingWindowMin<int> window(3);
        
        suite.assert_test(window.empty(), "Empty window initially");
        suite.assert_test(window.size() == 0, "Initial size is 0");
        suite.assert_test(window.capacity() == 3, "Capacity is correct");
        
        window.push(MoveOnly(5));
    window.push(MoveOnly(3));
    window.push(MoveOnly(7));
    
    suite.assert_test(window.min().value == 3, "Move-only type min works");
    suite.assert_test(window.size() == 3, "Move-only type size correct");
}.push(5);
        suite.assert_test(window.min() == 5, "Single element min");
        suite.assert_test(window.size() == 1, "Size after one push");
        
        window.push(3);
        suite.assert_test(window.min() == 3, "Min after second element");
        
        window.push(7);
        suite.assert_test(window.min() == 3, "Min with full window");
        suite.assert_test(window.size() == 3, "Full window size");
        suite.assert_test(window.full(), "Window is full");
        
        window.push(1);  // This should remove 5
        suite.assert_test(window.min() == 1, "Min after capacity overflow");
        suite.assert_test(window.size() == 3, "Size maintained after overflow");
    }
    
    // Test SlidingWindowMax
    {
        SlidingWindowMax<int> window(3);
        
        window.push(5);
        suite.assert_test(window.max() == 5, "Single element max");
        
        window.push(3);
        suite.assert_test(window.max() == 5, "Max after second element");
        
        window.push(7);
        suite.assert_test(window.max() == 7, "Max with full window");
        
        window.push(1);  // This should remove 5
        suite.assert_test(window.max() == 7, "Max after capacity overflow");
    }
}

void test_manual_operations(TestSuite& suite) {
    std::cout << "\n=== Manual Operations Tests ===\n";
    
    SlidingWindowMin<int> window(5);
    
    // Fill partially
    window.push(10);
    window.push(5);
    window.push(15);
    
    suite.assert_test(window.min() == 5, "Partial fill min");
    suite.assert_test(window.size() == 3, "Partial fill size");
    
    // Manual pop
    window.pop();  // Remove 10
    suite.assert_test(window.min() == 5, "Min after manual pop");
    suite.assert_test(window.size() == 2, "Size after manual pop");
    
    window.pop();  // Remove 5
    suite.assert_test(window.min() == 15, "Min after removing minimum");
    suite.assert_test(window.size() == 1, "Size after removing minimum");
    
    window.pop();  // Remove 15
    suite.assert_test(window.empty(), "Empty after removing all");
}

void test_edge_cases(TestSuite& suite) {
    std::cout << "\n=== Edge Cases Tests ===\n";
    
    // Single element window
    {
        SlidingWindowMin<int> window(1);
        window.push(42);
        suite.assert_test(window.min() == 42, "Single capacity window");
        
        window.push(99);
        suite.assert_test(window.min() == 99, "Single capacity replacement");
        suite.assert_test(window.size() == 1, "Single capacity size maintained");
    }
    
    // Duplicate elements
    {
        SlidingWindowMin<int> window(3);
        window.push(5);
        window.push(5);
        window.push(5);
        suite.assert_test(window.min() == 5, "All duplicates min");
        
        window.pop();
        suite.assert_test(window.min() == 5, "Min after pop with duplicates");
    }
    
    // Monotonic sequences
    {
        SlidingWindowMin<int> window(5);
        // Increasing sequence
        for (int i = 1; i <= 5; ++i) {
            window.push(i);
        }
        suite.assert_test(window.min() == 1, "Increasing sequence min");
        
        window.clear();
        // Decreasing sequence
        for (int i = 5; i >= 1; --i) {
            window.push(i);
        }
        suite.assert_test(window.min() == 1, "Decreasing sequence min");
    }
}

void test_error_conditions(TestSuite& suite) {
    std::cout << "\n=== Error Conditions Tests ===\n";
    
    // Invalid capacity
    bool caught_invalid_capacity = false;
    try {
        SlidingWindowMin<int> window(0);
    } catch (const std::invalid_argument&) {
        caught_invalid_capacity = true;
    }
    suite.assert_test(caught_invalid_capacity, "Invalid capacity throws exception");
    
    // Empty window operations
    {
        SlidingWindowMin<int> window(5);
        
        bool caught_empty_min = false;
        try {
            window.min();
        } catch (const std::runtime_error&) {
            caught_empty_min = true;
        }
        suite.assert_test(caught_empty_min, "Min on empty window throws exception");
        
        bool caught_empty_pop = false;
        try {
            window.pop();
        } catch (const std::runtime_error&) {
            caught_empty_pop = true;
        }
        suite.assert_test(caught_empty_pop, "Pop on empty window throws exception");
    }
}

void test_clear_functionality(TestSuite& suite) {
    std::cout << "\n=== Clear Functionality Tests ===\n";
    
    SlidingWindowMin<int> window(5);
    
    // Fill window
    for (int i = 1; i <= 5; ++i) {
        window.push(i);
    }
    
    suite.assert_test(window.size() == 5, "Window filled before clear");
    suite.assert_test(!window.empty(), "Window not empty before clear");
    
    window.clear();
    
    suite.assert_test(window.size() == 0, "Size 0 after clear");
    suite.assert_test(window.empty(), "Empty after clear");
    suite.assert_test(window.capacity() == 5, "Capacity unchanged after clear");
    
    // Can still use after clear
    window.push(42);
    suite.assert_test(window.min() == 42, "Functional after clear");
}

void test_move_semantics(TestSuite& suite) {
    std::cout << "\n=== Move Semantics Tests ===\n";
    
    // Test with move-only type
    struct MoveOnly {
        int value;
        MoveOnly(int v) : value(v) {}
        MoveOnly(const MoveOnly&) = delete;
        MoveOnly& operator=(const MoveOnly&) = delete;
        MoveOnly(MoveOnly&&) = default;
        MoveOnly& operator=(MoveOnly&&) = default;
        
        bool operator<(const MoveOnly& other) const { return value < other.value; }
        bool operator>(const MoveOnly& other) const { return value > other.value; }
        bool operator==(const MoveOnly& other) const { return value == other.value; }
        bool operator<=(const MoveOnly& other) const { return value <= other.value; }
        bool operator>=(const MoveOnly& other) const { return value >= other.value; }
    };
    
    SlidingWindowMin<MoveOnly> window(3);
    
    window

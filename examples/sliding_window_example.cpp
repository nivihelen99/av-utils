
#include "sliding_window_minmax.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <functional>
#include <cmath>

using namespace sliding_window;

void basic_min_example() {
    std::cout << "=== Basic Min Example ===\n";
    
    SlidingWindowMin<int> window(3);
    
    std::vector<int> data = {4, 2, 6, 1, 8, 3, 7};
    
    for (int val : data) {
        window.push(val);
        std::cout << "Added " << val << " -> Window size: " << window.size() 
                  << ", Min: " << window.min() << "\n";
    }
    std::cout << "\n";
}

void basic_max_example() {
    std::cout << "=== Basic Max Example ===\n";
    
    SlidingWindowMax<int> window(3);
    
    std::vector<int> data = {4, 2, 6, 1, 8, 3, 7};
    
    for (int val : data) {
        window.push(val);
        std::cout << "Added " << val << " -> Window size: " << window.size() 
                  << ", Max: " << window.max() << "\n";
    }
    std::cout << "\n";
}

void manual_pop_example() {
    std::cout << "=== Manual Pop Example ===\n";
    
    SlidingWindowMin<int> window(5);
    
    // Fill window
    for (int i = 1; i <= 5; ++i) {
        window.push(i * 10);
        std::cout << "Pushed " << (i * 10) << " -> Min: " << window.min() << "\n";
    }
    
    std::cout << "Window full: " << window.full() << "\n";
    
    // Manual pops
    while (!window.empty()) {
        std::cout << "Min: " << window.min() << " -> ";
        window.pop();
        std::cout << "After pop, size: " << window.size() << "\n";
    }
    std::cout << "\n";
}

void rate_limiting_example() {
    std::cout << "=== Rate Limiting Example ===\n";
    
    SlidingWindowMax<int> request_window(5);  // Track last 5 requests
    const int threshold = 100;
    
    std::vector<int> request_rates = {50, 75, 120, 80, 90, 110, 60, 40};
    
    for (int rate : request_rates) {
        request_window.push(rate);
        
        std::cout << "Request rate: " << rate << " req/s -> ";
        
        if (request_window.max() > threshold) {
            std::cout << "ALARM! Peak rate: " << request_window.max() << " req/s\n";
        } else {
            std::cout << "OK (peak: " << request_window.max() << " req/s)\n";
        }
    }
    std::cout << "\n";
}

void signal_processing_example() {
    std::cout << "=== Signal Processing Example ===\n";
    
    SlidingWindowMin<double> noise_floor(10);
    SlidingWindowMax<double> peak_detector(10);
    
    // Simulated signal with noise
    std::vector<double> signal = {
        1.2, 1.5, 1.1, 1.8, 2.3, 1.9, 1.4, 1.6, 1.3, 1.7,
        2.1, 2.5, 2.2, 2.8, 3.1, 2.9, 2.4, 2.6, 2.3, 2.7
    };
    
    for (double sample : signal) {
        noise_floor.push(sample);
        peak_detector.push(sample);
        
        if (noise_floor.size() >= 5) {  // Wait for some history
            double dynamic_range = peak_detector.max() - noise_floor.min();
            std::cout << "Sample: " << sample << " -> Dynamic range: " 
                      << dynamic_range << " (min: " << noise_floor.min() 
                      << ", max: " << peak_detector.max() << ")\n";
        }
    }
    std::cout << "\n";
}

void custom_comparator_example() {
    std::cout << "=== Custom Comparator Example ===\n";
    
    // Custom struct
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
    
    // Custom comparator for minimum distance
    auto distance_less = [](const Point& a, const Point& b) {
        return a.distance() < b.distance();
    };
    
    SlidingWindow<Point, decltype(distance_less)> closest_points(3, distance_less);
    
    std::vector<Point> points = {
        {1, 1}, {3, 4}, {0, 1}, {2, 2}, {5, 0}, {1, 0}
    };
    
    for (const auto& p : points) {
        closest_points.push(p);
        const auto& closest = closest_points.extreme();
        std::cout << "Added (" << p.x << ", " << p.y << ") -> "
                  << "Closest to origin: (" << closest.x << ", " << closest.y 
                  << ") distance: " << closest.distance() << "\n";
    }
    std::cout << "\n";
}

void stress_test() {
    std::cout << "=== Stress Test ===\n";
    
    SlidingWindowMin<int> min_window(1000);
    SlidingWindowMax<int> max_window(1000);
    
    // Add 10000 elements
    for (int i = 0; i < 10000; ++i) {
        int val = i % 2000 - 1000;  // Values from -1000 to 999
        min_window.push(val);
        max_window.push(val);
        
        if (i % 1000 == 0) {
            std::cout << "Processed " << i << " elements -> "
                      << "Min: " << min_window.min() << ", "
                      << "Max: " << max_window.max() << "\n";
        }
    }
    
    std::cout << "Final window size: " << min_window.size() << "\n";
    std::cout << "Final min: " << min_window.min() << ", max: " << max_window.max() << "\n";
    std::cout << "\n";
}

void error_handling_example() {
    std::cout << "=== Error Handling Example ===\n";
    
    try {
        // Invalid capacity
        SlidingWindowMin<int> invalid_window(0);
    } catch (const std::invalid_argument& e) {
        std::cout << "Caught expected error: " << e.what() << "\n";
    }
    
    try {
        // Empty window operations
        SlidingWindowMin<int> empty_window(5);
        std::cout << "Min from empty window: " << empty_window.min() << "\n";
    } catch (const std::runtime_error& e) {
        std::cout << "Caught expected error: " << e.what() << "\n";
    }
    
    try {
        // Pop from empty window
        SlidingWindowMax<int> empty_window(5);
        empty_window.pop();
    } catch (const std::runtime_error& e) {
        std::cout << "Caught expected error: " << e.what() << "\n";
    }
    
    std::cout << "\n";
}

void performance_comparison() {
    std::cout << "=== Performance Comparison ===\n";
    
    const int window_size = 1000;
    const int num_operations = 10000;
    
    // Our sliding window implementation
    SlidingWindowMin<int> sliding_min(window_size);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_operations; ++i) {
        sliding_min.push(i % 10000);
        volatile int min_val = sliding_min.min();  // Prevent optimization
        (void)min_val;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Sliding window min (" << num_operations << " operations): " 
              << duration.count() << " microseconds\n";
    std::cout << "Average per operation: " << (double)duration.count() / num_operations 
              << " microseconds\n";
    std::cout << "\n";
}

int main() {
    std::cout << "🔄 Sliding Window Min/Max - Comprehensive Examples\n";
    std::cout << "================================================\n\n";
    
    basic_min_example();
    basic_max_example();
    manual_pop_example();
    rate_limiting_example();
    signal_processing_example();
    custom_comparator_example();
    stress_test();
    error_handling_example();
    
    std::cout << "✅ All examples completed successfully!\n";
    
    return 0;
}

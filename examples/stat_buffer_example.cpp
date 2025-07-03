#include "stat_buffer.h"
#include <iostream>
#include <vector>
#include <iomanip> // For std::fixed and std::setprecision
#include <random>  // For generating random numbers

void print_stats(const StatBuffer<double, 10>& sb, const std::string& label) {
    std::cout << "\n--- " << label << " ---" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Size:     " << sb.size() << "/" << sb.capacity() << std::endl;
    if (sb.size() > 0) {
        std::cout << "Sum:      " << sb.sum() << std::endl;
        std::cout << "Min:      " << sb.min() << std::endl;
        std::cout << "Max:      " << sb.max() << std::endl;
        std::cout << "Mean:     " << sb.mean() << std::endl;
        std::cout << "Variance: " << sb.variance() << std::endl;
        std::cout << "StdDev:   " << sb.stddev() << std::endl;

        if (sb.stddev() > 25.0 && sb.size() >= sb.capacity() / 2) {
            std::cout << "ALERT: Standard deviation is high (" << sb.stddev() << ")!" << std::endl;
        }
    } else {
        std::cout << "Buffer is empty, no stats to display." << std::endl;
    }
    std::cout << "--------------------" << std::endl;
}

int main() {
    // Create a StatBuffer for doubles with a capacity of 10
    StatBuffer<double, 10> latency_stats;

    print_stats(latency_stats, "Initial State");

    // Simulate pushing some latency values
    std::cout << "\nPushing initial values..." << std::endl;
    latency_stats.push(15.2);
    latency_stats.push(18.9);
    latency_stats.push(17.1);
    latency_stats.push(22.5);
    latency_stats.push(16.8);
    print_stats(latency_stats, "After 5 pushes");

    std::cout << "\nPushing more values to fill the buffer..." << std::endl;
    latency_stats.push(19.5);
    latency_stats.push(20.3);
    latency_stats.push(14.7);
    latency_stats.push(25.1);
    latency_stats.push(18.3); // Buffer is now full {15.2, ..., 18.3}
    print_stats(latency_stats, "After 10 pushes (full)");

    std::cout << "\nPushing another value (eviction occurs)..." << std::endl;
    latency_stats.push(105.0); // This high value will affect stats significantly, 15.2 is evicted
    print_stats(latency_stats, "After pushing 105.0");
    // Expected: min might change if 15.2 was min, max should be 105.0. Mean and stddev will increase.

    std::cout << "\nPushing a few more realistic values..." << std::endl;
    latency_stats.push(21.0); // Evicts 18.9
    latency_stats.push(23.5); // Evicts 17.1
    print_stats(latency_stats, "After a few more pushes");

    // Example with integer types
    StatBuffer<int, 5> request_counts;
    std::cout << "\n--- Integer StatBuffer Example (Request Counts) ---" << std::endl;
    request_counts.push(100);
    request_counts.push(150);
    request_counts.push(120);
    request_counts.push(180);
    request_counts.push(130); // Full: {100, 150, 120, 180, 130}

    std::cout << "Size:     " << request_counts.size() << "/" << request_counts.capacity() << std::endl;
    std::cout << "Sum:      " << request_counts.sum() << std::endl;
    std::cout << "Min:      " << request_counts.min() << std::endl;
    std::cout << "Max:      " << request_counts.max() << std::endl;
    std::cout << "Mean:     " << request_counts.mean() << std::endl;
    std::cout << "StdDev:   " << request_counts.stddev() << std::endl;

    request_counts.push(50); // Evicts 100. New buffer: {150,120,180,130,50}. Min should be 50.
    std::cout << "\nAfter pushing 50 (evicting 100):" << std::endl;
    std::cout << "Min:      " << request_counts.min() << std::endl;
    std::cout << "Max:      " << request_counts.max() << std::endl;
    std::cout << "Mean:     " << request_counts.mean() << std::endl;
    std::cout << "StdDev:   " << request_counts.stddev() << std::endl;
    std::cout << "---------------------------------------------------" << std::endl;

    std::cout << "\nClearing latency_stats..." << std::endl;
    latency_stats.clear();
    print_stats(latency_stats, "After clear");

    return 0;
}

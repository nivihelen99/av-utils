// Example usage and benchmarks
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <numeric>

#include "spsc.h"



void basic_usage_example() {
    std::cout << "=== Basic Usage Example ===\n";
    
    concurrent::ring_buffer<int> buffer(8); // Power of 2 capacity
    buffer.enable_stats();
    
    // Producer
    std::thread producer([&buffer]() {
        for (int i = 0; i < 20; ++i) {
            while (!buffer.try_push(i)) {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
            std::cout << "Pushed: " << i << "\n";
        }
    });
    
    // Consumer
    std::thread consumer([&buffer]() {
        int received = 0;
        while (received < 20) {
            if (auto item = buffer.try_pop()) {
                std::cout << "Popped: " << *item << "\n";
                received++;
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        }
    });
    
    producer.join();
    consumer.join();
    
    if (auto stats = buffer.get_stats()) {
        std::cout << "Total pushes: " << stats->total_pushes.load() << "\n";
        std::cout << "Total pops: " << stats->total_pops.load() << "\n";
        std::cout << "Failed pushes: " << stats->failed_pushes.load() << "\n";
        std::cout << "Utilization: " << stats->utilization() * 100 << "%\n";
    }
    std::cout << "\n";
}

void performance_benchmark() {
    std::cout << "=== Performance Benchmark ===\n";
    
    constexpr std::size_t num_items = 1'000'000;
    constexpr std::size_t buffer_size = 1024;
    
    concurrent::ring_buffer<std::uint64_t> buffer(buffer_size);
    buffer.enable_stats();
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::thread producer([&buffer, num_items]() {
        for (std::uint64_t i = 0; i < num_items; ++i) {
            buffer.push(i); // Blocking push
        }
    });
    
    std::uint64_t sum = 0;
    std::thread consumer([&buffer, &sum, num_items]() {
        for (std::size_t i = 0; i < num_items; ++i) {
            sum += buffer.pop(); // Blocking pop
        }
    });
    
    producer.join();
    consumer.join();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    std::cout << "Processed " << num_items << " items in " << duration.count() << " μs\n";
    std::cout << "Throughput: " << (num_items * 1'000'000.0 / duration.count()) << " items/second\n";
    std::cout << "Expected sum: " << (num_items * (num_items - 1) / 2) << "\n";
    std::cout << "Actual sum: " << sum << "\n";
    std::cout << "Verification: " << (sum == num_items * (num_items - 1) / 2 ? "PASS" : "FAIL") << "\n\n";
}

void memory_ordering_comparison() {
    std::cout << "=== Memory Ordering Comparison ===\n";
    
    constexpr std::size_t num_items = 100'000;
    constexpr std::size_t buffer_size = 256;
    
    // Test different memory orderings
    std::vector<std::pair<std::string, std::chrono::microseconds>> results;
    
    // Relaxed ordering
    {
        concurrent::ring_buffer<int, concurrent::memory_ordering::relaxed> buffer(buffer_size);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::thread prod([&buffer, num_items]() {
            for (int i = 0; i < static_cast<int>(num_items); ++i) {
                buffer.push(i);
            }
        });
        
        std::thread cons([&buffer, num_items]() {
            for (std::size_t i = 0; i < num_items; ++i) {
                buffer.pop();
            }
        });
        
        prod.join();
        cons.join();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        results.emplace_back("Relaxed", duration);
    }
    
    // Acquire-Release ordering
    {
        concurrent::ring_buffer<int, concurrent::memory_ordering::acquire_release> buffer(buffer_size);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::thread prod([&buffer, num_items]() {
            for (int i = 0; i < static_cast<int>(num_items); ++i) {
                buffer.push(i);
            }
        });
        
        std::thread cons([&buffer, num_items]() {
            for (std::size_t i = 0; i < num_items; ++i) {
                buffer.pop();
            }
        });
        
        prod.join();
        cons.join();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        results.emplace_back("Acquire-Release", duration);
    }
    
    // Sequential ordering
    {
        concurrent::ring_buffer<int, concurrent::memory_ordering::sequential> buffer(buffer_size);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::thread prod([&buffer, num_items]() {
            for (int i = 0; i < static_cast<int>(num_items); ++i) {
                buffer.push(i);
            }
        });
        
        std::thread cons([&buffer, num_items]() {
            for (std::size_t i = 0; i < num_items; ++i) {
                buffer.pop();
            }
        });
        
        prod.join();
        cons.join();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        results.emplace_back("Sequential", duration);
    }
    
    for (const auto& [name, duration] : results) {
        std::cout << name << " ordering: " << duration.count() << " μs\n";
    }
    std::cout << "\n";
}

void custom_type_example() {
    std::cout << "=== Custom Type Example ===\n";
    
    struct Message {
        std::string content;
        int priority;
        std::chrono::steady_clock::time_point timestamp;
        
        Message() = default;
        Message(std::string c, int p) 
            : content(std::move(c)), priority(p), timestamp(std::chrono::steady_clock::now()) {}
    };
    
    concurrent::ring_buffer<Message> buffer(16);
    
    // Producer creating messages
    std::thread producer([&buffer]() {
        std::vector<std::string> messages = {
            "Hello", "World", "Lock-free", "Ring", "Buffer", "Performance"
        };
        
        for (int i = 0; i < static_cast<int>(messages.size()); ++i) {
            buffer.try_emplace(messages[i], i + 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    // Consumer processing messages
    std::thread consumer([&buffer]() {
        int received = 0;
        while (received < 6) {
            if (auto msg = buffer.try_pop()) {
                auto now = std::chrono::steady_clock::now();
                auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - msg->timestamp).count();
                
                std::cout << "Message: " << msg->content 
                         << ", Priority: " << msg->priority
                         << ", Latency: " << latency << "ms\n";
                received++;
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    });
    
    producer.join();
    consumer.join();
    std::cout << "\n";
}

int main()
{
    basic_usage_example();
    performance_benchmark();
    memory_ordering_comparison();
    custom_type_example();

    return 0;
}

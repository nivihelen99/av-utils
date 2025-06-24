#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <string>
#include <random>

// Include the NamedLock implementation
#include "named_lock.h"

// Example 1: Basic port-level locking
void example_port_locking() {
    std::cout << "=== Port Locking Example ===\n";
    
    NamedLock<std::string> portLocks;
    
    auto modify_port = [&](const std::string& port, int thread_id) {
        std::cout << "Thread " << thread_id << " attempting to lock port: " << port << "\n";
        
        auto guard = portLocks.acquire(port);
        std::cout << "Thread " << thread_id << " acquired lock for port: " << port << "\n";
        
        // Simulate port configuration work
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::cout << "Thread " << thread_id << " finished configuring port: " << port << "\n";
        // Lock automatically released when guard goes out of scope
    };
    
    // Launch multiple threads working on different ports
    std::vector<std::thread> threads;
    threads.emplace_back(modify_port, "Ethernet1", 1);
    threads.emplace_back(modify_port, "Ethernet2", 2);  // Different port - runs in parallel
    threads.emplace_back(modify_port, "Ethernet1", 3);  // Same port - waits for thread 1
    threads.emplace_back(modify_port, "Ethernet3", 4);  // Different port - runs in parallel
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "Port locks remaining: " << portLocks.key_count() << "\n\n";
}

// Example 2: Non-blocking try_acquire
void example_try_acquire() {
    std::cout << "=== Try Acquire Example ===\n";
    
    NamedLock<int> userLocks;
    
    auto process_user_request = [&](int user_id, int thread_id, bool should_wait = false) {
        if (should_wait) {
            // Blocking acquire
            auto guard = userLocks.acquire(user_id);
            std::cout << "Thread " << thread_id << " acquired lock for user " << user_id << " (blocking)\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        } else {
            // Non-blocking try_acquire
            auto guard = userLocks.try_acquire(user_id);
            if (guard) {
                std::cout << "Thread " << thread_id << " acquired lock for user " << user_id << " (non-blocking)\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            } else {
                std::cout << "Thread " << thread_id << " failed to acquire lock for user " << user_id << " (busy)\n";
            }
        }
    };
    
    std::vector<std::thread> threads;
    threads.emplace_back(process_user_request, 100, 1, true);   // Will acquire and hold
    threads.emplace_back(process_user_request, 100, 2, false);  // Will fail (non-blocking)
    threads.emplace_back(process_user_request, 101, 3, false);  // Will succeed (different user)
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "\n";
}

// Example 3: Timeout-based acquisition
void example_timed_acquire() {
    std::cout << "=== Timed Acquire Example ===\n";
    
    NamedLock<std::string> deviceLocks;
    
    auto access_device = [&](const std::string& device, int thread_id, 
                            std::chrono::milliseconds timeout = std::chrono::milliseconds(100)) {
        auto start = std::chrono::steady_clock::now();
        auto guard = deviceLocks.try_acquire_for(device, timeout);
        auto elapsed = std::chrono::steady_clock::now() - start;
        
        if (guard) {
            std::cout << "Thread " << thread_id << " acquired lock for device " << device 
                     << " after " << elapsed.count() / 1000000.0 << "ms\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        } else {
            std::cout << "Thread " << thread_id << " timed out waiting for device " << device 
                     << " after " << elapsed.count() / 1000000.0 << "ms\n";
        }
    };
    
    std::vector<std::thread> threads;
    threads.emplace_back(access_device, "camera1", 1, std::chrono::milliseconds(300));  // Long timeout
    threads.emplace_back(access_device, "camera1", 2, std::chrono::milliseconds(50));   // Short timeout
    threads.emplace_back(access_device, "camera1", 3, std::chrono::milliseconds(200));  // Medium timeout
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "\n";
}

// Example 4: Lock metrics and cleanup
void example_metrics_and_cleanup() {
    std::cout << "=== Metrics and Cleanup Example ===\n";
    
    NamedLock<int> resourceLocks;
    
    // Create some locks
    {
        auto lock1 = resourceLocks.acquire(1);
        auto lock2 = resourceLocks.acquire(2);
        auto lock3 = resourceLocks.acquire(3);
        
        auto metrics = resourceLocks.get_metrics();
        std::cout << "Active locks: " << metrics.active_locks << "\n";
        std::cout << "Total keys: " << metrics.total_keys << "\n";
        std::cout << "Unused keys: " << metrics.unused_keys << "\n";
        
    } // All locks released here
    
    // Check metrics again
    auto metrics = resourceLocks.get_metrics();
    std::cout << "After release - Active locks: " << metrics.active_locks << "\n";
    std::cout << "After release - Total keys: " << metrics.total_keys << "\n";
    std::cout << "After release - Unused keys: " << metrics.unused_keys << "\n";
    
    // Cleanup unused keys
    resourceLocks.cleanup_unused();
    
    metrics = resourceLocks.get_metrics();
    std::cout << "After cleanup - Total keys: " << metrics.total_keys << "\n";
    
    std::cout << "\n";
}

// Example 5: Scoped lock early release
void example_early_release() {
    std::cout << "=== Early Release Example ===\n";
    
    NamedLock<std::string> locks;
    
    {
        auto guard = locks.acquire("resource1");
        std::cout << "Lock acquired, owns_lock: " << guard.owns_lock() << "\n";
        
        // Do some work
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        // Release early
        guard.reset();
        std::cout << "Lock released early, owns_lock: " << guard.owns_lock() << "\n";
        
        // Do work that doesn't need the lock
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
    } // Destructor called, but lock already released
    
    std::cout << "\n";
}

// Stress test example
void stress_test() {
    std::cout << "=== Stress Test ===\n";
    
    NamedLock<int> locks;
    std::atomic<int> completed_operations{0};
    
    constexpr int num_threads = 10;
    constexpr int operations_per_thread = 100;
    constexpr int num_keys = 5;
    
    auto worker = [&](int thread_id) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> key_dist(1, num_keys);
        std::uniform_int_distribution<> work_dist(1, 10);  // 1-10ms work
        
        for (int i = 0; i < operations_per_thread; ++i) {
            int key = key_dist(gen);
            
            if (i % 3 == 0) {
                // Try non-blocking acquire occasionally
                auto guard = locks.try_acquire(key);
                if (guard) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(work_dist(gen)));
                    completed_operations.fetch_add(1);
                }
            } else {
                // Blocking acquire
                auto guard = locks.acquire(key);
                std::this_thread::sleep_for(std::chrono::milliseconds(work_dist(gen)));
                completed_operations.fetch_add(1);
            }
        }
    };
    
    auto start_time = std::chrono::steady_clock::now();
    
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Completed " << completed_operations.load() << " operations in " 
              << duration.count() << "ms\n";
    
    auto metrics = locks.get_metrics();
    std::cout << "Final metrics - Keys: " << metrics.total_keys 
              << ", Active: " << metrics.active_locks 
              << ", Unused: " << metrics.unused_keys << "\n";
    
    locks.cleanup_unused();
    std::cout << "After cleanup - Keys: " << locks.key_count() << "\n";
}

int main() {
    example_port_locking();
    example_try_acquire();
    example_timed_acquire();
    example_metrics_and_cleanup();
    example_early_release();
    stress_test();
    
    return 0;
}

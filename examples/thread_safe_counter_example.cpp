#include "thread_safe_counter.hpp"
#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <chrono> // For std::chrono::milliseconds
#include <random>   // For std::random_device, std::mt19937, std::uniform_int_distribution

// Function for a worker thread to perform operations on the counter
void worker_task(ThreadSafeCounter<std::string>& counter, int num_operations, int worker_id) {
    std::mt19937 rng(std::random_device{}() + worker_id); // Seed with worker_id for different sequences
    std::uniform_int_distribution<int> op_dist(0, 2); // 0: add "apple", 1: add "banana", 2: subtract "apple"
    std::uniform_int_distribution<int> val_dist(1, 5);  // Values to add/subtract

    for (int i = 0; i < num_operations; ++i) {
        int operation = op_dist(rng);
        int value = val_dist(rng);

        if (operation == 0) {
            counter.add("apple", value);
            // std::cout << "Worker " << worker_id << " added " << value << " to apple." << std::endl;
        } else if (operation == 1) {
            counter.add("banana", value);
            // std::cout << "Worker " << worker_id << " added " << value << " to banana." << std::endl;
        } else { // operation == 2
            counter.subtract("apple", value);
            // std::cout << "Worker " << worker_id << " subtracted " << value << " from apple." << std::endl;
        }
        // Small delay to increase chances of interleaving
        std::this_thread::sleep_for(std::chrono::microseconds(rng() % 100));
    }
}

int main() {
    std::cout << "ThreadSafeCounter Example" << std::endl;
    std::cout << "-------------------------" << std::endl;

    // Basic usage
    ThreadSafeCounter<std::string> basic_counter;
    basic_counter.add("hello", 2);
    basic_counter.add("world", 3);
    basic_counter.add("hello", 1);
    std::cout << "Initial counts:" << std::endl;
    std::cout << "hello: " << basic_counter.count("hello") << std::endl; // Expected: 3
    std::cout << "world: " << basic_counter.count("world") << std::endl; // Expected: 3
    std::cout << "Total items: " << basic_counter.total() << std::endl;  // Expected: 6
    std::cout << std::endl;

    basic_counter.subtract("world", 2);
    std::cout << "After subtracting 2 from 'world':" << std::endl;
    std::cout << "world: " << basic_counter.count("world") << std::endl; // Expected: 1
    std::cout << "Total items: " << basic_counter.total() << std::endl;  // Expected: 4
    std::cout << std::endl;

    basic_counter.set_count("new_item", 5);
    std::cout << "After setting 'new_item' to 5:" << std::endl;
    std::cout << "new_item: " << basic_counter.count("new_item") << std::endl; // Expected: 5
    std::cout << std::endl;

    std::cout << "Most common (top 2):" << std::endl;
    for (const auto& pair : basic_counter.most_common(2)) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }
    std::cout << std::endl;


    // Multithreaded usage
    std::cout << "Multithreaded test:" << std::endl;
    ThreadSafeCounter<std::string> concurrent_counter;
    std::vector<std::thread> threads;
    int num_threads = 5;
    int operations_per_thread = 1000;

    // Expected final counts can be complex due to subtractions potentially removing items.
    // We'll just observe the final state.

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker_task, std::ref(concurrent_counter), operations_per_thread, i + 1);
    }

    for (std::thread& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    std::cout << "\nFinal counts after multithreaded operations:" << std::endl;
    std::cout << "apple: " << concurrent_counter.count("apple") << std::endl;
    std::cout << "banana: " << concurrent_counter.count("banana") << std::endl;
    std::cout << "Total items in concurrent_counter: " << concurrent_counter.total() << std::endl;
    std::cout << "Size of concurrent_counter (unique keys): " << concurrent_counter.size() << std::endl;


    std::cout << "\nMost common items in concurrent_counter:" << std::endl;
    for (const auto& pair : concurrent_counter.most_common()) { // Get all
        std::cout << pair.first << ": " << pair.second << std::endl;
    }
    std::cout << std::endl;

    // Test copy constructor and assignment
    ThreadSafeCounter<std::string> copied_counter = concurrent_counter;
    std::cout << "Copied counter state:" << std::endl;
    std::cout << "apple (copied): " << copied_counter.count("apple") << std::endl;
    std::cout << "banana (copied): " << copied_counter.count("banana") << std::endl;

    ThreadSafeCounter<std::string> assigned_counter;
    assigned_counter.add("temp", 1);
    assigned_counter = concurrent_counter;
    std::cout << "Assigned counter state:" << std::endl;
    std::cout << "apple (assigned): " << assigned_counter.count("apple") << std::endl;
    std::cout << "banana (assigned): " << assigned_counter.count("banana") << std::endl;
    std::cout << "temp (assigned): " << assigned_counter.count("temp") << " (should be 0 if original didn't have it)" << std::endl;


    // Test arithmetic operations
    ThreadSafeCounter<std::string> counter_a;
    counter_a.add("x", 5);
    counter_a.add("y", 3);

    ThreadSafeCounter<std::string> counter_b;
    counter_b.add("y", 2);
    counter_b.add("z", 4);

    std::cout << "\nArithmetic operations:" << std::endl;
    ThreadSafeCounter<std::string> counter_sum = counter_a + counter_b;
    std::cout << "Sum (x:5, y:3) + (y:2, z:4):" << std::endl;
    std::cout << "x: " << counter_sum.count("x") << " (Expected 5)" << std::endl;
    std::cout << "y: " << counter_sum.count("y") << " (Expected 5)" << std::endl;
    std::cout << "z: " << counter_sum.count("z") << " (Expected 4)" << std::endl;

    ThreadSafeCounter<std::string> counter_diff = counter_a - counter_b;
    std::cout << "Diff (x:5, y:3) - (y:2, z:4):" << std::endl;
    std::cout << "x: " << counter_diff.count("x") << " (Expected 5)" << std::endl;
    std::cout << "y: " << counter_diff.count("y") << " (Expected 1)" << std::endl;
    std::cout << "z: " << counter_diff.count("z") << " (Expected -4, then 0 due to set_count logic)" << std::endl; // Python Counter would show -4. Ours removes if <=0.

    // Test intersection and union
    ThreadSafeCounter<std::string> intersection_res = counter_a.intersection(counter_b);
    std::cout << "Intersection (x:5, y:3) & (y:2, z:4):" << std::endl;
    std::cout << "y: " << intersection_res.count("y") << " (Expected 2)" << std::endl;
    std::cout << "x: " << intersection_res.count("x") << " (Expected 0)" << std::endl;

    ThreadSafeCounter<std::string> union_res = counter_a.union_with(counter_b);
    std::cout << "Union (x:5, y:3) | (y:2, z:4):" << std::endl;
    std::cout << "x: " << union_res.count("x") << " (Expected 5)" << std::endl;
    std::cout << "y: " << union_res.count("y") << " (Expected 3)" << std::endl;
    std::cout << "z: " << union_res.count("z") << " (Expected 4)" << std::endl;

    std::cout << "\nExample finished." << std::endl;

    return 0;
}

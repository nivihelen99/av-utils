#include "instrumented_ring_buffer.hpp" // Adjust path if necessary
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <chrono> // For std::chrono::milliseconds
#include <iomanip> // For std::setw

// Helper function to print metrics
void print_metrics(const cpp_utils::InstrumentedRingBuffer<int>& buffer, const std::string& title) {
    std::cout << "\n--- Metrics: " << title << " ---" << std::endl;
    std::cout << std::left << std::setw(25) << "Current Size:" << buffer.size() << std::endl;
    std::cout << std::left << std::setw(25) << "Peak Size:" << buffer.get_peak_size() << std::endl;
    std::cout << std::left << std::setw(25) << "Capacity:" << buffer.capacity() << std::endl;
    std::cout << std::left << std::setw(25) << "Push Success Count:" << buffer.get_push_success_count() << std::endl;
    std::cout << std::left << std::setw(25) << "Pop Success Count:" << buffer.get_pop_success_count() << std::endl;
    std::cout << std::left << std::setw(25) << "Push Wait Count:" << buffer.get_push_wait_count() << std::endl;
    std::cout << std::left << std::setw(25) << "Pop Wait Count:" << buffer.get_pop_wait_count() << std::endl;
    std::cout << std::left << std::setw(25) << "Try Push Fail Count:" << buffer.get_try_push_fail_count() << std::endl;
    std::cout << std::left << std::setw(25) << "Try Pop Fail Count:" << buffer.get_try_pop_fail_count() << std::endl;
    std::cout << "----------------------------------------" << std::endl;
}

void basic_operations_example() {
    std::cout << "--- Basic Operations Example ---" << std::endl;
    cpp_utils::InstrumentedRingBuffer<int> buffer(5);

    std::cout << "Initial state: empty() = " << std::boolalpha << buffer.empty() << ", full() = " << buffer.full() << std::endl;

    // Try_push
    std::cout << "Trying to push 1: " << buffer.try_push(1) << std::endl;
    std::cout << "Trying to push 2: " << buffer.try_push(2) << std::endl;
    buffer.try_push(3);
    buffer.try_push(4);
    buffer.try_push(5);
    std::cout << "Buffer size after 5 pushes: " << buffer.size() << std::endl;
    std::cout << "Trying to push 6 (should fail): " << buffer.try_push(6) << std::endl;

    print_metrics(buffer, "After try_push operations");

    // Try_pop
    int val;
    if (buffer.try_pop(val)) {
        std::cout << "Popped value (try_pop): " << val << std::endl;
    }
    if (buffer.try_pop(val)) {
        std::cout << "Popped value (try_pop): " << val << std::endl;
    }

    print_metrics(buffer, "After some try_pop operations");

    // Blocking push
    std::cout << "Pushing 10 (blocking)..." << std::endl;
    buffer.push(10); // Should not block as there is space
    std::cout << "Pushing 11 (blocking)..." << std::endl;
    buffer.push(11);
    std::cout << "Pushing 12 (blocking)..." << std::endl;
    buffer.push(12); // Now full

    // This would block if uncommented and run in main thread without consumer
    // std::cout << "Attempting to push 13 (will block if no consumer)..." << std::endl;
    // buffer.push(13);

    print_metrics(buffer, "After blocking push operations");

    // Blocking pop
    std::cout << "Popping (blocking): " << buffer.pop() << std::endl;
    std::cout << "Popping (blocking): " << buffer.pop() << std::endl;

    print_metrics(buffer, "After some blocking pop operations");

    buffer.reset_metrics();
    std::cout << "\nMetrics reset." << std::endl;
    print_metrics(buffer, "After reset");

    // Fill and empty
    for(int i=0; i<5; ++i) buffer.push(i*100);
    while(!buffer.empty()) {
        std::cout << "Popped: " << buffer.pop() << " ";
    }
    std::cout << std::endl;
    print_metrics(buffer, "After filling and emptying");
}

void producer_consumer_example() {
    std::cout << "\n--- Producer-Consumer Example ---" << std::endl;
    cpp_utils::InstrumentedRingBuffer<int> buffer(10);
    const int items_to_produce = 100;
    std::atomic<int> items_consumed_count(0);
    std::vector<int> consumed_items;
    consumed_items.reserve(items_to_produce);
    std::mutex cout_mutex; // For synchronized cout

    // Producer thread
    std::thread producer([&]() {
        for (int i = 0; i < items_to_produce; ++i) {
            buffer.push(i);
            if (i % 20 == 0) { // Occasionally print
                std::lock_guard<std::mutex> lock(cout_mutex);
                // std::cout << "Producer pushed: " << i << std::endl;
            }
            // Simulate some work
            if (i % 5 == 0) std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Producer finished." << std::endl;
        }
    });

    // Consumer thread
    std::thread consumer([&]() {
        for (int i = 0; i < items_to_produce; ++i) {
            int item = buffer.pop();
            // Store consumed items for verification if needed, not strictly necessary for example
            // For now, just increment count
            items_consumed_count++;
            // consumed_items.push_back(item); // If verification needed
            if (item % 20 == 0) { // Occasionally print
                 std::lock_guard<std::mutex> lock(cout_mutex);
                // std::cout << "Consumer popped: " << item << std::endl;
            }
             // Simulate some work
            if (i % 7 == 0) std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Consumer finished." << std::endl;
        }
    });

    // Another consumer thread to increase contention
    std::thread consumer2([&]() {
        // This consumer will try to pop half the items, but the main consumer
        // is also running. This is just to add more contention.
        // The total items_consumed_count will be tracked by the first consumer.
        // This is a simplified scenario. A more robust one would have each consumer
        // track its own items or use a shared counter.
        // For this example, let's assume it tries to consume some items but might not get all.
        int local_consume_count = 0;
        while(items_consumed_count.load() < items_to_produce && local_consume_count < items_to_produce / 2) {
            int item;
            if (buffer.try_pop(item)) { // Use try_pop to avoid blocking indefinitely if main consumer is faster
                items_consumed_count++;
                local_consume_count++;
                if (item % 25 == 0) {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    // std::cout << "Consumer2 popped: " << item << std::endl;
                }
            } else {
                // Yield or sleep if buffer is often empty for this consumer
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }
         {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Consumer2 finished its attempts (consumed " << local_consume_count << " items)." << std::endl;
        }
    });


    producer.join();
    consumer.join();
    consumer2.join();

    std::cout << "Producer and Consumers finished." << std::endl;
    std::cout << "Total items expected to be consumed by primary consumer: " << items_to_produce << std::endl;
    // Note: items_consumed_count might be higher than items_to_produce if consumer2 also successfully pops.
    // The example's primary check is that the main producer and main consumer complete their specific loop counts.
    // A more robust check would involve summing all items popped by all consumers.
    // For this example, we primarily observe the metrics.

    print_metrics(buffer, "After Producer-Consumer run");

    // Verify all items were processed (simple check for this example)
    // This check is a bit flawed due_to items_consumed_count being shared and potentially incremented by consumer2
    // A better check would be if (buffer.get_pop_success_count() == items_to_produce)
    if (buffer.get_pop_success_count() == items_to_produce) {
         std::cout << "Verification: Pop success count matches items produced." << std::endl;
    } else {
         std::cout << "Verification: Pop success count (" << buffer.get_pop_success_count()
                   << ") does NOT match items produced (" << items_to_produce << ")." << std::endl;
    }
    if (buffer.empty()){
        std::cout << "Verification: Buffer is empty after run." << std::endl;
    } else {
        std::cout << "Verification: Buffer is NOT empty after run. Size: " << buffer.size() << std::endl;
    }

}


int main() {
    basic_operations_example();
    producer_consumer_example();
    return 0;
}

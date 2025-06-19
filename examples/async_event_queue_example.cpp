#include "AsyncEventQueue.h" // Adjust path as necessary, assumes it's in include directory accessible
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <chrono> // For std::chrono::milliseconds
#include <atomic> // For std::atomic in multi-producer/consumer example

// Basic usage: single producer, single consumer
void basic_usage_example() {
    std::cout << "--- Basic Usage Example ---" << std::endl;
    AsyncEventQueue<int> queue(5); // Bounded queue with max size 5

    std::thread producer([&]() {
        for (int i = 0; i < 10; ++i) {
            std::cout << "Producer: putting " << i << std::endl;
            queue.put(i);
            std::cout << "Producer: q_size after put: " << queue.size() << std::endl;
            if (i % 3 == 0) { // Occasionally sleep
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    });

    std::thread consumer([&]() {
        for (int i = 0; i < 10; ++i) {
            std::cout << "Consumer: waiting for item..." << std::endl;
            int item = queue.get();
            std::cout << "Consumer: got " << item << ", q_size after get: " << queue.size() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(20)); // Simulate work
        }
    });

    producer.join();
    consumer.join();
    std::cout << "Queue is now empty: " << std::boolalpha << queue.empty() << std::endl;
    std::cout << "--- End Basic Usage Example ---" << std::endl << std::endl;
}

// Example with try_get
void try_get_example() {
    std::cout << "--- Try_Get Example ---" << std::endl;
    AsyncEventQueue<std::string> queue(3);
    queue.put("hello");
    queue.put("world");

    std::optional<std::string> item1 = queue.try_get();
    if (item1) {
        std::cout << "try_get got: " << *item1 << std::endl;
    }

    std::optional<std::string> item2 = queue.try_get();
    if (item2) {
        std::cout << "try_get got: " << *item2 << std::endl;
    }

    std::optional<std::string> item3 = queue.try_get(); // Should be empty
    if (!item3) {
        std::cout << "try_get on empty queue returned nullopt, as expected." << std::endl;
    }
    queue.put("another item");
    item3 = queue.try_get();
    if (item3) {
        std::cout << "try_get got: " << *item3 << std::endl;
    }
    std::cout << "--- End Try_Get Example ---" << std::endl << std::endl;
}

// Example with callback
void callback_example() {
    std::cout << "--- Callback Example ---" << std::endl;
    AsyncEventQueue<int> queue(2);

    std::cout << "Registering callback." << std::endl;
    queue.register_callback([]() {
        std::cout << "Callback: Hey, an item was added to an empty queue!" << std::endl;
    });

    std::cout << "Putting first item (100). Expect callback." << std::endl;
    queue.put(100); // Queue was empty, callback should fire

    std::cout << "Putting second item (101). Expect no callback." << std::endl;
    queue.put(101); // Queue not empty, callback should NOT fire

    std::cout << "Getting first item: " << queue.get() << std::endl;
    std::cout << "Getting second item: " << queue.get() << std::endl;

    std::cout << "Queue is empty now. Current size: " << queue.size() << std::endl;
    std::cout << "Putting third item (102). Expect callback." << std::endl;
    queue.put(102); // Queue was empty, callback should fire

    std::cout << "Getting third item: " << queue.get() << std::endl;
    std::cout << "--- End Callback Example ---" << std::endl << std::endl;
}

// Multi-producer, multi-consumer example
void multi_producer_multi_consumer_example() {
    std::cout << "--- Multi-Producer/Multi-Consumer Example ---" << std::endl;
    AsyncEventQueue<int> queue(10); // Bounded queue
    const int num_producers = 3;
    const int num_consumers = 2;
    const int items_per_producer = 5;
    std::atomic<int> items_produced_total(0);
    std::atomic<int> items_consumed_total(0);
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    // Producers
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&, producer_id = i]() {
            for (int j = 0; j < items_per_producer; ++j) {
                int item_value = producer_id * 100 + j;
                // Use a local string stream for thread-safe logging if needed, or ensure std::cout is safe enough for demo
                std::cout << "Producer " << producer_id << ": putting " << item_value << std::endl;
                queue.put(item_value);
                items_produced_total++;
                // std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 10 + 5)); // Simulate work
            }
            std::cout << "Producer " << producer_id << " finished." << std::endl;
        });
    }

    // Consumers
    for (int i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([&, consumer_id = i]() {
            while(true) {
                 if (items_consumed_total.load(std::memory_order_acquire) >= num_producers * items_per_producer) {
                    break;
                 }

                int item = queue.get(); // Blocking get
                // Check for a sentinel value if used, though not in this specific example's get()
                items_consumed_total.fetch_add(1, std::memory_order_release);
                std::cout << "Consumer " << consumer_id << ": got " << item
                          << " (total consumed: " << items_consumed_total.load(std::memory_order_relaxed) << ")" << std::endl;
                // std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 20 + 10)); // Simulate work
                 if (items_consumed_total.load(std::memory_order_acquire) >= num_producers * items_per_producer) {
                    break;
                 }
            }
            std::cout << "Consumer " << consumer_id << " finished or exiting." << std::endl;
        });
    }

    for (auto& p : producers) {
        p.join();
    }
    std::cout << "All producers finished." << std::endl;

    // Wait for consumers to finish.
    // The consumer loop condition `items_consumed_total.load(std::memory_order_acquire) >= num_producers * items_per_producer`
    // should ensure they terminate once all items are processed.
    for (auto& c : consumers) {
        c.join();
    }
    std::cout << "All consumers finished." << std::endl;

    std::cout << "Total items produced: " << items_produced_total.load() << std::endl;
    std::cout << "Total items consumed: " << items_consumed_total.load() << std::endl;
    std::cout << "Final queue size: " << queue.size() << std::endl;
    std::cout << "Queue is empty: " << std::boolalpha << queue.empty() << std::endl;

    std::cout << "--- End Multi-Producer/Multi-Consumer Example ---" << std::endl << std::endl;
}


int main() {
    std::cout << "Starting AsyncEventQueue Examples..." << std::endl << std::endl;

    basic_usage_example();
    try_get_example();
    callback_example();
    multi_producer_multi_consumer_example();

    std::cout << "All AsyncEventQueue examples finished." << std::endl;
    return 0;
}

```

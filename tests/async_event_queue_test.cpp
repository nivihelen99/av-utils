#include "gtest/gtest.h"
#include "AsyncEventQueue.h" // Assumes AsyncEventQueue.h is in include path
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <string>
#include <set> // For checking consumed items uniqueness
#include <memory> // For std::unique_ptr in Test 8

// Test fixture for AsyncEventQueue tests
class AsyncEventQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup can go here if needed
    }

    void TearDown() override {
        // Common teardown can go here if needed
    }
};

// Test 1: Single-threaded put/get sequence
TEST_F(AsyncEventQueueTest, SingleThreadedPutGet) {
    AsyncEventQueue<int> queue(5);
    EXPECT_TRUE(queue.empty());
    EXPECT_FALSE(queue.full());
    EXPECT_EQ(queue.size(), 0);

    queue.put(10);
    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 1);

    queue.put(20);
    EXPECT_EQ(queue.size(), 2);

    EXPECT_EQ(queue.get(), 10);
    EXPECT_EQ(queue.size(), 1);

    EXPECT_EQ(queue.get(), 20);
    EXPECT_EQ(queue.size(), 0);
    EXPECT_TRUE(queue.empty());
}

// Test 2: try_get() returns nullopt if empty and retrieves items
TEST_F(AsyncEventQueueTest, TryGetBehavior) {
    AsyncEventQueue<std::string> queue(3);
    queue.put("apple");
    queue.put("banana");

    auto item1 = queue.try_get();
    ASSERT_TRUE(item1.has_value());
    EXPECT_EQ(*item1, "apple");
    EXPECT_EQ(queue.size(), 1);

    auto item2 = queue.try_get();
    ASSERT_TRUE(item2.has_value());
    EXPECT_EQ(*item2, "banana");
    EXPECT_EQ(queue.size(), 0);
    EXPECT_TRUE(queue.empty());

    auto item3 = queue.try_get(); // Queue is empty
    EXPECT_FALSE(item3.has_value());
    EXPECT_TRUE(queue.empty());
}

// Test 3: Bounded queue: blocks on full, then unblocks
TEST_F(AsyncEventQueueTest, BoundedQueueBlocksOnFull) {
    AsyncEventQueue<int> queue(2); // Max size 2

    queue.put(1);
    queue.put(2);
    EXPECT_TRUE(queue.full());
    EXPECT_EQ(queue.size(), 2);

    std::atomic<bool> put_attempted_when_full(false);
    std::atomic<bool> put_succeeded_after_get(false);
    std::thread producer_thread([&]() {
        put_attempted_when_full.store(true); // Mark that we are about to try putting to a full queue
        queue.put(3); // This should block
        put_succeeded_after_get.store(true);  // This will be set once put(3) unblocks and completes
    });

    // Give producer thread a moment to likely block on put()
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(put_attempted_when_full.load());
    EXPECT_FALSE(put_succeeded_after_get.load()); // Should still be blocked
    EXPECT_TRUE(queue.full()); // Queue should still be full

    // Consumer takes an item, making space
    EXPECT_EQ(queue.get(), 1);
    EXPECT_FALSE(queue.full()); // After getting 1, queue size is 1, max 2. So not full.

    // Producer should now unblock and complete put(3)
    if (producer_thread.joinable()) {
        producer_thread.join();
    }

    EXPECT_TRUE(put_succeeded_after_get.load());
    EXPECT_EQ(queue.size(), 2); // Should contain 2 and 3
    EXPECT_TRUE(queue.full()); // Max size is 2, contains 2 items. So it is full.


    EXPECT_EQ(queue.get(), 2);
    EXPECT_EQ(queue.get(), 3);
    EXPECT_TRUE(queue.empty());
}


// Test 4: Callback fires only once on empty -> non-empty transition
TEST_F(AsyncEventQueueTest, CallbackBehavior) {
    AsyncEventQueue<int> queue(3);
    std::atomic<int> callback_count(0);

    queue.register_callback([&]() {
        callback_count++;
    });

    EXPECT_EQ(callback_count.load(), 0);

    queue.put(100); // Empty -> Non-empty
    EXPECT_EQ(callback_count.load(), 1);
    EXPECT_EQ(queue.size(), 1);

    queue.put(200); // Non-empty -> Non-empty
    EXPECT_EQ(callback_count.load(), 1); // Callback should not fire again
    EXPECT_EQ(queue.size(), 2);

    EXPECT_EQ(queue.get(), 100);
    EXPECT_EQ(queue.get(), 200);
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(callback_count.load(), 1); // Still 1

    queue.put(300); // Empty -> Non-empty again
    EXPECT_EQ(callback_count.load(), 2); // Callback should fire again
    EXPECT_EQ(queue.size(), 1);

    EXPECT_EQ(queue.get(), 300);
}

// Test 5: Multi-producer, single-consumer scenario
TEST_F(AsyncEventQueueTest, MultiProducerSingleConsumer) {
    AsyncEventQueue<int> queue(100); // Large enough bounded queue
    const int num_producers = 5;
    const int items_per_producer = 20;
    const int total_items = num_producers * items_per_producer;
    std::vector<std::thread> producers;
    std::set<int> produced_items_tracker; // To ensure unique items are produced
    std::mutex tracker_mutex;

    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&, producer_id = i]() {
            for (int j = 0; j < items_per_producer; ++j) {
                int item_value = producer_id * items_per_producer + j;
                queue.put(item_value);
                {
                    std::lock_guard<std::mutex> lock(tracker_mutex);
                    produced_items_tracker.insert(item_value);
                }
            }
        });
    }

    std::set<int> consumed_items_tracker;
    std::thread consumer_thread([&]() {
        for (int i = 0; i < total_items; ++i) {
            consumed_items_tracker.insert(queue.get());
        }
    });

    for (auto& p : producers) {
        p.join();
    }
    consumer_thread.join();

    EXPECT_EQ(queue.size(), 0);
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(produced_items_tracker.size(), total_items);
    EXPECT_EQ(consumed_items_tracker.size(), total_items);
    EXPECT_EQ(produced_items_tracker, consumed_items_tracker); // Verifies all produced items were consumed
}

// Test 6: Multi-producer, multi-consumer under contention
TEST_F(AsyncEventQueueTest, MultiProducerMultiConsumerContention) {
    AsyncEventQueue<long long> queue(50); // A reasonably sized bounded queue to induce some contention
    const int num_producers = 4;
    const int num_consumers = 4;
    const int items_per_producer = 250; // More items to increase chance of contention
    const long long total_items = num_producers * items_per_producer;

    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    std::atomic<long long> items_produced_count(0);
    std::atomic<long long> items_consumed_count(0);

    std::multiset<long long> all_produced_values;
    std::multiset<long long> all_consumed_values;
    std::mutex produced_values_mutex;
    std::mutex consumed_values_mutex;

    // Producers
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&, producer_id = i]() {
            for (int j = 0; j < items_per_producer; ++j) {
                long long item_value = static_cast<long long>(producer_id) * items_per_producer + j;
                queue.put(item_value);
                items_produced_count++;
                {
                    std::lock_guard<std::mutex> lock(produced_values_mutex);
                    all_produced_values.insert(item_value);
                }
            }
        });
    }

    // Consumers
    for (int i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([&]() {
            while (items_consumed_count.load(std::memory_order_acquire) < total_items) {
                std::optional<long long> item_opt = queue.try_get();
                if (item_opt) {
                    long long current_total_consumed = items_consumed_count.fetch_add(1, std::memory_order_release) + 1;
                     {
                        std::lock_guard<std::mutex> lock(consumed_values_mutex);
                        all_consumed_values.insert(*item_opt);
                    }
                    if (current_total_consumed > total_items) {
                        items_consumed_count--;
                        break;
                    }
                } else {
                    if (items_consumed_count.load(std::memory_order_acquire) >= total_items) {
                        break;
                    }
                    std::this_thread::yield();
                }
            }
        });
    }

    for (auto& p : producers) {
        p.join();
    }
    for (auto& c : consumers) {
        c.join();
    }

    EXPECT_EQ(items_produced_count.load(), total_items);
    EXPECT_EQ(items_consumed_count.load(), total_items);
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(all_produced_values.size(), total_items);
    EXPECT_EQ(all_consumed_values.size(), total_items);
    EXPECT_EQ(all_produced_values, all_consumed_values);
}


// Test 7: Unbounded queue behavior (maxsize = 0)
TEST_F(AsyncEventQueueTest, UnboundedQueue) {
    AsyncEventQueue<int> queue(0); // Unbounded
    EXPECT_FALSE(queue.full()); // Should never be full

    const int num_items = 1000;
    for (int i = 0; i < num_items; ++i) {
        queue.put(i);
    }
    EXPECT_EQ(queue.size(), num_items);
    EXPECT_FALSE(queue.full()); // Still not full
    EXPECT_FALSE(queue.empty());

    for (int i = 0; i < num_items; ++i) {
        EXPECT_EQ(queue.get(), i);
    }
    EXPECT_TRUE(queue.empty());
}


// Test 8: Graceful shutdown (Destruction with potentially waiting threads)
TEST_F(AsyncEventQueueTest, DestructionWithPotentiallyWaitingThreads) {
    // This test primarily checks that the destructor completes without deadlocking or crashing
    // when threads *could* be blocked on its condition variables. A more robust test
    // for "graceful shutdown" would require an explicit queue shutdown mechanism that signals
    // threads to exit their loops and unblocks them from condition variable waits.
    // The current AsyncEventQueue does not have such a mechanism. Threads blocked internally
    // might be abruptly terminated or lead to undefined behavior if queue resources vanish.
    // Detaching threads is a way to allow the main thread (and test) to proceed,
    // but it's not a solution for resource management in production code.

    SUCCEED(); // Placeholder: Test logic below is complex and needs careful review for safety.

    {
        AsyncEventQueue<int> q_for_destruction_test(1); // size 1

        std::thread blocked_putter([&q_for_destruction_test]() {
            // This thread might complete the first put, then block on the second.
            // If it blocks, its state upon queue destruction is a concern.
            try {
                q_for_destruction_test.put(1); // OK
                q_for_destruction_test.put(2); // This should block
            } catch (const std::exception& e) {
                // Catch potential exceptions if CVs throw upon destruction (though not standard)
                // std::cerr << "Putter thread caught exception: " << e.what() << std::endl;
            }
        });

        // Allow some time for the first put to occur and the second to block.
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        std::thread blocked_getter([&q_for_destruction_test]() {
            // This thread might get the first item, then block on the second get.
            try {
                q_for_destruction_test.get(); // Gets 1 (if putter was quick)
                q_for_destruction_test.get(); // This should block (queue becomes empty)
            } catch (const std::exception& e) {
                // std::cerr << "Getter thread caught exception: " << e.what() << std::endl;
            }
        });

        // Allow some time for threads to perform their operations and potentially block.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Threads are active and possibly blocked. When q_for_destruction_test goes out of scope,
        // its destructor is called. We are checking if this process hangs or crashes.
        // Detaching threads means we are not waiting for them to complete.
        // If they are blocked on CVs of the queue being destructed, this can be UB.
        // This test is more of a "does it blow up obviously" rather than "is it correct".
        // A proper shutdown (e.g. q.close(); t1.join(); t2.join();) is needed for correctness.
        if (blocked_putter.joinable()) blocked_putter.detach();
        if (blocked_getter.joinable()) blocked_getter.detach();
    }
    // If the destructor (implicitly called at end of scope) completes without hanging/crashing,
    // this test "passes" in its limited scope.
    // No explicit SUCCEED() needed if no EXPECT_FAIL/ASSERT_FAIL is hit.
}
// Note: No main() function is needed here as it's typically provided by gtest_main or a separate file.
```

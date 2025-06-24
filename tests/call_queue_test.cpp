#include "call_queue.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <set>

using namespace callqueue;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::SizeIs;

// --- CallQueue Tests ---

TEST(CallQueueTest, PushAndDrainAll) {
    CallQueue queue;
    std::vector<int> results;

    queue.push([&results]() { results.push_back(1); });
    queue.push([&results]() { results.push_back(2); });

    ASSERT_EQ(queue.size(), 2);
    ASSERT_FALSE(queue.empty());

    queue.drain_all();

    EXPECT_THAT(results, ElementsAre(1, 2));
    ASSERT_EQ(queue.size(), 0);
    ASSERT_TRUE(queue.empty());
}

TEST(CallQueueTest, PushMoveAndDrainAll) {
    CallQueue queue;
    std::vector<int> results;
    std::function<void()> task1 = [&results]() { results.push_back(1); };
    
    queue.push(std::move(task1));
    queue.push([&results]() { results.push_back(2); });

    ASSERT_EQ(queue.size(), 2);
    queue.drain_all();
    EXPECT_THAT(results, ElementsAre(1, 2));
    ASSERT_TRUE(queue.empty());
}


TEST(CallQueueTest, DrainOne) {
    CallQueue queue;
    std::vector<int> results;

    queue.push([&results]() { results.push_back(1); });
    queue.push([&results]() { results.push_back(2); });

    ASSERT_EQ(queue.size(), 2);

    ASSERT_TRUE(queue.drain_one());
    EXPECT_THAT(results, ElementsAre(1));
    ASSERT_EQ(queue.size(), 1);

    ASSERT_TRUE(queue.drain_one());
    EXPECT_THAT(results, ElementsAre(1, 2));
    ASSERT_EQ(queue.size(), 0);

    ASSERT_FALSE(queue.drain_one()); // Empty queue
    ASSERT_TRUE(queue.empty());
}

TEST(CallQueueTest, CoalesceNewTask) {
    CallQueue queue;
    std::string result;

    queue.coalesce("key1", [&result]() { result += "key1_v1"; });
    ASSERT_EQ(queue.size(), 1);

    queue.drain_all();
    EXPECT_EQ(result, "key1_v1");
}

TEST(CallQueueTest, CoalesceUpdateTask) {
    CallQueue queue;
    std::string result;

    queue.coalesce("key1", [&result]() { result += "key1_v1"; });
    queue.coalesce("key2", [&result]() { result += "key2_v1"; });
    queue.coalesce("key1", [&result]() { result += "key1_v2"; }); // Update key1

    ASSERT_EQ(queue.size(), 2); // key1 (updated) and key2

    queue.drain_all();
    // Order depends on initial insertion, not update time for coalesced items
    // If key1 was first, its slot is updated.
    EXPECT_EQ(result, "key1_v2key2_v1"); // Assuming key1 was pushed before key2
}

TEST(CallQueueTest, CoalesceUpdateTaskOrder) {
    CallQueue queue;
    std::vector<std::string> results;

    queue.coalesce("B", [&results](){ results.push_back("B_v1"); });
    queue.coalesce("A", [&results](){ results.push_back("A_v1"); });
    queue.push([&results](){ results.push_back("C_plain"); }); // Plain push
    queue.coalesce("B", [&results](){ results.push_back("B_v2"); }); // Update B

    ASSERT_EQ(queue.size(), 3);
    queue.drain_all();
    // B_v2 (original B slot), A_v1, C_plain
    EXPECT_THAT(results, ElementsAre("B_v2", "A_v1", "C_plain"));
}


TEST(CallQueueTest, CancelTask) {
    CallQueue queue;
    std::string result;

    queue.coalesce("key1", [&result]() { result += "key1"; });
    queue.coalesce("key2", [&result]() { result += "key2"; });
    ASSERT_EQ(queue.size(), 2);

    ASSERT_TRUE(queue.cancel("key1"));
    ASSERT_EQ(queue.size(), 1);

    queue.drain_all();
    EXPECT_EQ(result, "key2");

    ASSERT_FALSE(queue.cancel("key1")); // Already cancelled
    ASSERT_FALSE(queue.cancel("non_existent_key"));
}

TEST(CallQueueTest, CancelTaskUpdatesIndices) {
    CallQueue queue;
    std::vector<std::string> results;

    queue.coalesce("A", [&results](){ results.push_back("A"); }); // index 0
    queue.coalesce("B", [&results](){ results.push_back("B"); }); // index 1
    queue.coalesce("C", [&results](){ results.push_back("C"); }); // index 2

    ASSERT_TRUE(queue.cancel("B")); // Cancel middle element
    ASSERT_EQ(queue.size(), 2);

    // Now A should be at 0, C should be at 1
    // To verify coalesce_map is correct, try to update C
    queue.coalesce("C", [&results](){ results.push_back("C_updated"); });
    ASSERT_EQ(queue.size(), 2); // Still 2, C was updated

    queue.drain_all();
    EXPECT_THAT(results, ElementsAre("A", "C_updated"));
}


TEST(CallQueueTest, MaxSizeLimit) {
    CallQueue queue(2);
    ASSERT_EQ(queue.max_size(), 2);
    std::vector<int> results;

    ASSERT_TRUE(queue.push([&results]() { results.push_back(1); }));
    ASSERT_TRUE(queue.push([&results]() { results.push_back(2); }));
    ASSERT_FALSE(queue.push([&results]() { results.push_back(3); })); // Queue full

    ASSERT_EQ(queue.size(), 2);
    queue.drain_all();
    EXPECT_THAT(results, ElementsAre(1, 2));

    results.clear();
    // Test coalesce with full queue
    ASSERT_TRUE(queue.coalesce("key1", [&results](){ results.push_back(10);})); // 1
    ASSERT_TRUE(queue.coalesce("key2", [&results](){ results.push_back(20);})); // 2
    ASSERT_FALSE(queue.coalesce("key3", [&results](){ results.push_back(30);})); // Full
    
    // Coalescing an existing item should still work even if queue is "full" by count
    // because it replaces an item.
    ASSERT_TRUE(queue.coalesce("key1", [&results](){ results.push_back(11);}));
    ASSERT_EQ(queue.size(), 2);

    queue.drain_all();
    EXPECT_THAT(results, ElementsAre(11, 20));
}

TEST(CallQueueTest, Clear) {
    CallQueue queue;
    bool task_executed = false;

    queue.push([&task_executed]() { task_executed = true; });
    queue.coalesce("key1", [](){});
    ASSERT_EQ(queue.size(), 2);
    ASSERT_FALSE(queue.empty());

    queue.clear();
    ASSERT_EQ(queue.size(), 0);
    ASSERT_TRUE(queue.empty());
    // Also check coalesce_map is cleared
    ASSERT_FALSE(queue.cancel("key1"));


    queue.drain_all(); // Should do nothing
    ASSERT_FALSE(task_executed);
}

TEST(CallQueueTest, Reentrancy) {
    CallQueue queue;
    std::vector<std::string> results;

    queue.push([&]() {
        results.push_back("outer1");
        queue.push([&]() { results.push_back("inner1"); });
    });
    queue.push([&]() {
        results.push_back("outer2");
    });

    ASSERT_EQ(queue.size(), 2);
    queue.drain_all(); // Drains outer1, outer2. Queues inner1.
    EXPECT_THAT(results, ElementsAre("outer1", "outer2"));
    ASSERT_EQ(queue.size(), 1); // inner1 is now in the queue

    results.clear();
    queue.drain_all(); // Drains inner1
    EXPECT_THAT(results, ElementsAre("inner1"));
    ASSERT_TRUE(queue.empty());
}

TEST(CallQueueTest, DrainOneWithCoalesce) {
    CallQueue queue;
    std::vector<std::string> results;

    queue.coalesce("A", [&results](){ results.push_back("A_v1"); }); // idx 0
    queue.push([&results](){ results.push_back("B_plain"); });    // idx 1
    queue.coalesce("C", [&results](){ results.push_back("C_v1"); }); // idx 2

    ASSERT_TRUE(queue.drain_one()); // Drains A_v1
    EXPECT_THAT(results, ElementsAre("A_v1"));
    ASSERT_EQ(queue.size(), 2);

    // Coalesce map for C should be updated. B is not in map.
    // C was at index 2, now at index 1.
    // Let's try to update C
    queue.coalesce("C", [&results](){ results.push_back("C_v2"); });
    ASSERT_EQ(queue.size(), 2); // C updated, not added

    queue.drain_all();
    EXPECT_THAT(results, ElementsAre("A_v1", "B_plain", "C_v2"));
}

TEST(CallQueueTest, CancelAndDrainOneInteraction) {
    CallQueue queue;
    std::vector<std::string> results;

    queue.coalesce("A", [&results](){ results.push_back("A"); }); // idx 0
    queue.coalesce("B", [&results](){ results.push_back("B"); }); // idx 1
    queue.coalesce("C", [&results](){ results.push_back("C"); }); // idx 2

    ASSERT_TRUE(queue.cancel("A")); // Cancel first
    // B is now at 0, C is at 1
    ASSERT_EQ(queue.size(), 2);

    ASSERT_TRUE(queue.drain_one()); // Drains B
    EXPECT_THAT(results, ElementsAre("B"));
    // C is now at 0
    ASSERT_EQ(queue.size(), 1);

    queue.coalesce("C", [&results](){ results.push_back("C_updated"); });
    ASSERT_EQ(queue.size(), 1);

    queue.drain_all();
    EXPECT_THAT(results, ElementsAre("B", "C_updated"));
}

TEST(CallQueueTest, EmptyQueueOperations) {
    CallQueue queue;
    ASSERT_TRUE(queue.empty());
    ASSERT_EQ(queue.size(), 0);
    ASSERT_FALSE(queue.drain_one());
    queue.drain_all(); // Should not crash
    ASSERT_FALSE(queue.cancel("any_key"));
    ASSERT_TRUE(queue.empty());
}


// --- ThreadSafeCallQueue Tests ---

TEST(ThreadSafeCallQueueTest, PushAndDrainAllConcurrent) {
    ThreadSafeCallQueue queue;
    std::atomic<int> counter{0};
    const int num_threads = 4;
    const int tasks_per_thread = 100;

    std::vector<std::thread> producers;
    for (int t = 0; t < num_threads; ++t) {
        producers.emplace_back([&]() {
            for (int i = 0; i < tasks_per_thread; ++i) {
                queue.push([&counter]() { counter.fetch_add(1); });
            }
        });
    }

    // Give some time for threads to push
    while (queue.size() < num_threads * tasks_per_thread / 2 && queue.size() != num_threads * tasks_per_thread) {
         std::this_thread::sleep_for(std::chrono::milliseconds(1));
         if (queue.size() == num_threads * tasks_per_thread) break; // All pushed
    }
    
    size_t current_size = queue.size();
    EXPECT_GT(current_size, 0);

    queue.drain_all(); // Drain whatever is there

    for (auto& t : producers) {
        t.join();
    }

    queue.drain_all(); // Final drain

    EXPECT_EQ(counter.load(), num_threads * tasks_per_thread);
    ASSERT_TRUE(queue.empty());
}

TEST(ThreadSafeCallQueueTest, CoalesceConcurrent) {
    ThreadSafeCallQueue queue;
    std::atomic<int> coalesced_val{0};
    const int num_threads = 4;
    const int updates_per_thread = 50;

    std::vector<std::thread> updaters;
    for (int t = 0; t < num_threads; ++t) {
        updaters.emplace_back([&, t]() {
            for (int i = 0; i < updates_per_thread; ++i) {
                // Each thread attempts to set its own value + iteration
                // The actual value will be one of these, but we want to ensure only one task for "key"
                int val_to_set = t * 1000 + i;
                queue.coalesce("shared_key", [&coalesced_val, val_to_set]() {
                    coalesced_val.store(val_to_set);
                });
            }
        });
    }

    for (auto& t : updaters) {
        t.join();
    }

    ASSERT_EQ(queue.size(), 1); // Only one task for "shared_key"
    queue.drain_all();
    ASSERT_NE(coalesced_val.load(), 0); // It should have been set to one of the values
    ASSERT_TRUE(queue.empty());
}

TEST(ThreadSafeCallQueueTest, DrainOneConcurrent) {
    ThreadSafeCallQueue queue;
    std::atomic<int> tasks_done{0};
    const int total_tasks = 100;

    // Producer
    std::thread producer([&]() {
        for (int i = 0; i < total_tasks; ++i) {
            queue.push([&tasks_done]() { tasks_done.fetch_add(1); });
            std::this_thread::sleep_for(std::chrono::microseconds(100)); // Small delay
        }
    });

    // Consumer
    std::thread consumer([&]() {
        int drained_count = 0;
        while(drained_count < total_tasks) {
            if (queue.drain_one()) {
                drained_count++;
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(50)); // Wait if empty
            }
        }
    });

    producer.join();
    consumer.join();

    EXPECT_EQ(tasks_done.load(), total_tasks);
    ASSERT_TRUE(queue.empty());
}

TEST(ThreadSafeCallQueueTest, CancelConcurrent) {
    ThreadSafeCallQueue queue;
    std::atomic<bool> task_executed{false};

    // Add a task that might be cancelled
    queue.coalesce("cancel_key", [&task_executed]() { task_executed.store(true); });

    std::vector<std::thread> workers;
    // Thread 1: tries to cancel
    workers.emplace_back([&]() {
        queue.cancel("cancel_key");
    });
    // Thread 2: tries to drain (might execute before cancel or not)
    workers.emplace_back([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(5)); // Give cancel a chance
        queue.drain_all();
    });

    for(auto& t : workers) {
        t.join();
    }
    // We don't know for sure if it executed or was cancelled first,
    // but the operations should be safe.
    // If it was cancelled, size is 0. If executed, size is 0.
    ASSERT_TRUE(queue.empty());
    // If task_executed is true, it means drain happened before cancel or cancel failed.
    // If false, cancel happened first. Both are valid outcomes in terms of safety.
}


TEST(ThreadSafeCallQueueTest, MixedOperationsStress) {
    ThreadSafeCallQueue queue(50); // Max size to test full queue logic
    std::atomic<int> push_count{0};
    std::atomic<int> executed_count{0};
    std::atomic<int> coalesced_final_val{0};
    std::set<std::string> unique_executed_coalesced; // Using set for thread-safe check later

    const int num_threads = 5;
    const int ops_per_thread = 200;

    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < ops_per_thread; ++i) {
                int op_type = i % 5;
                std::string key = "key" + std::to_string(i % 10); // A few shared keys

                if (op_type == 0) { // Push
                    if (queue.push([&executed_count]() { executed_count.fetch_add(1); })) {
                        push_count.fetch_add(1);
                    }
                } else if (op_type == 1) { // Coalesce
                    int val = t * 1000 + i;
                    queue.coalesce(key, [&executed_count, &coalesced_final_val, val, key, &unique_executed_coalesced]() {
                        executed_count.fetch_add(1);
                        coalesced_final_val.store(val); // This will be racy by design, last one wins
                        // To check which ones actually ran:
                        // Not perfectly thread-safe to insert here, but good enough for stress test outcome
                        // unique_executed_coalesced.insert(key + "_" + std::to_string(val));
                    });
                } else if (op_type == 2) { // Drain one
                    queue.drain_one();
                } else if (op_type == 3) { // Cancel
                    queue.cancel(key);
                } else { // Drain all
                    if (i % 20 == 0) { // Not too often
                         queue.drain_all();
                    }
                }
                if (i % 10 == 0) std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    queue.drain_all(); // Final drain

    // We can't know exact counts due to concurrency and cancellations
    // But we want to ensure it doesn't crash and some tasks got through
    EXPECT_GT(executed_count.load(), 0);
    EXPECT_TRUE(queue.empty());
    std::cout << "Stress test: Pushed (approx): " << push_count.load()
              << ", Executed: " << executed_count.load()
              << ", Final coalesced val for some key: " << coalesced_final_val.load() << std::endl;
}

TEST(ThreadSafeCallQueueTest, MaxSizeConcurrent) {
    ThreadSafeCallQueue queue(10); // Small max size
    std::atomic<int> successful_pushes{0};
    const int num_threads = 5;
    const int pushes_per_thread = 20; // More than max_size

    std::vector<std::thread> producers;
    for (int t = 0; t < num_threads; ++t) {
        producers.emplace_back([&]() {
            for (int i = 0; i < pushes_per_thread; ++i) {
                if (queue.push([](){ /* no-op */ })) {
                    successful_pushes.fetch_add(1);
                }
                 // Brief sleep to allow other threads to interleave and fill/drain the queue
                std::this_thread::sleep_for(std::chrono::microseconds(t*10 + i*5));
            }
        });
    }

    std::thread consumer([&]() {
        int drained = 0;
        while(drained < num_threads * pushes_per_thread && successful_pushes.load() < num_threads * pushes_per_thread) { // Heuristic stop
             if(queue.drain_one()) drained++;
             else std::this_thread::sleep_for(std::chrono::milliseconds(1));
             if(successful_pushes.load() >= num_threads * pushes_per_thread && queue.empty()) break; // All pushed and drained
        }
        // Drain any remaining
        while(queue.drain_one()) {drained++;}
    });


    for (auto& t : producers) {
        t.join();
    }
    consumer.join();
    queue.drain_all();


    // successful_pushes can be up to total pushes if consumer is fast enough,
    // or less if queue gets full.
    // The key is that queue.size() should not exceed max_size.
    // This is hard to assert directly without instrumenting the queue or complex sync.
    // We mainly test for stability and that it doesn't deadlock.
    EXPECT_LE(queue.size(), queue.max_size());
    std::cout << "MaxSizeConcurrent: Successful pushes: " << successful_pushes.load() << std::endl;
    ASSERT_TRUE(queue.empty());
}

TEST(ThreadSafeCallQueueTest, ThreadSafeDrainAllReentrancy) {
    ThreadSafeCallQueue queue;
    std::atomic<int> count{0};

    // Task that re-queues itself once
    std::function<void()> reentrant_task = [&]() {
        int current_val = count.fetch_add(1);
        if (current_val == 0) { // First execution
            queue.push(reentrant_task); // Re-queue
        }
    };

    queue.push(reentrant_task);

    std::thread t1([&](){ queue.drain_all(); }); // First drain
    t1.join();

    // After t1, the task has run once, and re-queued itself.
    // Count should be 1. Queue should have 1 item.
    // Need a small delay to ensure drain_all completes and re-queued item is visible.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));


    EXPECT_EQ(count.load(), 1);
    EXPECT_EQ(queue.size(), 1);


    std::thread t2([&](){ queue.drain_all(); }); // Second drain
    t2.join();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));


    EXPECT_EQ(count.load(), 2); // Task ran twice
    EXPECT_TRUE(queue.empty());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

```

#include "gtest/gtest.h"
#include "retain_latest.h"
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>

using namespace retain_latest;
using namespace std::chrono_literals;

// --- Tests for RetainLatest<T> ---

TEST(RetainLatestTest, DefaultConstructor) {
    RetainLatest<int> rl_int;
    EXPECT_FALSE(rl_int.has_value());
    EXPECT_FALSE(rl_int.peek().has_value());
    EXPECT_FALSE(rl_int.consume().has_value());

    RetainLatest<std::string> rl_str;
    EXPECT_FALSE(rl_str.has_value());
    EXPECT_FALSE(rl_str.peek().has_value());
    EXPECT_FALSE(rl_str.consume().has_value());
}

TEST(RetainLatestTest, UpdateAndPeek) {
    RetainLatest<int> rl;
    rl.update(42);
    ASSERT_TRUE(rl.has_value());
    ASSERT_TRUE(rl.peek().has_value());
    EXPECT_EQ(rl.peek().value(), 42);
    // Peek should not remove the value
    ASSERT_TRUE(rl.has_value());
    EXPECT_EQ(rl.peek().value(), 42);
}

TEST(RetainLatestTest, UpdateAndConsume) {
    RetainLatest<std::string> rl;
    rl.update("hello");
    ASSERT_TRUE(rl.has_value());

    auto val = rl.consume();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), "hello");

    // Consume should remove the value
    EXPECT_FALSE(rl.has_value());
    EXPECT_FALSE(rl.peek().has_value());
    EXPECT_FALSE(rl.consume().has_value());
}

TEST(RetainLatestTest, Emplace) {
    RetainLatest<std::pair<int, std::string>> rl;
    rl.emplace(10, "world");
    ASSERT_TRUE(rl.has_value());
    ASSERT_TRUE(rl.peek().has_value());
    EXPECT_EQ(rl.peek()->first, 10);
    EXPECT_EQ(rl.peek()->second, "world");

    auto val = rl.consume();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val->first, 10);
    EXPECT_EQ(val->second, "world");
    EXPECT_FALSE(rl.has_value());
}

TEST(RetainLatestTest, OverwriteBehavior) {
    RetainLatest<int> rl;
    rl.update(1);
    rl.update(2);
    rl.update(3); // Only this should be retained

    ASSERT_TRUE(rl.has_value());
    EXPECT_EQ(rl.peek().value(), 3);

    auto val = rl.consume();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), 3);
    EXPECT_FALSE(rl.has_value());
}

TEST(RetainLatestTest, Clear) {
    RetainLatest<int> rl;
    rl.update(100);
    ASSERT_TRUE(rl.has_value());

    rl.clear();
    EXPECT_FALSE(rl.has_value());
    EXPECT_FALSE(rl.peek().has_value());
    EXPECT_FALSE(rl.consume().has_value());

    // Clear on empty
    rl.clear();
    EXPECT_FALSE(rl.has_value());
}

TEST(RetainLatestTest, OnUpdateCallback) {
    RetainLatest<std::string> rl;
    std::string callback_val;
    int callback_count = 0;

    rl.on_update([&](const std::string& val) {
        callback_val = val;
        callback_count++;
    });

    rl.update("one");
    EXPECT_EQ(callback_count, 1);
    EXPECT_EQ(callback_val, "one");
    EXPECT_EQ(rl.peek().value(), "one");

    rl.update("two");
    EXPECT_EQ(callback_count, 2);
    EXPECT_EQ(callback_val, "two");
    EXPECT_EQ(rl.peek().value(), "two");

    rl.emplace("three");
    EXPECT_EQ(callback_count, 3);
    EXPECT_EQ(callback_val, "three");
    EXPECT_EQ(rl.peek().value(), "three");
}

TEST(RetainLatestTest, MoveUpdate) {
    RetainLatest<std::unique_ptr<int>> rl;
    auto ptr = std::make_unique<int>(123);
    rl.update(std::move(ptr));

    ASSERT_TRUE(rl.has_value());
    auto consumed_ptr_opt = rl.consume();
    ASSERT_TRUE(consumed_ptr_opt.has_value());
    ASSERT_NE(consumed_ptr_opt.value(), nullptr);
    EXPECT_EQ(*(consumed_ptr_opt.value()), 123);
    EXPECT_FALSE(rl.has_value());
}

// --- Tests for VersionedRetainLatest<T> ---

TEST(VersionedRetainLatestTest, DefaultConstructor) {
    VersionedRetainLatest<int> vrl_int;
    EXPECT_FALSE(vrl_int.has_value());
    EXPECT_FALSE(vrl_int.peek().has_value());
    EXPECT_FALSE(vrl_int.consume().has_value());
    EXPECT_FALSE(vrl_int.current_version().has_value());
}

TEST(VersionedRetainLatestTest, UpdateAndPeek) {
    VersionedRetainLatest<int> vrl;
    vrl.update(42); // Version 0
    ASSERT_TRUE(vrl.has_value());
    ASSERT_TRUE(vrl.peek().has_value());
    EXPECT_EQ(vrl.peek()->value, 42);
    EXPECT_EQ(vrl.peek()->version, 0);
    EXPECT_EQ(vrl.current_version().value(), 0);

    // Peek should not remove the value or change version
    ASSERT_TRUE(vrl.has_value());
    EXPECT_EQ(vrl.peek()->value, 42);
    EXPECT_EQ(vrl.peek()->version, 0);
}

TEST(VersionedRetainLatestTest, UpdateAndConsume) {
    VersionedRetainLatest<std::string> vrl;
    vrl.update("hello"); // Version 0
    ASSERT_TRUE(vrl.has_value());

    auto val_opt = vrl.consume();
    ASSERT_TRUE(val_opt.has_value());
    EXPECT_EQ(val_opt->value, "hello");
    EXPECT_EQ(val_opt->version, 0);

    // Consume should remove the value
    EXPECT_FALSE(vrl.has_value());
    EXPECT_FALSE(vrl.peek().has_value());
    EXPECT_FALSE(vrl.consume().has_value());
    EXPECT_FALSE(vrl.current_version().has_value());
}

TEST(VersionedRetainLatestTest, Emplace) {
    VersionedRetainLatest<std::pair<int, std::string>> vrl;
    vrl.emplace(10, "world"); // Version 0
    ASSERT_TRUE(vrl.has_value());
    ASSERT_TRUE(vrl.peek().has_value());
    EXPECT_EQ(vrl.peek()->value.first, 10);
    EXPECT_EQ(vrl.peek()->value.second, "world");
    EXPECT_EQ(vrl.peek()->version, 0);

    auto val_opt = vrl.consume();
    ASSERT_TRUE(val_opt.has_value());
    EXPECT_EQ(val_opt->value.first, 10);
    EXPECT_EQ(val_opt->value.second, "world");
    EXPECT_EQ(val_opt->version, 0);
    EXPECT_FALSE(vrl.has_value());
}

TEST(VersionedRetainLatestTest, VersionIncrement) {
    VersionedRetainLatest<int> vrl;
    vrl.update(1); // Version 0
    EXPECT_EQ(vrl.peek()->version, 0);
    EXPECT_EQ(vrl.current_version().value(), 0);

    vrl.update(2); // Version 1
    EXPECT_EQ(vrl.peek()->value, 2);
    EXPECT_EQ(vrl.peek()->version, 1);
    EXPECT_EQ(vrl.current_version().value(), 1);

    vrl.emplace(3); // Version 2
    EXPECT_EQ(vrl.peek()->value, 3);
    EXPECT_EQ(vrl.peek()->version, 2);
    EXPECT_EQ(vrl.current_version().value(), 2);

    auto val_opt = vrl.consume();
    ASSERT_TRUE(val_opt.has_value());
    EXPECT_EQ(val_opt->value, 3);
    EXPECT_EQ(val_opt->version, 2);

    // After consume, version should be gone
    EXPECT_FALSE(vrl.current_version().has_value());

    vrl.update(4); // Version should be 3 (next_version_ continues)
    EXPECT_EQ(vrl.peek()->value, 4);
    EXPECT_EQ(vrl.peek()->version, 3);
    EXPECT_EQ(vrl.current_version().value(), 3);
}

TEST(VersionedRetainLatestTest, Clear) {
    VersionedRetainLatest<int> vrl;
    vrl.update(100); // Version 0
    ASSERT_TRUE(vrl.has_value());
    ASSERT_TRUE(vrl.current_version().has_value());

    vrl.clear();
    EXPECT_FALSE(vrl.has_value());
    EXPECT_FALSE(vrl.peek().has_value());
    EXPECT_FALSE(vrl.consume().has_value());
    EXPECT_FALSE(vrl.current_version().has_value());

    // Clear on empty
    vrl.clear();
    EXPECT_FALSE(vrl.has_value());
}

TEST(VersionedRetainLatestTest, OnUpdateCallback) {
    VersionedRetainLatest<std::string> vrl;
    Versioned<std::string> callback_val_versioned("", 0);
    int callback_count = 0;

    vrl.on_update([&](const Versioned<std::string>& val_ver) {
        callback_val_versioned = val_ver;
        callback_count++;
    });

    vrl.update("one"); // Version 0
    EXPECT_EQ(callback_count, 1);
    EXPECT_EQ(callback_val_versioned.value, "one");
    EXPECT_EQ(callback_val_versioned.version, 0);
    EXPECT_EQ(vrl.peek()->value, "one");
    EXPECT_EQ(vrl.peek()->version, 0);

    vrl.update("two"); // Version 1
    EXPECT_EQ(callback_count, 2);
    EXPECT_EQ(callback_val_versioned.value, "two");
    EXPECT_EQ(callback_val_versioned.version, 1);
    EXPECT_EQ(vrl.peek()->value, "two");
    EXPECT_EQ(vrl.peek()->version, 1);

    vrl.emplace("three"); // Version 2
    EXPECT_EQ(callback_count, 3);
    EXPECT_EQ(callback_val_versioned.value, "three");
    EXPECT_EQ(callback_val_versioned.version, 2);
    EXPECT_EQ(vrl.peek()->value, "three");
    EXPECT_EQ(vrl.peek()->version, 2);
}

TEST(VersionedRetainLatestTest, IsStale) {
    VersionedRetainLatest<int> vrl;

    // Initially, nothing is stale as there's no value
    EXPECT_FALSE(vrl.is_stale(0));
    EXPECT_FALSE(vrl.is_stale(100));

    vrl.update(10); // Version 0
    EXPECT_FALSE(vrl.is_stale(0)); // Consumer has same version
    EXPECT_FALSE(vrl.is_stale(1)); // Consumer has newer version (unlikely but test)

    vrl.update(20); // Version 1
    EXPECT_TRUE(vrl.is_stale(0));  // Consumer has version 0, current is 1
    EXPECT_FALSE(vrl.is_stale(1)); // Consumer has same version
    EXPECT_FALSE(vrl.is_stale(2)); // Consumer has newer version
}

TEST(VersionedRetainLatestTest, CompareAndUpate) {
    VersionedRetainLatest<std::string> vrl;

    // Initial update
    vrl.update("initial"); // Version 0
    ASSERT_EQ(vrl.peek()->value, "initial");
    ASSERT_EQ(vrl.peek()->version, 0);

    // Successful CAS: expected version matches current (0)
    bool cas_success = vrl.compare_and_update("cas_success_1", 0);
    EXPECT_TRUE(cas_success);
    ASSERT_TRUE(vrl.peek().has_value());
    EXPECT_EQ(vrl.peek()->value, "cas_success_1");
    EXPECT_EQ(vrl.peek()->version, 1); // Version increments

    // Failed CAS: expected version (0) is stale, current is 1
    bool cas_fail_stale = vrl.compare_and_update("cas_fail_stale", 0);
    EXPECT_FALSE(cas_fail_stale);
    ASSERT_TRUE(vrl.peek().has_value());
    EXPECT_EQ(vrl.peek()->value, "cas_success_1"); // Value remains unchanged
    EXPECT_EQ(vrl.peek()->version, 1);             // Version remains unchanged

    // Successful CAS again: expected version matches current (1)
    bool cas_success_2 = vrl.compare_and_update("cas_success_2", 1);
    EXPECT_TRUE(cas_success_2);
    ASSERT_TRUE(vrl.peek().has_value());
    EXPECT_EQ(vrl.peek()->value, "cas_success_2");
    EXPECT_EQ(vrl.peek()->version, 2); // Version increments

    // CAS on empty buffer (after clear)
    vrl.clear();
    EXPECT_FALSE(vrl.current_version().has_value());
    bool cas_on_empty = vrl.compare_and_update("cas_on_empty", 0); // Expected version doesn't matter if no value_ptr_
    EXPECT_FALSE(cas_on_empty);
    EXPECT_FALSE(vrl.has_value());

    // CAS on empty buffer (after consume)
    vrl.update("re-init"); // Version 3 (next_version_ keeps incrementing)
    ASSERT_EQ(vrl.peek()->version, 3);
    vrl.consume();
    EXPECT_FALSE(vrl.current_version().has_value());
    bool cas_on_consumed = vrl.compare_and_update("cas_on_consumed", 3);
    EXPECT_FALSE(cas_on_consumed);
    EXPECT_FALSE(vrl.has_value());
}

// --- Concurrency Tests (Basic) ---
// These tests are basic and might not catch all race conditions without more sophisticated stress testing.

TEST(RetainLatestTest, ConcurrentUpdatesAndConsume) {
    RetainLatest<int> rl;
    std::atomic<int> sum_consumed{0};
    std::atomic<int> items_consumed{0};
    const int num_producers = 4;
    const int updates_per_producer = 100;

    std::vector<std::thread> producers;
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&rl, i, updates_per_producer]() {
            for (int j = 0; j < updates_per_producer; ++j) {
                rl.update(i * updates_per_producer + j);
                std::this_thread::sleep_for(1us); // Small delay
            }
        });
    }

    std::thread consumer([&]() {
        int consumed_count = 0;
        while(consumed_count < updates_per_producer * num_producers / 10 && items_consumed < 100) { // Limit consumption for test termination
            if (auto val_opt = rl.consume()) {
                sum_consumed += val_opt.value();
                items_consumed++;
            }
            std::this_thread::sleep_for(5us);
            consumed_count++;
        }
        // Try one last consume after producers likely done
        std::this_thread::sleep_for(50ms);
         if (auto val_opt = rl.consume()) {
            sum_consumed += val_opt.value();
            items_consumed++;
        }
    });

    for (auto& p : producers) {
        p.join();
    }
    consumer.join();

    EXPECT_GT(items_consumed, 0); // At least one item should have been consumed.
    // The exact value of sum_consumed is non-deterministic due to dropped updates.
    // The final value, if any, should be among the last produced.
    std::cout << "ConcurrentUpdatesAndConsume: Consumed " << items_consumed << " items." << std::endl;
}


TEST(VersionedRetainLatestTest, ConcurrentVersionedUpdatesAndPeek) {
    VersionedRetainLatest<int> vrl;
    const int num_producers = 3;
    const int updates_per_producer = 50;
    std::atomic<bool> keep_peeking{true};

    std::vector<std::thread> producers;
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&vrl, i, updates_per_producer]() {
            for (int j = 0; j < updates_per_producer; ++j) {
                vrl.update(i * updates_per_producer + j); // Value helps identify producer
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
    }

    std::thread peeker([&]() {
        uint64_t max_seen_version = 0;
        int distinct_values_seen = 0;
        std::vector<int> values;

        while (keep_peeking.load()) {
            if (auto val_ver_opt = vrl.peek()) {
                if (val_ver_opt->version > max_seen_version) {
                    max_seen_version = val_ver_opt->version;
                     if (std::find(values.begin(), values.end(), val_ver_opt->value) == values.end()) {
                        values.push_back(val_ver_opt->value);
                        distinct_values_seen++;
                    }
                }
                EXPECT_GE(val_ver_opt->version, 0); // Version should be non-negative
            }
            std::this_thread::sleep_for(std::chrono::microseconds(5));
        }
        // Final peek
        if (auto val_ver_opt = vrl.peek()) {
             if (val_ver_opt->version > max_seen_version) {
                max_seen_version = val_ver_opt->version;
             }
        }
        std::cout << "Peeker saw max version: " << max_seen_version << std::endl;
        std::cout << "Peeker saw distinct values: " << distinct_values_seen << std::endl;

        // Max version should be related to total updates, but not necessarily total_updates - 1
        // because next_version_ is shared. It should be less than total_updates.
        EXPECT_LT(max_seen_version, num_producers * updates_per_producer);
        EXPECT_GT(distinct_values_seen, 0); // Should see some values
    });

    for (auto& p : producers) {
        p.join();
    }
    keep_peeking.store(false); // Signal peeker to stop
    peeker.join();

    // Final state check
    auto final_state = vrl.peek();
    ASSERT_TRUE(final_state.has_value());
    // The final version should be (num_producers * updates_per_producer - 1)
    // because each update increments the shared next_version_ counter.
    EXPECT_EQ(final_state->version, num_producers * updates_per_producer - 1);
}

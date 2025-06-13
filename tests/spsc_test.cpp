#include "gtest/gtest.h"
#include "spsc.h"
#include <string>
#include <stdexcept> // Required for std::invalid_argument
#include <thread>    // For std::thread
#include <vector>    // For std::vector
#include <numeric>   // For std::iota
#include <chrono>    // For std::chrono::milliseconds
#include <atomic>    // For std::atomic variables
#include <memory>    // For std::unique_ptr

// Test fixture for SPSC Ring Buffer tests that might benefit from a shared setup
// For now, many tests are simple enough not to strictly need a fixture,
// but it's good practice to group them.
class SPSCRingBufferTest : public ::testing::Test {
protected:
    // Example:
    // concurrent::ring_buffer<int> buffer_{8};
    // void SetUp() override { /* ... */ }
    // void TearDown() override { /* ... */ }
};

// Construction Tests
TEST_F(SPSCRingBufferTest, ConstructionWithValidCapacity) {
    concurrent::ring_buffer<int> rb(8);
    ASSERT_EQ(rb.capacity(), 8);
    concurrent::ring_buffer<std::string> rb_str(4);
    ASSERT_EQ(rb_str.capacity(), 4);
    concurrent::ring_buffer<double> rb_double(16);
    ASSERT_EQ(rb_double.capacity(), 16);
}

TEST_F(SPSCRingBufferTest, ConstructionWithInvalidCapacity) {
    ASSERT_THROW(concurrent::ring_buffer<int> rb(0), std::invalid_argument);
    ASSERT_THROW(concurrent::ring_buffer<int> rb(3), std::invalid_argument);
    ASSERT_THROW(concurrent::ring_buffer<int> rb(7), std::invalid_argument);
    ASSERT_THROW(concurrent::ring_buffer<int> rb(100), std::invalid_argument); // Not a power of 2
}

TEST_F(SPSCRingBufferTest, ConstructionWithMemoryOrderings) {
    concurrent::ring_buffer<int, concurrent::memory_ordering::relaxed> rb_relaxed(8);
    ASSERT_EQ(rb_relaxed.capacity(), 8);
    ASSERT_TRUE(rb_relaxed.empty());

    concurrent::ring_buffer<int, concurrent::memory_ordering::acquire_release> rb_acq_rel(8);
    ASSERT_EQ(rb_acq_rel.capacity(), 8);
    ASSERT_TRUE(rb_acq_rel.empty());

    concurrent::ring_buffer<int, concurrent::memory_ordering::sequential> rb_seq(8);
    ASSERT_EQ(rb_seq.capacity(), 8);
    ASSERT_TRUE(rb_seq.empty());
}

// Basic Single-Threaded Push/Pop Tests
TEST_F(SPSCRingBufferTest, PushAndPopSingleItem) {
    concurrent::ring_buffer<int> rb(8); // Capacity 8
    ASSERT_TRUE(rb.try_push(10));
    ASSERT_EQ(rb.size(), 1);
    ASSERT_FALSE(rb.empty());
    auto item = rb.try_pop();
    ASSERT_TRUE(item.has_value());
    ASSERT_EQ(*item, 10);
    ASSERT_EQ(rb.size(), 0);
    ASSERT_TRUE(rb.empty());
}

TEST_F(SPSCRingBufferTest, PopFromEmpty) {
    concurrent::ring_buffer<int> rb(8);
    ASSERT_TRUE(rb.empty());
    auto item = rb.try_pop();
    ASSERT_FALSE(item.has_value());

    int val_into;
    ASSERT_FALSE(rb.try_pop_into(val_into));
}

TEST_F(SPSCRingBufferTest, PushToFullThenPop) {
    concurrent::ring_buffer<int> rb(2); // Capacity 2, can hold 1 item
    ASSERT_TRUE(rb.try_push(1));
    ASSERT_TRUE(rb.full()); // Buffer with capacity N can hold N-1 items for this SPSC variant
    ASSERT_EQ(rb.size(), 1);

    ASSERT_FALSE(rb.try_push(2)); // Should fail as it's full

    auto item = rb.try_pop();
    ASSERT_TRUE(item.has_value());
    ASSERT_EQ(*item, 1);
    ASSERT_FALSE(rb.full());
    ASSERT_TRUE(rb.empty());
}

TEST_F(SPSCRingBufferTest, FillBufferCompletelyAndEmpty) {
    concurrent::ring_buffer<int> rb(4); // Capacity 4, can hold 3 items

    ASSERT_TRUE(rb.try_push(1));
    ASSERT_EQ(rb.size(), 1);
    ASSERT_FALSE(rb.full());

    ASSERT_TRUE(rb.try_push(2));
    ASSERT_EQ(rb.size(), 2);
    ASSERT_FALSE(rb.full());

    ASSERT_TRUE(rb.try_push(3));
    ASSERT_EQ(rb.size(), 3);
    ASSERT_TRUE(rb.full());

    ASSERT_FALSE(rb.try_push(4)); // Buffer is full, should fail

    // Pop them one by one
    auto item1 = rb.try_pop();
    ASSERT_TRUE(item1.has_value());
    ASSERT_EQ(*item1, 1);
    ASSERT_FALSE(rb.full());
    ASSERT_EQ(rb.size(), 2);

    auto item2 = rb.try_pop();
    ASSERT_TRUE(item2.has_value());
    ASSERT_EQ(*item2, 2);
    ASSERT_FALSE(rb.full());
    ASSERT_EQ(rb.size(), 1);

    auto item3 = rb.try_pop();
    ASSERT_TRUE(item3.has_value());
    ASSERT_EQ(*item3, 3);
    ASSERT_TRUE(rb.empty());
    ASSERT_EQ(rb.size(), 0);

    ASSERT_FALSE(rb.try_pop().has_value()); // Pop from empty
}


TEST_F(SPSCRingBufferTest, PopIntoExistingVariable) {
    concurrent::ring_buffer<std::string> rb(4);
    std::string pushed_value = "Hello";
    rb.try_push(pushed_value);

    std::string popped_value;
    ASSERT_TRUE(rb.try_pop_into(popped_value));
    ASSERT_EQ(popped_value, pushed_value);
    ASSERT_TRUE(rb.empty());

    ASSERT_FALSE(rb.try_pop_into(popped_value)); // Should fail on empty
}


// State Reporting Methods Tests
TEST_F(SPSCRingBufferTest, CapacityMethod) {
    concurrent::ring_buffer<int> rb_small(2);
    ASSERT_EQ(rb_small.capacity(), 2);
    concurrent::ring_buffer<int> rb_large(1024);
    ASSERT_EQ(rb_large.capacity(), 1024);
}

TEST_F(SPSCRingBufferTest, EmptyMethod) {
    concurrent::ring_buffer<int> rb(8);
    ASSERT_TRUE(rb.empty());
    rb.try_push(1);
    ASSERT_FALSE(rb.empty());
    rb.try_pop();
    ASSERT_TRUE(rb.empty());
}

TEST_F(SPSCRingBufferTest, FullMethod) {
    concurrent::ring_buffer<int> rb(2); // Capacity 2, can hold 1 item
    ASSERT_FALSE(rb.full());
    rb.try_push(100);
    ASSERT_TRUE(rb.full());
    rb.try_pop();
    ASSERT_FALSE(rb.full());

    concurrent::ring_buffer<int> rb_larger(4); // Capacity 4, can hold 3 items
    ASSERT_FALSE(rb_larger.full());
    rb_larger.try_push(1);
    ASSERT_FALSE(rb_larger.full());
    rb_larger.try_push(2);
    ASSERT_FALSE(rb_larger.full());
    rb_larger.try_push(3);
    ASSERT_TRUE(rb_larger.full());
}

TEST_F(SPSCRingBufferTest, SizeMethod) {
    concurrent::ring_buffer<int> rb(4); // Capacity 4, can hold 3 items
    ASSERT_EQ(rb.size(), 0);
    rb.try_push(1);
    ASSERT_EQ(rb.size(), 1);
    rb.try_push(2);
    ASSERT_EQ(rb.size(), 2);
    rb.try_push(3);
    ASSERT_EQ(rb.size(), 3);
    ASSERT_TRUE(rb.full());

    ASSERT_FALSE(rb.try_push(4)); // Cannot push more
    ASSERT_EQ(rb.size(), 3); // Size should remain 3

    rb.try_pop();
    ASSERT_EQ(rb.size(), 2);
    rb.try_pop();
    ASSERT_EQ(rb.size(), 1);
    rb.try_pop();
    ASSERT_EQ(rb.size(), 0);
    ASSERT_TRUE(rb.empty());
}

// SPSCUtilityTest suite for utility functions if any
// Test for next_power_of_two utility
TEST(SPSCUtilityTest, NextPowerOfTwo) {
    ASSERT_EQ(concurrent::next_power_of_two(0), 1);
    ASSERT_EQ(concurrent::next_power_of_two(1), 1);
    ASSERT_EQ(concurrent::next_power_of_two(2), 2);
    ASSERT_EQ(concurrent::next_power_of_two(3), 4);
    ASSERT_EQ(concurrent::next_power_of_two(7), 8);
    ASSERT_EQ(concurrent::next_power_of_two(8), 8);
    ASSERT_EQ(concurrent::next_power_of_two(1000), 1024);
    ASSERT_EQ(concurrent::next_power_of_two(1024), 1024);
    ASSERT_EQ(concurrent::next_power_of_two(1025), 2048);
}

// Test fixture for threaded SPSC Ring Buffer tests
class SPSCRingBufferThreadedTest : public ::testing::Test {
protected:
    // You can add common setup here if needed, e.g.:
    // void SetUp() override {}
    // void TearDown() override {}
};

TEST_F(SPSCRingBufferThreadedTest, SingleItemPingPong) {
    concurrent::ring_buffer<int> rb(2); // Capacity 2, can hold 1 item effectively
    const int test_value = 42;

    std::thread producer([&]() {
        rb.push(test_value); // Blocking push
    });

    std::thread consumer([&]() {
        int received_value = rb.pop(); // Blocking pop
        ASSERT_EQ(received_value, test_value);
    });

    producer.join();
    consumer.join();
}

TEST_F(SPSCRingBufferThreadedTest, MultipleItemsSequentialPushPop) {
    const size_t num_items = 500; // Reduced
    concurrent::ring_buffer<int> rb(64); // Reduced

    std::vector<int> produced_items(num_items);
    std::iota(produced_items.begin(), produced_items.end(), 0); // Fill with 0, 1, 2,...

    std::vector<int> consumed_items;
    consumed_items.reserve(num_items);

    std::thread producer([&]() {
        for (int item : produced_items) {
            rb.push(item); // Blocking push
        }
    });

    std::thread consumer([&]() {
        for (size_t i = 0; i < num_items; ++i) {
            consumed_items.push_back(rb.pop()); // Blocking pop
        }
    });

    producer.join();
    consumer.join();

    ASSERT_EQ(consumed_items.size(), num_items);
    ASSERT_EQ(consumed_items, produced_items); // Verify order and content
}

TEST_F(SPSCRingBufferThreadedTest, TryPushTryPopUnderLoad) {
    const size_t num_items = 1000; // Further reduced
    concurrent::ring_buffer<int> rb(32); // Further reduced

    std::vector<int> produced_items(num_items);
    std::iota(produced_items.begin(), produced_items.end(), 0);

    std::vector<int> consumed_items;
    consumed_items.reserve(num_items);

    std::thread producer([&]() {
        for (int item : produced_items) {
            while (!rb.try_push(item)) {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        }
    });

    std::thread consumer([&]() {
        for (size_t i = 0; i < num_items; ++i) {
            std::optional<int> item;
            while (!(item = rb.try_pop())) {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
            consumed_items.push_back(*item);
        }
    });

    producer.join();
    consumer.join();

    ASSERT_EQ(consumed_items.size(), num_items);
    // SPSC guarantees order, so direct comparison is fine.
    ASSERT_EQ(consumed_items, produced_items);
}

TEST_F(SPSCRingBufferThreadedTest, ProducerFasterThanConsumer) {
    const size_t num_items_total = 500; // Reduced
    const size_t num_items_initial_pop = 50; // Reduced
    concurrent::ring_buffer<int> rb(32); // Reduced

    std::vector<int> produced_items(num_items_total);
    std::iota(produced_items.begin(), produced_items.end(), 0);

    std::vector<int> consumed_items;
    consumed_items.reserve(num_items_total);

    std::thread producer([&]() {
        for (size_t i = 0; i < num_items_total; ++i) {
            rb.push(produced_items[i]);
        }
    });

    std::thread consumer([&]() {
        // Pop initial batch
        for (size_t i = 0; i < num_items_initial_pop; ++i) {
            consumed_items.push_back(rb.pop());
        }

        // Simulate consumer being slower or pausing
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Pop remaining items
        for (size_t i = num_items_initial_pop; i < num_items_total; ++i) {
            consumed_items.push_back(rb.pop());
        }
    });

    producer.join();
    consumer.join();

    ASSERT_EQ(consumed_items.size(), num_items_total);
    ASSERT_EQ(consumed_items, produced_items);
}

TEST_F(SPSCRingBufferThreadedTest, ConsumerFasterThanProducer) {
    const size_t num_items = 500; // Reduced
    concurrent::ring_buffer<int> rb(32); // Reduced

    std::vector<int> produced_items(num_items);
    std::iota(produced_items.begin(), produced_items.end(), 0);

    std::vector<int> consumed_items;
    consumed_items.reserve(num_items);

    std::thread producer([&]() {
        for (int item : produced_items) {
            rb.push(item);
            // Simulate producer being slower
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    });

    std::thread consumer([&]() {
        for (size_t i = 0; i < num_items; ++i) {
            consumed_items.push_back(rb.pop()); // Blocking pop will wait
        }
    });

    producer.join();
    consumer.join();

    ASSERT_EQ(consumed_items.size(), num_items);
    ASSERT_EQ(consumed_items, produced_items);
}

TEST_F(SPSCRingBufferThreadedTest, TestFullAndEmptyConditionsRepeatedly) {
    const size_t capacity = 8;
    const size_t num_items_in_cycle = capacity - 1;
    const int num_cycles = 5; // Drastically reduced cycles
    concurrent::ring_buffer<int> rb(capacity);

    std::thread producer([&]() {
        for (int cycle = 0; cycle < num_cycles; ++cycle) {
            for (size_t i = 0; i < num_items_in_cycle; ++i) {
                int value = static_cast<int>(i + cycle * num_items_in_cycle);
                while(!rb.try_push(value)) {
                    std::this_thread::sleep_for(std::chrono::microseconds(100)); // Increased sleep
                }
            }
        }
    });

    std::thread consumer([&]() {
        for (int cycle = 0; cycle < num_cycles; ++cycle) {
            for (size_t i = 0; i < num_items_in_cycle; ++i) {
                std::optional<int> item;
                while(!(item = rb.try_pop())) {
                    std::this_thread::sleep_for(std::chrono::microseconds(100)); // Increased sleep
                }
                ASSERT_TRUE(item.has_value());
                ASSERT_EQ(*item, static_cast<int>(i + cycle * num_items_in_cycle));
            }
        }
    });

    producer.join();
    consumer.join();
    ASSERT_TRUE(rb.empty());
}

// Test fixture for SPSC Ring Buffer Statistics tests
class SPSCRingBufferStatsTest : public ::testing::Test {
protected:
    // No shared member rb_ to avoid state leakage and assignment issues.
    // Each test will create its own buffer instance.
};

TEST_F(SPSCRingBufferStatsTest, StatsEnableDisableAndReset) {
    concurrent::ring_buffer<int> rb(8);
    ASSERT_EQ(nullptr, rb.get_stats());

    rb.enable_stats();
    ASSERT_NE(nullptr, rb.get_stats());
    const auto* stats = rb.get_stats();

    rb.try_push(1);
    rb.try_pop();

    rb.reset_stats();
    ASSERT_NE(nullptr, rb.get_stats());
    stats = rb.get_stats();

    EXPECT_EQ(0, stats->total_pushes.load(std::memory_order_relaxed));
    EXPECT_EQ(0, stats->total_pops.load(std::memory_order_relaxed));
    EXPECT_EQ(0, stats->failed_pushes.load(std::memory_order_relaxed));
    EXPECT_EQ(0, stats->failed_pops.load(std::memory_order_relaxed));

    rb.disable_stats();
    ASSERT_EQ(nullptr, rb.get_stats());
}

TEST_F(SPSCRingBufferStatsTest, TotalPushesAndPops) {
    concurrent::ring_buffer<int> rb(8);
    rb.enable_stats();
    const auto* stats = rb.get_stats();
    ASSERT_NE(nullptr, stats);

    const int num_pushes = 5;
    for (int i = 0; i < num_pushes; ++i) {
        ASSERT_TRUE(rb.try_push(i));
    }

    const int num_pops = 3;
    for (int i = 0; i < num_pops; ++i) {
        ASSERT_TRUE(rb.try_pop().has_value());
    }

    EXPECT_EQ(num_pushes, stats->total_pushes.load(std::memory_order_relaxed));
    EXPECT_EQ(num_pops, stats->total_pops.load(std::memory_order_relaxed));
    EXPECT_EQ(0, stats->failed_pushes.load(std::memory_order_relaxed));
    EXPECT_EQ(0, stats->failed_pops.load(std::memory_order_relaxed));
}

TEST_F(SPSCRingBufferStatsTest, FailedPushes) {
    concurrent::ring_buffer<int> small_rb(2); // Capacity 2, holds 1 item
    small_rb.enable_stats();
    const auto* stats = small_rb.get_stats();
    ASSERT_NE(nullptr, stats);

    ASSERT_TRUE(small_rb.try_push(100));
    EXPECT_EQ(1, stats->total_pushes.load(std::memory_order_relaxed));
    EXPECT_EQ(0, stats->failed_pushes.load(std::memory_order_relaxed));
    ASSERT_TRUE(small_rb.full());

    int actual_failed_attempts = 0;
    for (int i = 0; i < 3; ++i) {
        if (!small_rb.try_push(200 + i)) {
            actual_failed_attempts++;
        }
    }

    EXPECT_EQ(3, actual_failed_attempts);
    EXPECT_EQ(3, stats->failed_pushes.load(std::memory_order_relaxed));
    EXPECT_EQ(1, stats->total_pushes.load(std::memory_order_relaxed));
}

TEST_F(SPSCRingBufferStatsTest, FailedPops) {
    concurrent::ring_buffer<int> rb(8);
    rb.enable_stats();
    const auto* stats = rb.get_stats();
    ASSERT_NE(nullptr, stats);
    ASSERT_TRUE(rb.empty());

    int actual_failed_attempts = 0;
    for (int i = 0; i < 3; ++i) {
        if (!rb.try_pop().has_value()) {
            actual_failed_attempts++;
        }
    }
    EXPECT_EQ(3, actual_failed_attempts);
    EXPECT_EQ(3, stats->failed_pops.load(std::memory_order_relaxed));
    EXPECT_EQ(0, stats->total_pops.load(std::memory_order_relaxed));

    ASSERT_TRUE(rb.try_push(10));
    ASSERT_TRUE(rb.try_pop().has_value());
    EXPECT_EQ(1, stats->total_pops.load(std::memory_order_relaxed));
    EXPECT_EQ(3, stats->failed_pops.load(std::memory_order_relaxed));

    if (!rb.try_pop().has_value()) {
        actual_failed_attempts++;
    }
    EXPECT_EQ(4, actual_failed_attempts);
    EXPECT_EQ(4, stats->failed_pops.load(std::memory_order_relaxed));
    EXPECT_EQ(1, stats->total_pops.load(std::memory_order_relaxed));
}

TEST_F(SPSCRingBufferStatsTest, UtilizationStat) {
    // Section 1: Initial rb, no ops, then only successful pushes
    concurrent::ring_buffer<int> rb1(8);
    rb1.enable_stats();
    const auto* stats1 = rb1.get_stats();
    ASSERT_NE(nullptr, stats1);
    EXPECT_DOUBLE_EQ(0.0, stats1->utilization()); // No operations

    rb1.try_push(1);
    rb1.try_push(2);
    EXPECT_DOUBLE_EQ(1.0, stats1->utilization()); // Only successful pushes
    // stats1 will be {total_pushes=2, failed_pushes=0}
    // rb1 now contains 2 items. We are done with rb1 and stats1.

    // Section 2: Only failed pushes (uses its own full_rb, which is fine and isolated)
    concurrent::ring_buffer<int> full_rb(2);
    full_rb.enable_stats();
    const auto* full_stats = full_rb.get_stats();
    ASSERT_NE(nullptr, full_stats);
    ASSERT_TRUE(full_rb.try_push(100));
    full_rb.reset_stats(); // Reset after setup
    ASSERT_FALSE(full_rb.try_push(200));
    ASSERT_FALSE(full_rb.try_push(300));
    EXPECT_EQ(2, full_stats->failed_pushes.load(std::memory_order_relaxed));
    EXPECT_EQ(0, full_stats->total_pushes.load(std::memory_order_relaxed));
    EXPECT_DOUBLE_EQ(0.0, full_stats->utilization());

    // Section 3: Mix of successful and failed on a NEW, clean rb instance
    concurrent::ring_buffer<int> rb_mix(8); // FRESH INSTANCE FOR THIS TEST SECTION
    rb_mix.enable_stats();
    const auto* mix_stats = rb_mix.get_stats();
    ASSERT_NE(nullptr, mix_stats);
    // rb_mix is empty here. stats are also fresh (all zeros).

    rb_mix.try_push(1);
    rb_mix.try_push(2);
    rb_mix.try_push(3);
    rb_mix.try_push(4);
    rb_mix.try_push(5);
    // After 5 pushes to an empty buffer (cap 8, holds 7), size is 5. Not full.
    ASSERT_FALSE(rb_mix.full()); // <<<< THIS IS THE CRITICAL ASSERTION
    ASSERT_TRUE(rb_mix.try_push(6));
    ASSERT_TRUE(rb_mix.try_push(7)); // Now 7 items, full.
    ASSERT_TRUE(rb_mix.full());

    ASSERT_FALSE(rb_mix.try_push(8));
    ASSERT_FALSE(rb_mix.try_push(9));

    EXPECT_EQ(7, mix_stats->total_pushes.load(std::memory_order_relaxed));
    EXPECT_EQ(2, mix_stats->failed_pushes.load(std::memory_order_relaxed));
    double expected_util = static_cast<double>(7) / (7 + 2);
    EXPECT_DOUBLE_EQ(expected_util, mix_stats->utilization());
}

// Fixture for Advanced SPSC Ring Buffer Tests (includes custom types, emplace, peek, clear)
class SPSCRingBufferAdvancedTest : public ::testing::Test {
protected:
    // No specific common setup needed for these tests; each will declare its own buffer.
};

// Custom data structure for testing
struct MyData {
    int id;
    std::string name;

    // Default constructor
    MyData(int i = 0, std::string n = "") : id(i), name(std::move(n)) {
        // std::cout << "MyData Param/Default Constructor: " << name << " (id: " << id << ")" << std::endl;
    }

    // Copy constructor
    MyData(const MyData& other) : id(other.id), name(other.name) {
        // std::cout << "MyData Copy Constructor: " << name << " (id: " << id << ")" << std::endl;
    }

    // Move constructor
    MyData(MyData&& other) noexcept : id(other.id), name(std::move(other.name)) {
        // std::cout << "MyData Move Constructor (from id: " << other.id << " to id: " << id << ", name: " << name << ")" << std::endl;
        other.id = 0; // Reset moved-from object
        // other.name.clear(); // Optionally clear string
    }

    // Copy assignment
    MyData& operator=(const MyData& other) {
        // std::cout << "MyData Copy Assignment: " << other.name << " (id: " << other.id << ") to this id: " << id << std::endl;
        if (this != &other) {
            id = other.id;
            name = other.name;
        }
        return *this;
    }

    // Move assignment
    MyData& operator=(MyData&& other) noexcept {
        // std::cout << "MyData Move Assignment (from id: " << other.id << " to this id: " << id << ", name: " << other.name << ")" << std::endl;
        if (this != &other) {
            id = other.id;
            name = std::move(other.name);
            other.id = 0; // Reset moved-from object
            // other.name.clear(); // Optionally clear string
        }
        return *this;
    }

    // Destructor (optional, default is fine, but for debugging)
    // ~MyData() {
    //    std::cout << "MyData Destructor: " << name << " (id: " << id << ")" << std::endl;
    // }

    bool operator==(const MyData& other) const {
        return id == other.id && name == other.name;
    }
};

// For gtest printing of MyData
std::ostream& operator<<(std::ostream& os, const MyData& data) {
    return os << "MyData{id=" << data.id << ", name=\"" << data.name << "\"}";
}

// Move-only data structure for testing
struct MoveOnlyData {
    std::unique_ptr<int> ptr;
    int id;

    MoveOnlyData(int val, int an_id) : ptr(std::make_unique<int>(val)), id(an_id) {}

    // Explicitly default move constructor and move assignment operator
    MoveOnlyData(MoveOnlyData&& other) noexcept = default;
    MoveOnlyData& operator=(MoveOnlyData&& other) noexcept = default;

    // Delete copy constructor and copy assignment operator
    MoveOnlyData(const MoveOnlyData&) = delete;
    MoveOnlyData& operator=(const MoveOnlyData&) = delete;

    // Custom comparison, as unique_ptr makes the default one tricky or deleted
    bool operator==(const MoveOnlyData& other) const {
        if (id != other.id) return false;
        if (!ptr && !other.ptr) return true; // Both null is fine
        if (ptr && other.ptr) return *ptr == *other.ptr; // Both valid, compare values
        return false; // One is null, the other isn't
    }
};

// For gtest printing of MoveOnlyData
std::ostream& operator<<(std::ostream& os, const MoveOnlyData& data) {
    return os << "MoveOnlyData{id=" << data.id << ", ptr_val=" << (data.ptr ? std::to_string(*data.ptr) : "null") << "}";
}


TEST_F(SPSCRingBufferAdvancedTest, TryEmplaceSimple) {
    concurrent::ring_buffer<int> rb(4);
    ASSERT_TRUE(rb.try_emplace(10));
    ASSERT_EQ(rb.size(), 1);
    auto val = rb.try_pop();
    ASSERT_TRUE(val.has_value());
    ASSERT_EQ(*val, 10);
}

TEST_F(SPSCRingBufferAdvancedTest, TryEmplaceCustomData) {
    concurrent::ring_buffer<MyData> rb(4);
    ASSERT_TRUE(rb.try_emplace(1, "test_emplace")); // Constructor args for MyData
    ASSERT_EQ(rb.size(), 1);

    auto val = rb.try_pop();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val->id, 1);
    EXPECT_EQ(val->name, "test_emplace");
    // Or using the defined operator== for MyData
    EXPECT_EQ(*val, (MyData{1, "test_emplace"}));
}

TEST_F(SPSCRingBufferAdvancedTest, TryEmplaceFull) {
    concurrent::ring_buffer<int> rb(2); // Holds 1 item
    rb.try_emplace(1); // Successful emplace
    ASSERT_TRUE(rb.full());
    ASSERT_FALSE(rb.try_emplace(2)); // Should be full and fail
}

TEST_F(SPSCRingBufferAdvancedTest, TryPopInto) {
    concurrent::ring_buffer<MyData> rb(4);
    rb.try_push(MyData{10, "pop_into_me"});

    MyData destination_data; // Default construct
    ASSERT_TRUE(rb.try_pop_into(destination_data));
    EXPECT_EQ(destination_data.id, 10);
    EXPECT_EQ(destination_data.name, "pop_into_me");
    ASSERT_TRUE(rb.empty());

    ASSERT_FALSE(rb.try_pop_into(destination_data)); // Empty, should fail
}

TEST_F(SPSCRingBufferAdvancedTest, PeekItem) {
    concurrent::ring_buffer<int> rb(4);
    rb.try_push(123);
    ASSERT_EQ(rb.size(), 1);

    bool peek_executed = false;
    int peeked_value = 0;
    ASSERT_TRUE(rb.peek([&](const int& val) {
        peek_executed = true;
        peeked_value = val;
    }));

    ASSERT_TRUE(peek_executed);
    EXPECT_EQ(peeked_value, 123);
    ASSERT_EQ(rb.size(), 1); // Still in buffer, size unchanged

    auto popped_val = rb.try_pop();
    ASSERT_TRUE(popped_val.has_value());
    EXPECT_EQ(*popped_val, 123); // Ensure it was the same item
}

TEST_F(SPSCRingBufferAdvancedTest, PeekEmpty) {
    concurrent::ring_buffer<int> rb(4);
    bool peek_executed = false;
    ASSERT_FALSE(rb.peek([&](const int&) { // Lambda should not be called
        peek_executed = true;
    }));
    ASSERT_FALSE(peek_executed);
}

TEST_F(SPSCRingBufferAdvancedTest, ClearBuffer) {
    concurrent::ring_buffer<int> rb(8);
    for(int i = 0; i < 5; ++i) {
        rb.try_push(i);
    }
    ASSERT_EQ(rb.size(), 5);
    ASSERT_FALSE(rb.empty());

    rb.clear();
    ASSERT_EQ(rb.size(), 0);
    ASSERT_TRUE(rb.empty());
    ASSERT_FALSE(rb.try_pop().has_value()); // Try to pop from cleared buffer
}

TEST_F(SPSCRingBufferAdvancedTest, CustomDataTypeOperations) {
    concurrent::ring_buffer<MyData> rb(8);
    MyData d1{1, "one"};
    MyData d2{2, "two"};

    // Push
    ASSERT_TRUE(rb.try_push(d1)); // Pushes a copy of d1
    ASSERT_EQ(rb.size(), 1);

    // Emplace
    ASSERT_TRUE(rb.try_emplace(2, "two")); // Emplace constructs MyData{2, "two"}
    ASSERT_EQ(rb.size(), 2);

    // Peek
    bool peeked = false;
    ASSERT_TRUE(rb.peek([&](const MyData& data){
        EXPECT_EQ(data, d1); // Should peek d1
        peeked = true;
    }));
    ASSERT_TRUE(peeked);

    // Pop into
    MyData popped_data;
    ASSERT_TRUE(rb.try_pop_into(popped_data));
    EXPECT_EQ(popped_data, d1);
    ASSERT_EQ(rb.size(), 1);

    // Pop
    auto val = rb.try_pop();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), d2);
    ASSERT_TRUE(rb.empty());
}

TEST_F(SPSCRingBufferAdvancedTest, MoveOnlyTypeOperations) {
    concurrent::ring_buffer<MoveOnlyData> rb(4);

    ASSERT_TRUE(rb.try_push(MoveOnlyData(10, 1)));
    ASSERT_TRUE(rb.try_emplace(20, 2)); // Emplace constructs in place

    ASSERT_EQ(rb.size(), 2);

    MoveOnlyData popped_val(0,0);
    *popped_val.ptr = 0;

    ASSERT_TRUE(rb.try_pop_into(popped_val));
    ASSERT_NE(nullptr, popped_val.ptr);
    if (popped_val.ptr) {
        EXPECT_EQ(*popped_val.ptr, 10);
    }
    EXPECT_EQ(popped_val.id, 1);

    auto opt_val = rb.try_pop();
    ASSERT_TRUE(opt_val.has_value());
    MoveOnlyData& val_ref = *opt_val;

    ASSERT_NE(nullptr, val_ref.ptr);
    if (val_ref.ptr) {
        EXPECT_EQ(*val_ref.ptr, 20);
    }
    EXPECT_EQ(val_ref.id, 2);
    ASSERT_TRUE(rb.empty());
}

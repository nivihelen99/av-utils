#include "chrono_ring.h" // Assuming this path will be correct via CMake include_directories
#include "gtest/gtest.h"
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm> // For std::sort, std::equal, std::find
#include <stdexcept> // For std::out_of_range

// Helper to extract values from entries for comparison
template<typename T>
std::vector<T> get_entry_values(const std::vector<typename anomeda::ChronoRing<T, std::chrono::steady_clock>::Entry>& entries) {
    std::vector<T> values;
    for (const auto& entry : entries) {
        values.push_back(entry.value);
    }
    return values;
}


TEST(ChronoRingTest, ConstructionAndBasicProperties) {
    using namespace anomeda;
    ChronoRing<int> ring(5);
    EXPECT_EQ(ring.capacity(), 5);
    EXPECT_EQ(ring.size(), 0);
    EXPECT_TRUE(ring.empty());

    ChronoRing<std::string> ring_str(10);
    EXPECT_EQ(ring_str.capacity(), 10);
    EXPECT_EQ(ring_str.size(), 0);
    EXPECT_TRUE(ring_str.empty());

    EXPECT_THROW(ChronoRing<int> zero_cap_ring(0), std::out_of_range);
}

TEST(ChronoRingTest, PushAndOverwrite) {
    using namespace anomeda;
    using TestRingInt = ChronoRing<int, std::chrono::steady_clock>; // Alias for clarity
    TestRingInt ring(3);

    ring.push(10); // {10}
    EXPECT_EQ(ring.size(), 1);
    ring.push(20); // {10, 20}
    EXPECT_EQ(ring.size(), 2);
    ring.push(30); // {10, 20, 30}
    EXPECT_EQ(ring.size(), 3);

    // Get all entries to check order (should be oldest to newest)
    auto entries1 = ring.entries_between(TestRingInt::TimePoint::min(), TestRingInt::TimePoint::max());
    EXPECT_EQ(get_entry_values<int>(entries1), std::vector<int>({10, 20, 30}));

    ring.push(40); // Overwrites 10 -> {20, 30, 40}
    EXPECT_EQ(ring.size(), 3);
    auto entries2 = ring.entries_between(TestRingInt::TimePoint::min(), TestRingInt::TimePoint::max());
    EXPECT_EQ(get_entry_values<int>(entries2), std::vector<int>({20, 30, 40}));

    ring.push(50); // Overwrites 20 -> {30, 40, 50}
    EXPECT_EQ(ring.size(), 3);
    auto entries3 = ring.entries_between(TestRingInt::TimePoint::min(), TestRingInt::TimePoint::max());
    EXPECT_EQ(get_entry_values<int>(entries3), std::vector<int>({30, 40, 50}));

    ring.push(60); // Overwrites 30 -> {40, 50, 60}
    EXPECT_EQ(ring.size(), 3);
    auto entries4 = ring.entries_between(TestRingInt::TimePoint::min(), TestRingInt::TimePoint::max());
    EXPECT_EQ(get_entry_values<int>(entries4), std::vector<int>({40, 50, 60}));
}

TEST(ChronoRingTest, Timestamps) {
    using namespace anomeda;
    using namespace std::chrono_literals;
    using TestRingInt = ChronoRing<int, std::chrono::steady_clock>;
    TestRingInt ring(3);

    auto t_start = std::chrono::steady_clock::now();
    ring.push(1);
    std::this_thread::sleep_for(5ms);
    ring.push(2);
    auto t_end_push = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(5ms);

    auto specific_time = std::chrono::steady_clock::now() + 1h; // A distinct future time
    ring.push_at(3, specific_time);

    auto entries = ring.entries_between(TestRingInt::TimePoint::min(), TestRingInt::TimePoint::max());
    ASSERT_EQ(entries.size(), 3);

    // Check timestamps are roughly correct and ordered for first two
    EXPECT_EQ(entries[0].value, 1);
    EXPECT_GE(entries[0].timestamp, t_start);
    EXPECT_LE(entries[0].timestamp, t_end_push);

    EXPECT_EQ(entries[1].value, 2);
    EXPECT_GT(entries[1].timestamp, entries[0].timestamp);
    EXPECT_LE(entries[1].timestamp, t_end_push);

    EXPECT_EQ(entries[2].value, 3);
    EXPECT_EQ(entries[2].timestamp, specific_time); // Exact match for push_at
}

TEST(ChronoRingTest, Clear) {
    using namespace anomeda;
    using TestRingInt = ChronoRing<int, std::chrono::steady_clock>;
    TestRingInt ring(3);
    ring.push(1);
    ring.push(2);
    EXPECT_EQ(ring.size(), 2);
    EXPECT_FALSE(ring.empty());

    ring.clear();
    EXPECT_EQ(ring.size(), 0);
    EXPECT_TRUE(ring.empty());

    auto entries = ring.entries_between(TestRingInt::TimePoint::min(), TestRingInt::TimePoint::max());
    EXPECT_TRUE(entries.empty());

    ring.push(3); // Should work after clear
    EXPECT_EQ(ring.size(), 1);
}


TEST(ChronoRingTest, RecentQueries) {
    using namespace anomeda;
    using namespace std::chrono_literals;
    using TestRingInt = ChronoRing<int, std::chrono::steady_clock>;
    TestRingInt ring(5);

    auto time_0 = std::chrono::steady_clock::now();
    ring.push_at(10, time_0); // 10 @ t=0ms
    std::this_thread::sleep_for(10ms);
    auto time_10 = std::chrono::steady_clock::now();
    ring.push_at(20, time_10); // 20 @ t=10ms
    std::this_thread::sleep_for(10ms);
    auto time_20 = std::chrono::steady_clock::now();
    ring.push_at(30, time_20); // 30 @ t=20ms
    std::this_thread::sleep_for(10ms);
    auto time_30 = std::chrono::steady_clock::now();
    ring.push_at(40, time_30); // 40 @ t=30ms
    std::this_thread::sleep_for(10ms);
    auto time_40 = std::chrono::steady_clock::now();
    ring.push_at(50, time_40); // 50 @ t=40ms (ring full)

    std::this_thread::sleep_for(5ms); // Make "now" definitively after time_40

    // Test with current time being ~45ms from time_0
    // recent(20ms) => items from 25ms up to 45ms. Should be 40 (at 30ms), 50 (at 40ms).
    auto recent_items1 = ring.recent(20ms);
    EXPECT_EQ(recent_items1, std::vector<int>({40, 50}));

    // recent(30ms) => items from 15ms up to 45ms. Should be 30 (at 20ms), 40 (at 30ms), 50 (at 40ms).
    auto recent_items2 = ring.recent(30ms);
    EXPECT_EQ(recent_items2, std::vector<int>({30, 40, 50}));

    // recent(50ms) => items from -5ms up to 45ms. Should be all: 10,20,30,40,50.
    auto recent_items3 = ring.recent(50ms);
    EXPECT_EQ(recent_items3, std::vector<int>({10, 20, 30, 40, 50}));

    // Check with an empty ring
    TestRingInt empty_ring(3);
    auto recent_empty = empty_ring.recent(100ms);
    EXPECT_TRUE(recent_empty.empty());
}

TEST(ChronoRingTest, ExpireOlderThan) {
    using namespace anomeda;
    using TestRingInt = ChronoRing<int, std::chrono::steady_clock>;
    TestRingInt ring(5);

    auto t0 = std::chrono::steady_clock::now();
    ring.push_at(1, t0);
    auto t1 = t0 + std::chrono::milliseconds(10);
    ring.push_at(2, t1);
    auto t2 = t0 + std::chrono::milliseconds(20);
    ring.push_at(3, t2);
    auto t3 = t0 + std::chrono::milliseconds(30);
    ring.push_at(4, t3);
    auto t4 = t0 + std::chrono::milliseconds(40);
    ring.push_at(5, t4); // Ring: {1,2,3,4,5} with timestamps t0..t4

    // Expire items older than t2 (exclusive t2, so 1 and 2 should go)
    ring.expire_older_than(t2);
    EXPECT_EQ(ring.size(), 3); // Should keep 3,4,5
    auto entries1 = ring.entries_between(TestRingInt::TimePoint::min(), TestRingInt::TimePoint::max());
    EXPECT_EQ(get_entry_values<int>(entries1), std::vector<int>({3,4,5}));
    ASSERT_EQ(entries1.size(), 3); // Ensure we can access elements
    EXPECT_EQ(entries1[0].timestamp, t2);
    EXPECT_EQ(entries1[1].timestamp, t3);
    EXPECT_EQ(entries1[2].timestamp, t4);

    // Expire items older than t4 (3 and 4 should go)
    ring.expire_older_than(t4);
    EXPECT_EQ(ring.size(), 1); // Should keep 5
    auto entries2 = ring.entries_between(TestRingInt::TimePoint::min(), TestRingInt::TimePoint::max());
    EXPECT_EQ(get_entry_values<int>(entries2), std::vector<int>({5}));
    ASSERT_EQ(entries2.size(), 1);
    EXPECT_EQ(entries2[0].timestamp, t4);

    // Expire all
    ring.expire_older_than(t4 + std::chrono::milliseconds(1));
    EXPECT_EQ(ring.size(), 0);
    EXPECT_TRUE(ring.empty());

    // Test on non-full buffer
    TestRingInt ring2(5);
    ring2.push_at(10, t0);
    ring2.push_at(20, t1); // {10,20}
    ring2.expire_older_than(t1); // Expire 10
    EXPECT_EQ(ring2.size(), 1);
    auto entries3 = ring2.entries_between(TestRingInt::TimePoint::min(), TestRingInt::TimePoint::max());
    EXPECT_EQ(get_entry_values<int>(entries3), std::vector<int>({20}));
    ASSERT_EQ(entries3.size(), 1);
    EXPECT_EQ(entries3[0].timestamp, t1);

    // Expire nothing
    TestRingInt ring3(3);
    ring3.push_at(100,t0);
    ring3.push_at(200,t1);
    ring3.expire_older_than(t0); // Should not expire 100 as cutoff is exclusive for <
    EXPECT_EQ(ring3.size(), 2);
    auto entries4 = ring3.entries_between(TestRingInt::TimePoint::min(), TestRingInt::TimePoint::max());
    EXPECT_EQ(get_entry_values<int>(entries4), std::vector<int>({100,200}));
}

TEST(ChronoRingTest, TimeWindowQueriesWithWrap) {
    using namespace anomeda;
    using TestRingInt = ChronoRing<int, std::chrono::steady_clock>;
    TestRingInt ring(3); // Capacity 3

    // Timestamps for precise control
    auto base_time = std::chrono::steady_clock::now();
    auto t0 = base_time;
    auto t1 = base_time + std::chrono::milliseconds(10);
    auto t2 = base_time + std::chrono::milliseconds(20);
    auto t3 = base_time + std::chrono::milliseconds(30);
    auto t4 = base_time + std::chrono::milliseconds(40);

    ring.push_at(10, t0); // {10@t0}
    ring.push_at(20, t1); // {10@t0, 20@t1}
    ring.push_at(30, t2); // {10@t0, 20@t1, 30@t2} - full

    // Check initial state
    auto all0 = ring.entries_between(base_time - std::chrono::seconds(1), base_time + std::chrono::seconds(1));
    EXPECT_EQ(get_entry_values<int>(all0), std::vector<int>({10,20,30}));

    ring.push_at(40, t3); // Wraps 10. Ring: {20@t1, 30@t2, 40@t3}
    auto all1 = ring.entries_between(base_time - std::chrono::seconds(1), base_time + std::chrono::seconds(1));
    ASSERT_EQ(all1.size(), 3);
    EXPECT_EQ(all1[0].value, 20); EXPECT_EQ(all1[0].timestamp, t1);
    EXPECT_EQ(all1[1].value, 30); EXPECT_EQ(all1[1].timestamp, t2);
    EXPECT_EQ(all1[2].value, 40); EXPECT_EQ(all1[2].timestamp, t3);


    ring.push_at(50, t4); // Wraps 20. Ring: {30@t2, 40@t3, 50@t4}
    auto all2 = ring.entries_between(base_time - std::chrono::seconds(1), base_time + std::chrono::seconds(1));
    ASSERT_EQ(all2.size(), 3);
    EXPECT_EQ(all2[0].value, 30); EXPECT_EQ(all2[0].timestamp, t2);
    EXPECT_EQ(all2[1].value, 40); EXPECT_EQ(all2[1].timestamp, t3);
    EXPECT_EQ(all2[2].value, 50); EXPECT_EQ(all2[2].timestamp, t4);

    // Query specific window that spans the wrap point of internal buffer but not logical timeline
    // Current: 30@t2, 40@t3, 50@t4
    // Query [t2, t3] should give {30, 40}
    auto window1 = ring.entries_between(t2, t3);
    ASSERT_EQ(window1.size(), 2);
    EXPECT_EQ(window1[0].value, 30); EXPECT_EQ(window1[0].timestamp, t2);
    EXPECT_EQ(window1[1].value, 40); EXPECT_EQ(window1[1].timestamp, t3);

    // Query [t3, t4] should give {40, 50}
    auto window2 = ring.entries_between(t3, t4);
    EXPECT_EQ(get_entry_values<int>(window2), std::vector<int>({40,50}));

    // Query [t2, t4] should give {30, 40, 50}
    auto window3 = ring.entries_between(t2, t4);
    EXPECT_EQ(get_entry_values<int>(window3), std::vector<int>({30, 40, 50}));

    // Query for a window that includes only the newest element
    auto window_newest = ring.entries_between(t4, t4 + std::chrono::milliseconds(1));
    EXPECT_EQ(get_entry_values<int>(window_newest), std::vector<int>({50}));

    // Query for a window that includes only the oldest element (in current buffer)
    auto window_oldest = ring.entries_between(t2, t2 + std::chrono::milliseconds(1));
    EXPECT_EQ(get_entry_values<int>(window_oldest), std::vector<int>({30}));

    // Query for an empty window
    auto window_empty = ring.entries_between(base_time - std::chrono::seconds(2), base_time - std::chrono::seconds(1));
    EXPECT_TRUE(window_empty.empty());

    auto window_empty2 = ring.entries_between(t2 + std::chrono::microseconds(1), t2 + std::chrono::microseconds(500)); // between entries
    EXPECT_TRUE(window_empty2.empty());
}
// Removed custom main function, gtest provides its own.

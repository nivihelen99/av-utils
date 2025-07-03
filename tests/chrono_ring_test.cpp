#include "chrono_ring.h" // Assuming this path will be correct via CMake include_directories
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm> // For std::sort, std::equal, std::find
#include <stdexcept> // For std::out_of_range

// Helper to compare entry vectors (ignoring timestamp for some tests, or comparing values)
template<typename T>
bool compare_entry_values(std::vector<typename anomeda::ChronoRing<T>::Entry> entries, std::vector<T> expected_values) {
    if (entries.size() != expected_values.size()) {
        return false;
    }
    std::vector<T> actual_values;
    for (const auto& entry : entries) {
        actual_values.push_back(entry.value);
    }
    // Order might matter depending on how ChronoRing returns them (should be oldest to newest)
    return actual_values == expected_values;
}

template<typename T>
bool compare_values(std::vector<T> actual_values, std::vector<T> expected_values) {
    // For `recent`, the order is not strictly guaranteed if timestamps are identical,
    // but ChronoRing returns oldest to newest from its internal order.
    // If expected_values are also sorted by expected insertion time, this should work.
    // For simplicity, let's assume test inputs are designed such that order is predictable OR sort if necessary.
    // The ChronoRing query methods return them ordered by timestamp (oldest first from the buffer).
    return actual_values == expected_values;
}


void test_construction_and_basic_properties() {
    std::cout << "Test: Construction and Basic Properties" << std::endl;
    using namespace anomeda;
    ChronoRing<int> ring(5);
    assert(ring.capacity() == 5);
    assert(ring.size() == 0);
    assert(ring.empty());

    ChronoRing<std::string> ring_str(10);
    assert(ring_str.capacity() == 10);
    assert(ring_str.size() == 0);
    assert(ring_str.empty());

    bool caught_exception = false;
    try {
        ChronoRing<int> zero_cap_ring(0);
    } catch (const std::out_of_range& e) {
        caught_exception = true;
    }
    assert(caught_exception);
    std::cout << "  Passed." << std::endl;
}

void test_push_and_overwrite() {
    std::cout << "Test: Push and Overwrite" << std::endl;
    using namespace anomeda;
    using TestRingInt = ChronoRing<int, std::chrono::steady_clock>; // Alias for clarity
    TestRingInt ring(3);

    ring.push(10); // {10}
    assert(ring.size() == 1);
    ring.push(20); // {10, 20}
    assert(ring.size() == 2);
    ring.push(30); // {10, 20, 30}
    assert(ring.size() == 3);

    // Get all entries to check order (should be oldest to newest)
    auto entries1 = ring.entries_between(TestRingInt::TimePoint::min(), TestRingInt::TimePoint::max());
    assert(compare_entry_values<int>(entries1, {10, 20, 30}));

    ring.push(40); // Overwrites 10 -> {20, 30, 40}
    assert(ring.size() == 3);
    auto entries2 = ring.entries_between(TestRingInt::TimePoint::min(), TestRingInt::TimePoint::max());
    assert(compare_entry_values<int>(entries2, {20, 30, 40}));

    ring.push(50); // Overwrites 20 -> {30, 40, 50}
    assert(ring.size() == 3);
    auto entries3 = ring.entries_between(TestRingInt::TimePoint::min(), TestRingInt::TimePoint::max());
    assert(compare_entry_values<int>(entries3, {30, 40, 50}));

    ring.push(60); // Overwrites 30 -> {40, 50, 60}
    assert(ring.size() == 3);
    auto entries4 = ring.entries_between(TestRingInt::TimePoint::min(), TestRingInt::TimePoint::max());
    assert(compare_entry_values<int>(entries4, {40, 50, 60}));

    std::cout << "  Passed." << std::endl;
}

void test_timestamps() {
    std::cout << "Test: Timestamps" << std::endl;
    using namespace anomeda;
    using namespace std::chrono_literals;
    using TestRingInt = ChronoRing<int, std::chrono::steady_clock>;
    TestRingInt ring(3);

    auto t_start = TestRingInt::Clock::now();
    ring.push(1);
    std::this_thread::sleep_for(5ms);
    ring.push(2);
    auto t_end_push = TestRingInt::Clock::now();
    std::this_thread::sleep_for(5ms);

    auto specific_time = TestRingInt::Clock::now() + 1h; // A distinct future time
    ring.push_at(3, specific_time);

    auto entries = ring.entries_between(TestRingInt::TimePoint::min(), TestRingInt::TimePoint::max());
    assert(entries.size() == 3);

    // Check timestamps are roughly correct and ordered for first two
    assert(entries[0].value == 1);
    assert(entries[0].timestamp >= t_start && entries[0].timestamp <= t_end_push);

    assert(entries[1].value == 2);
    assert(entries[1].timestamp > entries[0].timestamp && entries[1].timestamp <= t_end_push);

    assert(entries[2].value == 3);
    assert(entries[2].timestamp == specific_time); // Exact match for push_at

    std::cout << "  Passed." << std::endl;
}

void test_clear() {
    std::cout << "Test: Clear" << std::endl;
    using namespace anomeda;
    using TestRingInt = ChronoRing<int, std::chrono::steady_clock>;
    TestRingInt ring(3);
    ring.push(1);
    ring.push(2);
    assert(ring.size() == 2);
    assert(!ring.empty());

    ring.clear();
    assert(ring.size() == 0);
    assert(ring.empty());

    auto entries = ring.entries_between(TestRingInt::TimePoint::min(), TestRingInt::TimePoint::max());
    assert(entries.empty());

    ring.push(3); // Should work after clear
    assert(ring.size() == 1);
    std::cout << "  Passed." << std::endl;
}


void test_recent_queries() {
    std::cout << "Test: Recent Queries" << std::endl;
    using namespace anomeda;
    using namespace std::chrono_literals;
    using TestRingInt = ChronoRing<int, std::chrono::steady_clock>;
    TestRingInt ring(5);

    auto time_0 = TestRingInt::Clock::now();
    ring.push_at(10, time_0); // 10 @ t=0ms
    std::this_thread::sleep_for(10ms);
    auto time_10 = TestRingInt::Clock::now();
    ring.push_at(20, time_10); // 20 @ t=10ms
    std::this_thread::sleep_for(10ms);
    auto time_20 = TestRingInt::Clock::now();
    ring.push_at(30, time_20); // 30 @ t=20ms
    std::this_thread::sleep_for(10ms);
    auto time_30 = TestRingInt::Clock::now();
    ring.push_at(40, time_30); // 40 @ t=30ms
    std::this_thread::sleep_for(10ms);
    auto time_40 = TestRingInt::Clock::now();
    ring.push_at(50, time_40); // 50 @ t=40ms (ring full)

    // At this point, "now" is roughly time_40 + a bit (let's say t=40ms for simplicity of reasoning about duration)
    // recent(15ms) should get items from t=25ms onwards. So, 40 (at 30ms), 50 (at 40ms).
    // std::chrono::steady_clock::now() will be slightly after time_40
    std::this_thread::sleep_for(5ms); // Make "now" definitively after time_40

    auto recent1 = ring.recent(1ms); // Should only get 50 if "now" is very close to time_40
    // This is tricky because Clock::now() in recent() is called *later* than time_40.
    // Let's make a small delay to ensure 'now' is clearly past the last push.

    // Test with current time being ~45ms from time_0
    // recent(20ms) => items from 25ms up to 45ms. Should be 40 (at 30ms), 50 (at 40ms).
    auto recent_items1 = ring.recent(20ms);
    assert(compare_values<int>(recent_items1, {40, 50}));

    // recent(30ms) => items from 15ms up to 45ms. Should be 30 (at 20ms), 40 (at 30ms), 50 (at 40ms).
    auto recent_items2 = ring.recent(30ms);
    assert(compare_values<int>(recent_items2, {30, 40, 50}));

    // recent(50ms) => items from -5ms up to 45ms. Should be all: 10,20,30,40,50.
    auto recent_items3 = ring.recent(50ms); // Gets all items as now is ~45ms from t0
    assert(compare_values<int>(recent_items3, {10, 20, 30, 40, 50}));

    // Check with an empty ring
    TestRingInt empty_ring(3);
    auto recent_empty = empty_ring.recent(100ms);
    assert(recent_empty.empty());

    std::cout << "  Passed." << std::endl;
}

void test_expire_older_than() {
    std::cout << "Test: Expire Older Than" << std::endl;
    using namespace anomeda;
    using TestRingInt = ChronoRing<int, std::chrono::steady_clock>;
    TestRingInt ring(5);

    auto t0 = TestRingInt::Clock::now();
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
    assert(ring.size() == 3); // Should keep 3,4,5
    auto entries1 = ring.entries_between(TestRingInt::TimePoint::min(), TestRingInt::TimePoint::max());
    assert(compare_entry_values<int>(entries1, {3,4,5}));
    assert(entries1[0].timestamp == t2);
    assert(entries1[1].timestamp == t3);
    assert(entries1[2].timestamp == t4);

    // Expire items older than t4 (3 and 4 should go)
    ring.expire_older_than(t4);
    assert(ring.size() == 1); // Should keep 5
    auto entries2 = ring.entries_between(TestRingInt::TimePoint::min(), TestRingInt::TimePoint::max());
    assert(compare_entry_values<int>(entries2, {5}));
    assert(entries2[0].timestamp == t4);

    // Expire all
    ring.expire_older_than(t4 + std::chrono::milliseconds(1));
    assert(ring.size() == 0);
    assert(ring.empty());

    // Test on non-full buffer
    TestRingInt ring2(5);
    ring2.push_at(10, t0);
    ring2.push_at(20, t1); // {10,20}
    ring2.expire_older_than(t1); // Expire 10
    assert(ring2.size() == 1);
    auto entries3 = ring2.entries_between(TestRingInt::TimePoint::min(), TestRingInt::TimePoint::max());
    assert(compare_entry_values<int>(entries3, {20}));
    assert(entries3[0].timestamp == t1);

    // Expire nothing
    TestRingInt ring3(3);
    ring3.push_at(100,t0);
    ring3.push_at(200,t1);
    ring3.expire_older_than(t0); // Should not expire 100 as cutoff is exclusive for <
    assert(ring3.size() == 2);
    auto entries4 = ring3.entries_between(TestRingInt::TimePoint::min(), TestRingInt::TimePoint::max());
    assert(compare_entry_values<int>(entries4, {100,200}));


    std::cout << "  Passed." << std::endl;
}

void test_time_window_queries_with_wrap() {
    std::cout << "Test: Time Window Queries with Wrap" << std::endl;
    using namespace anomeda;
    using TestRingInt = ChronoRing<int, std::chrono::steady_clock>;
    TestRingInt ring(3); // Capacity 3

    // Timestamps for precise control
    auto base_time = TestRingInt::Clock::now();
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
    assert(compare_entry_values<int>(all0, {10,20,30}));

    ring.push_at(40, t3); // Wraps 10. Ring: {20@t1, 30@t2, 40@t3}
    auto all1 = ring.entries_between(base_time - std::chrono::seconds(1), base_time + std::chrono::seconds(1));
    assert(compare_entry_values<int>(all1, {20,30,40}));
    assert(all1[0].timestamp == t1 && all1[0].value == 20);
    assert(all1[1].timestamp == t2 && all1[1].value == 30);
    assert(all1[2].timestamp == t3 && all1[2].value == 40);


    ring.push_at(50, t4); // Wraps 20. Ring: {30@t2, 40@t3, 50@t4}
    auto all2 = ring.entries_between(base_time - std::chrono::seconds(1), base_time + std::chrono::seconds(1));
    assert(compare_entry_values<int>(all2, {30,40,50}));
    assert(all2[0].timestamp == t2 && all2[0].value == 30);
    assert(all2[1].timestamp == t3 && all2[1].value == 40);
    assert(all2[2].timestamp == t4 && all2[2].value == 50);

    // Query specific window that spans the wrap point of internal buffer but not logical timeline
    // Current: 30@t2, 40@t3, 50@t4
    // Query [t2, t3] should give {30, 40}
    auto window1 = ring.entries_between(t2, t3);
    assert(compare_entry_values<int>(window1, {30, 40}));
    assert(window1[0].timestamp == t2 && window1[0].value == 30);
    assert(window1[1].timestamp == t3 && window1[1].value == 40);

    // Query [t3, t4] should give {40, 50}
    auto window2 = ring.entries_between(t3, t4);
    assert(compare_entry_values<int>(window2, {40, 50}));

    // Query [t2, t4] should give {30, 40, 50}
    auto window3 = ring.entries_between(t2, t4);
    assert(compare_entry_values<int>(window3, {30, 40, 50}));

    // Query for a window that includes only the newest element
    auto window_newest = ring.entries_between(t4, t4 + std::chrono::milliseconds(1));
    assert(compare_entry_values<int>(window_newest, {50}));

    // Query for a window that includes only the oldest element (in current buffer)
    auto window_oldest = ring.entries_between(t2, t2 + std::chrono::milliseconds(1));
    assert(compare_entry_values<int>(window_oldest, {30}));

    // Query for an empty window
    auto window_empty = ring.entries_between(base_time - std::chrono::seconds(2), base_time - std::chrono::seconds(1));
    assert(window_empty.empty());

    auto window_empty2 = ring.entries_between(t2 + std::chrono::microseconds(1), t2 + std::chrono::microseconds(500)); // between entries
    assert(window_empty2.empty());


    std::cout << "  Passed." << std::endl;
}


int main() {
    std::cout << "Running ChronoRing tests...\n" << std::endl;

    test_construction_and_basic_properties();
    test_push_and_overwrite();
    test_timestamps();
    test_clear();

    // Stubs for more complex tests
    test_recent_queries();
    test_expire_older_than();
    test_time_window_queries_with_wrap();

    std::cout << "\nAll ChronoRing tests finished." << std::endl;
    return 0;
}

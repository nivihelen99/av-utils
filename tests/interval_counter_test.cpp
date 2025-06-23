#include "gtest/gtest.h"
#include "interval_counter.h"
#include <thread>
#include <vector>
#include <chrono>
#include <set>

using namespace util;
using namespace std::chrono_literals;

// Test fixture for IntervalCounter
class IntervalCounterTest : public ::testing::Test {
protected:
    IntervalCounter counter_{60s, 1s}; // Default 60s window, 1s resolution
};

// Test fixture for IntervalCounterST
class IntervalCounterSTTest : public ::testing::Test {
protected:
    IntervalCounterST counter_st_{60s, 1s}; // Default 60s window, 1s resolution
};

// Tests for IntervalCounter (thread-safe version)
TEST_F(IntervalCounterTest, InitialState) {
    EXPECT_EQ(counter_.count(), 0);
    EXPECT_DOUBLE_EQ(counter_.rate_per_second(), 0.0);
    EXPECT_EQ(counter_.window_duration(), 60s);
    EXPECT_EQ(counter_.resolution(), 1s);
}

TEST_F(IntervalCounterTest, RecordSingleEvent) {
    counter_.record();
    EXPECT_EQ(counter_.count(), 1);
}

TEST_F(IntervalCounterTest, RecordMultipleEvents) {
    counter_.record(5);
    EXPECT_EQ(counter_.count(), 5);
    counter_.record(3);
    EXPECT_EQ(counter_.count(), 8);
}

TEST_F(IntervalCounterTest, RateCalculation) {
    counter_.record(60); // 60 events in 60s window
    EXPECT_DOUBLE_EQ(counter_.rate_per_second(), 1.0);
}

TEST_F(IntervalCounterTest, ClearEvents) {
    counter_.record(10);
    counter_.clear();
    EXPECT_EQ(counter_.count(), 0);
    EXPECT_DOUBLE_EQ(counter_.rate_per_second(), 0.0);
}

TEST_F(IntervalCounterTest, WindowExpiration) {
    IntervalCounter counter(1s, 100ms); // 1s window
    counter.record(5);
    EXPECT_EQ(counter.count(), 5);
    std::this_thread::sleep_for(1200ms); // Wait for window to pass
    EXPECT_EQ(counter.count(), 0); // Events should have expired
}

TEST_F(IntervalCounterTest, WindowExpirationWithContinuousRecording) {
    IntervalCounter counter(1s, 100ms); // 1s window
    counter.record(1); // t = 0s
    std::this_thread::sleep_for(500ms);
    counter.record(2); // t = 0.5s, count = 3
    EXPECT_EQ(counter.count(), 3);
    std::this_thread::sleep_for(600ms); // t = 1.1s, first event expired
    EXPECT_EQ(counter.count(), 2);    // Only second event should remain
    std::this_thread::sleep_for(500ms); // t = 1.6s, second event expired
    EXPECT_EQ(counter.count(), 0);
}


TEST_F(IntervalCounterTest, ResolutionBoundary) {
    IntervalCounter counter(10s, 1s); // 1s resolution

    // Test 1: Two records very close together should be in the same bucket.
    counter.record(); // Record 1
    counter.record(); // Record 2 (immediately after)
    EXPECT_EQ(counter.bucket_counts().size(), 1) << "Two immediate records should be in one bucket.";

    // Test 2: Wait for longer than resolution, a new bucket should be created.
    // counter currently has 1 bucket.
    std::this_thread::sleep_for(1200ms); // Wait > 1s resolution + some buffer
    counter.record(); // Record 3
    EXPECT_EQ(counter.bucket_counts().size(), 2) << "Record after resolution period should create a new bucket.";

    // Test 3: Record again, should still be in the new (second) bucket (no significant sleep).
    counter.record(); // Record 4
    EXPECT_EQ(counter.bucket_counts().size(), 2) << "Fourth record should go into the second bucket.";
}

TEST_F(IntervalCounterTest, BucketCounts) {
    IntervalCounter counter(5s, 1s);
    counter.record(3); // Bucket 1
    std::this_thread::sleep_for(1100ms);
    counter.record(5); // Bucket 2

    auto counts = counter.bucket_counts();
    EXPECT_EQ(counts.size(), 2);

    // Note: Comparing timestamps directly can be flaky. Check counts.
    std::multiset<int> actual_counts;
    for(const auto& pair : counts) {
        actual_counts.insert(pair.second);
    }
    std::multiset<int> expected_counts = {3, 5};
    EXPECT_EQ(actual_counts, expected_counts);
}

TEST_F(IntervalCounterTest, InvalidConstructorArgs) {
    EXPECT_THROW(IntervalCounter(0s, 1s), std::invalid_argument);
    EXPECT_THROW(IntervalCounter(-1s, 1s), std::invalid_argument);
    EXPECT_THROW(IntervalCounter(1s, 0ms), std::invalid_argument);
    EXPECT_THROW(IntervalCounter(1s, -1ms), std::invalid_argument);
}

TEST_F(IntervalCounterTest, RecordZeroOrNegative) {
    counter_.record(5);
    counter_.record(0);
    EXPECT_EQ(counter_.count(), 5);
    counter_.record(-2);
    EXPECT_EQ(counter_.count(), 5);
}

TEST_F(IntervalCounterTest, ThreadSafety) {
    IntervalCounter counter(5s, 100ms);
    std::vector<std::thread> threads;
    int num_threads = 10;
    int events_per_thread = 1000;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&counter, events_per_thread]() {
            for (int j = 0; j < events_per_thread; ++j) {
                counter.record();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(counter.count(), num_threads * events_per_thread);
}


// Tests for IntervalCounterST (single-threaded version)
TEST_F(IntervalCounterSTTest, InitialState) {
    EXPECT_EQ(counter_st_.count(), 0);
    EXPECT_DOUBLE_EQ(counter_st_.rate_per_second(), 0.0);
    EXPECT_EQ(counter_st_.window_duration(), 60s);
    EXPECT_EQ(counter_st_.resolution(), 1s);
}

TEST_F(IntervalCounterSTTest, RecordSingleEvent) {
    counter_st_.record();
    EXPECT_EQ(counter_st_.count(), 1);
}

TEST_F(IntervalCounterSTTest, RecordMultipleEvents) {
    counter_st_.record(5);
    EXPECT_EQ(counter_st_.count(), 5);
    counter_st_.record(3);
    EXPECT_EQ(counter_st_.count(), 8);
}

TEST_F(IntervalCounterSTTest, RateCalculation) {
    counter_st_.record(60); // 60 events in 60s window
    EXPECT_DOUBLE_EQ(counter_st_.rate_per_second(), 1.0);
}

TEST_F(IntervalCounterSTTest, ClearEvents) {
    counter_st_.record(10);
    counter_st_.clear();
    EXPECT_EQ(counter_st_.count(), 0);
    EXPECT_DOUBLE_EQ(counter_st_.rate_per_second(), 0.0);
}

TEST_F(IntervalCounterSTTest, WindowExpiration) {
    IntervalCounterST counter(1s, 100ms); // 1s window
    counter.record(5);
    EXPECT_EQ(counter.count(), 5);
    std::this_thread::sleep_for(1200ms); // Wait for window to pass
    EXPECT_EQ(counter.count(), 0); // Events should have expired
}

TEST_F(IntervalCounterSTTest, WindowExpirationWithContinuousRecording) {
    IntervalCounterST counter(1s, 100ms); // 1s window
    counter.record(1); // t = 0s
    std::this_thread::sleep_for(500ms);
    counter.record(2); // t = 0.5s, count = 3
    EXPECT_EQ(counter.count(), 3);
    std::this_thread::sleep_for(600ms); // t = 1.1s, first event expired
    EXPECT_EQ(counter.count(), 2);    // Only second event should remain
    std::this_thread::sleep_for(500ms); // t = 1.6s, second event expired
    EXPECT_EQ(counter.count(), 0);
}

TEST_F(IntervalCounterSTTest, ResolutionBoundary) {
    IntervalCounterST counter(10s, 1s); // 1s resolution

    // Test 1: Two records very close together should be in the same bucket.
    counter.record(); // Record 1
    counter.record(); // Record 2 (immediately after)
    EXPECT_EQ(counter.bucket_counts().size(), 1) << "Two immediate records should be in one bucket.";

    // Test 2: Wait for longer than resolution, a new bucket should be created.
    // counter currently has 1 bucket.
    std::this_thread::sleep_for(1200ms); // Wait > 1s resolution + some buffer
    counter.record(); // Record 3
    EXPECT_EQ(counter.bucket_counts().size(), 2) << "Record after resolution period should create a new bucket.";

    // Test 3: Record again, should still be in the new (second) bucket (no significant sleep).
    counter.record(); // Record 4
    EXPECT_EQ(counter.bucket_counts().size(), 2) << "Fourth record should go into the second bucket.";
}

TEST_F(IntervalCounterSTTest, BucketCounts) {
    IntervalCounterST counter(5s, 1s);
    counter.record(3); // Bucket 1
    std::this_thread::sleep_for(1100ms);
    counter.record(5); // Bucket 2

    auto counts = counter.bucket_counts();
    EXPECT_EQ(counts.size(), 2);

    std::multiset<int> actual_counts;
    for(const auto& pair : counts) {
        actual_counts.insert(pair.second);
    }
    std::multiset<int> expected_counts = {3, 5};
    EXPECT_EQ(actual_counts, expected_counts);
}

TEST_F(IntervalCounterSTTest, InvalidConstructorArgs) {
    EXPECT_THROW(IntervalCounterST(0s, 1s), std::invalid_argument);
    EXPECT_THROW(IntervalCounterST(-1s, 1s), std::invalid_argument);
    EXPECT_THROW(IntervalCounterST(1s, 0ms), std::invalid_argument);
    EXPECT_THROW(IntervalCounterST(1s, -1ms), std::invalid_argument);
}

TEST_F(IntervalCounterSTTest, RecordZeroOrNegative) {
    counter_st_.record(5);
    counter_st_.record(0);
    EXPECT_EQ(counter_st_.count(), 5);
    counter_st_.record(-2);
    EXPECT_EQ(counter_st_.count(), 5);
}

// Main function for running tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

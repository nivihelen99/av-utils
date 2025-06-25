#include "rate_limiter.h"
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <numeric> // For std::iota
#include <set>     // For std::set in concurrent test
#include <chrono>  // For std::this_thread::sleep_for

using cpp_utils::TokenBucketRateLimiter;

TEST(TokenBucketRateLimiterTest, ConstructorValidation) {
    EXPECT_THROW(TokenBucketRateLimiter(0, 10.0), std::invalid_argument);
    EXPECT_THROW(TokenBucketRateLimiter(10, 0.0), std::invalid_argument);
    EXPECT_THROW(TokenBucketRateLimiter(10, -1.0), std::invalid_argument);
    EXPECT_NO_THROW(TokenBucketRateLimiter(10, 10.0));
}

TEST(TokenBucketRateLimiterTest, BasicAcquisition) {
    TokenBucketRateLimiter limiter(5, 10.0); // 5 tokens capacity, 10 tokens/sec
    EXPECT_EQ(limiter.get_capacity(), 5);
    EXPECT_EQ(limiter.get_tokens_per_second(), 10.0);

    EXPECT_EQ(limiter.get_current_tokens(), 5);
    EXPECT_TRUE(limiter.try_acquire(1));
    EXPECT_EQ(limiter.get_current_tokens(), 4);
    EXPECT_TRUE(limiter.try_acquire(3));
    EXPECT_EQ(limiter.get_current_tokens(), 1);
    EXPECT_TRUE(limiter.try_acquire(1));
    EXPECT_EQ(limiter.get_current_tokens(), 0);
    EXPECT_FALSE(limiter.try_acquire(1)); // No tokens left
    EXPECT_EQ(limiter.get_current_tokens(), 0);
}

TEST(TokenBucketRateLimiterTest, AcquireZeroTokens) {
    TokenBucketRateLimiter limiter(5, 10.0);
    EXPECT_TRUE(limiter.try_acquire(0));
    EXPECT_EQ(limiter.get_current_tokens(), 5); // Should not consume any tokens
}

TEST(TokenBucketRateLimiterTest, ExhaustAndFail) {
    TokenBucketRateLimiter limiter(2, 1.0);
    EXPECT_TRUE(limiter.try_acquire(2));
    EXPECT_EQ(limiter.get_current_tokens(), 0);
    EXPECT_FALSE(limiter.try_acquire(1));
}

TEST(TokenBucketRateLimiterTest, TokenRefillOverTime) {
    TokenBucketRateLimiter limiter(10, 10.0); // 10 tokens capacity, 10 tokens/sec

    // Consume all tokens
    EXPECT_TRUE(limiter.try_acquire(10));
    EXPECT_EQ(limiter.get_current_tokens(), 0);
    EXPECT_FALSE(limiter.try_acquire(1));

    // Wait for 0.5 seconds, should refill 5 tokens (10 tokens/sec * 0.5 sec)
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // get_current_tokens() itself calls refill.
    // Due to timing inaccuracies, we check for a range.
    // Expecting 5, but could be 4 or 6 due to sleep precision and execution overhead.
    size_t tokens = limiter.get_current_tokens();
    EXPECT_GE(tokens, 4);
    EXPECT_LE(tokens, 6);
    EXPECT_TRUE(limiter.try_acquire(static_cast<unsigned int>(tokens))); // acquire whatever was refilled
    EXPECT_EQ(limiter.get_current_tokens(), 0);


    // Wait for 1 second, should refill 10 tokens (up to capacity)
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_EQ(limiter.get_current_tokens(), 10); // Should be full
    EXPECT_TRUE(limiter.try_acquire(10));
    EXPECT_EQ(limiter.get_current_tokens(), 0);

    // Wait for 2 seconds, should still be at capacity (10)
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    EXPECT_EQ(limiter.get_current_tokens(), 10);
}

TEST(TokenBucketRateLimiterTest, RefillNotExceedingCapacity) {
    TokenBucketRateLimiter limiter(5, 100.0); // High refill rate
    EXPECT_TRUE(limiter.try_acquire(2)); // Current tokens: 3
    EXPECT_EQ(limiter.get_current_tokens(), 3);

    // Wait long enough to refill more than capacity if it wasn't capped
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // 0.2s * 100 = 20 tokens
    EXPECT_EQ(limiter.get_current_tokens(), 5); // Should be capped at 5
}


TEST(TokenBucketRateLimiterTest, AcquireMultipleChunks) {
    TokenBucketRateLimiter limiter(10, 1.0); // 10 capacity, 1 token/sec
    EXPECT_TRUE(limiter.try_acquire(5));
    EXPECT_EQ(limiter.get_current_tokens(), 5);
    EXPECT_TRUE(limiter.try_acquire(5));
    EXPECT_EQ(limiter.get_current_tokens(), 0);
    EXPECT_FALSE(limiter.try_acquire(1));

    std::this_thread::sleep_for(std::chrono::milliseconds(1100)); // Wait >1 sec
    EXPECT_TRUE(limiter.try_acquire(1)); // Should have 1 token now
    EXPECT_EQ(limiter.get_current_tokens(), 0); // Assuming 1 token refilled and taken

    std::this_thread::sleep_for(std::chrono::milliseconds(3100)); // Wait >3 sec
    // Should have refilled 3 tokens.
    // Depending on exact timing, it might be slightly more or less.
    size_t tokens_after_3_sec = limiter.get_current_tokens();
    EXPECT_GE(tokens_after_3_sec, 2); // Expect at least 2 (allowing for some initial debt)
    EXPECT_LE(tokens_after_3_sec, 4); // Expect at most 4 (allowing for some extra time)

    EXPECT_TRUE(limiter.try_acquire(tokens_after_3_sec));
    EXPECT_EQ(limiter.get_current_tokens(), 0);
}

TEST(TokenBucketRateLimiterTest, ConcurrentAcquisition) {
    const int num_threads = 10;
    const int acquisitions_per_thread = 5;
    const int total_capacity = 20;
    const double refill_rate = 10.0; // tokens per second

    // Bucket capacity is less than total requests to force some threads to wait/fail if not refilled
    TokenBucketRateLimiter limiter(total_capacity, refill_rate);

    std::vector<std::thread> threads;
    std::vector<int> successful_acquisitions(num_threads, 0);
    std::atomic<int> total_successful_acquisitions(0);

    // Start all threads at roughly the same time
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < acquisitions_per_thread; ++j) {
                // Try to acquire tokens, with small waits to allow refills
                // This loop makes it more likely that refills happen and affect outcomes
                for (int k=0; k < 5; ++k) { // try up to 5 times with delay
                    if (limiter.try_acquire(1)) {
                        successful_acquisitions[i]++;
                        total_successful_acquisitions++;
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(20)); // 20ms wait
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Total acquisitions should be limited by capacity + refills over time.
    // This test is non-deterministic due to thread scheduling and sleep.
    // We expect that not all (num_threads * acquisitions_per_thread = 50) requests succeed initially if capacity is low.
    // However, with refills, more can succeed over time.
    // The test duration is roughly (acquisitions_per_thread * 5 * 20ms) in the worst case for a thread,
    // which is 5 * 5 * 20 = 500ms. During this time, ~5 tokens would refill (0.5s * 10 tokens/s).
    // So, total successful could be initial_capacity + refilled_tokens.
    // Initial 20 + ~5 = ~25.

    // This is a basic check. A more rigorous test would control time precisely.
    EXPECT_GT(total_successful_acquisitions.load(), 0); // At least some should succeed
    // If total_capacity was very small (e.g. 1) and refill rate low, this might be tight.
    // With capacity 20 and refill 10/s, over ~0.5s, we expect around 20 + 5 = 25 successes.
    // Max possible is 50.
    EXPECT_LE(total_successful_acquisitions.load(), num_threads * acquisitions_per_thread);

    // A loose check: more than initial capacity if test ran long enough for refills
    // but not more than total requests.
    // If the test runs for approx 0.5s (as estimated above), then ~5 tokens are refilled.
    // So, total successes should be around initial_capacity (20) + 5 = 25.
    // This assertion is a bit weak due to timing variability.
    if (total_capacity < num_threads * acquisitions_per_thread) {
         EXPECT_GE(total_successful_acquisitions.load(), std::min((size_t)total_capacity, (size_t)num_threads * acquisitions_per_thread));
    }
    std::cout << "Total successful acquisitions in concurrent test: " << total_successful_acquisitions.load() << std::endl;
}


TEST(TokenBucketRateLimiterTest, SteadyRateAcquisition) {
    // Acquire tokens exactly at the refill rate.
    // Capacity 1, refill 1 token per 100ms (10 tokens/sec)
    TokenBucketRateLimiter limiter(1, 10.0);

    // Initially full
    EXPECT_TRUE(limiter.try_acquire(1));
    EXPECT_EQ(limiter.get_current_tokens(), 0);
    EXPECT_FALSE(limiter.try_acquire(1)); // Empty

    // Wait for 100ms to refill 1 token
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(limiter.try_acquire(1));
    EXPECT_EQ(limiter.get_current_tokens(), 0);

    // Wait for 50ms (not enough to refill)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(limiter.try_acquire(1));
    EXPECT_EQ(limiter.get_current_tokens(), 0); // still 0, but some fraction of a token has refilled

    // Wait another 50ms (total 100ms since last successful acquisition)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_TRUE(limiter.try_acquire(1));
    EXPECT_EQ(limiter.get_current_tokens(), 0);
}

TEST(TokenBucketRateLimiterTest, HighPrecisionRefill) {
    // Capacity 100, refill 1000 tokens/sec.
    // Acquire 1 token every 1ms.
    TokenBucketRateLimiter limiter(100, 1000.0);

    // Consume initial tokens to make refills more critical
    EXPECT_TRUE(limiter.try_acquire(50));
    EXPECT_EQ(limiter.get_current_tokens(), 50);

    int successful_acquires = 0;
    for (int i = 0; i < 100; ++i) { // Try 100 times
        if (limiter.try_acquire(1)) {
            successful_acquires++;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(990)); // Sleep slightly less than 1ms to test precision
                                                                     // (1ms = 1 token)
    }
    // In 100 * ~1ms = ~100ms, we should refill ~100 tokens.
    // Initial: 50. After 100 acquires, current_tokens should be around 50.
    // We acquired `successful_acquires` tokens.
    // Expected successful_acquires is high, near 100, because capacity + refill should cover it.
    // Initial 50 + 100 (refilled over 100ms) = 150 available over the period.
    EXPECT_GE(successful_acquires, 95); // Allow for slight timing issues
    EXPECT_LE(successful_acquires, 100);

    std::cout << "High precision test: " << successful_acquires << "/100 successful acquires." << std::endl;
    // Check remaining tokens - should be roughly initial (50) + refilled (100) - acquired (successful_acquires)
    // size_t remaining = limiter.get_current_tokens();
    // This is tricky because current_tokens also triggers a refill based on the very latest time.
}

TEST(TokenBucketRateLimiterTest, FractionalTokenAccumulation) {
    // Low rate: 1 token per second (0.1 token per 100ms)
    // Capacity: 5
    TokenBucketRateLimiter limiter(5, 1.0);

    // Empty the bucket
    EXPECT_TRUE(limiter.try_acquire(5));
    EXPECT_EQ(limiter.get_current_tokens(), 0);

    // Wait 500ms. Should accumulate 0.5 tokens. Not enough for 1.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_FALSE(limiter.try_acquire(1));
    // current_tokens_ (internal double) should be around 0.5. get_current_tokens() returns size_t (0).
    EXPECT_EQ(limiter.get_current_tokens(), 0);

    // Wait another 500ms. Total 1s. Should accumulate 1 token in total.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_TRUE(limiter.try_acquire(1));
    EXPECT_EQ(limiter.get_current_tokens(), 0);

    // Wait 2500ms. Should accumulate 2.5 tokens.
    std::this_thread::sleep_for(std::chrono::milliseconds(2500));
    EXPECT_TRUE(limiter.try_acquire(1)); // Takes 1, 1.5 left (approx)
    EXPECT_TRUE(limiter.try_acquire(1)); // Takes 1, 0.5 left (approx)
    EXPECT_FALSE(limiter.try_acquire(1)); // Not enough
    // Check current tokens, should be close to 0 after taking 2.
    // (e.g. if 2.5 tokens were available, 0.5 should be left, so get_current_tokens() is 0)
    EXPECT_EQ(limiter.get_current_tokens(), 0);

    // Wait another 500ms. Total accumulated since last success: 0.5 (from before) + 0.5 = 1.0
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_TRUE(limiter.try_acquire(1));
    EXPECT_EQ(limiter.get_current_tokens(), 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

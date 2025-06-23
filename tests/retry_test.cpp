#include "gtest/gtest.h"
#include "retry.h" // Assumes include paths are set up by CMake for tests
#include <string>
#include <vector>
#include <chrono>
#include <thread> // For std::this_thread::sleep_for
#include <stdexcept> // For std::runtime_error etc.
#include <atomic> // For counters in tests

// Using namespace for convenience in test file
using namespace retry_util;

// --- Test Fixtures and Helper Functions ---

struct RetryTest : public ::testing::Test {
    std::atomic<int> call_count{0};
    std::atomic<int> success_after_n_calls{0}; // Configurable point of success

    // Generic function that fails N times then succeeds
    int flaky_function() {
        call_count++;
        if (call_count < success_after_n_calls) {
            throw std::runtime_error("Flaky function: temporary failure #" + std::to_string(call_count.load()));
        }
        return 42; // Success value
    }

    // Generic void function that fails N times then succeeds
    void flaky_void_function() {
        call_count++;
        if (call_count < success_after_n_calls) {
            throw std::runtime_error("Flaky void function: temporary failure #" + std::to_string(call_count.load()));
        }
        // On success, does nothing but return
    }
    
    // Function that returns a value, to be used with 'until' predicate
    bool value_based_function() {
        call_count++;
        return call_count >= success_after_n_calls;
    }

    // Custom exception types for testing specific exception handling
    struct CustomRetryableException : public std::runtime_error {
        CustomRetryableException(const std::string& msg) : std::runtime_error(msg) {}
    };
    struct AnotherCustomException : public std::runtime_error {
        AnotherCustomException(const std::string& msg) : std::runtime_error(msg) {}
    };
};

// --- Basic Retry Tests ---

TEST_F(RetryTest, SuccessOnFirstAttempt) {
    success_after_n_calls = 1; // Succeeds on the first call
    int result = retry([this]() { return flaky_function(); }).times(3).run();
    EXPECT_EQ(result, 42);
    EXPECT_EQ(call_count, 1);
}

TEST_F(RetryTest, SuccessAfterFewFailures) {
    success_after_n_calls = 3; // Succeeds on the 3rd call
    int result = retry([this]() { return flaky_function(); })
                 .times(5)
                 .with_delay(std::chrono::milliseconds(1)) // Minimal delay for test speed
                 .run();
    EXPECT_EQ(result, 42);
    EXPECT_EQ(call_count, 3);
}

TEST_F(RetryTest, FailureAfterExhaustingRetries) {
    success_after_n_calls = 10; // Will always fail within 3 attempts
    EXPECT_THROW(
        retry([this]() { return flaky_function(); }).times(3).with_delay(std::chrono::milliseconds(1)).run(),
        std::runtime_error
    );
    EXPECT_EQ(call_count, 3); // Called 3 times
}

// --- Void Function Tests ---

TEST_F(RetryTest, VoidSuccessOnFirstAttempt) {
    success_after_n_calls = 1;
    EXPECT_NO_THROW(
        retry([this]() { flaky_void_function(); }).times(3).run()
    );
    EXPECT_EQ(call_count, 1);
}

TEST_F(RetryTest, VoidSuccessAfterFewFailures) {
    success_after_n_calls = 3;
    EXPECT_NO_THROW(
        retry([this]() { flaky_void_function(); })
            .times(5)
            .with_delay(std::chrono::milliseconds(1))
            .run()
    );
    EXPECT_EQ(call_count, 3);
}

TEST_F(RetryTest, VoidFailureAfterExhaustingRetries) {
    success_after_n_calls = 10; // Always fails
    EXPECT_THROW(
        retry([this]() { flaky_void_function(); }).times(3).with_delay(std::chrono::milliseconds(1)).run(),
        std::runtime_error
    );
    EXPECT_EQ(call_count, 3);
}

// --- Delay Tests ---

TEST_F(RetryTest, FixedDelayIsApplied) {
    success_after_n_calls = 3; // Fails twice, succeeds on third
    auto start_time = std::chrono::steady_clock::now();
    retry([this]() { return flaky_function(); })
        .times(3)
        .with_delay(std::chrono::milliseconds(50))
        .run();
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    EXPECT_EQ(call_count, 3);
    // Expect at least 2 delays of 50ms = 100ms. Allow for some scheduling overhead.
    EXPECT_GE(duration.count(), 100); 
    // And not excessively long (e.g., < 150ms, depends on system load)
    EXPECT_LT(duration.count(), 200); // Increased upper bound for CI environments
}

// --- Exponential Backoff Tests ---

TEST_F(RetryTest, ExponentialBackoffIsApplied) {
    success_after_n_calls = 4; // Fail 3 times, succeed on 4th
    // Delays: 50ms, then 50*2=100ms, then 50*2*2=200ms. Total delay ~350ms
    auto start_time = std::chrono::steady_clock::now();
    retry([this]() { return flaky_function(); })
        .times(4)
        .with_delay(std::chrono::milliseconds(50))
        .with_backoff(2.0)
        .run();
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    EXPECT_EQ(call_count, 4);
    // Expected cumulative delay: 50 (after 1st fail) + 100 (after 2nd fail) + 200 (after 3rd fail) = 350ms
    EXPECT_GE(duration.count(), 350);
    EXPECT_LT(duration.count(), 550); // Allow for overhead
}

// --- Timeout Tests ---

TEST_F(RetryTest, TimeoutThrowsException) {
    auto long_task = []() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return 1;
    };
    EXPECT_THROW(
        retry(long_task)
            .times(5) // Enough attempts that timeout should be the primary failure reason
            .with_delay(std::chrono::milliseconds(100)) // Some delay
            .timeout(std::chrono::milliseconds(200)) // Timeout well before 1s task + delays complete
            .run(),
        std::runtime_error // Expecting the generic timeout error
    );
}

TEST_F(RetryTest, TimeoutNotExceededIfCompletesInTime) {
    success_after_n_calls = 1;
    EXPECT_NO_THROW(
        retry([this]() { return flaky_function(); })
            .times(1)
            .timeout(std::chrono::seconds(1))
            .run()
    );
    EXPECT_EQ(call_count, 1);
}

// --- 'until' Predicate Tests ---

TEST_F(RetryTest, UntilPredicateSuccess) {
    success_after_n_calls = 3; // value_based_function returns true on 3rd call
    bool result = retry([this]() { return value_based_function(); })
                  .times(5)
                  .with_delay(std::chrono::milliseconds(1))
                  .until([](bool val) { return val; }) // Retry until true
                  .run();
    EXPECT_TRUE(result);
    EXPECT_EQ(call_count, 3);
}

TEST_F(RetryTest, UntilPredicateFailsIfConditionNotMet) {
    success_after_n_calls = 10; // Condition will not be met in 3 attempts
    EXPECT_THROW(
        retry([this]() { return value_based_function(); })
            .times(3)
            .with_delay(std::chrono::milliseconds(1))
            .until([](bool val) { return val; })
            .run(),
        std::runtime_error // "Retry failed: condition not met"
    );
    EXPECT_EQ(call_count, 3);
}

// --- Exception Handling Tests ---

TEST_F(RetryTest, OnSpecificExceptionType) {
    call_count = 0; // Reset for this test
    auto fn_throws_custom = [this]() {
        call_count++;
        if (call_count < 3) throw CustomRetryableException("Custom error");
        return 42;
    };
    int result = retry(fn_throws_custom)
                 .times(3)
                 .template on_exception<CustomRetryableException>() // Note: .template keyword for dependent template name
                 .with_delay(std::chrono::milliseconds(1))
                 .run();
    EXPECT_EQ(result, 42);
    EXPECT_EQ(call_count, 3);
}

TEST_F(RetryTest, DoesNotRetryOnUnspecifiedExceptionType) {
    call_count = 0;
    auto fn_throws_another = [this]() {
        call_count++;
        if (call_count < 3) throw AnotherCustomException("Another error"); // This one is not specified for retry
        return 42;
    };
    EXPECT_THROW(
        retry(fn_throws_another)
            .times(3)
            .template on_exception<CustomRetryableException>() // Only retry on CustomRetryableException
            .with_delay(std::chrono::milliseconds(1))
            .run(),
        AnotherCustomException // Should rethrow AnotherCustomException on first failure
    );
    EXPECT_EQ(call_count, 1);
}

TEST_F(RetryTest, OnExceptionPredicate) {
    call_count = 0;
    auto fn_throws_custom = [this]() {
        call_count++;
        if (call_count < 3) throw CustomRetryableException("Custom error for predicate");
        return 42;
    };
    int result = retry(fn_throws_custom)
                 .times(3)
                 .on_exception([](const std::exception& e) {
                     return dynamic_cast<const CustomRetryableException*>(&e) != nullptr;
                 })
                 .with_delay(std::chrono::milliseconds(1))
                 .run();
    EXPECT_EQ(result, 42);
    EXPECT_EQ(call_count, 3);
}

// --- on_retry Callback Tests ---

TEST_F(RetryTest, OnRetryCallbackIsCalled) {
    success_after_n_calls = 3; // Fails twice
    int retry_callback_count = 0;
    std::vector<std::size_t> attempts_recorded;

    retry([this]() { return flaky_function(); })
        .times(3)
        .with_delay(std::chrono::milliseconds(1))
        .on_retry([&](std::size_t attempt, const std::exception* e) {
            retry_callback_count++;
            attempts_recorded.push_back(attempt);
            if (attempt == 1) EXPECT_NE(e, nullptr); // First retry (attempt 1) is due to exception
            if (attempt == 2) EXPECT_NE(e, nullptr); // Second retry (attempt 2) is due to exception
        })
        .run();
    
    EXPECT_EQ(call_count, 3);
    EXPECT_EQ(retry_callback_count, 2); // Called before 2nd and 3rd attempts
    ASSERT_EQ(attempts_recorded.size(), 2);
    EXPECT_EQ(attempts_recorded[0], 1); // Attempt numbers are 1-based for callback
    EXPECT_EQ(attempts_recorded[1], 2);
}

TEST_F(RetryTest, OnRetryCallbackForUntil) {
    success_after_n_calls = 3; // value_based_function returns false twice
    int retry_callback_count = 0;
    std::vector<std::size_t> attempts_recorded_val;

    retry([this]() { return value_based_function(); })
        .times(3)
        .with_delay(std::chrono::milliseconds(1))
        .until([](bool val){ return val; })
        .on_retry([&](std::size_t attempt, const std::exception* e) {
            retry_callback_count++;
            attempts_recorded_val.push_back(attempt);
            EXPECT_EQ(e, nullptr); // Retry due to value predicate, not exception
        })
        .run();
    
    EXPECT_EQ(call_count, 3);
    EXPECT_EQ(retry_callback_count, 2);
    ASSERT_EQ(attempts_recorded_val.size(), 2);
    EXPECT_EQ(attempts_recorded_val[0], 1);
    EXPECT_EQ(attempts_recorded_val[1], 2);
}


// --- Jitter and Max Delay Tests ---

TEST_F(RetryTest, JitterIsApplied) {
    success_after_n_calls = 4; // Fail 3 times
    // Base delays: 50ms, then 50*2=100ms, then 50*2*2=200ms. Total base delay = 350ms.
    // Jitter can make it shorter or longer.
    auto start_time = std::chrono::steady_clock::now();
    retry([this]() { return flaky_function(); })
        .times(4)
        .with_delay(std::chrono::milliseconds(50))
        .with_backoff(2.0)
        .with_jitter(true, 0.2) // 20% jitter
        .run();
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    EXPECT_EQ(call_count, 4);
    // With 20% jitter, the 350ms could be roughly 350 * (1 +/- 0.2) for each component.
    // Delay1: 50ms. Jittered: 50*(1 +/- 0.2) = [40, 60]
    // Delay2: 100ms. Jittered: 100*(1 +/- 0.2) = [80, 120]
    // Delay3: 200ms. Jittered: 200*(1 +/- 0.2) = [160, 240]
    // Min total: 40+80+160 = 280ms
    // Max total: 60+120+240 = 420ms
    // This is a statistical property, so a strict range is hard.
    // We expect it to be around 350ms, but not exactly.
    // Check it's greater than a significantly reduced delay and less than a significantly increased one.
    EXPECT_GE(duration.count(), 250); // Lower bound (allowing for some systemic fastness)
    EXPECT_LT(duration.count(), 600); // Upper bound (allowing for systemic slowness + jitter max)
    // A more robust test would run this many times and check distribution, or mock std::this_thread::sleep_for.
}


TEST_F(RetryTest, MaxDelayCapsBackoff) {
    success_after_n_calls = 4; // Fail 3 times
    // Delays without cap: 10ms, 10*3=30ms, 10*3*3=90ms. Total: 130ms.
    // With max_delay 25ms:
    // Delay 1: 10ms ( < 25ms)
    // Delay 2: 30ms (capped to 25ms)
    // Delay 3: 90ms (capped to 25ms)
    // Total expected delay: 10 + 25 + 25 = 60ms.
    
    auto start_time = std::chrono::steady_clock::now();
    retry([this]() { return flaky_function(); })
        .times(4)
        .with_delay(std::chrono::milliseconds(10))
        .with_backoff(3.0)
        .with_max_delay(std::chrono::milliseconds(25))
        .run();
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    EXPECT_EQ(call_count, 4);
    EXPECT_GE(duration.count(), 60);
    EXPECT_LT(duration.count(), 100); // Allow some buffer
}

TEST_F(RetryTest, MaxDelayWithJitter) {
    success_after_n_calls = 4; // Fail 3 times
    // Base delays for backoff: 10ms, 30ms, 90ms.
    // With max_delay 40ms:
    //   Sleep 1 (base 10ms): Jittered around 10ms. Stays < 40ms.
    //   Sleep 2 (base 30ms): Jittered around 30ms. Stays < 40ms.
    //   Sleep 3 (base 90ms): Jittered around 90ms. Capped at 40ms *before* jitter.
    //      Wait, jitter should be applied to the calculated delay *before* capping, or *after*?
    //      The current implementation: calculates backoff, then applies jitter, then caps.
    //      So if backoff=90ms, jittered could be e.g. 90*1.1 = 99ms. Then capped to 40ms.
    //      If jittered 90*0.9 = 81ms. Then capped to 40ms.
    //      This means the cap is a hard cap.
    //
    // Let's test:
    // Delay 1: 10ms. Jitter(10, 0.1) -> [9, 11]. Max_delay(40). Result: [9,11]
    // Delay 2: 10*2=20ms. Jitter(20, 0.1) -> [18, 22]. Max_delay(40). Result: [18,22]
    // Delay 3: 10*2*2=40ms. Jitter(40, 0.1) -> [36, 44]. Max_delay(40). Result: [36,40] (as 44 is capped)
    // Expected total: [9+18+36, 11+22+40] = [63, 73]
    
    auto start_time = std::chrono::steady_clock::now();
    retry([this]() { return flaky_function(); })
        .times(4)
        .with_delay(std::chrono::milliseconds(10))
        .with_backoff(2.0) // Delays: 10, 20, 40
        .with_jitter(true, 0.1) // 10% jitter
        .with_max_delay(std::chrono::milliseconds(40)) // Max delay matches the last calculated delay before jitter
        .run();
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    EXPECT_EQ(call_count, 4);
    // Sum of min delays: 10*0.9 + 20*0.9 + min(40*0.9, 40) = 9 + 18 + 36 = 63
    // Sum of max delays: 10*1.1 + 20*1.1 + min(40*1.1, 40) = 11 + 22 + 40 = 73
    EXPECT_GE(duration.count(), 60); // Looser lower bound for system variance
    EXPECT_LT(duration.count(), 120); // Looser upper bound
}


// --- RetryBuilder Tests ---
TEST_F(RetryTest, RetryBuilderSimple) {
    call_count = 0;
    success_after_n_calls = 2;
    auto result = RetryBuilder::simple([this](){ return flaky_function(); }, 3, std::chrono::milliseconds(10)).run();
    EXPECT_EQ(result, 42);
    EXPECT_EQ(call_count, 2);
}

TEST_F(RetryTest, RetryBuilderWithBackoff) {
    call_count = 0;
    success_after_n_calls = 3; // Fails twice
    // Delays: 10ms, then 10*2=20ms. Total ~30ms
    auto start_time = std::chrono::steady_clock::now();
    auto result = RetryBuilder::with_backoff([this](){ return flaky_function(); }, 3, std::chrono::milliseconds(10), 2.0).run();
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    EXPECT_EQ(result, 42);
    EXPECT_EQ(call_count, 3);
    EXPECT_GE(duration.count(), 10 + 20); // 30ms
    EXPECT_LT(duration.count(), 80); 
}

TEST_F(RetryTest, RetryBuilderOnException) {
    call_count = 0;
    auto fn_throws_custom = [this]() {
        call_count++;
        if (call_count < 2) throw CustomRetryableException("Builder custom error");
        return 42;
    };
    // The first template argument to RetryBuilder::on_exception is the function type, 
    // but it's deduced. We only need to specify the ExceptionType.
    auto result = RetryBuilder::on_exception<CustomRetryableException>(
                    fn_throws_custom, 3, std::chrono::milliseconds(10))
                  .run();
    EXPECT_EQ(result, 42);
    EXPECT_EQ(call_count, 2);
}

// Test for with_jitter argument validation
TEST(RetryConfigTest, JitterFactorOutOfRangeThrows) {
    auto f = [](){ return 1; };
    EXPECT_THROW(retry(f).with_jitter(true, -0.1), std::out_of_range);
    EXPECT_THROW(retry(f).with_jitter(true, 1.1), std::out_of_range);
    EXPECT_NO_THROW(retry(f).with_jitter(true, 0.0));
    EXPECT_NO_THROW(retry(f).with_jitter(true, 1.0));
}

// Test for with_max_delay argument validation
TEST(RetryConfigTest, MaxDelayNegativeThrows) {
    auto f = [](){ return 1; };
    EXPECT_THROW(retry(f).with_max_delay(std::chrono::milliseconds(-1)), std::out_of_range);
    EXPECT_NO_THROW(retry(f).with_max_delay(std::chrono::milliseconds(0)));
}

// Note on time-based tests: These tests (FixedDelay, ExponentialBackoff, Jitter, MaxDelay)
// rely on std::this_thread::sleep_for and system clock. They can be flaky on heavily loaded
// systems or CI environments. Acceptable ranges for durations might need adjustment.
// For more robust timing tests, mocking sleep/time would be necessary, which is more involved.

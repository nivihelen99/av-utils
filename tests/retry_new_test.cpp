#include "gtest/gtest.h"
#include "retry_new.h" // Assuming retry_new.h is in the include path
#include <string>
#include <vector>
#include <chrono>
#include <stdexcept> // For std::runtime_error

// Helper function for tests: A simple function that can be made to fail a specific number of times.
struct ControllableFunction {
    int fail_count;
    int call_count = 0;
    int value_to_return;

    ControllableFunction(int fc, int val) : fail_count(fc), value_to_return(val) {}

    int operator()() {
        call_count++;
        if (call_count <= fail_count) {
            throw std::runtime_error("Controlled failure");
        }
        return value_to_return;
    }

    void reset() {
        call_count = 0;
    }
};

// Helper void function for tests
struct ControllableVoidFunction {
    int fail_count;
    int call_count = 0;

    ControllableVoidFunction(int fc) : fail_count(fc) {}

    void operator()() {
        call_count++;
        if (call_count <= fail_count) {
            throw std::runtime_error("Controlled void failure");
        }
        // Does nothing on success
    }
     void reset() {
        call_count = 0;
    }
};


// Test fixture for retry tests
class RetryNewTest : public ::testing::Test {
protected:
    // You can put setup logic here if needed
};

// Test basic retry functionality: function succeeds on the first try
TEST_F(RetryNewTest, SucceedsOnFirstTry) {
    ControllableFunction func(0, 123); // Fails 0 times
    auto result = retry_util::retry(std::ref(func)).times(3).run();
    EXPECT_EQ(result, 123);
    EXPECT_EQ(func.call_count, 1);
}

// Test basic retry functionality: function fails once, then succeeds
TEST_F(RetryNewTest, SucceedsAfterOneFailure) {
    ControllableFunction func(1, 456); // Fails 1 time
    auto result = retry_util::retry(std::ref(func))
                      .times(3)
                      .on_exception([](const std::exception&){ return true; })
                      .run();
    EXPECT_EQ(result, 456);
    EXPECT_EQ(func.call_count, 2);
}

// Test retry with specified number of attempts
TEST_F(RetryNewTest, FailsAfterMaxRetries) {
    ControllableFunction func(5, 789); // Fails 5 times
    EXPECT_THROW(
        retry_util::retry(std::ref(func))
            .times(3)
            .on_exception([](const std::exception&){ return true; })
            .run(), // Max 3 attempts
        std::runtime_error
    );
    EXPECT_EQ(func.call_count, 3); // Called 3 times (initial + 2 retries)
}

// Test retry with delay
TEST_F(RetryNewTest, SucceedsWithDelay) {
    ControllableFunction func(1, 111); // Fails 1 time
    auto start_time = std::chrono::steady_clock::now();
    auto result = retry_util::retry(std::ref(func))
                      .times(3)
                      .with_delay(std::chrono::milliseconds(50))
                      .on_exception([](const std::exception&){ return true; })
                      .run();
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    EXPECT_EQ(result, 111);
    EXPECT_EQ(func.call_count, 2);
    EXPECT_GE(duration.count(), 50); // Check if delay was applied
}

// Test retry with exponential backoff
TEST_F(RetryNewTest, SucceedsWithExponentialBackoff) {
    ControllableFunction func(2, 222); // Fails 2 times
    auto start_time = std::chrono::steady_clock::now();
    auto result = retry_util::retry(std::ref(func))
                      .times(5)
                      .with_delay(std::chrono::milliseconds(10))
                      .with_backoff(2.0) // 10ms, then 20ms
                      .on_exception([](const std::exception&){ return true; })
                      .run();
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    EXPECT_EQ(result, 222);
    EXPECT_EQ(func.call_count, 3);
    // Total delay should be at least 10ms (1st retry) + 20ms (2nd retry) = 30ms
    EXPECT_GE(duration.count(), 25); // Allow some leeway for timing inaccuracies
}

// Test retry with a value predicate: retries until the value is acceptable
TEST_F(RetryNewTest, RetryUntilValuePredicateIsMet) {
    int attempt_count = 0;
    auto func_returning_increasing_value = [&attempt_count]() {
        attempt_count++;
        return attempt_count; // Returns 1, then 2, then 3
    };

    auto result = retry_util::retry(func_returning_increasing_value)
                      .times(5)
                      .until([](int val) { return val == 3; }) // Succeed when value is 3
                      .run();

    EXPECT_EQ(result, 3);
    EXPECT_EQ(attempt_count, 3);
}

// Test retry with a value predicate: fails if predicate is never met
TEST_F(RetryNewTest, FailsIfValuePredicateNotMet) {
    int attempt_count = 0;
    auto func_returning_small_value = [&attempt_count]() {
        attempt_count++;
        return attempt_count; // Returns 1, 2, 3
    };

    EXPECT_THROW(
        retry_util::retry(func_returning_small_value)
            .times(3)
            .until([](int val) { return val == 5; }) // Condition never met within 3 tries
            .run(),
        std::runtime_error // Should throw because condition not met
    );
    EXPECT_EQ(attempt_count, 3);
}


// Test specific exception handling: retry only on specific exception type
struct CustomException1 : public std::runtime_error {
    CustomException1(const std::string& msg) : std::runtime_error(msg) {}
};

struct CustomException2 : public std::runtime_error {
    CustomException2(const std::string& msg) : std::runtime_error(msg) {}
};

TEST_F(RetryNewTest, RetryOnSpecificException) {
    int call_count = 0;
    auto func_throws_custom_exception = [&call_count]() {
        call_count++;
        if (call_count == 1) throw CustomException1("First failure");
        if (call_count == 2) throw CustomException2("Second failure, should not retry");
        return 100;
    };

    // This should retry on CustomException1 but not on CustomException2
    EXPECT_THROW(
        retry_util::retry(func_throws_custom_exception)
            .times(3)
            .on_exception<CustomException1>() // Only retry on CustomException1
            .run(),
        CustomException2 // Expecting CustomException2 to be thrown and not handled by retry
    );
    EXPECT_EQ(call_count, 2); // Called once (throws CE1, retries), called again (throws CE2, fails)
}


TEST_F(RetryNewTest, RetryOnAnySpecifiedException) {
    int call_count = 0;
    auto func_throws_custom_exception = [&call_count]() {
        call_count++;
        if (call_count == 1) throw CustomException1("First failure");
        if (call_count == 2) throw CustomException2("Second failure");
        return 100; // Success on 3rd call
    };

    auto result = retry_util::retry(func_throws_custom_exception)
        .times(3)
        .on_exception([](const std::exception& e) {
            return dynamic_cast<const CustomException1*>(&e) != nullptr ||
                   dynamic_cast<const CustomException2*>(&e) != nullptr;
        })
        .run();

    EXPECT_EQ(result, 100);
    EXPECT_EQ(call_count, 3);
}


// Test on_retry callback
TEST_F(RetryNewTest, OnRetryCallbackIsCalled) {
    ControllableFunction func(2, 333); // Fails 2 times
    int retry_callback_count = 0;
    std::vector<std::size_t> attempts_logged;

    auto result = retry_util::retry(std::ref(func))
                      .times(5)
                      .with_delay(std::chrono::milliseconds(1)) // Minimal delay
                      .on_exception([](const std::exception&){ return true; }) // ADDED
                      .on_retry([&](std::size_t attempt, const std::exception* e) {
                          retry_callback_count++;
                          attempts_logged.push_back(attempt);
                          if (attempt == 1) EXPECT_NE(e, nullptr); // First retry due to exception
                          if (attempt == 2) EXPECT_NE(e, nullptr); // Second retry due to exception
                      })
                      .run();

    EXPECT_EQ(result, 333);
    EXPECT_EQ(func.call_count, 3);
    EXPECT_EQ(retry_callback_count, 2); // Called for 2 retries
    ASSERT_EQ(attempts_logged.size(), 2);
    EXPECT_EQ(attempts_logged[0], 1); // Attempt number is 1-based for callback
    EXPECT_EQ(attempts_logged[1], 2);
}

// Test timeout functionality
TEST_F(RetryNewTest, TimeoutThrowsException) {
    ControllableFunction func_fails_and_slow(2, 123); // Fails 2 times

    auto task_that_fails_and_is_slow = [&func_fails_and_slow]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(30)); // Each attempt takes 30ms
        return func_fails_and_slow(); // Will throw for first 2 calls
    };

    EXPECT_THROW(
        retry_util::retry(task_that_fails_and_is_slow)
            .times(5)
            .with_delay(std::chrono::milliseconds(10))      // Delay of 10ms
            .on_exception([](const std::exception&){ return true; })
            .timeout(std::chrono::milliseconds(80))         // Timeout 80ms
            .run(),
        std::runtime_error // Expect timeout exception
    );
    // Trace:
    // A1: task (30ms, throws), call_count=1. Delay (10ms). Total: 40ms.
    // A2: task (30ms, throws), call_count=2. Delay (10ms). Total: 40+30+10 = 80ms.
    // A3: Start of loop. Elapsed is ~80ms. Timeout check (80 >= 80) -> Timeout!
    EXPECT_EQ(func_fails_and_slow.call_count, 2);
}

// Test void return type function: succeeds on first try
TEST_F(RetryNewTest, VoidFunctionSucceedsOnFirstTry) {
    ControllableVoidFunction func(0); // Fails 0 times
    EXPECT_NO_THROW(
        retry_util::retry(std::ref(func)).times(3).run()
    );
    EXPECT_EQ(func.call_count, 1);
}

// Test void return type function: succeeds after one failure
TEST_F(RetryNewTest, VoidFunctionSucceedsAfterOneFailure) {
    ControllableVoidFunction func(1); // Fails 1 time
    EXPECT_NO_THROW(
        retry_util::retry(std::ref(func))
            .times(3)
            .on_exception([](const std::exception&){ return true; })
            .run()
    );
    EXPECT_EQ(func.call_count, 2);
}

// Test void return type function: fails after max retries
TEST_F(RetryNewTest, VoidFunctionFailsAfterMaxRetries) {
    ControllableVoidFunction func(5); // Fails 5 times
    EXPECT_THROW(
        retry_util::retry(std::ref(func))
            .times(3)
            .on_exception([](const std::exception&){ return true; })
            .run(), // Max 3 attempts
        std::runtime_error
    );
    EXPECT_EQ(func.call_count, 3);
}

// Test RetryBuilder::simple
TEST_F(RetryNewTest, RetryBuilderSimpleSucceeds) {
    ControllableFunction func(1, 999); // Fails 1 time
    // RetryBuilder::simple already includes on_exception for any std::exception by default if not specified.
    // However, the current implementation of RetryBuilder::simple in retry_new.h does NOT add a generic exception handler.
    // It just sets times and delay. So we need to add it for the test, or modify RetryBuilder.
    // For now, adding to test:
    auto retryable = retry_util::RetryBuilder::simple(std::ref(func), 3, std::chrono::milliseconds(10));
    auto result = retryable.on_exception([](const std::exception&){ return true; }).run();
    EXPECT_EQ(result, 999);
    EXPECT_EQ(func.call_count, 2);
}

// Test RetryBuilder::with_backoff
TEST_F(RetryNewTest, RetryBuilderWithBackoffSucceeds) {
    ControllableFunction func(2, 888); // Fails 2 times
    auto start_time = std::chrono::steady_clock::now();
    // Similar to simple, RetryBuilder::with_backoff doesn't add a generic handler by default.
    auto retryable = retry_util::RetryBuilder::with_backoff(std::ref(func), 3, std::chrono::milliseconds(10), 2.0);
    auto result = retryable.on_exception([](const std::exception&){ return true; }).run();
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    EXPECT_EQ(result, 888);
    EXPECT_EQ(func.call_count, 3);
    EXPECT_GE(duration.count(), 25); // 10ms + 20ms = 30ms. Allowing some leeway.
}

// Test RetryBuilder::on_exception
TEST_F(RetryNewTest, RetryBuilderOnExceptionSucceeds) {
    int call_count = 0;
    auto func_throws_custom_exception = [&call_count]() {
        call_count++;
        if (call_count < 3) throw CustomException1("Test error");
        return 777;
    };

    auto result = retry_util::RetryBuilder::on_exception<decltype(func_throws_custom_exception), CustomException1>(
                      std::move(func_throws_custom_exception), 5, std::chrono::milliseconds(10))
                      .run();

    EXPECT_EQ(result, 777);
    EXPECT_EQ(call_count, 3);
}

// Test that timeout is checked before each attempt
TEST_F(RetryNewTest, TimeoutCheckedBeforeAttempt) {
    // Part 1: Test that a single succeeding call that is shorter than timeout does not cause timeout.
    int call_count_first_part = 0;
    auto func_takes_time_succeeds = [&call_count_first_part]() {
        call_count_first_part++;
        std::this_thread::sleep_for(std::chrono::milliseconds(30)); // 30ms call
        return 1;
    };

    // This should NOT throw, as the first attempt (30ms) is within timeout (50ms) and succeeds.
    // The timeout in retry_new.h is for total duration across retries, not for interrupting a single call.
    EXPECT_NO_THROW(
        retry_util::retry(func_takes_time_succeeds)
            .times(3)
            .with_delay(std::chrono::milliseconds(10))
            .timeout(std::chrono::milliseconds(50))
            .run()
    );
    EXPECT_EQ(call_count_first_part, 1);

    // Part 2: Test that timeout is triggered during retries of a failing function.
    int call_count_second_part = 0;
    auto func_throws_and_takes_time_part2 = [&call_count_second_part]() {
        call_count_second_part++;
        std::this_thread::sleep_for(std::chrono::milliseconds(40)); // Takes 40ms
        throw std::runtime_error("failure part2");
    };

    EXPECT_THROW(
        retry_util::retry(func_throws_and_takes_time_part2)
            .times(3)
            .with_delay(std::chrono::milliseconds(20))
            .on_exception([](const std::exception&){ return true; })
            .timeout(std::chrono::milliseconds(70))
            .run(),
        std::runtime_error
    );
    // Trace for part 2:
    // A1: func (40ms, throws), call_count_second_part=1. Delay (20ms). Total elapsed ~60ms.
    // A2: timeout check (60ms < 70ms OK). func (40ms, throws), call_count_second_part=2.
    //     Function call itself takes it over 70ms (to ~100ms). Exception from func. Delay (20ms). Total elapsed ~120ms.
    // A3: timeout check (120ms >= 70ms -> Timeout!). Throws "Retry timeout exceeded".
    EXPECT_EQ(call_count_second_part, 2);
}

// Test that the function is not called more than `times`
TEST_F(RetryNewTest, FunctionNotCalledMoreThanMaxRetries) {
    ControllableFunction func(10, 123); // Will always fail within 5 attempts
    EXPECT_THROW(
        retry_util::retry(std::ref(func))
            .times(5)
            .on_exception([](const std::exception&){ return true; })
            .run(),
        std::runtime_error
    );
    EXPECT_EQ(func.call_count, 5); // Should be called exactly 5 times
}

// Test that the void function is not called more than `times`
TEST_F(RetryNewTest, VoidFunctionNotCalledMoreThanMaxRetries) {
    ControllableVoidFunction func(10); // Will always fail within 5 attempts
    EXPECT_THROW(
        retry_util::retry(std::ref(func))
            .times(5)
            .on_exception([](const std::exception&){ return true; })
            .run(),
        std::runtime_error
    );
    EXPECT_EQ(func.call_count, 5); // Should be called exactly 5 times
}

// Test with zero retries (i.e., 'times(1)')
TEST_F(RetryNewTest, SucceedsWithTimesOneIfSuccessful) {
    ControllableFunction func(0, 100); // Succeeds on first try
    auto result = retry_util::retry(std::ref(func)).times(1).run();
    EXPECT_EQ(result, 100);
    EXPECT_EQ(func.call_count, 1);
}

TEST_F(RetryNewTest, FailsWithTimesOneIfInitiallyFails) {
    ControllableFunction func(1, 100); // Fails on first try
    EXPECT_THROW(
        retry_util::retry(std::ref(func))
            .times(1)
            .on_exception([](const std::exception&){ return true; }) // ADDED
            .run(),
        std::runtime_error
    );
    EXPECT_EQ(func.call_count, 1);
}

// Test predicate that is initially false, then true
TEST_F(RetryNewTest, PredicateInitiallyFalseThenTrue) {
    int call_count = 0;
    auto func_changes_mind = [&call_count]() {
        call_count++;
        if (call_count < 3) return false; // Returns false for first 2 calls
        return true; // Returns true on 3rd call
    };

    bool result = retry_util::retry(func_changes_mind)
                    .times(5)
                    .until([](bool val){ return val == true;}) // Retry if val is false
                    .run();

    EXPECT_TRUE(result);
    EXPECT_EQ(call_count, 3);
}

// Test predicate that is always false, leading to failure
TEST_F(RetryNewTest, PredicateAlwaysFalse) {
    int call_count = 0;
    auto func_always_false = [&call_count]() {
        call_count++;
        return false;
    };

    EXPECT_THROW(
        retry_util::retry(func_always_false)
            .times(3)
            .until([](bool val){ return val == true; })
            .run(),
        std::runtime_error // "Retry failed: condition not met after all attempts"
    );
    EXPECT_EQ(call_count, 3);
}


// Main function to run tests (standard GTest setup)
// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }
// Note: The main function is usually handled by the build system (CMake) when linking GTest.
// It's not needed in the test source file itself if using common GTest setup.

// TODO: Add tests for on_exception with a predicate function.
// TODO: Add tests for timeout with void functions.
// TODO: Add tests for backoff factor less than or equal to 1.0 (should behave like fixed delay).
// TODO: Add tests for callable objects (functors, lambdas with captures) for all scenarios.

TEST_F(RetryNewTest, BackoffFactorOneOrLess) {
    ControllableFunction func(2, 123); // Fails 2 times
    auto start_time = std::chrono::steady_clock::now();
    retry_util::retry(std::ref(func))
        .times(3)
        .with_delay(std::chrono::milliseconds(20))
        .with_backoff(1.0) // No actual backoff
        .on_exception([](const std::exception&){ return true; })
        .run();
    auto end_time = std::chrono::steady_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    // Expected delay: 20ms (1st retry) + 20ms (2nd retry) = 40ms. (func calls 3 times)
    // func.call_count will be 3.
    // Duration should be around 2 * 20ms = 40ms (plus execution time of func)

    func.reset();
    start_time = std::chrono::steady_clock::now();
    retry_util::retry(std::ref(func))
        .times(3)
        .with_delay(std::chrono::milliseconds(20))
        .with_backoff(0.5) // Invalid backoff, should behave like 1.0
        .on_exception([](const std::exception&){ return true; })
        .run();
    end_time = std::chrono::steady_clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    EXPECT_GE(duration1.count(), 35); // Approx 2 * 20ms, allow some slack
    EXPECT_LE(duration1.count(), 65); // Allow some slack
    EXPECT_GE(duration2.count(), 35); // Approx 2 * 20ms
    EXPECT_LE(duration2.count(), 65);
    // The key is that duration1 and duration2 should be roughly similar if backoff <= 1.0 means fixed delay
    // For current implementation, backoff_factor_ > 1.0 is the condition for applying multiplier
    // So backoff_factor <= 1.0 means it uses the initial delay_ only.
}

TEST_F(RetryNewTest, TimeoutWithVoidFunction) {
    ControllableVoidFunction func(5); // Fails many times
    auto long_running_task = [&func]() {
        func(); // This will throw for a few calls
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    };

    int call_count_proxy = 0;
    auto proxy_task = [&call_count_proxy]() {
        call_count_proxy++;
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        if (call_count_proxy < 5) { // Fail first 4 times
             throw std::runtime_error("void controlled failure");
        }
    };

    EXPECT_THROW(
        retry_util::retry(proxy_task)
            .times(10)
            .with_delay(std::chrono::milliseconds(10))
            .on_exception([](const std::exception&){ return true; }) // ADDED
            .timeout(std::chrono::milliseconds(100)) // Timeout
            .run(),
        std::runtime_error // Expect timeout
    );
    // Attempt 1: call (40ms). Total 40ms. Throws.
    //            delay (10ms). Total 50ms.
    // Attempt 2: call (40ms). Total 90ms. Throws.
    //            delay (10ms). Total 100ms.
    // Attempt 3: Check timeout. 100ms is not < 100ms. (elapsed >= max_timeout).
    //            If elapsed == max_timeout, it throws. So, this should throw.
    //            call_count_proxy should be 2.
    EXPECT_EQ(call_count_proxy, 2);
}

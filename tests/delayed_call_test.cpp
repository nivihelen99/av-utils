#include "gtest/gtest.h"
#include "delayed_call.h"
#include <thread>
#include <chrono>
#include <atomic>

using namespace std::chrono_literals;
using namespace util;

TEST(DelayedCallTest, BasicDelayedExecution) {
    std::atomic<bool> executed = false;

    {
        DelayedCall task([&executed]() {
            executed = true;
        }, 100ms);

        ASSERT_FALSE(executed);
        ASSERT_TRUE(task.valid());

        // Wait for execution
        std::this_thread::sleep_for(150ms);
    }

    ASSERT_TRUE(executed);
}

TEST(DelayedCallTest, Cancellation) {
    std::atomic<bool> executed = false;

    DelayedCall task([&executed]() {
        executed = true;
    }, 200ms);

    // Cancel after short delay
    std::this_thread::sleep_for(50ms);
    task.cancel();

    ASSERT_TRUE(task.expired());
    ASSERT_FALSE(task.valid());

    // Wait longer than original delay
    std::this_thread::sleep_for(300ms);

    ASSERT_FALSE(executed);
}

TEST(DelayedCallTest, Rescheduling) {
    std::atomic<bool> executed = false;
    // auto start_time = std::chrono::steady_clock::now(); // Not easily testable with GTest without more complex sync

    DelayedCall task([&executed /*, start_time*/]() {
        executed = true;
        // auto elapsed = std::chrono::steady_clock::now() - start_time;
        // auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        // std::cout << "Task executed after " + std::to_string(ms) + "ms" << std::endl; // Logging removed
    }, 100ms);

    // Reschedule after 50ms to run in 200ms more
    std::this_thread::sleep_for(50ms);
    task.reschedule(200ms);

    ASSERT_TRUE(task.valid()); // Should still be valid after reschedule

    // Task should not execute yet (original 100ms + 50ms sleep < 50ms + 200ms)
    std::this_thread::sleep_for(100ms); // Total sleep = 50ms + 100ms = 150ms from reschedule point
    ASSERT_FALSE(executed); // Original delay was 100ms, rescheduled for 200ms more.
                            // After 50ms + 100ms (total 150ms from start), it should not have fired.
                            // Rescheduled time is 50 (initial sleep) + 200 = 250ms from start.

    // Wait for execution
    std::this_thread::sleep_for(150ms); // Total sleep = 50ms + 100ms + 150ms = 300ms from reschedule point
                                        // Total time from start: 50 (initial) + 200 (rescheduled delay) = 250ms.
                                        // We've waited 50 + 100 + 150 = 300ms total.
    ASSERT_TRUE(executed);
}

TEST(DelayedCallTest, RemainingTime) {
    DelayedCall task([]() { /* Do nothing */ }, 300ms);

    // Check remaining time
    auto remaining1 = task.remaining_time();
    ASSERT_GE(remaining1, 250ms - 20ms /* tolerance */ ); // Allow for some scheduling overhead
    ASSERT_LE(remaining1, 300ms);

    std::this_thread::sleep_for(100ms);

    auto remaining2 = task.remaining_time();
    ASSERT_GE(remaining2, 150ms - 20ms /* tolerance */);
    ASSERT_LE(remaining2, 200ms);

    // Wait until near expiry
    std::this_thread::sleep_for(180ms); // Total sleep: 100 + 180 = 280ms
    auto remaining3 = task.remaining_time();
    ASSERT_GE(remaining3, 0ms);
    ASSERT_LE(remaining3, 20ms + 20ms /* tolerance for sleep inaccuracy and scheduling */);


    std::this_thread::sleep_for(50ms); // Total sleep: 100 + 180 + 50 = 330ms

    // Should be expired now
    ASSERT_TRUE(task.expired());
    ASSERT_EQ(task.remaining_time(), std::chrono::milliseconds::zero());
}

TEST(DelayedCallTest, FutureSupportVoid) {
    auto task = make_delayed_call_with_future([]() {
        // Executed
    }, 100ms);

    auto& future = task.get_future();
    ASSERT_EQ(future.wait_for(50ms), std::future_status::timeout);

    future.wait(); // Should complete without exception
    SUCCEED(); // If future.wait() doesn't throw and completes, it's a success.
}

TEST(DelayedCallTest, FutureSupportWithValue) {
    auto task = make_delayed_call_with_future<int>([]() {
        return 42;
    }, 100ms);

    auto& future = task.get_future();
    ASSERT_EQ(future.wait_for(50ms), std::future_status::timeout);

    int result = future.get();
    ASSERT_EQ(result, 42);
}

TEST(DelayedCallTest, ExceptionHandlingNoCrash) {
    // Test that exceptions in DelayedCall task don't crash the program
    {
        DelayedCall task([]() {
            throw std::runtime_error("Test exception");
        }, 100ms);

        // Wait for the task to execute and potentially throw
        std::this_thread::sleep_for(150ms);
        // If we reach here, the thread didn't terminate the program
        SUCCEED();
    }
}

TEST(DelayedCallTest, ExceptionPropagationInFuture) {
    // Test exception propagation with futures
    {
        auto task = make_delayed_call_with_future<int>([]() -> int {
            throw std::runtime_error("Test exception in future");
            // return 42; // Should not be reached
        }, 100ms);

        auto& future = task.get_future();
        std::this_thread::sleep_for(50ms); // Ensure task is scheduled

        ASSERT_THROW({
            try {
                future.get();
            } catch (const std::runtime_error& e) {
                ASSERT_STREQ("Test exception in future", e.what());
                throw; // Re-throw to satisfy ASSERT_THROW
            }
        }, std::runtime_error);
    }
}

TEST(DelayedCallTest, MoveSemanticsConstructor) {
    std::atomic<bool> executed = false;

    DelayedCall task1([&executed]() {
        executed = true;
    }, 100ms);

    ASSERT_TRUE(task1.valid());
    DelayedCall task2 = std::move(task1);

    ASSERT_FALSE(task1.valid()); // Original should be invalid (or in a valid but unspecified state, check docs or behavior)
                                // The current implementation sets is_cancelled_ = true for the moved-from object.
                                // So task1.valid() should be false.
    ASSERT_TRUE(task2.valid());  // Moved-to should be valid

    std::this_thread::sleep_for(150ms);
    ASSERT_TRUE(executed);
}

TEST(DelayedCallTest, MoveSemanticsAssignment) {
    std::atomic<bool> executed1 = false;
    std::atomic<bool> executed2 = false;

    DelayedCall task1([&executed1]() {
        executed1 = true;
    }, 100ms);

    DelayedCall task2([&executed2]() {
        executed2 = true;
    }, 100ms);

    ASSERT_TRUE(task1.valid());
    ASSERT_TRUE(task2.valid());

    task1 = std::move(task2); // task1's original thread should be cancelled/joined.
                              // task2 becomes invalid. task1 now manages task2's original job.

    ASSERT_TRUE(task1.valid());
    ASSERT_FALSE(task2.valid()); // task2 should be invalid.

    std::this_thread::sleep_for(150ms);

    ASSERT_FALSE(executed1); // Original task1's job should have been cancelled.
    ASSERT_TRUE(executed2);  // task2's (now task1's) job should execute.
}

TEST(DelayedCallTest, MultipleTimers) {
    std::vector<std::atomic<bool>> executed(3);
    for(size_t i = 0; i < executed.size(); ++i) {
        executed[i] = false;
    }

    std::vector<DelayedCall> timers; // Must keep timers in scope

    // Create multiple timers with different delays
    // Use a raw pointer to std::atomic<bool> for capture in lambda, as std::atomic<bool> is not copyable.
    // Ensure 'executed' outlives the threads.
    timers.emplace_back([ptr = &executed[0]]() { ptr->store(true); }, 100ms);
    timers.emplace_back([ptr = &executed[1]]() { ptr->store(true); }, 150ms);
    timers.emplace_back([ptr = &executed[2]]() { ptr->store(true); }, 200ms);

    // Check initial state
    ASSERT_FALSE(executed[0]);
    ASSERT_FALSE(executed[1]);
    ASSERT_FALSE(executed[2]);

    std::this_thread::sleep_for(125ms); // Timer 0 should have fired
    ASSERT_TRUE(executed[0]);
    ASSERT_FALSE(executed[1]);
    ASSERT_FALSE(executed[2]);

    std::this_thread::sleep_for(50ms);  // Total 175ms. Timer 1 should have fired.
    ASSERT_TRUE(executed[0]);
    ASSERT_TRUE(executed[1]);
    ASSERT_FALSE(executed[2]);

    std::this_thread::sleep_for(50ms);  // Total 225ms. Timer 2 should have fired.
    ASSERT_TRUE(executed[0]);
    ASSERT_TRUE(executed[1]);
    ASSERT_TRUE(executed[2]);
}

TEST(DelayedCallTest, FactoryFunction) {
    std::atomic<bool> executed = false;

    // Test make_delayed_call
    auto task = make_delayed_call([&executed]() {
        executed = true;
    }, 100ms);

    ASSERT_FALSE(executed);
    std::this_thread::sleep_for(150ms);
    ASSERT_TRUE(executed);
}

TEST(DelayedCallTest, FactoryFunctionWithFuture) {
    // Test make_delayed_call_with_future
    auto task = make_delayed_call_with_future<std::string>([]() {
        return "done";
    }, 100ms);

    auto& future = task.get_future();
    ASSERT_EQ(future.wait_for(50ms), std::future_status::timeout);
    ASSERT_EQ(future.get(), "done");
}

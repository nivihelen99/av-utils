#include "run_once.h"
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <string>
#include <stdexcept>

// Test fixture for RunOnce tests
class RunOnceTest : public ::testing::Test {
protected:
    RunOnce once; // Default member for most RunOnce tests
    // For RunOnceReturn tests, instances are created locally in the test cases
    // to allow for different types and cleaner separation of concerns.
};

TEST_F(RunOnceTest, BasicFunctionality) {
    int counter = 0;
    
    ASSERT_FALSE(once.has_run());
    
    once([&counter] { counter++; });
    ASSERT_EQ(counter, 1);
    ASSERT_TRUE(once.has_run());
    
    once([&counter] { counter++; });
    once([&counter] { counter++; });
    ASSERT_EQ(counter, 1);
    ASSERT_TRUE(once.has_run());
}

TEST_F(RunOnceTest, ExceptionHandling) {
    int attempt = 0;
    
    ASSERT_THROW(
        once([&attempt] {
            attempt++;
            throw std::runtime_error("Test exception");
        }),
        std::runtime_error
    );
    
    ASSERT_EQ(attempt, 1);
    ASSERT_FALSE(once.has_run());
    
    once([&attempt] {
        attempt++;
        // No exception this time
    });
    
    ASSERT_EQ(attempt, 2);
    ASSERT_TRUE(once.has_run());
    
    once([&attempt] { attempt++; });
    ASSERT_EQ(attempt, 2);
}

TEST_F(RunOnceTest, ThreadSafety) {
    std::atomic<int> counter{0};
    std::atomic<int> threads_completed{0};
    
    const int num_threads = 10;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, &counter, &threads_completed] {
            once([&counter] {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                counter++;
            });
            threads_completed++;
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    ASSERT_EQ(counter.load(), 1);
    ASSERT_EQ(threads_completed.load(), num_threads);
    ASSERT_TRUE(once.has_run());
}

TEST_F(RunOnceTest, MultipleInstances) {
    RunOnce once1, once2;
    int counter1 = 0, counter2 = 0;
    
    once1([&counter1] { counter1++; });
    once2([&counter2] { counter2++; });
    
    ASSERT_EQ(counter1, 1);
    ASSERT_EQ(counter2, 1);
    ASSERT_TRUE(once1.has_run());
    ASSERT_TRUE(once2.has_run());
    
    once1([&counter1] { counter1++; });
    once2([&counter2] { counter2++; });
    
    ASSERT_EQ(counter1, 1);
    ASSERT_EQ(counter2, 1);
}

TEST_F(RunOnceTest, ResetFunctionality) {
    int counter = 0;
    
    once([&counter] { counter++; });
    ASSERT_EQ(counter, 1);
    ASSERT_TRUE(once.has_run());
    
    once.reset();
    ASSERT_FALSE(once.has_run());
    
    once([&counter] { counter++; });
    ASSERT_EQ(counter, 2);
    ASSERT_TRUE(once.has_run());
}

TEST_F(RunOnceTest, RunOnceReturnBasic) {
    RunOnceReturn<int> once_return_int;
    int computation_count = 0;
    
    ASSERT_FALSE(once_return_int.has_run());
    int result1 = once_return_int([&computation_count] {
        computation_count++;
        return 42;
    });
    
    ASSERT_EQ(result1, 42);
    ASSERT_EQ(computation_count, 1);
    ASSERT_TRUE(once_return_int.has_run());
    ASSERT_EQ(once_return_int.get(), 42);
    
    int result2 = once_return_int([&computation_count] {
        computation_count++;
        return 99;
    });
    
    ASSERT_EQ(result2, 42);
    ASSERT_EQ(computation_count, 1);
    ASSERT_EQ(once_return_int.get(), 42);
}

TEST_F(RunOnceTest, RunOnceReturnString) {
    RunOnceReturn<std::string> once_return_string;
    ASSERT_FALSE(once_return_string.has_run());
    std::string result = once_return_string([] {
        return std::string("Hello, World!");
    });
    
    ASSERT_EQ(result, "Hello, World!");
    ASSERT_TRUE(once_return_string.has_run());
    ASSERT_EQ(once_return_string.get(), "Hello, World!");
    
    std::string result2 = once_return_string([] {
        return std::string("Different string");
    });
    
    ASSERT_EQ(result2, "Hello, World!");
}

TEST_F(RunOnceTest, LambdaWithCaptures) {
    int captured_value = 100;
    int result = 0;
    
    once([captured_value, &result] {
        result = captured_value * 2;
    });
    
    ASSERT_EQ(result, 200);
    ASSERT_TRUE(once.has_run());
}

// Helper for DifferentCallableTypes test
static bool function_ptr_called = false;
void test_function_for_run_once() {
    function_ptr_called = true;
}

struct TestFunctor {
    bool* called_flag;
    TestFunctor(bool* flag) : called_flag(flag) {}
    void operator()() {
        if (called_flag) *called_flag = true;
    }
};

TEST_F(RunOnceTest, DifferentCallableTypes) {
    // Function pointer
    RunOnce once_func_ptr;
    function_ptr_called = false; // Reset static variable
    once_func_ptr(test_function_for_run_once);
    ASSERT_TRUE(function_ptr_called);
    
    // Functor
    RunOnce once_functor;
    bool functor_flag = false;
    once_functor(TestFunctor(&functor_flag));
    ASSERT_TRUE(functor_flag);
}

// Tests for RunOnceReturn
class RunOnceReturnTest : public ::testing::Test {};

TEST_F(RunOnceReturnTest, ExceptionHandling) {
    RunOnceReturn<int> ror_int;
    int attempts = 0;
    EXPECT_THROW(
        ror_int([&attempts] {
            attempts++;
            throw std::runtime_error("Failed computation");
            return 1; // Should not be reached
        }),
        std::runtime_error
    );
    EXPECT_EQ(attempts, 1);
    EXPECT_FALSE(ror_int.has_run());

    // Should attempt again
    int result = ror_int([&attempts] {
        attempts++;
        return 42;
    });
    EXPECT_EQ(attempts, 2);
    EXPECT_TRUE(ror_int.has_run());
    EXPECT_EQ(result, 42);
    EXPECT_EQ(ror_int.get(), 42);

    // Should not run again
    result = ror_int([&attempts] {
        attempts++;
        return 99;
    });
    EXPECT_EQ(attempts, 2); // Not incremented
    EXPECT_EQ(result, 42);
}

TEST_F(RunOnceReturnTest, ResetFunctionality) {
    RunOnceReturn<std::string> ror_str;
    int counter = 0;

    std::string val = ror_str([&counter] {
        counter++;
        return "first run";
    });
    EXPECT_EQ(counter, 1);
    EXPECT_TRUE(ror_str.has_run());
    EXPECT_EQ(val, "first run");

    ror_str.reset();
    EXPECT_FALSE(ror_str.has_run());
    EXPECT_EQ(counter, 1); // Counter is external, not reset

    val = ror_str([&counter] {
        counter++;
        return "second run";
    });
    EXPECT_EQ(counter, 2);
    EXPECT_TRUE(ror_str.has_run());
    EXPECT_EQ(val, "second run");
    EXPECT_EQ(ror_str.get(), "second run");
}

TEST_F(RunOnceReturnTest, ThreadSafety) {
    RunOnceReturn<long> ror_long;
    std::atomic<int> execution_count{0};
    const int num_threads = 10;
    std::vector<std::thread> threads;
    std::vector<long> results(num_threads);

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&ror_long, &execution_count, &results, i] {
            results[i] = ror_long([&execution_count] {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                execution_count++;
                return 12345L;
            });
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(execution_count.load(), 1);
    EXPECT_TRUE(ror_long.has_run());
    EXPECT_EQ(ror_long.get(), 12345L);
    for (int i = 0; i < num_threads; ++i) {
        EXPECT_EQ(results[i], 12345L);
    }
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#include "run_once.h"
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

// Simple test framework macros
#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "Running " #name "..."; \
    test_##name(); \
    std::cout << " ✅\n"; \
} while(0)

TEST(basic_functionality) {
    RunOnce once;
    int counter = 0;
    
    // Should not have run initially
    assert(!once.has_run());
    
    // First call should execute
    once([&counter] { counter++; });
    assert(counter == 1);
    assert(once.has_run());
    
    // Subsequent calls should not execute
    once([&counter] { counter++; });
    once([&counter] { counter++; });
    assert(counter == 1);
    assert(once.has_run());
}

TEST(exception_handling) {
    RunOnce once;
    int attempt = 0;
    
    // First attempt throws
    try {
        once([&attempt] {
            attempt++;
            throw std::runtime_error("Test exception");
        });
        assert(false); // Should not reach here
    } catch (const std::runtime_error&) {
        // Expected
    }
    
    assert(attempt == 1);
    assert(!once.has_run()); // Should not be marked as run due to exception
    
    // Second attempt succeeds
    once([&attempt] {
        attempt++;
        // No exception this time
    });
    
    assert(attempt == 2);
    assert(once.has_run()); // Now it should be marked as run
    
    // Third attempt should not execute
    once([&attempt] { attempt++; });
    assert(attempt == 2);
}

TEST(thread_safety) {
    RunOnce once;
    std::atomic<int> counter{0};
    std::atomic<int> threads_completed{0};
    
    const int num_threads = 10;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&once, &counter, &threads_completed] {
            once([&counter] {
                // Simulate some work
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                counter++;
            });
            threads_completed++;
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    assert(counter.load() == 1); // Only executed once
    assert(threads_completed.load() == num_threads); // All threads completed
    assert(once.has_run());
}

TEST(multiple_instances) {
    RunOnce once1, once2;
    int counter1 = 0, counter2 = 0;
    
    once1([&counter1] { counter1++; });
    once2([&counter2] { counter2++; });
    
    assert(counter1 == 1);
    assert(counter2 == 1);
    assert(once1.has_run());
    assert(once2.has_run());
    
    // Second calls should not execute
    once1([&counter1] { counter1++; });
    once2([&counter2] { counter2++; });
    
    assert(counter1 == 1);
    assert(counter2 == 1);
}

TEST(reset_functionality) {
    RunOnce once;
    int counter = 0;
    
    // First execution
    once([&counter] { counter++; });
    assert(counter == 1);
    assert(once.has_run());
    
    // Reset and execute again
    once.reset();
    assert(!once.has_run());
    
    once([&counter] { counter++; });
    assert(counter == 2);
    assert(once.has_run());
}

TEST(run_once_return_basic) {
    RunOnceReturn<int> once_return;
    int computation_count = 0;
    
    // First call should compute
    int result1 = once_return([&computation_count] {
        computation_count++;
        return 42;
    });
    
    assert(result1 == 42);
    assert(computation_count == 1);
    assert(once_return.has_run());
    
    // Second call should return cached value
    int result2 = once_return([&computation_count] {
        computation_count++;
        return 99; // This should not be executed
    });
    
    assert(result2 == 42); // Should be the cached value
    assert(computation_count == 1); // Should not have incremented
    assert(once_return.get() == 42);
}

TEST(run_once_return_string) {
    RunOnceReturn<std::string> once_return;
    
    std::string result = once_return([] {
        return std::string("Hello, World!");
    });
    
    assert(result == "Hello, World!");
    assert(once_return.get() == "Hello, World!");
    
    // Second call should return same string
    std::string result2 = once_return([] {
        return std::string("Different string");
    });
    
    assert(result2 == "Hello, World!");
}

TEST(lambda_with_captures) {
    RunOnce once;
    int captured_value = 100;
    int result = 0;
    
    once([captured_value, &result] {
        result = captured_value * 2;
    });
    
    assert(result == 200);
    assert(once.has_run());
}

TEST(different_callable_types) {
    // Function pointer
    RunOnce once1;
    static bool function_called = false;
    once1([]() { function_called = true; });
    assert(function_called);
    
    // Functor
    struct TestFunctor {
        bool* called;
        TestFunctor(bool* c) : called(c) {}
        void operator()() { *called = true; }
    };
    
    RunOnce once2;
    bool functor_called = false;
    once2(TestFunctor(&functor_called));
    assert(functor_called);
}

int main() {
    std::cout << "🧪 Running RunOnce tests...\n\n";
    
    RUN_TEST(basic_functionality);
    RUN_TEST(exception_handling);
    RUN_TEST(thread_safety);
    RUN_TEST(multiple_instances);
    RUN_TEST(reset_functionality);
    RUN_TEST(run_once_return_basic);
    RUN_TEST(run_once_return_string);
    RUN_TEST(lambda_with_captures);
    RUN_TEST(different_callable_types);
    
    std::cout << "\n🎉 All tests passed!\n";
    return 0;
}

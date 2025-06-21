#include "delayed_call.h"
#include <iostream>
#include <string>
#include <vector>
#include <cassert>

using namespace std::chrono_literals;
using namespace util;

// Test helper to capture output
std::vector<std::string> test_output;
std::mutex output_mutex;

void log_output(const std::string& msg) {
    std::lock_guard<std::mutex> lock(output_mutex);
    test_output.push_back(msg);
    std::cout << msg << std::endl;
}

void test_basic_delayed_execution() {
    std::cout << "\n=== Test: Basic Delayed Execution ===" << std::endl;
    
    bool executed = false;
    
    {
        DelayedCall task([&executed]() {
            executed = true;
            log_output("Task executed after delay");
        }, 100ms);
        
        // Task should not be executed immediately
        assert(!executed);
        assert(task.valid());
        
        // Wait for execution
        std::this_thread::sleep_for(150ms);
    }
    
    // Task should have executed
    assert(executed);
    std::cout << "✓ Basic delayed execution works" << std::endl;
}

void test_cancellation() {
    std::cout << "\n=== Test: Cancellation ===" << std::endl;
    
    bool executed = false;
    
    DelayedCall task([&executed]() {
        executed = true;
        log_output("This should not execute");
    }, 200ms);
    
    // Cancel after short delay
    std::this_thread::sleep_for(50ms);
    task.cancel();
    
    assert(task.expired());
    assert(!task.valid());
    
    // Wait longer than original delay
    std::this_thread::sleep_for(300ms);
    
    // Task should not have executed
    assert(!executed);
    std::cout << "✓ Cancellation works" << std::endl;
}

void test_rescheduling() {
    std::cout << "\n=== Test: Rescheduling ===" << std::endl;
    
    bool executed = false;
    auto start_time = std::chrono::steady_clock::now();
    
    DelayedCall task([&executed, start_time]() {
        executed = true;
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        log_output("Task executed after " + std::to_string(ms) + "ms");
    }, 100ms);
    
    // Reschedule after 50ms to run in 200ms more
    std::this_thread::sleep_for(50ms);
    task.reschedule(200ms);
    
    // Should still be valid after reschedule
    assert(task.valid());
    
    // Wait for execution
    std::this_thread::sleep_for(250ms);
    
    assert(executed);
    std::cout << "✓ Rescheduling works" << std::endl;
}

void test_remaining_time() {
    std::cout << "\n=== Test: Remaining Time ===" << std::endl;
    
    DelayedCall task([]() {
        log_output("Task with remaining time check");
    }, 300ms);
    
    // Check remaining time
    auto remaining = task.remaining_time();
    assert(remaining > 250ms && remaining <= 300ms);
    
    std::this_thread::sleep_for(100ms);
    
    remaining = task.remaining_time();
    assert(remaining > 150ms && remaining <= 200ms);
    
    std::this_thread::sleep_for(250ms);
    
    // Should be expired now
    assert(task.expired());
    assert(task.remaining_time() == std::chrono::milliseconds::zero());
    
    std::cout << "✓ Remaining time calculation works" << std::endl;
}

void test_future_support() {
    std::cout << "\n=== Test: Future Support ===" << std::endl;
    
    // Test void return
    {
        auto task = make_delayed_call_with_future([]() {
            log_output("Future task executed");
        }, 100ms);
        
        auto& future = task.get_future();
        assert(future.wait_for(50ms) == std::future_status::timeout);
        
        future.wait();
        std::cout << "✓ Void future support works" << std::endl;
    }
    
    // Test with return value
    {
        auto task = make_delayed_call_with_future<int>([]() {
            log_output("Future task with return value");
            return 42;
        }, 100ms);
        
        auto& future = task.get_future();
        int result = future.get();
        assert(result == 42);
        std::cout << "✓ Future with return value works" << std::endl;
    }
}

void test_exception_handling() {
    std::cout << "\n=== Test: Exception Handling ===" << std::endl;
    
    // Test that exceptions don't crash the program
    {
        DelayedCall task([]() {
            log_output("About to throw exception");
            throw std::runtime_error("Test exception");
        }, 100ms);
        
        std::this_thread::sleep_for(150ms);
        std::cout << "✓ Exception handling works (no crash)" << std::endl;
    }
    
    // Test exception propagation with futures
    {
        auto task = make_delayed_call_with_future<int>([]() {
            throw std::runtime_error("Test exception in future");
            return 42;
        }, 100ms);
        
        auto& future = task.get_future();
        try {
            future.get();
            assert(false && "Should have thrown");
        } catch (const std::runtime_error& e) {
            assert(std::string(e.what()) == "Test exception in future");
            std::cout << "✓ Exception propagation in futures works" << std::endl;
        }
    }
}

void test_move_semantics() {
    std::cout << "\n=== Test: Move Semantics ===" << std::endl;
    
    bool executed = false;
    
    // Test move constructor
    DelayedCall task1([&executed]() {
        executed = true;
        log_output("Moved task executed");
    }, 100ms);
    
    DelayedCall task2 = std::move(task1);
    
    assert(!task1.valid()); // Original should be invalid
    assert(task2.valid());  // Moved-to should be valid
    
    std::this_thread::sleep_for(150ms);
    assert(executed);
    
    std::cout << "✓ Move semantics work" << std::endl;
}

void test_multiple_timers() {
    std::cout << "\n=== Test: Multiple Timers ===" << std::endl;
    
    std::vector<bool> executed(3, false);
    std::vector<DelayedCall> timers;
    
    // Create multiple timers with different delays
    for (int i = 0; i < 3; ++i) {
        timers.emplace_back([&executed, i]() {
            executed[i] = true;
            log_output("Timer " + std::to_string(i) + " executed");
        }, std::chrono::milliseconds(100 + i * 50));
    }
    
    // Wait for all to complete
    std::this_thread::sleep_for(300ms);
    
    for (int i = 0; i < 3; ++i) {
        assert(executed[i]);
    }
    
    std::cout << "✓ Multiple timers work correctly" << std::endl;
}

void test_factory_functions() {
    std::cout << "\n=== Test: Factory Functions ===" << std::endl;
    
    bool executed = false;
    
    // Test make_delayed_call
    auto task = make_delayed_call([&executed]() {
        executed = true;
        log_output("Factory function task executed");
    }, 100ms);
    
    std::this_thread::sleep_for(150ms);
    assert(executed);
    
    std::cout << "✓ Factory functions work" << std::endl;
}

// Example usage scenarios
void example_timeout_handler() {
    std::cout << "\n=== Example: Timeout Handler ===" << std::endl;
    
    DelayedCall timeout([]() {
        log_output("Timeout expired - handling cleanup");
    }, 2s);
    
    // Simulate some operation that might complete before timeout
    std::this_thread::sleep_for(500ms);
    
    // Cancel timeout if operation completed successfully
    timeout.cancel();
    log_output("Operation completed, timeout cancelled");
}

void example_retry_mechanism() {
    std::cout << "\n=== Example: Retry Mechanism ===" << std::endl;
    
    int retry_count = 0;
    const int max_retries = 3;
    
    std::function<void()> retry_fn = [&]() {
        retry_count++;
        log_output("Retry attempt " + std::to_string(retry_count));
        
        // Simulate operation that might fail
        if (retry_count < max_retries) {
            // Schedule next retry
            DelayedCall retry_timer(retry_fn, 500ms);
            std::this_thread::sleep_for(600ms); // Wait for next retry
        } else {
            log_output("Max retries reached");
        }
    };
    
    // Start first retry
    DelayedCall initial_retry(retry_fn, 100ms);
    std::this_thread::sleep_for(2s); // Wait for all retries
}

void example_fsm_timeout() {
    std::cout << "\n=== Example: FSM State Timeout ===" << std::endl;
    
    enum class State { Idle, Waiting, Timeout, Success };
    State current_state = State::Idle;
    
    // Transition to waiting state
    current_state = State::Waiting;
    log_output("FSM: Entered waiting state");
    
    // Set timeout for state transition
    DelayedCall state_timeout([&current_state]() {
        current_state = State::Timeout;
        log_output("FSM: Timeout occurred, transitioning to timeout state");
    }, 300ms);
    
    // Simulate some condition that might complete before timeout
    std::this_thread::sleep_for(150ms);
    
    // Simulate successful completion
    bool success = true; // In real scenario, this would be some condition
    if (success) {
        state_timeout.cancel();
        current_state = State::Success;
        log_output("FSM: Operation successful, cancelled timeout");
    }
}

int main() {
    std::cout << "DelayedCall C++17 Utility - Comprehensive Test Suite" << std::endl;
    std::cout << "====================================================" << std::endl;
    
    try {
        // Run all tests
        test_basic_delayed_execution();
        test_cancellation();
        test_rescheduling();
        test_remaining_time();
        test_future_support();
        test_exception_handling();
        test_move_semantics();
        test_multiple_timers();
        test_factory_functions();
        
        // Run examples
        example_timeout_handler();
        example_retry_mechanism();
        example_fsm_timeout();
        
        std::cout << "\n🎉 All tests passed!" << std::endl;
        std::cout << "\nCapture output:" << std::endl;
        for (const auto& msg : test_output) {
            std::cout << "  " << msg << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

# Using the Retry Utility (`retry.h`)

The `retry.h` header provides a flexible and powerful utility for adding retry logic to function calls in C++. It allows you to retry operations that might fail intermittently, with support for various backoff strategies, exception filtering, value-based success conditions, and timeouts.

## Core Concepts

The primary component is the `Retriable<Func>` class, which wraps a callable function (`Func`). You configure its behavior using a fluent API and then execute it using the `run()` method or the call operator `()`.

A factory function `retry_util::retry(your_function)` is the main entry point to create a `Retriable` object.

## Including the Utility

```cpp
#include "retry.h" // Adjust path as necessary
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <vector>

// For convenience with time literals
using namespace std::chrono_literals;
```

## Basic Usage

### 1. Simple Retries

Retry a function a specific number of times with no delay.

```cpp
int might_fail_count = 0;
int task_simple() {
    might_fail_count++;
    if (might_fail_count < 3) {
        std::cout << "Task_simple: Attempt " << might_fail_count << " failed." << std::endl;
        throw std::runtime_error("Failed!");
    }
    std::cout << "Task_simple: Attempt " << might_fail_count << " succeeded." << std::endl;
    return 42;
}

void example_simple_retry() {
    might_fail_count = 0; // Reset for example
    try {
        int result = retry_util::retry(task_simple)
                        .times(3) // Total 3 attempts
                        .run();
        std::cout << "Simple_retry successful, result: " << result << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Simple_retry ultimately failed: " << e.what() << std::endl;
    }
}
```

### 2. Retries with Fixed Delay

Retry with a fixed delay between attempts.

```cpp
int might_fail_delayed_count = 0;
void task_delayed_void() {
    might_fail_delayed_count++;
    if (might_fail_delayed_count < 2) {
        std::cout << "Task_delayed_void: Attempt " << might_fail_delayed_count << " failed." << std::endl;
        throw std::runtime_error("Void task failed");
    }
    std::cout << "Task_delayed_void: Attempt " << might_fail_delayed_count << " succeeded." << std::endl;
}

void example_fixed_delay() {
    might_fail_delayed_count = 0; // Reset
    try {
        retry_util::retry(task_delayed_void)
            .times(3)
            .with_delay(100ms) // Wait 100ms between retries
            .run();
        std::cout << "Fixed_delay (void) successful." << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Fixed_delay (void) ultimately failed: " << e.what() << std::endl;
    }
}
```

## Advanced Configuration

### 1. Exponential Backoff

Increase the delay exponentially between retries.

```cpp
int backoff_task_count = 0;
int task_exponential_backoff() {
    backoff_task_count++;
    std::cout << "Task_exponential_backoff: Attempt " << backoff_task_count << " at "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count()
              << "ms" << std::endl;
    if (backoff_task_count < 4) {
        throw std::runtime_error("Backoff task failed");
    }
    return 100;
}

void example_exponential_backoff() {
    backoff_task_count = 0;
    try {
        int result = retry_util::retry(task_exponential_backoff)
                        .times(5)
                        .with_delay(50ms)     // Initial delay
                        .with_backoff(2.0)    // Factor: 50ms, 100ms, 200ms, 400ms
                        .run();
        std::cout << "Exponential_backoff successful, result: " << result << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Exponential_backoff ultimately failed: " << e.what() << std::endl;
    }
}
```

### 2. Jitter

Add randomness to delay calculations to prevent thundering herd scenarios. Jitter is a factor applied to the current delay (e.g., 0.1 means +/- 10%).

```cpp
void example_jitter() {
    // Assuming task_exponential_backoff or similar
    backoff_task_count = 0;
    std::cout << "\nStarting Jitter Example:\n";
    try {
        retry_util::retry(task_exponential_backoff)
            .times(4)
            .with_delay(100ms)
            .with_backoff(1.5)
            .with_jitter(true, 0.25) // Enable jitter, +/- 25% of calculated delay
            .run();
        std::cout << "Jitter example successful." << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Jitter example failed: " << e.what() << std::endl;
    }
}
```

### 3. Maximum Delay

Cap the maximum delay, even with exponential backoff.

```cpp
void example_max_delay() {
    backoff_task_count = 0;
    std::cout << "\nStarting Max Delay Example:\n";
    try {
        retry_util::retry(task_exponential_backoff)
            .times(5)
            .with_delay(50ms)
            .with_backoff(3.0)      // Would lead to 50, 150, 450, 1350ms
            .with_max_delay(200ms)  // Caps delay at 200ms (so 50, 150, 200, 200ms)
            .run();
        std::cout << "Max_delay example successful." << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Max_delay example failed: " << e.what() << std::endl;
    }
}
```

### 4. Overall Timeout

Set a maximum total time for all retry attempts.

```cpp
int timeout_task_count = 0;
void task_takes_long() {
    timeout_task_count++;
    std::cout << "Task_takes_long: Attempt " << timeout_task_count << std::endl;
    if (timeout_task_count < 5) { // Will try to run many times
        std::this_thread::sleep_for(70ms); // Each attempt takes time
        throw std::runtime_error("Still failing");
    }
}

void example_timeout() {
    timeout_task_count = 0;
    try {
        retry_util::retry(task_takes_long)
            .times(10)              // Max 10 attempts
            .with_delay(10ms)       // Small delay between attempts
            .timeout(200ms)         // But overall timeout is 200ms
            .run();
        std::cout << "Timeout example successful (should not happen if configured correctly)." << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Timeout example correctly failed: " << e.what() << std::endl;
    }
}
```

### 5. Handling Specific Exceptions

Only retry on specific types of exceptions or based on a predicate.

```cpp
// Custom exception types
struct TransientError : std::runtime_error {
    TransientError(const std::string& msg) : std::runtime_error(msg) {}
};
struct FatalError : std::runtime_error {
    FatalError(const std::string& msg) : std::runtime_error(msg) {}
};

int specific_exception_count = 0;
void task_specific_exceptions() {
    specific_exception_count++;
    std::cout << "Task_specific_exceptions: Attempt " << specific_exception_count << std::endl;
    if (specific_exception_count == 1) {
        throw TransientError("Network glitch");
    }
    if (specific_exception_count == 2) {
        throw FatalError("Config missing"); // Should not retry on this
    }
    // Success on 3rd if TransientError was retried
}

void example_on_exception_type() {
    specific_exception_count = 0;
    try {
        retry_util::retry(task_specific_exceptions)
            .times(3)
            .on_exception<TransientError>() // Only retry if TransientError is thrown
            .run();
        std::cout << "On_exception_type successful." << std::endl;
    } catch (const FatalError& fe) {
        std::cout << "On_exception_type correctly caught FatalError: " << fe.what() << std::endl;
    } catch (const std::exception& e) {
        std::cout << "On_exception_type failed with other exception: " << e.what() << std::endl;
    }
}

void example_on_exception_predicate() {
    specific_exception_count = 0; // Reset
    auto should_retry_pred = [](const std::exception& e) {
        // Example: Retry on std::runtime_error but not its derivatives like FatalError
        // Note: dynamic_cast check for base would include derived. Be specific.
        if (dynamic_cast<const FatalError*>(&e)) return false; // Don't retry FatalError
        return dynamic_cast<const std::runtime_error*>(&e) != nullptr; // Retry other runtime_errors
    };

    try {
        retry_util::retry(task_specific_exceptions)
            .times(3)
            .on_exception(should_retry_pred)
            .run();
        std::cout << "On_exception_predicate successful." << std::endl;
    } catch (const FatalError& fe) {
        std::cout << "On_exception_predicate correctly caught FatalError: " << fe.what() << std::endl;
    } catch (const std::exception& e) {
        std::cout << "On_exception_predicate failed with other: " << e.what() << std::endl;
    }
}
```

### 6. Value Predicate (`until`)

For functions that return a value, you can specify a condition that the result must meet for the attempt to be considered successful. Retries will continue until the predicate returns `true` or attempts are exhausted.

```cpp
int until_task_count = 0;
std::vector<int> task_until_value() {
    until_task_count++;
    std::cout << "Task_until_value: Attempt " << until_task_count << std::endl;
    if (until_task_count == 1) return {1, 2};       // Not empty, but not size 3
    if (until_task_count == 2) return {};           // Empty
    if (until_task_count == 3) return {1, 2, 3};    // Meets condition
    return {1, 2, 3, 4}; // Also meets condition
}

void example_until() {
    until_task_count = 0;
    try {
        auto result = retry_util::retry(task_until_value)
            .times(4)
            .with_delay(50ms)
            .until([](const std::vector<int>& v) { // Predicate: vector is not empty and size is >=3
                return !v.empty() && v.size() >=3;
            })
            .run();
        std::cout << "Until successful, result size: " << result.size() << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Until ultimately failed: " << e.what() << std::endl;
    }
}
```
**Note:** `.until()` is only available for functions that do not return `void`.

### 7. `on_retry` Callback

Execute a callback function just before a retry attempt is made (i.e., after a failure and before the delay).

```cpp
void example_on_retry_callback() {
    might_fail_count = 0; // Reset task_simple's counter
    auto retry_logger = [](std::size_t attempt_num, const std::exception* e_ptr) {
        std::cout << "[Callback] Retrying... Next attempt will be " << attempt_num << "." << std::endl;
        if (e_ptr) {
            std::cout << "[Callback] Failed due to exception: " << e_ptr->what() << std::endl;
        } else {
            std::cout << "[Callback] Failed due to value predicate." << std::endl;
        }
    };

    try {
        retry_util::retry(task_simple)
            .times(3)
            .with_delay(20ms)
            .on_retry(retry_logger)
            .run();
        std::cout << "On_retry_callback example successful." << std::endl;
    } catch (const std::exception& e) {
        std::cout << "On_retry_callback example ultimately failed: " << e.what() << std::endl;
    }
}
```

## `RetryBuilder` Convenience Class

The `RetryBuilder` class provides static helper methods for common retry scenarios.

```cpp
void example_retry_builder() {
    might_fail_count = 0;
    try {
        // Simple retry with defaults (3 times, 100ms delay)
        // int result = retry_util::RetryBuilder::simple(task_simple).run();

        // Retry on specific exception with backoff
        specific_exception_count = 0;
        retry_util::RetryBuilder::on_exception<TransientError>(task_specific_exceptions, 3, 50ms)
            .with_backoff(2.0) // Can chain further configurations
            .run();
        std::cout << "RetryBuilder example successful." << std::endl;

    } catch (const std::exception& e) {
        std::cout << "RetryBuilder example failed: " << e.what() << std::endl;
    }
}
```
**Note on `RetryBuilder::on_exception`**: The template parameters are `template<typename ExceptionType, typename Func>`. You call it like `RetryBuilder::on_exception<MyExceptionType>(my_function, ...)`.

## Full Example List
To run these examples, you would typically include them in a main function:
```cpp
// int main() {
//     std::cout << "--- Running Simple Retry Example ---" << std::endl;
//     example_simple_retry();
//     std::cout << "\n--- Running Fixed Delay Example ---" << std::endl;
//     example_fixed_delay();
//     std::cout << "\n--- Running Exponential Backoff Example ---" << std::endl;
//     example_exponential_backoff();
//     std::cout << "\n--- Running Jitter Example ---" << std::endl;
//     example_jitter(); // Needs task_exponential_backoff or similar for visible effect
//     std::cout << "\n--- Running Max Delay Example ---" << std::endl;
//     example_max_delay(); // Needs task_exponential_backoff or similar
//     std::cout << "\n--- Running Timeout Example ---" << std::endl;
//     example_timeout();
//     std::cout << "\n--- Running On Exception Type Example ---" << std::endl;
//     example_on_exception_type();
//     std::cout << "\n--- Running On Exception Predicate Example ---" << std::endl;
//     example_on_exception_predicate();
//     std::cout << "\n--- Running Until Example ---" << std::endl;
//     example_until();
//     std::cout << "\n--- Running On Retry Callback Example ---" << std::endl;
//     example_on_retry_callback();
//     std::cout << "\n--- Running RetryBuilder Example ---" << std::endl;
//     example_retry_builder();
//
//     return 0;
// }
```

This comprehensive guide should help users understand and effectively use the `retry.h` utility.

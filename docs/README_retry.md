# Retry Utility (`retry_util::retry`)

## Overview

The `retry.h` header provides a flexible and fluent C++ utility for adding retry logic to function calls. It allows you to easily wrap any callable (function, lambda, member function) and specify conditions under which it should be retried if it fails or if its result isn't satisfactory.

The core of the utility is the `retry_util::retry(Func&& fn)` factory function, which returns a `Retriable<Func>` object. This object can then be configured using a chain of methods (e.g., number of attempts, delays, backoff strategy, exception filters, value predicates, timeouts) before finally executing the operation with `.run()` or `()`.

## Namespace
All utilities are within the `retry_util` namespace.

## Core Components

-   **`Retriable<Func>` Class**:
    -   Wraps the callable `Func` to be executed.
    -   Provides a fluent API for configuring retry behavior.
    -   Specialized for functions returning `void` and non-`void`.
-   **`retry_util::retry(Func&& fn)` Factory Function**:
    -   The main entry point to create a `Retriable` object from a given callable `fn`.

## Configuration Methods (Fluent API on `Retriable` object)

-   **`.times(std::size_t n)`**: Sets the maximum number of attempts (total, including the first). Default is 3. `times(0)` means the function will not be attempted. `times(1)` means one attempt, no retries.
-   **`.with_delay(std::chrono::milliseconds delay)`**: Sets a fixed delay between retry attempts. Default is 0ms.
-   **`.with_backoff(double factor)`**: Applies an exponential backoff to the delay. The delay is multiplied by `factor` for each subsequent retry. `factor` must be >= 1.0.
-   **`.with_max_delay(std::chrono::milliseconds max_val_delay)`**: Caps the maximum delay between retries, even with backoff.
-   **`.with_jitter(bool jitter_enabled = true, double factor = 0.1)`**: Enables/disables jitter. If enabled, a random amount (based on `factor`, e.g., 0.1 for +/-10%) is added/subtracted from the calculated delay.
-   **`.timeout(std::chrono::milliseconds max_timeout)`**: Sets an overall maximum duration for all retry attempts combined. If this timeout is exceeded, the operation fails.
-   **`.until(Predicate pred)`**: (For non-void functions only) Specifies a predicate `pred(const ReturnType& result)` that must return `true` for the result to be considered successful. If it returns `false`, a retry is triggered.
-   **`.on_exception(ExceptionPred handler)`**: Provides a predicate `handler(const std::exception& e)` that returns `true` if a caught exception `e` should trigger a retry.
-   **`.on_exception<ExceptionType>()`**: A convenience method to retry only if a specific `ExceptionType` (or derived) is caught.
-   **`.on_retry(Callback callback)`**: Sets a callback `callback(std::size_t attempt_num, const std::exception* e_ptr)` that is invoked before each retry. `e_ptr` is non-null if the retry is due to an exception.

### Execution
-   **`.run()`**: Executes the configured retry logic. Returns the result of the function if successful and non-void. Throws the last exception if all attempts fail due to exceptions, or `std::runtime_error` if timeout/value predicate failures occur after all attempts.
-   **`operator()()`**: Alias for `.run()`.

## `RetryBuilder` Convenience Class
Provides static factory methods for common retry patterns:
-   **`RetryBuilder::simple(Func&& fn, times, delay)`**
-   **`RetryBuilder::with_backoff(Func&& fn, times, initial_delay, factor)`**
-   **`RetryBuilder::on_exception<ExceptionType>(Func&& fn, times, delay)`**

## Usage Examples

(Based on `examples/retry_example.cpp`)

### 1. Basic Retry with Delay and Backoff

```cpp
#include "retry.h" // Adjust path as necessary
#include <iostream>
#include <stdexcept> // For std::runtime_error
#include <chrono>    // For time literals

// For convenience with time literals
using namespace std::chrono_literals;

int attempt_count = 0;
int task_that_fails_twice() {
    attempt_count++;
    std::cout << "Executing task_that_fails_twice, attempt #" << attempt_count << std::endl;
    if (attempt_count < 3) {
        throw std::runtime_error("Simulated temporary failure");
    }
    return 100; // Success
}

int main() {
    try {
        int result = retry_util::retry(task_that_fails_twice)
                        .times(4)                         // Max 4 attempts
                        .with_delay(100ms)                // Initial delay 100ms
                        .with_backoff(2.0)                // Delay doubles: 100ms, 200ms, 400ms
                        .on_retry([](std::size_t num, const std::exception* e) {
                            std::cout << "  Retry attempt #" << num;
                            if (e) std::cout << " due to: " << e->what();
                            std::cout << std::endl;
                        })
                        .run();
        std::cout << "Task succeeded with result: " << result << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Task ultimately failed: " << e.what() << std::endl;
    }
}
```

### 2. Retry Based on Return Value (`.until()`)

```cpp
#include "retry.h"
#include <iostream>
#include <chrono>
#include <random>

using namespace std::chrono_literals;

// Simulates an operation that eventually returns true
bool check_status() {
    static int call_num = 0;
    call_num++;
    bool success = (call_num >= 3); // Succeeds on the 3rd call
    std::cout << "check_status call #" << call_num << ", result: " << std::boolalpha << success << std::endl;
    return success;
}

int main() {
    try {
        bool final_status = retry_util::retry(check_status)
                                .times(5)
                                .with_delay(50ms)
                                .until([](bool status_result) { // Retry if lambda returns false
                                    return status_result == true;
                                })
                                .on_retry([](std::size_t attempt, const std::exception* /*e_ptr will be null*/){
                                    std::cout << "  Value predicate failed, retrying (attempt #" << attempt << ")" << std::endl;
                                })
                                .run();
        std::cout << "Status check succeeded: " << std::boolalpha << final_status << std::endl;
    } catch (const std::exception& e) {
        // This might be std::runtime_error("Retry failed: condition not met after all attempts")
        std::cout << "Status check ultimately failed: " << e.what() << std::endl;
    }
}
```

### 3. Handling Specific Exceptions and Timeout

```cpp
#include "retry.h"
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <thread> // For std::this_thread::sleep_for

using namespace std::chrono_literals;

struct MyCustomError : std::runtime_error {
    MyCustomError(const std::string& msg) : std::runtime_error(msg) {}
};

int operation_with_custom_error(int& counter) {
    counter++;
    std::cout << "Operation with custom error, attempt #" << counter << std::endl;
    if (counter == 1) throw MyCustomError("A custom error occurred!");
    if (counter == 2) throw std::runtime_error("A generic runtime error!"); // This won't be retried
    // Success on 3rd attempt if only MyCustomError is retried
    std::this_thread::sleep_for(60ms); // Simulate work
    return counter;
}

int main() {
    int attempt_counter = 0;
    try {
        int result = retry_util::retry([&attempt_counter](){ return operation_with_custom_error(attempt_counter); })
                        .times(5)
                        .with_delay(20ms)
                        .on_exception<MyCustomError>() // Only retry if MyCustomError is thrown
                        .timeout(100ms)               // Overall timeout for all attempts
                        .run();
        std::cout << "Operation succeeded with result: " << result << std::endl;
    } catch (const MyCustomError& e) {
        std::cout << "Failed due to MyCustomError after retries: " << e.what() << std::endl;
    } catch (const std::runtime_error& e) {
        std::cout << "Failed due to non-retried std::runtime_error or timeout: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Failed with other exception: " << e.what() << std::endl;
    }
}
```

## Dependencies
- `<functional>` (for `std::function`)
- `<chrono>`, `<thread>` (for delays and timeouts)
- `<exception>`, `<stdexcept>` (for exception handling)
- `<type_traits>` (for SFINAE and `std::invoke_result_t`)
- `<random>` (for jitter)
- `<memory>` (for `std::unique_ptr` in some internal details)

This retry utility offers a fluent and powerful way to make operations more resilient to transient issues by providing fine-grained control over the retry logic.

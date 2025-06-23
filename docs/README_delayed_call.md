# `util::DelayedCall` and `util::DelayedCallWithFuture`

## Overview

The `delayed_call.h` header provides C++17 utilities for scheduling callable objects (functions, lambdas, etc.) to be executed after a specified delay. This is useful for tasks like timeouts, retries, or any operation that needs to be deferred.

Two main classes are provided:
-   `DelayedCall`: Schedules a task and manages its lifecycle (cancellation, rescheduling). Each `DelayedCall` instance spawns its own thread for the timer.
-   `DelayedCallWithFuture<T>`: A wrapper around `DelayedCall` that returns a `std::future<T>` associated with the task's execution, allowing retrieval of results or exceptions.

Factory functions (`make_delayed_call`, `make_delayed_call_with_future`) are also available for convenient instantiation.

## Features

-   **Delayed Execution:** Schedule any callable to run after a specified duration (millisecond precision by default).
-   **Thread-Based:** Each delayed call typically runs its timer in a separate thread.
-   **Cancellation:** Pending tasks can be cancelled before execution.
-   **Rescheduling:** The delay for a pending task can be changed.
-   **Move Semantics:** `DelayedCall` objects are move-only, ensuring clear ownership of the scheduled task and its thread.
-   **Future Support (`DelayedCallWithFuture`):**
    -   Track task completion.
    -   Retrieve return values from tasks.
    -   Handle exceptions propagated from tasks.
-   **Exception Safety:** Exceptions thrown by the user-provided task within a `DelayedCall` are caught to prevent thread termination (logged to stderr in some internal stubs, but generally swallowed by default in the timer thread). For `DelayedCallWithFuture`, exceptions are propagated via the `std::future`.
-   **STL Chrono Integration:** Uses `std::chrono` for specifying delays.

## `util::DelayedCall`

### Public Interface Highlights
-   **Constructor**:
    ```cpp
    template<typename Callable>
    DelayedCall(Callable&& task, std::chrono::milliseconds delay);
    // Overload for any std::chrono::duration
    template<typename Callable, typename Rep, typename Period>
    DelayedCall(Callable&& task, std::chrono::duration<Rep, Period> delay);
    ```
-   **`void cancel()`**: Cancels the pending call.
-   **`void reschedule(std::chrono::milliseconds new_delay)`**: Reschedules with a new delay. Overload for `std::chrono::duration` also exists.
-   **`bool expired() const`**: True if the task has executed or been cancelled.
-   **`bool valid() const`**: True if the task is still scheduled to run.
-   **`std::chrono::milliseconds remaining_time() const`**: Gets the time remaining until execution.
-   **`std::chrono::milliseconds delay() const`**: Gets the original (or last rescheduled) delay.

## `util::DelayedCallWithFuture<T>`

Wraps a `DelayedCall` and provides a `std::future<T>`. `T` is `void` if the task has no return value.

### Public Interface Highlights
-   **Constructor**:
    ```cpp
    template<typename Callable>
    DelayedCallWithFuture(Callable&& task, std::chrono::milliseconds delay);
    // Overload for any std::chrono::duration
    template<typename Callable, typename Rep, typename Period>
    DelayedCallWithFuture(Callable&& task, std::chrono::duration<Rep, Period> delay);
    ```
-   **`void cancel()`**: Cancels the underlying `DelayedCall`.
-   **`void reschedule(std::chrono::milliseconds new_delay)`**: Reschedules the underlying `DelayedCall`.
-   **`bool expired() const` / `bool valid() const`**: Checks status of the underlying `DelayedCall`.
-   **`std::future<T>& get_future()`**: Returns a reference to the `std::future` associated with the task.

## Factory Functions (in `util` namespace)

```cpp
// For DelayedCall
template<typename Callable>
auto make_delayed_call(Callable&& task, std::chrono::milliseconds delay);
template<typename Callable, typename Rep, typename Period>
auto make_delayed_call(Callable&& task, std::chrono::duration<Rep, Period> delay);

// For DelayedCallWithFuture
template<typename T = void, typename Callable>
auto make_delayed_call_with_future(Callable&& task, std::chrono::milliseconds delay);
template<typename T = void, typename Callable, typename Rep, typename Period>
auto make_delayed_call_with_future(Callable&& task, std::chrono::duration<Rep, Period> delay);
```

## Usage Examples

(Based on `examples/delayed_call_example.cpp`)

### Basic Delayed Execution

```cpp
#include "delayed_call.h"
#include <iostream>
#include <chrono>
#include <thread> // For std::this_thread::sleep_for

using namespace std::chrono_literals; // For ms, s, etc.

int main() {
    bool task_done = false;
    std::cout << "Scheduling a task to run in 200ms." << std::endl;

    {
        util::DelayedCall delayed_task([&task_done]() {
            task_done = true;
            std::cout << "Delayed task executed!" << std::endl;
        }, 200ms);

        std::cout << "Task scheduled. Waiting..." << std::endl;
        std::this_thread::sleep_for(100ms);
        std::cout << "Is task done yet? " << std::boolalpha << task_done << std::endl; // false

        // Wait for task to complete or DelayedCall destructor to run
        std::this_thread::sleep_for(150ms);
    } // delayed_task destructor will cancel if not yet fired, and join thread

    std::cout << "Final task status: " << std::boolalpha << task_done << std::endl; // true
}
```

### Cancellation and Rescheduling

```cpp
#include "delayed_call.h"
#include <iostream>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

int main() {
    int execution_count = 0;
    util::DelayedCall task([&execution_count]() {
        execution_count++;
        std::cout << "Task run attempt: " << execution_count << std::endl;
    }, 500ms);

    std::cout << "Task scheduled for 500ms." << std::endl;
    std::this_thread::sleep_for(100ms);

    task.reschedule(100ms); // Reschedule to run sooner (100ms from now)
    std::cout << "Task rescheduled for 100ms from now." << std::endl;

    std::this_thread::sleep_for(50ms);
    if (task.valid()) {
        std::cout << "Task is still valid, cancelling it." << std::endl;
        task.cancel();
    }

    std::this_thread::sleep_for(200ms); // Wait past original rescheduled time
    std::cout << "Total executions: " << execution_count << std::endl; // Should be 0
}
```

### Using `DelayedCallWithFuture`

```cpp
#include "delayed_call.h"
#include <iostream>
#include <string>
#include <future> // For std::future
#include <chrono>

using namespace std::chrono_literals;

int main() {
    // Task returning a value
    auto delayed_string_task = util::make_delayed_call_with_future<std::string>(
        []() {
            std::cout << "Future task (string): Executing..." << std::endl;
            return std::string("Hello from the future!");
        },
        150ms
    );

    std::future<std::string>& future_str = delayed_string_task.get_future();
    std::cout << "Waiting for string future..." << std::endl;
    std::string result = future_str.get(); // Blocks until task completes
    std::cout << "String future result: \"" << result << "\"" << std::endl;

    // Task returning void
    auto delayed_void_task = util::make_delayed_call_with_future( // T defaults to void
        []() {
            std::cout << "Future task (void): Executing and completing." << std::endl;
        },
        100ms
    );

    std::future<void>& future_void = delayed_void_task.get_future();
    std::cout << "Waiting for void future..." << std::endl;
    future_void.wait(); // Blocks until task completes
    std::cout << "Void future completed." << std::endl;
}
```

## Dependencies
- `<functional>` (for `std::function`)
- `<thread>`
- `<chrono>`
- `<atomic>`
- `<future>` (for `DelayedCallWithFuture`)
- `<memory>` (for `std::unique_ptr` in `DelayedCallWithFuture`)
- `<mutex>`
- `<condition_variable>`

This utility provides a convenient way to manage time-deferred operations in C++, especially when dealing with asynchronous-like patterns or simple timeouts.

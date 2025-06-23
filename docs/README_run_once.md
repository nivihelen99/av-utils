# `RunOnce` and `RunOnceReturn<T>` Utilities

This document describes the `RunOnce` and `RunOnceReturn<T>` utility classes, designed to ensure that a callable object (like a function or lambda) is executed exactly once, even in multi-threaded environments.

## Overview

In many applications, especially those involving concurrency, there's a need to perform an initialization step, a costly computation, or some setup logic precisely once. `RunOnce` and `RunOnceReturn<T>` provide a clean, thread-safe, and exception-aware mechanism to achieve this, offering a more ergonomic alternative to `std::once_flag` and `std::call_once`.

### Features:
- **Thread-Safe Execution**: Guarantees that the callable is invoked only once across multiple threads.
- **Exception Safety**: If the callable throws an exception, it's not considered "run," and subsequent attempts will try to execute it again.
- **Value Return (`RunOnceReturn<T>`)**: The `RunOnceReturn<T>` variant captures and stores the return value of the callable, making it available for subsequent calls without re-computation.
- **State Introspection**: `has_run()` method allows checking if the callable has successfully completed.
- **Reset Functionality**: A `reset()` method is provided, primarily for testing purposes, to revert the state and allow re-execution.
- **Header-Only**: Easy to integrate into any C++17 project.

## `RunOnce`

The `RunOnce` class ensures that a void-returning callable is executed exactly once.

### Usage

```cpp
#include "run_once.h"
#include <iostream>
#include <thread>

static RunOnce global_initializer;

void initialize_shared_resource() {
    global_initializer([] {
        std::cout << "Initializing shared resource...\n";
        // Perform actual initialization
        std::cout << "Shared resource initialized.\n";
    });
}

int main() {
    std::thread t1(initialize_shared_resource);
    std::thread t2(initialize_shared_resource);

    t1.join();
    t2.join();

    // The initialization messages will appear only once.
    return 0;
}
```

### Key Methods:
- **`RunOnce()`**: Default constructor.
- **`template <typename Callable> void operator()(Callable&& fn)`**: Executes `fn` if it hasn't been run successfully before. If `fn` throws, `has_run()` remains `false`.
- **`bool has_run() const noexcept`**: Returns `true` if `fn` has completed successfully, `false` otherwise.
- **`void reset() noexcept`**: Resets the internal state. **Warning**: Not thread-safe. Use only when you can guarantee no other threads are accessing this instance (e.g., in single-threaded test environments).

## `RunOnceReturn<T>`

The `RunOnceReturn<T>` class extends `RunOnce` to handle callables that return a value of type `T`. It executes the callable once, stores its result, and returns the stored result on subsequent invocations.

### Usage

```cpp
#include "run_once.h"
#include <iostream>
#include <string>
#include <thread>
#include <vector>

RunOnceReturn<std::string> config_loader;

const std::string& get_configuration() {
    return config_loader([] {
        std::cout << "Loading configuration...\n";
        // Simulate expensive loading
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return std::string("LoadedConfigurationData");
    });
}

void worker_thread_func() {
    const std::string& config = get_configuration();
    std::cout << "Thread " << std::this_thread::get_id()
              << " using config: " << config << std::endl;
}

int main() {
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(worker_thread_func);
    }

    for (auto& t : threads) {
        t.join();
    }
    // The "Loading configuration..." message appears only once.
    // All threads will use the same "LoadedConfigurationData".

    // Accessing again will return the cached value
    std::cout << "Main thread accessing config: " << get_configuration() << std::endl;

    return 0;
}
```

### Key Methods:
- **`RunOnceReturn()`**: Default constructor.
- **`template <typename Callable> const T& operator()(Callable&& fn)`**: Executes `fn` if it hasn't been run successfully. Stores and returns its result. If `fn` throws, `has_run()` remains `false`, and no value is stored. Subsequent calls will retry `fn`.
- **`bool has_run() const noexcept`**: Returns `true` if `fn` has completed successfully and the result is available.
- **`const T& get() const noexcept`**: Returns a reference to the stored result. **Undefined behavior if called before `fn` has successfully run.** Always check `has_run()` first if unsure.
- **`void reset() noexcept`**: Resets the internal state, clearing any stored value. **Warning**: Not thread-safe. Use with caution.

## Thread Safety

Both `RunOnce` and `RunOnceReturn<T>` are designed to be thread-safe for their main operation: invoking the callable (`operator()`) and checking `has_run()`. Multiple threads can call `operator()` concurrently, and the callable will still execute only once.

The `reset()` method is **not thread-safe**. It should only be called in situations where it's guaranteed that no other thread is concurrently accessing the `RunOnce` or `RunOnceReturn<T>` instance. Its primary use case is for resetting state in unit tests.

The `get()` method in `RunOnceReturn<T>` is thread-safe for reads *after* the callable has successfully executed and the value has been stored. Calling `get()` before `has_run()` is true leads to undefined behavior.

## Exception Handling

If the callable provided to `operator()` throws an exception:
1. The exception is propagated to the caller of `operator()`.
2. The `RunOnce` / `RunOnceReturn<T>` instance is **not** marked as "run" (i.e., `has_run()` will return `false`).
3. For `RunOnceReturn<T>`, no value is stored.
4. Subsequent calls to `operator()` will attempt to execute the callable again.

This behavior ensures that a transient failure during the one-time execution doesn't permanently prevent the operation from succeeding on a later attempt.

## Non-Copyable and Non-Movable

Due to the use of `std::once_flag`, both `RunOnce` and `RunOnceReturn<T>` are non-copyable and non-movable. If you need to store them in containers or pass them around, consider using pointers (e.g., `std::unique_ptr` or `std::shared_ptr`) or references.

## When to Use

- Initializing singletons or global resources.
- Performing expensive computations whose results can be cached and reused.
- Ensuring setup code in a class or module runs only once, regardless of how many times its methods are called.
- Any scenario requiring a "call exactly once" semantic in a potentially concurrent environment.
```

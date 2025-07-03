# `AsyncValue<T>` - Lightweight Future/Promise Abstraction

## Overview

`AsyncValue<T>` is a header-only C++17 utility that provides a minimal, thread-safe, single-value future/promise-like mechanism for deferred value delivery and event signaling. It is designed for scenarios where `std::future` and `std::promise` might be too heavyweight or when direct blocking is undesirable.

Key features:
- **Header-only:** Easy to integrate (`include/async_value.h`).
- **Thread-safe:** Can be used to communicate values/events between threads.
- **Non-blocking by default:** Operations like `get_if()` and `ready()` do not block. `get()` will throw an exception or assert if the value is not ready.
- **Callback-based notification:** Use `on_ready()` to register a function that will be called when the value is set or the event is signaled.
- **Exactly-once delivery:** A value or signal is set only once.
- **`AsyncValue<void>` specialization:** For pure event signaling without an associated value.
- **Reset capability:** Can be reset to its initial state for reuse.
- **Supports move-only types:** Such as `std::unique_ptr<T>`.

## Core API

```cpp
template<typename T>
class AsyncValue {
public:
    AsyncValue();

    // Sets the value. Can only be called once unless reset.
    void set_value(T value);

    // Returns true if the value has been set.
    bool ready() const;

    // Returns a pointer to the value if ready, else nullptr. Non-blocking.
    const T* get_if() const;
    T* get_if();

    // Returns a reference to the value.
    // Throws std::logic_error or asserts if not ready.
    const T& get() const;
    T& get();

    // Registers a callback. If value is already set, callback is invoked immediately.
    // Callback signature: void(const T& value)
    void on_ready(std::function<void(const T&)> callback);

    // Resets to initial state. Clears value and callbacks.
    void reset();
};

// Specialization for void (event signaling)
template<>
class AsyncValue<void> {
public:
    AsyncValue();

    // Signals the event. Can only be called once unless reset.
    void set();

    // Returns true if the event has been signaled.
    bool ready() const;

    // Checks if ready; asserts if not.
    void get() const;

    // Registers a callback. If event is already signaled, callback is invoked immediately.
    // Callback signature: void()
    void on_ready(std::function<void()> callback);

    // Resets to initial state. Clears event state and callbacks.
    void reset();
};
```

## Basic Usage

**Producing a value:**
```cpp
#include "async_value.h"
#include <thread>
#include <iostream>

AsyncValue<int> av_int;

void work() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    av_int.set_value(42);
}
```

**Consuming a value:**
```cpp
// Using on_ready callback
av_int.on_ready([](int value) {
    std::cout << "Callback: Value is ready: " << value << std::endl;
});

// Or polling/direct get (after ensuring readiness or handling exceptions)
// std::thread t(work);
// t.join(); // Ensure work is done
// if (av_int.ready()) {
//     std::cout << "Main: Value is " << av_int.get() << std::endl;
// }
```

## Building

The provided `CMakeLists.txt` can be used to build the example and tests.
It defines an `INTERFACE` library target `async_value`.

```bash
mkdir build && cd build
cmake ..
make
./examples/async_value_example  # Run the example
./tests/async_value_test      # Run tests
```

## Thread Safety Notes

- `set_value()` / `set()` are thread-safe.
- `ready()`, `get()`, `get_if()` are thread-safe for concurrent reads and concurrent with `set_value`.
- `on_ready()` is thread-safe for concurrent registrations and concurrent with `set_value`.
- Callbacks are invoked on the thread that calls `set_value()` or `set()`. If `on_ready()` is called after the value is set, the callback is invoked immediately on the caller's thread.
- The user must ensure that the lifetime of the `AsyncValue` object outlives any threads interacting with it.
- Callbacks should generally be lightweight. If a callback needs to perform a long-running operation or could potentially call back into the `AsyncValue` in a complex way, consider offloading that work.

## Limitations / Considerations

- **No built-in blocking `get()`:** The current design is non-blocking. If a blocking get is required, it would need to be implemented using the `std::condition_variable` (which is present but not currently used for blocking gets).
- **Error Handling for `set_value`:** `set_value` (and `set` for `void`) currently `assert`s if called more than once. In a production environment, this might be changed to throw an exception or return an error status.
- **Callback Execution Context:** Callbacks are executed synchronously on the thread calling `set_value()` (or `on_ready()` if the value is already available). This means `set_value()` will not return until all initially registered callbacks have completed.
- **No cancellation:** Once a value is being "produced", there's no mechanism in `AsyncValue` itself to cancel the operation or invalidate the `AsyncValue`.
- **No timeout for getting value:** Users would need to implement timeout logic externally if needed.

This utility is intended for simpler cases of asynchronous value passing where the overhead or complexity of `std::future`/`std::promise` is not desired.

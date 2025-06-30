# InstrumentedRingBuffer

## Table of Contents
1.  [Overview](#overview)
2.  [Purpose and Use Cases](#purpose-and-use-cases)
3.  [API Description](#api-description)
    *   [Constructor](#constructor)
    *   [Core Methods](#core-methods)
    *   [State and Capacity Methods](#state-and-capacity-methods)
    *   [Introspection Metrics](#introspection-metrics)
4.  [Usage Example](#usage-example)
5.  [Thread Safety](#thread-safety)

## Overview

`InstrumentedRingBuffer<T>` is a thread-safe, fixed-size circular buffer (also known as a ring buffer) that collects and exposes various metrics about its operation. This allows for monitoring its performance, diagnosing bottlenecks, and tuning its capacity in concurrent producer-consumer scenarios.

It provides both blocking and non-blocking methods for pushing items into and popping items from the buffer. When the buffer is full, blocking push operations will wait until space becomes available. When the buffer is empty, blocking pop operations will wait until an item is available. Non-blocking operations return immediately, indicating success or failure.

## Purpose and Use Cases

The primary purpose of `InstrumentedRingBuffer` is to provide a bounded queue mechanism for inter-thread communication while offering insights into its runtime behavior.

Common use cases include:
*   **Producer-Consumer Scenarios**: Decoupling threads or components where one or more producers generate data and one or more consumers process it. The metrics help understand if the buffer size is appropriate, or if producers/consumers are frequently blocked.
*   **Data Streaming**: Buffering data chunks in a streaming pipeline, where monitoring buffer utilization and potential overflows/underflows is crucial.
*   **Task Queues**: Managing a fixed number of pending tasks where it's important to know how often the queue hits its capacity or how often workers are waiting for tasks.
*   **Debugging and Performance Tuning**: The introspection metrics can be invaluable for identifying performance issues related to shared data structures in concurrent applications. For example, a high `push_wait_count` might indicate that consumers are too slow or the buffer is too small. A high `try_pop_fail_count` might indicate producers are too slow or consumers are polling too aggressively.

## API Description

The `InstrumentedRingBuffer` is a template class `InstrumentedRingBuffer<T>`.

### Constructor

*   `explicit InstrumentedRingBuffer(size_t capacity)`
    *   Creates an instrumented ring buffer with the specified `capacity`.
    *   The capacity must be greater than 0. If 0 is provided, it defaults to a capacity of 1.

### Core Methods

These methods are for adding and removing items from the buffer.

*   `bool try_push(const T& item)`
*   `bool try_push(T&& item)`
    *   Attempts to push an item into the buffer without blocking.
    *   Returns `true` if the item was successfully pushed.
    *   Returns `false` if the buffer is full, in which case the item is not pushed.
    *   Updates `try_push_fail_count` on failure, or `push_success_count` and `peak_size` on success.

*   `void push(const T& item)`
*   `void push(T&& item)`
    *   Pushes an item into the buffer. If the buffer is full, this call blocks until space becomes available.
    *   Updates `push_success_count` and `peak_size` on success.
    *   Updates `push_wait_count` if the operation had to wait.

*   `bool try_pop(T& item)`
    *   Attempts to pop an item from the buffer into the `item` reference without blocking.
    *   Returns `true` if an item was successfully popped.
    *   Returns `false` if the buffer is empty.
    *   Updates `try_pop_fail_count` on failure, or `pop_success_count` on success.

*   `T pop()`
    *   Pops an item from the buffer. If the buffer is empty, this call blocks until an item becomes available.
    *   Returns the popped item.
    *   Updates `pop_success_count` on success.
    *   Updates `pop_wait_count` if the operation had to wait.

### State and Capacity Methods

*   `size_t size() const`
    *   Returns the current number of items in the buffer.
*   `size_t capacity() const`
    *   Returns the maximum capacity of the buffer.
*   `bool empty() const`
    *   Returns `true` if the buffer is empty, `false` otherwise.
*   `bool full() const`
    *   Returns `true` if the buffer is full, `false` otherwise.

### Introspection Metrics

These methods provide access to the collected operational metrics. All metric access is thread-safe.

*   `uint64_t get_push_success_count() const`: Total number of successful push operations (both `push` and `try_push`).
*   `uint64_t get_pop_success_count() const`: Total number of successful pop operations (both `pop` and `try_pop`).
*   `uint64_t get_push_wait_count() const`: Number of times a blocking `push` operation had to wait because the buffer was full.
*   `uint64_t get_pop_wait_count() const`: Number of times a blocking `pop` operation had to wait because the buffer was empty.
*   `uint64_t get_try_push_fail_count() const`: Number of times `try_push` failed because the buffer was full.
*   `uint64_t get_try_pop_fail_count() const`: Number of times `try_pop` failed because the buffer was empty.
*   `size_t get_peak_size() const`: The maximum number of items observed in the buffer simultaneously since its creation or the last metric reset.
*   `void reset_metrics()`: Resets all collected metrics (including `peak_size`) to zero.

## Usage Example

```cpp
#include "instrumented_ring_buffer.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <string>

void print_buffer_metrics(const cpp_utils::InstrumentedRingBuffer<int>& buffer, const std::string& context) {
    std::cout << "\nMetrics (" << context << "):" << std::endl;
    std::cout << "  Size/Capacity: " << buffer.size() << "/" << buffer.capacity() << std::endl;
    std::cout << "  Peak Size: " << buffer.get_peak_size() << std::endl;
    std::cout << "  Push Success: " << buffer.get_push_success_count() << std::endl;
    std::cout << "  Pop Success: " << buffer.get_pop_success_count() << std::endl;
    std::cout << "  Push Wait: " << buffer.get_push_wait_count() << std::endl;
    std::cout << "  Pop Wait: " << buffer.get_pop_wait_count() << std::endl;
    std::cout << "  Try Push Fail: " << buffer.get_try_push_fail_count() << std::endl;
    std::cout << "  Try Pop Fail: " << buffer.get_try_pop_fail_count() << std::endl;
}

int main() {
    cpp_utils::InstrumentedRingBuffer<int> buffer(5);

    // Producer thread
    std::thread producer([&]() {
        for (int i = 0; i < 10; ++i) {
            buffer.push(i);
            std::cout << "Producer pushed: " << i << std::endl;
            if (i % 3 == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    });

    // Consumer thread
    std::thread consumer([&]() {
        for (int i = 0; i < 10; ++i) {
            if (i % 4 == 0) { // Let consumer be slower sometimes
                 std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
            int item = buffer.pop();
            std::cout << "Consumer popped: " << item << std::endl;
        }
    });

    producer.join();
    consumer.join();

    print_buffer_metrics(buffer, "After producer-consumer run");

    // Example of using try_push and try_pop
    buffer.reset_metrics();
    std::cout << "\n--- TryPush/TryPop Example ---" << std::endl;
    for(int i=0; i<7; ++i) {
        if (buffer.try_push(i*10)) {
            std::cout << "try_push succeeded for " << i*10 << std::endl;
        } else {
            std::cout << "try_push failed for " << i*10 << " (buffer full)" << std::endl;
        }
    }
    print_buffer_metrics(buffer, "After try_push attempts");

    int val;
    for(int i=0; i<7; ++i) {
        if (buffer.try_pop(val)) {
            std::cout << "try_pop succeeded, got " << val << std::endl;
        } else {
            std::cout << "try_pop failed (buffer empty)" << std::endl;
        }
    }
    print_buffer_metrics(buffer, "After try_pop attempts");

    return 0;
}
```

## Thread Safety

The `InstrumentedRingBuffer` is designed to be thread-safe. All public methods that access or modify shared state (the underlying buffer, head/tail pointers, current size, and metrics) are protected by an internal mutex. Condition variables (`std::condition_variable`) are used to manage blocking for `push` and `pop` operations efficiently, preventing busy-waiting.

Metric counters are implemented using `std::atomic` types, ensuring that increments and reads are atomic operations and do not require additional locking for individual metric updates if accessed directly, though most metric updates are implicitly covered by the main mutex during core operations. Getters for metrics also ensure safe reading.
The `reset_metrics` method also handles metrics atomically or under a lock if necessary for consistency.

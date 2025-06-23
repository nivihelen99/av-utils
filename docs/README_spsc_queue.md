# `concurrent::ring_buffer` (SPSC Lock-Free Queue)

## Overview

The `spsc.h` header provides the `concurrent::ring_buffer<T, Ordering, Allocator>` class, a high-performance, lock-free (or nearly lock-free, relying on atomic operations) Single-Producer, Single-Consumer (SPSC) queue. It is implemented as a ring buffer, which is a fixed-size circular array.

This type of queue is specifically optimized for scenarios where there is exactly one thread producing data (the producer) and exactly one thread consuming data (the consumer). By leveraging atomic operations and careful memory ordering, it aims to achieve high throughput and low latency without the need for traditional locks (like mutexes) for its core push/pop operations.

## Namespace
All utilities are within the `concurrent` namespace.

## Template Parameters

-   `T`: The type of elements stored in the queue. `T` must be nothrow destructible for true lock-free operation, and generally copy/move constructible/assignable as needed by the operations used.
-   `Ordering`: A `concurrent::memory_ordering` enum value specifying the memory synchronization constraints for atomic operations.
    -   `memory_ordering::relaxed`: Offers the highest performance with the weakest guarantees.
    -   `memory_ordering::acquire_release` (Default): Provides a balance, ensuring that operations before a store (push) are visible after a load (pop) that sees that store.
    -   `memory_ordering::sequential`: Provides the strongest guarantees (sequential consistency), potentially at a higher performance cost.
-   `Allocator`: The allocator type for memory management of elements `T` and internal slots. Defaults to `std::allocator<T>`.

## Key Features

-   **SPSC Optimized:** Designed for exactly one producer thread and one consumer thread. Using it with multiple producers or multiple consumers will lead to data races and undefined behavior.
-   **Lock-Free Design:** Uses `std::atomic` for head and tail indices and for slot ready flags to manage synchronization without mutexes for core operations.
-   **Ring Buffer Implementation:** Uses a fixed-size array internally. The capacity **must be a power of 2**.
-   **Cache-Friendly:** Head and tail indices are cache-line aligned to help prevent false sharing between producer and consumer threads.
-   **In-Place Construction/Destruction:** Elements are constructed in-place in the buffer slots using placement new (via `std::allocator_traits`) and explicitly destructed.
-   **Memory Ordering Control:** Allows choosing the memory ordering policy for atomic operations to tune performance vs. safety guarantees.
-   **Blocking and Non-Blocking Operations:** Offers both `try_push`/`try_pop` (non-blocking) and `push`/`pop` (blocking, busy-wait) variants. Timed versions (`push_for`/`pop_for`) are also available.
-   **Optional Statistics:** Can track metrics like total pushes/pops, failed attempts, and utilization.
-   **Move-Only:** The `ring_buffer` itself is move-only.

## Internal Structure

-   **Slots:** The buffer is an array of `slot` structs. Each `slot` contains:
    -   Raw byte storage for an object of type `T`.
    -   An `std::atomic<bool> ready` flag, indicating if the producer has finished writing to the slot and it's ready for the consumer.
-   **Head/Tail Pointers:** `std::atomic<size_type> head_` (consumer index) and `std::atomic<size_type> tail_` (producer index) track the state of the queue. These are cache-aligned.

## Public Interface Highlights

### Constructor
-   **`explicit ring_buffer(size_type capacity, const Allocator& alloc = Allocator{})`**:
    -   `capacity`: Must be a power of 2 and greater than 0. Throws `std::invalid_argument` otherwise.

### Producer Operations (Call from producer thread only)
-   **`bool try_push(U&& item)`**: Non-blocking push. Returns `false` if full.
-   **`bool try_emplace(Args&&... args)`**: Non-blocking in-place construction. Returns `false` if full.
-   **`void push(U&& item)`**: Blocking push (busy-waits until space is available).
-   **`bool push_for(U&& item, duration)`**: Blocking push with timeout.

### Consumer Operations (Call from consumer thread only)
-   **`std::optional<T> try_pop()`**: Non-blocking pop. Returns `std::nullopt` if empty.
-   **`bool try_pop_into(T& item)`**: Non-blocking pop into an existing object `item`.
-   **`T pop()`**: Blocking pop (busy-waits until an item is available).
-   **`std::optional<T> pop_for(duration)`**: Blocking pop with timeout.

### Capacity & State
-   **`size_type capacity() const noexcept`**
-   **`size_type size() const noexcept`**: Approximate number of elements currently in the queue.
-   **`bool empty() const noexcept`**: Checks if empty.
-   **`bool full() const noexcept`**: Checks if full.
-   **`double utilization() const noexcept`**: Approximate current utilization.

### Statistics
-   **`void enable_stats()` / `void disable_stats()`**
-   **`const ring_buffer_stats* get_stats() const noexcept`**
-   **`void reset_stats()`**

### Other
-   **`template<typename F> bool peek(F&& func) const`**: Applies `func` to the front element if available, without popping.
-   **`void clear()`**: Clears the queue (Note: stated as not thread-safe in header; use with caution when no concurrent operations).

### Factory Functions
-   **`make_ring_buffer<T, Ordering>(capacity)`**
-   **`make_relaxed_ring_buffer<T>(capacity)`**
-   **`make_sequential_ring_buffer<T>(capacity)`**

### Utility
-   **`constexpr std::size_t next_power_of_two(std::size_t n)`**

## Usage Examples

(Based on `examples/use_spsc.cpp`)

### Basic Producer-Consumer

```cpp
#include "spsc.h" // Adjust path as needed
#include <iostream>
#include <thread>
#include <vector>
#include <chrono> // For std::chrono literals

int main() {
    concurrent::ring_buffer<int> queue(16); // Capacity must be a power of 2

    std::thread producer_thread([&]() {
        for (int i = 0; i < 50; ++i) {
            // try_push is non-blocking
            while (!queue.try_push(i)) {
                // std::this_thread::yield(); // Or sleep for very short duration
            }
            // For guaranteed push, use queue.push(i);
            std::cout << "Pushed: " << i << std::endl;
        }
    });

    std::thread consumer_thread([&]() {
        int items_received = 0;
        while (items_received < 50) {
            std::optional<int> item = queue.try_pop();
            if (item) {
                std::cout << "Popped: " << *item << std::endl;
                items_received++;
            } else {
                // std::this_thread::yield(); // Or sleep
            }
            // For guaranteed pop, use int val = queue.pop();
        }
    });

    producer_thread.join();
    consumer_thread.join();

    std::cout << "Basic SPSC queue usage finished." << std::endl;
}
```

### Performance Benchmark Snippet

```cpp
#include "spsc.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <numeric> // For std::iota in a real sum check

constexpr size_t NUM_ITEMS = 1000000;
constexpr size_t BUFFER_CAPACITY = 1024;

int main() {
    concurrent::ring_buffer<uint64_t> buffer(BUFFER_CAPACITY);
    buffer.enable_stats();

    auto start = std::chrono::high_resolution_clock::now();

    std::thread producer([&]() {
        for (uint64_t i = 0; i < NUM_ITEMS; ++i) {
            buffer.push(i); // Blocking push
        }
    });

    uint64_t sum = 0;
    std::thread consumer([&]() {
        for (size_t i = 0; i < NUM_ITEMS; ++i) {
            sum += buffer.pop(); // Blocking pop
        }
    });

    producer.join();
    consumer.join();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "Processed " << NUM_ITEMS << " items in " << duration_us << " us." << std::endl;
    std::cout << "Throughput: " << (NUM_ITEMS * 1000000.0 / duration_us) << " items/sec." << std::endl;

    if (const auto* stats = buffer.get_stats()) {
        std::cout << "Pushes: " << stats->total_pushes.load()
                  << ", Pops: " << stats->total_pops.load() << std::endl;
    }
}
```

## Dependencies
- `<atomic>`, `<memory>`, `<type_traits>`, `<utility>`, `<functional>`, `<cstddef>`, `<new>`, `<bit>`, `<concepts>` (C++20, used for `std::floating_point` in `safe_divide`, but core queue doesn't depend on it), `<optional>`, `<chrono>`, `<thread>`, `<stdexcept>`, `<iostream>` (for examples/logging).

The `concurrent::ring_buffer` is a specialized queue for high-throughput communication between a single producer and a single consumer thread, minimizing synchronization overhead.

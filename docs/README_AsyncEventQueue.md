# `AsyncEventQueue<T>`

## Overview

The `AsyncEventQueue<T>` is a thread-safe, template-based queue designed for asynchronous communication between different parts of an application, typically running in separate threads. It allows producers to add items to the queue and consumers to retrieve items from it, with built-in synchronization mechanisms to handle concurrent access.

The queue can be configured as bounded (with a maximum size) or unbounded. When bounded, operations that would exceed the capacity (like `put` on a full queue) will block until space becomes available. Similarly, `get` operations on an empty queue will block until an item is available. A non-blocking `try_get` method is also provided.

Additionally, a callback mechanism allows for notification when an item is added to a previously empty queue.

## Template Parameter

-   `T`: The type of elements to be stored in the queue.

## Public Interface

### Constructor

-   **`AsyncEventQueue(size_t maxsize = 0)`**
    -   Constructs an `AsyncEventQueue`.
    -   `maxsize`: The maximum number of items the queue can hold. If `0` (default), the queue is unbounded (limited by available memory).

### Core Methods

-   **`void put(const T& item)`**
    -   Adds a copy of `item` to the end of the queue.
    -   If the queue is bounded and full, this call blocks until space becomes available.
    -   Thread-safe.

-   **`void put(T&& item)`**
    -   Moves `item` to the end of the queue.
    -   If the queue is bounded and full, this call blocks until space becomes available.
    -   Thread-safe.

-   **`T get()`**
    -   Removes and returns the item from the front of the queue.
    -   If the queue is empty, this call blocks until an item is available.
    -   Thread-safe.

-   **`std::optional<T> try_get()`**
    -   Attempts to remove and return the item from the front of the queue.
    -   If the queue is empty, it returns `std::nullopt` immediately (non-blocking).
    -   Otherwise, it returns an `std::optional<T>` containing the retrieved item.
    -   Thread-safe.

### State and Configuration

-   **`size_t size() const`**
    -   Returns the current number of items in the queue.
    -   Thread-safe.

-   **`bool empty() const`**
    -   Returns `true` if the queue is empty, `false` otherwise.
    -   Thread-safe.

-   **`bool full() const`**
    -   Returns `true` if the queue is bounded and has reached its maximum capacity, `false` otherwise.
    -   Thread-safe.

-   **`void register_callback(std::function<void()> cb)`**
    -   Registers a callback function `cb` to be invoked.
    -   The callback is triggered after an item is successfully `put` into the queue *only if the queue was empty before that `put` operation*.
    -   Pass `nullptr` or an empty `std::function` to effectively unregister or disable the callback.
    -   Thread-safe.

## Usage Examples

### Basic Producer-Consumer

```cpp
#include "AsyncEventQueue.h"
#include <iostream>
#include <thread>
#include <string>

int main() {
    // Create a queue for strings with a max size of 5
    AsyncEventQueue<std::string> message_queue(5);

    // Producer thread
    std::thread producer_thread([&]() {
        for (int i = 0; i < 7; ++i) {
            std::string message = "Message " + std::to_string(i);
            std::cout << "Producer: Sending '" << message << "'" << std::endl;
            message_queue.put(message); // Blocks if queue is full
            std::cout << "Producer: Sent. Queue size: " << message_queue.size() << std::endl;
        }
    });

    // Consumer thread
    std::thread consumer_thread([&]() {
        for (int i = 0; i < 7; ++i) {
            std::cout << "Consumer: Waiting for message..." << std::endl;
            std::string received_message = message_queue.get(); // Blocks if queue is empty
            std::cout << "Consumer: Received '" << received_message << "'. Queue size: "
                      << message_queue.size() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simulate work
        }
    });

    producer_thread.join();
    consumer_thread.join();

    std::cout << "Main: All messages processed. Queue is empty: " << std::boolalpha << message_queue.empty() << std::endl;

    return 0;
}
```

### Using `try_get` for Non-Blocking Retrieval

```cpp
#include "AsyncEventQueue.h"
#include <iostream>
#include <optional>

int main() {
    AsyncEventQueue<int> data_queue(2);
    data_queue.put(10);
    data_queue.put(20);

    std::optional<int> val1 = data_queue.try_get();
    if (val1) {
        std::cout << "try_get item 1: " << *val1 << std::endl; // Output: 10
    }

    std::optional<int> val2 = data_queue.try_get();
    if (val2) {
        std::cout << "try_get item 2: " << *val2 << std::endl; // Output: 20
    }

    std::optional<int> val3 = data_queue.try_get(); // Queue is now empty
    if (!val3) {
        std::cout << "try_get on empty queue returned nullopt." << std::endl;
    }
    return 0;
}
```

### Callback Notification

```cpp
#include "AsyncEventQueue.h"
#include <iostream>

void on_item_added_to_empty_queue() {
    std::cout << "[Callback] An item was added to an empty queue!" << std::endl;
}

int main() {
    AsyncEventQueue<int> event_queue(3);

    event_queue.register_callback(on_item_added_to_empty_queue);

    std::cout << "Putting 1st item (queue is empty). Expect callback." << std::endl;
    event_queue.put(1); // Callback will be triggered

    std::cout << "Putting 2nd item (queue not empty). Expect no callback." << std::endl;
    event_queue.put(2); // Callback will NOT be triggered

    event_queue.get(); // Remove 1
    event_queue.get(); // Remove 2 (queue becomes empty)

    std::cout << "Queue is empty. Putting 3rd item. Expect callback." << std::endl;
    event_queue.put(3); // Callback will be triggered again

    event_queue.get(); // Remove 3
    return 0;
}
```

## Thread Safety

All public methods of `AsyncEventQueue` are designed to be thread-safe. Internal synchronization is handled using `std::mutex` and `std::condition_variable` to coordinate access between threads.
Producers can call `put` concurrently with other producers and consumers. Consumers can call `get` or `try_get` concurrently with other consumers and producers. `register_callback` is also thread-safe.
The `size()`, `empty()`, and `full()` methods acquire a lock to ensure they provide a consistent snapshot of the queue's state, though the state might change immediately after the call in a highly concurrent environment.

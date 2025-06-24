# CallQueue and ThreadSafeCallQueue

The `call_queue.h` header provides two classes, `CallQueue` and `ThreadSafeCallQueue`, designed for managing and executing deferred function calls. This is useful in scenarios where operations need to be queued and processed sequentially, either in a single-threaded or multi-threaded environment.

## `CallQueue`

`CallQueue` is a single-threaded queue that stores `std::function<void()>` objects (tasks) and executes them in First-In-First-Out (FIFO) order.

### Features

-   **Task Queuing**: Add tasks using `push()`.
-   **Task Coalescing**: Add or update tasks associated with a specific key using `coalesce()`. Only the latest task for a given key is stored.
-   **Execution**:
    -   `drain_all()`: Executes all tasks in the queue. Tasks added during a `drain_all()` call are deferred to the next cycle to prevent reentrancy issues.
    -   `drain_one()`: Executes a single task from the front of the queue.
-   **Cancellation**: `cancel()`: Removes a coalesced task by its key.
-   **Queue Management**:
    -   `empty()`: Checks if the queue is empty.
    -   `size()`: Returns the number of tasks in the queue.
    -   `clear()`: Removes all tasks without execution.
    -   `max_size()`: Returns the maximum capacity of the queue (0 for unlimited). Tasks cannot be added if the queue is at maximum capacity.

### Basic Usage

```cpp
#include "call_queue.h"
#include <iostream>

int main() {
    callqueue::CallQueue queue;

    // Push tasks
    queue.push([]() { std::cout << "Task 1 executed." << std::endl; });
    queue.push([]() { std::cout << "Task 2 executed." << std::endl; });

    std::cout << "Queue size: " << queue.size() << std::endl; // Output: 2
    queue.drain_all(); // Executes Task 1, then Task 2

    // Coalesce tasks
    queue.coalesce("config_update", []() { std::cout << "Config update v1." << std::endl; });
    queue.coalesce("config_update", []() { std::cout << "Config update v2 (latest)." << std::endl; }); // Replaces v1

    queue.drain_all(); // Executes "Config update v2 (latest)."

    return 0;
}
```

### Reentrancy

If a task executed via `drain_all()` adds new tasks to the queue, these new tasks are not executed within the current `drain_all()` cycle. They are queued and will be processed by a subsequent `drain_all()` or `drain_one()` call.

```cpp
callqueue::CallQueue queue;
queue.push([&queue]() {
    std::cout << "Outer task executing." << std::endl;
    queue.push([]() { std::cout << "Inner task (added by outer) executing." << std::endl; });
});

std::cout << "First drain:" << std::endl;
queue.drain_all(); // Executes "Outer task". "Inner task" is added.

std::cout << "Second drain:" << std::endl;
queue.drain_all(); // Executes "Inner task".
```

## `ThreadSafeCallQueue`

`ThreadSafeCallQueue` is a wrapper around `CallQueue` that provides thread-safe access to all its operations using an internal mutex. This allows tasks to be pushed, coalesced, drained, and managed from multiple threads concurrently.

### Features

It mirrors all functionalities of `CallQueue` (`push`, `coalesce`, `drain_all`, `drain_one`, `cancel`, `empty`, `size`, `clear`, `max_size`) but with added thread safety.

The `drain_all()` method is optimized to minimize lock contention by swapping the internal queue with a local one under lock, and then executing the tasks from the local queue outside the lock.

### Basic Usage

```cpp
#include "call_queue.h"
#include <iostream>
#include <thread>
#include <vector>

int main() {
    callqueue::ThreadSafeCallQueue safe_queue;
    std::vector<std::thread> threads;
    int task_count = 0;

    // Producer threads
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&safe_queue, &task_count, i]() {
            for (int j = 0; j < 10; ++j) {
                safe_queue.push([i, j, &task_count]() {
                    // Note: std::cout itself is not always thread-safe without external sync
                    // For simplicity, we assume it works for this example or use a thread-safe logger
                    printf("Task from thread %d, job %d executed. Total: %d\n", i, j, ++task_count);
                });
            }
        });
    }

    // Give some time for producers to push tasks
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Consumer (can be in a separate thread or the main thread)
    std::cout << "Draining tasks. Queue size: " << safe_queue.size() << std::endl;
    safe_queue.drain_all();

    for (auto& t : threads) {
        t.join();
    }
    
    // Final drain for any tasks pushed after the first drain_all started but before threads finished
    std::cout << "Final drain. Queue size: " << safe_queue.size() << std::endl;
    safe_queue.drain_all();

    std::cout << "All tasks completed. Total executed: " << task_count << std::endl;

    return 0;
}
```

## Use Cases

-   **Event Loops**: Processing events or messages sequentially.
-   **UI Updates**: Batching UI redraws or updates to be performed on a main UI thread.
-   **Deferred Operations**: Scheduling cleanup tasks or side effects after a primary operation.
-   **State Machines**: Executing actions associated with state transitions.
-   **Resource Management**: Serializing access to a shared resource in a multi-threaded application.
-   **Network Control Planes**: Applying network configuration changes in a specific order.
```

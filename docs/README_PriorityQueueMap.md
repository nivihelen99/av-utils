# PriorityQueueMap (Indexed Priority Queue)

## Overview

The `PriorityQueueMap` is a C++ data structure that combines the features of a priority queue with a map. It allows for efficient retrieval of the element with the highest (or lowest) priority, like a standard priority queue, but also provides efficient mechanisms to update the priority of any element or remove any element by its key. This is often referred to as an Indexed Priority Queue or Updatable Priority Queue.

This implementation is templated, allowing users to specify the types for keys, values, priorities, and a custom comparison function for priorities. By default, it behaves as a min-priority queue (smallest priority value is considered highest priority).

## Template Parameters

```cpp
template <
    typename KeyType,     // Type of the keys used to identify elements
    typename ValueType,   // Type of the values associated with keys
    typename PriorityType, // Type of the priorities
    typename Compare = std::greater<PriorityType> // Comparator for priorities.
                                                // std::greater for min-heap (default)
                                                // std::less for max-heap
>
class PriorityQueueMap;
```

-   `KeyType`: The type of the unique keys that identify items in the queue. Must be hashable for use with `std::unordered_map`.
-   `ValueType`: The type of the values stored alongside the keys.
-   `PriorityType`: The type used for priorities. Must be comparable by the `Compare` functor.
-   `Compare`: A comparison functor that determines the order of priorities.
    -   Defaults to `std::greater<PriorityType>`, which results in a **min-priority queue** (the element with the *smallest* priority value is at the top).
    -   To create a **max-priority queue** (element with the *largest* priority value at the top), use `std::less<PriorityType>`.

## Core Features

-   **Efficient Priority Updates:** Change the priority of an existing element in O(log N) time.
-   **Efficient Key-Based Removal:** Remove any element by its key in O(log N) time.
-   **Efficient Top Element Access:** Get the element with the highest (or lowest) priority in O(1) time.
-   **Efficient Pop:** Remove the element with the highest (or lowest) priority in O(log N) time.
-   **Key Existence Check:** Check if a key is present in the queue in O(1) average time.
-   **Value Access:** Retrieve the value associated with a key in O(1) average time.

## Time Complexity

-   `push(key, value, priority)`: O(log N) if key is new. If key exists, it's an update, also O(log N).
-   `pop()`: O(log N)
-   `top_key()`, `top_priority()`: O(1)
-   `update_priority(key, new_priority)`: O(log N)
-   `remove(key)`: O(log N)
-   `contains(key)`: O(1) on average (due to `std::unordered_map` lookup)
-   `get_value(key)`: O(1) on average
-   `empty()`, `size()`: O(1)

N is the number of elements in the `PriorityQueueMap`.

## Usage Example

```cpp
#include "PriorityQueueMap.h"
#include <iostream>
#include <string>

int main() {
    // Min-priority queue (default: smallest priority value is top)
    cpp_utils::PriorityQueueMap<int, std::string, int> tasks;

    tasks.push(1, "Write report", 20);
    tasks.push(2, "Fix bug #101", 10); // Higher priority (lower value)
    tasks.push(3, "Attend meeting", 15);

    std::cout << "Top task: Key=" << tasks.top_key() << ", Prio=" << tasks.top_priority()
              << ", Val=" << tasks.get_value(tasks.top_key()) << std::endl; // Expected: Key=2, Prio=10

    tasks.update_priority(1, 5); // Update report priority to be highest
    std::cout << "After update, top task: Key=" << tasks.top_key()
              << ", Prio=" << tasks.top_priority() << std::endl; // Expected: Key=1, Prio=5

    tasks.remove(3); // Remove meeting task

    std::cout << "Tasks remaining: " << tasks.size() << std::endl;

    while (!tasks.empty()) {
        int key = tasks.top_key();
        std::string val = tasks.get_value(key);
        int prio = tasks.pop(); // pop() returns priority
        std::cout << "Processing: Key=" << key << ", Val=" << val << ", Prio=" << prio << std::endl;
    }

    // Max-priority queue (largest priority value is top)
    cpp_utils::PriorityQueueMap<std::string, std::string, double, std::less<double>> alerts;
    alerts.push("CPU_Usage", "Server A CPU high", 0.95); // Priority 0.95
    alerts.push("Mem_Usage", "Server B Mem high", 0.80); // Priority 0.80
    alerts.push("Disk_Space", "Server A Disk low", 0.98); // Priority 0.98 (highest)

    std::cout << "\nTop alert: " << alerts.top_key()
              << " with priority " << alerts.top_priority() << std::endl; // Expected: Disk_Space, 0.98

    return 0;
}
```

## When to Use

-   Algorithms where priorities of elements change dynamically (e.g., Dijkstra's algorithm with edge relaxations, A* search).
-   Event-driven simulations where event times (priorities) can be rescheduled.
-   Task schedulers where task priorities need to be adjusted or tasks can be cancelled.
-   Any scenario requiring a priority queue with efficient random access, update, and removal capabilities.

## Internal Implementation

The `PriorityQueueMap` is typically implemented using:
1.  A `std::vector` managed as a binary heap to store pairs of `(PriorityType, KeyType)`. This allows for efficient O(log N) heap operations.
2.  An `std::unordered_map<KeyType, ValueType>` to store the actual values associated with the keys. This gives O(1) average time access to values.
3.  An `std::unordered_map<KeyType, size_t>` to map keys to their current index within the heap vector. This is crucial for enabling O(log N) updates and removals by key, as it allows direct access to the element's position in the heap.

When an element's priority is updated or an element is removed, its position in the heap is found using the `key_to_heap_index_` map, the operation is performed, and the heap property is restored by sifting the affected element(s) up or down.
```

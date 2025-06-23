# `HeapQueue<T, KeyFn, Compare>`

## Overview

The `HeapQueue<T, ...>` class (`heap_queue.h`) implements a priority queue data structure using a binary heap. It allows for efficient insertion of elements and retrieval/removal of the element with the highest priority.

The priority of elements is determined by:
1.  A **key function (`KeyFn`)**: Extracts a key from an element of type `T`.
2.  A **comparison functor (`Compare`)**: Compares these extracted keys to establish an ordering.

By default, `HeapQueue` is configured as a **min-heap**, meaning that the element whose key is considered the "smallest" by the `Compare` functor (defaulting to `std::less`) has the highest priority and will be at the `top()` of the queue.

## Template Parameters

-   `T`: The type of elements stored in the heap queue.
-   `KeyFn`: A callable type that takes an element `const T&` and returns its key.
    -   Defaults to `std::function<T(const T&)>` which effectively means `[](const T& val){ return val; }`. This implies `T` itself is used as the key if `KeyFn` is not specified.
    -   The result type of `KeyFn` is what `Compare` operates on.
-   `Compare`: A binary predicate that defines the strict weak ordering for keys.
    -   Defaults to `std::less<std::invoke_result_t<KeyFn, const T&>>`.
    -   When `Compare` is `std::less`, the `HeapQueue` behaves as a min-heap (smallest key has highest priority). To achieve max-heap behavior (largest key has highest priority), you can provide `std::greater` as the `Compare` type.

## Features

-   **Priority Queue Semantics:** Efficiently manages elements based on priority.
-   **Customizable Priority:** Allows user-defined key extraction and comparison logic.
-   **Standard Heap Operations:** `push`, `pop`, `top`.
-   **Efficient `update_top`:** Allows replacing the top element and re-heapifying in O(log N).
-   **`heapify`:** Build a heap from an existing collection of items.
-   **STL Algorithm Backend:** Uses `std::push_heap`, `std::pop_heap`, `std::make_heap` internally.

## Public Interface Highlights

### Constructors
-   **`HeapQueue()`**: Default constructor. Assumes `T` is its own key and uses `std::less` for comparison (min-heap).
-   **`HeapQueue(KeyFn key_fn, Compare compare_fn)`**: Custom key extraction function and custom comparison.
-   **`HeapQueue(KeyFn key_fn)`**: Custom key extraction, default comparison (`std::less` on keys, min-heap).
-   **`HeapQueue(Compare compare_fn)`**: Default key extraction (element is key), custom comparison.

### Core Operations
-   **`void push(const T& value)` / `void push(T&& value)`**: Adds `value` to the heap. (O(log N))
-   **`T pop()`**: Removes and returns the highest-priority element (e.g., smallest for min-heap). Throws `std::out_of_range` if empty. (O(log N))
-   **`const T& top() const`**: Returns a const reference to the highest-priority element. Throws `std::out_of_range` if empty. (O(1))
-   **`T update_top(const T& item)`**: Replaces the top element with `item`, then re-heapifies. Returns the old top element. Throws `std::out_of_range` if empty. (O(log N))

### State & Utility
-   **`bool empty() const`**: Checks if the queue is empty.
-   **`size_t size() const`**: Returns the number of elements.
-   **`void clear()`**: Removes all elements.
-   **`const std::vector<T>& as_vector() const`**: Returns a const reference to the underlying vector (heap representation, not sorted).
-   **`void heapify(const std::vector<T>& items)` / `void heapify(std::vector<T>&& items)`**: Replaces current content and builds a heap from `items`. (O(N))

## Usage Examples

### Basic Min-Heap (Default Behavior)

```cpp
#include "heap_queue.h" // Assuming heap_queue.h is in the include path
#include <iostream>
#include <vector>

int main() {
    HeapQueue<int> min_heap; // Default: KeyFn is identity, Compare is std::less -> min-heap

    min_heap.push(30);
    min_heap.push(10);
    min_heap.push(20);
    min_heap.push(5);

    std::cout << "Min-Heap (top is smallest):" << std::endl;
    while (!min_heap.empty()) {
        std::cout << "Top: " << min_heap.top() << ", Popping: " << min_heap.pop() << std::endl;
    }
    // Expected Output:
    // Top: 5, Popping: 5
    // Top: 10, Popping: 10
    // Top: 20, Popping: 20
    // Top: 30, Popping: 30
}
```

### Max-Heap using Custom Comparator

```cpp
#include "heap_queue.h"
#include <iostream>
#include <functional> // For std::greater

int main() {
    // KeyFn is still identity, Compare is std::greater -> max-heap
    HeapQueue<int, std::function<int(const int&)>, std::greater<int>> max_heap;

    max_heap.push(30);
    max_heap.push(10);
    max_heap.push(20);
    max_heap.push(50);

    std::cout << "Max-Heap (top is largest):" << std::endl;
    while (!max_heap.empty()) {
        std::cout << "Top: " << max_heap.top() << ", Popping: " << max_heap.pop() << std::endl;
    }
    // Expected Output:
    // Top: 50, Popping: 50
    // Top: 30, Popping: 30
    // Top: 20, Popping: 20
    // Top: 10, Popping: 10
}
```

### Heap with Custom Objects and Key Function

```cpp
#include "heap_queue.h"
#include <iostream>
#include <string>
#include <vector>

struct Task {
    int priority;
    std::string name;

    // For printing
    friend std::ostream& operator<<(std::ostream& os, const Task& t) {
        os << "Task(name: \"" << t.name << "\", prio: " << t.priority << ")";
        return os;
    }
};

int main() {
    // Key function extracts priority
    auto get_priority = [](const Task& t) { return t.priority; };

    // Default std::less on priority means min-heap by priority
    HeapQueue<Task, decltype(get_priority)> task_queue(get_priority);

    task_queue.push({2, "Low Prio Task"});
    task_queue.push({0, "High Prio Task"}); // Lower number = higher priority for min-heap
    task_queue.push({1, "Medium Prio Task"});

    std::cout << "Task Queue (min-heap by priority):" << std::endl;
    while (!task_queue.empty()) {
        std::cout << "Processing: " << task_queue.pop() << std::endl;
    }
    // Expected Output:
    // Processing: Task(name: "High Prio Task", prio: 0)
    // Processing: Task(name: "Medium Prio Task", prio: 1)
    // Processing: Task(name: "Low Prio Task", prio: 2)
}
```

### `heapify` and `update_top`

```cpp
#include "heap_queue.h"
#include <iostream>
#include <vector>

int main() {
    std::vector<int> data = {40, 100, 5, 25};
    HeapQueue<int> heap;
    heap.heapify(data); // Builds a min-heap from data

    std::cout << "Heap after heapify({40, 100, 5, 25}):" << std::endl;
    std::cout << "Top: " << heap.top() << std::endl; // Expected: 5

    Task old_top_task = heap.update_top(15); // Replaces 5 with 15, re-heapifies
    std::cout << "Old top was: " << old_top_task << std::endl; // Expected: 5
    std::cout << "New top after update_top(15): " << heap.top() << std::endl; // Expected: 15 (or 25 if 15 causes 25 to be smaller)
                                                                          // Actually, 15 is smaller than 25, 40, 100. So 15.

    std::cout << "Final heap contents (order in underlying vector is heap order): ";
    for(int val : heap.as_vector()) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
}
```

## Dependencies
- `<vector>`
- `<functional>` (for `std::function`, `std::less`, `std::greater`)
- `<algorithm>` (for `std::make_heap`, `std::push_heap`, `std::pop_heap`)
- `<utility>` (for `std::move`)
- `<stdexcept>` (for `std::out_of_range`)
- `<type_traits>` (for `std::invoke_result_t`)

This `HeapQueue` provides a flexible and efficient priority queue implementation suitable for various applications requiring ordered element processing.

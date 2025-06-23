# `RoundRobinQueue<T>`

## Overview

The `RoundRobinQueue<T>` class (`round_robin_queue.h`) implements a queue that provides round-robin (circular) access to its elements. When an element is retrieved using `next()`, an internal pointer advances to the subsequent element, wrapping around to the beginning when it reaches the end of the queue.

This data structure is particularly useful for:
-   **Load Balancing:** Distributing tasks or requests evenly among a set of workers or servers.
-   **Fair Scheduling:** Giving each element a turn in a repeated cycle.
-   **Cycling through Resources:** Iterating through a collection of items in a circular fashion.

It is built on top of `std::deque` for its underlying storage.

## Template Parameter
-   `T`: The type of elements stored in the queue.

## Features

-   **Round-Robin Access:** The `next()` method retrieves the current element and advances the pointer for the next call, cycling through elements.
-   **FIFO Enqueue:** New elements are typically added to the back using `enqueue()`.
-   **Priority Enqueue:** `insert_front()` allows adding elements to the front, effectively giving them earlier access in the round-robin sequence.
-   **Element Manipulation:** Supports peeking at the current element (`peek()`), skipping/removing the current element (`skip()`), removing a specific value (`remove()`), and clearing the queue.
-   **State Control:** `reset()` to move the pointer to the beginning, `rotate()` to shift the current pointer.
-   **STL-like Interface:** Provides `empty()`, `size()`, and iterators (`begin`, `end`) to the underlying deque storage (note: these iterators reflect storage order, not necessarily the round-robin access order from the current pointer).
-   **Inspection:** `contains()` to check for an element, `current_position()` to get the current index, `for_each()` to visit elements in round-robin order from the current position.

## Public Interface Highlights

### Constructors
-   **`RoundRobinQueue()`**: Default constructor.
-   **`template<typename InputIt> RoundRobinQueue(InputIt first, InputIt last)`**: Constructs from an iterator range.

### Modifiers
-   **`void enqueue(const T& item)` / `void enqueue(T&& item)`**: Adds an item to the back.
-   **`void insert_front(const T& item)` / `void insert_front(T&& item)`**: Adds an item to the front; adjusts the current pointer.
-   **`T& next()`**: Returns a reference to the current element and advances the round-robin pointer. Throws `std::runtime_error` if empty.
-   **`void skip()`**: Removes the element currently pointed to by the round-robin mechanism. Adjusts pointer. Throws if empty.
-   **`bool remove(const T& value)`**: Removes the first occurrence of `value`. Adjusts pointer.
-   **`void clear()`**: Removes all elements.
-   **`void reset()`**: Resets the round-robin pointer to the beginning.
-   **`void rotate(int n)`**: Rotates the current pointer `n` positions (positive for forward, negative for backward).

### Accessors & Status
-   **`T& peek()` / `const T& peek() const`**: Returns a reference to the current element without advancing the pointer. Throws `std::runtime_error` if empty.
-   **`bool empty() const`**: Checks if the queue is empty.
-   **`size_t size() const`**: Returns the number of elements.
-   **`bool contains(const T& value) const`**: Checks if the queue contains `value`.
-   **`size_t current_position() const`**: Returns the index of the current round-robin element.
-   **`template<typename Callback> void for_each(Callback callback) const`**: Calls `callback` for each element in round-robin order, starting from the current element, without modifying the main round-robin pointer.

### Iterators (to underlying `std::deque`)
-   **`iterator begin() / end()`**
-   **`const_iterator cbegin() / cend()`**

## Usage Examples

(Based on `examples/round_robin_queue_example.cpp`)

### Basic Round-Robin Task Scheduling

```cpp
#include "round_robin_queue.h"
#include <iostream>
#include <string>
#include <vector>

int main() {
    RoundRobinQueue<std::string> task_queue;
    task_queue.enqueue("Task A");
    task_queue.enqueue("Task B");
    task_queue.enqueue("Task C");

    std::cout << "Processing tasks in round-robin order for 7 cycles:" << std::endl;
    for (int i = 0; i < 7; ++i) {
        if (task_queue.empty()) break;
        std::cout << "Cycle " << (i + 1) << ": Assigning '" << task_queue.next() << "'" << std::endl;
    }
    // Expected Output:
    // Cycle 1: Assigning 'Task A'
    // Cycle 2: Assigning 'Task B'
    // Cycle 3: Assigning 'Task C'
    // Cycle 4: Assigning 'Task A'
    // Cycle 5: Assigning 'Task B'
    // Cycle 6: Assigning 'Task C'
    // Cycle 7: Assigning 'Task A'
}
```

### Load Balancing Simulation

```cpp
#include "round_robin_queue.h"
#include <iostream>
#include <string>

struct Server {
    std::string name;
    int requests_handled = 0;
    Server(std::string n) : name(std::move(n)) {}
};

int main() {
    RoundRobinQueue<Server> server_pool;
    server_pool.enqueue({"Server1"});
    server_pool.enqueue({"Server2"});
    server_pool.enqueue({"Server3"});

    std::cout << "\nSimulating 10 client requests to servers:" << std::endl;
    for (int i = 0; i < 10; ++i) {
        Server& assigned_server = server_pool.next();
        assigned_server.requests_handled++;
        std::cout << "Request #" << (i + 1) << " routed to " << assigned_server.name
                  << " (total for server: " << assigned_server.requests_handled << ")" << std::endl;
    }

    std::cout << "\nFinal request counts per server:" << std::endl;
    server_pool.for_each([](const Server& s){ // for_each starts from current round-robin pos
        std::cout << "  " << s.name << ": " << s.requests_handled << std::endl;
    });
}
```

### Using `peek`, `skip`, `rotate`, `remove`

```cpp
#include "round_robin_queue.h"
#include <iostream>

int main() {
    RoundRobinQueue<int> numbers;
    for(int i=1; i<=5; ++i) numbers.enqueue(i*10); // 10 20 30 40 50

    std::cout << "Initial queue. Current (peek): " << numbers.peek() << std::endl; // 10

    numbers.skip(); // Removes 10. Current points to 20. Queue: [20 30 40 50]
    std::cout << "After skip. Next element: " << numbers.next() << std::endl; // 20 (and current moves to 30)
    std::cout << "Queue size: " << numbers.size() << std::endl; // 4. Current points to 30.

    // Queue: [30 40 50]. Current element is 30.
    // Let's print using for_each from current
    std::cout << "Queue state (current=" << numbers.peek() << "): ";
    numbers.for_each([](int n){ std::cout << n << " ";}); // 30 40 50
    std::cout << std::endl;

    numbers.rotate(1); // Current was 30, now 40.
    std::cout << "After rotate(1). Current (peek): " << numbers.peek() << std::endl; // 40

    numbers.remove(30); // Removes 30. Queue: [40 50]. Current was 40. 30 was "before" it logically.
                        // Current pointer might adjust. If it was 40, it should remain 40.
    std::cout << "After remove(30). Current (peek): " << numbers.peek() << std::endl; // 40
    std::cout << "Contains 30? " << std::boolalpha << numbers.contains(30) << std::endl; // false
    std::cout << "Queue size: " << numbers.size() << std::endl; // 2
}
```

## Dependencies
- `<deque>`
- `<stdexcept>`
- `<functional>` (not directly, but often used with `for_each` callbacks)
- `<algorithm>` (for `std::find`)
- `<iterator>` (for `std::distance`)

The `RoundRobinQueue` provides a convenient and efficient way to manage collections where circular access or fair distribution is needed.

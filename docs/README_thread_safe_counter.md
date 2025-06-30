# ThreadSafeCounter

## Overview

`ThreadSafeCounter<T, Hash, KeyEqual>` is a C++ template class that provides a thread-safe frequency counter, similar in functionality to Python's `collections.Counter`. It is designed for scenarios where multiple threads need to concurrently increment, decrement, or query counts of items.

The class uses `std::unordered_map` as its underlying storage and protects all access to this map with a `std::mutex` to ensure thread safety.

## Template Parameters

-   `T`: The type of elements to be counted. This type must be hashable and equality comparable.
-   `Hash`: The hash function to use for elements of type `T`. Defaults to `std::hash<T>`.
-   `KeyEqual`: The equality comparison function for elements of type `T`. Defaults to `std::equal_to<T>`.

## Features

-   **Thread Safety**: All public methods that access or modify the counter's state are protected by a mutex.
-   **Rich API**: Offers a comprehensive set of methods for managing and querying counts.
    -   Adding/subtracting counts: `add()`, `subtract()`.
    -   Setting specific counts: `set_count()`.
    -   Querying counts: `count()`, `operator[]` (const).
    -   Checking for item presence: `contains()`.
    -   Removing items: `erase()`.
    -   Utility functions: `clear()`, `size()`, `empty()`, `total()`.
    -   Retrieving most common items: `most_common()`.
    -   Copying internal data: `get_data_copy()`.
-   **Arithmetic Operations**: Supports counter arithmetic (`+`, `-`, `+=`, `-=`) in a thread-safe manner.
-   **Set Operations**: Supports intersection (`intersection()`) and union (`union_with()`) operations, returning new counters.
-   **STL-like Interface**: Where appropriate, methods mimic standard C++ container interfaces.
-   **Construction Flexibility**: Can be constructed from initializer lists (of items or item-count pairs) or iterator ranges.
-   **Deduction Guides**: Class template argument deduction (CTAD) is supported for easier instantiation.

## Usage

### Basic Operations

```cpp
#include "thread_safe_counter.hpp"
#include <iostream>
#include <string>

int main() {
    ThreadSafeCounter<std::string> counter;

    // Add items
    counter.add("apple", 3);
    counter.add("banana"); // Adds 1 to banana
    counter.add("apple", 2); // Now apple is 5

    // Get counts
    std::cout << "Count of apple: " << counter.count("apple") << std::endl;   // Output: 5
    std::cout << "Count of banana: " << counter["banana"] << std::endl; // Output: 1
    std::cout << "Count of orange: " << counter.count("orange") << std::endl; // Output: 0

    // Subtract items
    counter.subtract("apple", 2); // apple is now 3
    std::cout << "Count of apple after subtract: " << counter.count("apple") << std::endl; // Output: 3

    // Set specific count
    counter.set_count("banana", 10);
    std::cout << "Count of banana after set_count: " << counter.count("banana") << std::endl; // Output: 10
    counter.set_count("apple", 0); // Setting count to 0 removes the item
    std::cout << "Contains apple after set_count to 0: " << counter.contains("apple") << std::endl; // Output: 0 (false)


    // Total items
    std::cout << "Total items: " << counter.total() << std::endl; // e.g., 10 (if only banana:10 exists)

    // Most common
    counter.add("grape", 5);
    counter.add("orange", 8);
    auto common = counter.most_common(2); // Get top 2
    for (const auto& p : common) {
        std::cout << p.first << ": " << p.second << std::endl;
    }
    // Example Output:
    // banana: 10
    // orange: 8
}

```

### Multithreaded Usage

`ThreadSafeCounter` ensures that operations from different threads are correctly synchronized.

```cpp
#include "thread_safe_counter.hpp"
#include <iostream>
#include <vector>
#include <thread>
#include <string>

void increment_task(ThreadSafeCounter<std::string>& counter, const std::string& item, int times) {
    for (int i = 0; i < times; ++i) {
        counter.add(item);
    }
}

int main() {
    ThreadSafeCounter<std::string> concurrent_counter;
    std::vector<std::thread> threads;
    int num_threads = 10;
    int ops_per_thread = 1000;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(increment_task, std::ref(concurrent_counter), "event_type_A", ops_per_thread);
        threads.emplace_back(increment_task, std::ref(concurrent_counter), "event_type_B", ops_per_thread / 2);
    }

    for (std::thread& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    std::cout << "Concurrent operations result:" << std::endl;
    std::cout << "event_type_A: " << concurrent_counter.count("event_type_A") << std::endl;
    // Expected: num_threads * ops_per_thread
    std::cout << "event_type_B: " << concurrent_counter.count("event_type_B") << std::endl;
    // Expected: num_threads * (ops_per_thread / 2)

    return 0;
}
```

## Method Reference

(A selection of key methods)

-   `ThreadSafeCounter()`: Default constructor.
-   `ThreadSafeCounter(std::initializer_list<T> init)`: Constructs from an initializer list of items.
-   `ThreadSafeCounter(std::initializer_list<std::pair<T, int>> init)`: Constructs from an initializer list of item-count pairs.
-   `void add(const T& value, int count_val = 1)`: Increments the count of `value` by `count_val`. If `count_val` is negative, it behaves like `subtract`.
-   `void subtract(const T& value, int count_val = 1)`: Decrements the count of `value` by `count_val`. If the count becomes zero or less, the item is removed from the counter.
-   `void set_count(const T& key, int val)`: Sets the count of `key` to `val`. If `val` is zero or less, the item is removed.
-   `[[nodiscard]] int count(const T& value) const`: Returns the count of `value`. Returns 0 if `value` is not in the counter.
-   `[[nodiscard]] int operator[](const T& value) const`: Equivalent to `count(value)`.
-   `[[nodiscard]] bool contains(const T& value) const`: Checks if `value` is present in the counter (i.e., its count > 0).
-   `size_type erase(const T& value)`: Removes `value` from the counter. Returns 1 if an element was removed, 0 otherwise.
-   `void clear()`: Removes all items from the counter.
-   `[[nodiscard]] size_type size() const`: Returns the number of unique items in the counter.
-   `[[nodiscard]] bool empty() const`: Checks if the counter is empty.
-   `[[nodiscard]] int total() const`: Returns the sum of all counts in the counter.
-   `[[nodiscard]] std::vector<std::pair<T, int>> most_common(size_type n = 0) const`: Returns a vector of the `n` most common items and their counts. If `n` is 0, all items are returned, sorted by frequency (highest first), with ties broken by key comparison (if `T` is comparable).
-   `[[nodiscard]] std::unordered_map<T, int, Hash, KeyEqual> get_data_copy() const`: Returns a copy of the underlying `unordered_map`.
-   `ThreadSafeCounter& operator+=(const ThreadSafeCounter& other)`: Adds counts from `other` counter into `this` counter.
-   `ThreadSafeCounter& operator-=(const ThreadSafeCounter& other)`: Subtracts counts from `other` counter from `this` counter. Items whose counts become non-positive are removed.
-   `[[nodiscard]] ThreadSafeCounter operator+(const ThreadSafeCounter& other) const`: Returns a new counter with combined counts.
-   `[[nodiscard]] ThreadSafeCounter operator-(const ThreadSafeCounter& other) const`: Returns a new counter with subtracted counts. Resulting counts can be negative (items are not removed if count becomes non-positive in this specific operation, matching Python's Counter behavior for `-`).
-   `[[nodiscard]] ThreadSafeCounter intersection(const ThreadSafeCounter& other) const`: Returns a new counter with counts being the minimum of counts from `this` and `other` for common keys.
-   `[[nodiscard]] ThreadSafeCounter union_with(const ThreadSafeCounter& other) const`: Returns a new counter with counts being the maximum of counts from `this` and `other` for all keys present in either.

## Thread Safety Considerations

-   All methods are internally synchronized using `std::mutex`.
-   Operations like `most_common()` and `get_data_copy()` return copies of data, so the returned data is not affected by subsequent modifications to the original counter.
-   Iterators are not directly provided for the underlying map to avoid complexities with concurrent modification. Instead, methods like `most_common()` or `get_data_copy()` can be used to get a snapshot of the data for iteration.
-   Copy/Move constructors and assignment operators are implemented with appropriate locking to ensure thread safety during these operations, including deadlock avoidance for self-assignment and assignment between two different counters.

This `ThreadSafeCounter` aims to be a robust and easy-to-use component for multithreaded C++ applications requiring frequency counting.

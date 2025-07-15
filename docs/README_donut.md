# Donut

## Overview

The `Donut` data structure is a fixed-size, circular buffer that overwrites its oldest elements when it becomes full. It's similar to a ring buffer but with a key feature: it provides a view of the elements in insertion order, even when the buffer has wrapped around. This makes it particularly useful for scenarios where you need to maintain a history of the most recent N items, such as logging, event tracking, or caching the latest entries.

## Template Parameters

-   `T`: The type of element to be stored.

## Public Interface

### `Donut(size_t capacity)`

Constructs a `Donut` with the specified capacity.

-   `capacity`: The maximum number of elements the `Donut` can hold.

### `void push(T item)`

Adds an element to the `Donut`. If the `Donut` is full, the oldest element is overwritten.

-   `item`: The element to be added.

### `const T& operator[](size_t index) const`

Accesses the element at the specified index. The index is relative to the insertion order.

-   `index`: The index of the element to access.
-   **Returns**: A const reference to the element.

### `T& operator[](size_t index)`

Accesses the element at the specified index. The index is relative to the insertion order.

-   `index`: The index of the element to access.
-   **Returns**: a reference to the element.

### `size_t size() const`

-   **Returns**: The current number of elements in the `Donut`.

### `size_t capacity() const`

-   **Returns**: The maximum number of elements the `Donut` can hold.

### `begin()` and `end()`

The `Donut` provides `begin()` and `end()` methods that return iterators for traversing the elements in insertion order.

## Example Usage

```cpp
#include "donut.h"
#include <iostream>

int main() {
    // Create a Donut with a capacity of 5
    cpp_utils::Donut<int> donut(5);

    // Add some elements
    donut.push(1);
    donut.push(2);
    donut.push(3);
    donut.push(4);
    donut.push(5);

    std::cout << "Donut elements: ";
    for (int i : donut) {
        std::cout << i << " ";
    }
    std::cout << std::endl; // Output: Donut elements: 1 2 3 4 5

    // Add more elements to overflow the capacity
    donut.push(6);
    donut.push(7);

    std::cout << "Donut elements after overflow: ";
    for (int i : donut) {
        std::cout << i << " ";
    }
    std::cout << std::endl; // Output: Donut elements after overflow: 3 4 5 6 7

    // Access elements by index
    std::cout << "Element at index 0: " << donut[0] << std::endl; // Output: Element at index 0: 3
    std::cout << "Element at index 2: " << donut[2] << std::endl; // Output: Element at index 2: 5

    return 0;
}
```

# CircularBuffer

`CircularBuffer` is a container that supports adding and removing elements from both ends. It has a fixed capacity, and when it's full, adding a new element will overwrite the oldest element. It is implemented as a circular array, which allows for efficient rotation of its elements.

This data structure is useful for a variety of purposes, including:
- Caching a fixed number of items.
- Implementing a fixed-size log or history.
- Buffering data for I/O operations.
- Any situation where you need a queue-like container with a fixed capacity and efficient rotation.

## Template Parameters

```cpp
template<
    typename T,
    typename Allocator = std::allocator<T>
>
class CircularBuffer;
```

- `T`: The type of the elements.
- `Allocator`: The allocator to use for all memory allocations of this container.

## Member Functions

| Function | Description | Time Complexity |
|----------|-------------|-----------------|
| `(constructor)` | Constructs the `CircularBuffer` with a given capacity. | O(N) |
| `size` | Returns the number of elements in the buffer. | O(1) |
| `capacity` | Returns the maximum number of elements the buffer can hold. | O(1) |
| `empty` | Checks if the buffer is empty. | O(1) |
| `full` | Checks if the buffer is full. | O(1) |
| `push_back` | Adds an element to the back of the buffer. If the buffer is full, the oldest element is overwritten. | O(1) |
| `push_front` | Adds an element to the front of the buffer. If the buffer is full, the oldest element is overwritten. | O(1) |
| `pop_back` | Removes the last element from the buffer. | O(1) |
| `pop_front` | Removes the first element from the buffer. | O(1) |
| `front` | Returns a reference to the first element in the buffer. | O(1) |
| `back` | Returns a reference to the last element in the buffer. | O(1) |
| `operator[]` | Returns a reference to the element at the specified index. | O(1) |
| `rotate` | Rotates the elements in the buffer by a given amount. | O(1) |
| `clear` | Clears the contents of the buffer. | O(1) |
| `begin`, `end`, `cbegin`, `cend` | Returns an iterator to the beginning or end of the buffer. | O(1) |
| `rbegin`, `rend`, `crbegin`, `crend` | Returns a reverse iterator to the beginning or end of the buffer. | O(1) |

## Usage Example

```cpp
#include "circular_buffer.h"
#include <iostream>

int main() {
    CircularBuffer<int> buffer(5);

    for (int i = 1; i <= 5; ++i) {
        buffer.push_back(i);
    }

    std::cout << "Initial buffer: ";
    for (int x : buffer) {
        std::cout << x << " ";
    }
    std::cout << std::endl; // Output: 1 2 3 4 5

    buffer.rotate(2);

    std::cout << "Buffer after rotating by 2: ";
    for (int x : buffer) {
        std::cout << x << " ";
    }
    std::cout << std::endl; // Output: 4 5 1 2 3

    buffer.push_back(6); // Overwrites 1

    std::cout << "Buffer after pushing 6: ";
    for (int x : buffer) {
        std::cout << x << " ";
    }
    std::cout << std::endl; // Output: 5 1 2 3 6

    return 0;
}
```

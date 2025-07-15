# BitQueue

## Overview

`BitQueue` is a C++ data structure that provides a queue for individual bits. It's useful for applications that require bit-level manipulation, such as custom serialization formats, network protocols, or compression algorithms.

`BitQueue` is a header-only library, so you can simply include `bit_queue.h` in your project.

## Features

- Enqueue and dequeue single bits.
- Enqueue and dequeue sequences of bits up to 64 bits at a time.
- Peek at the next bit without removing it.
- Check the number of bits in the queue.
- Clear the queue.

## Usage

Here's a simple example of how to use `BitQueue`:

```cpp
#include "bit_queue.h"
#include <iostream>

int main() {
    cpp_utils::BitQueue bq;

    // Enqueue some bits
    bq.push(true);
    bq.push(false);
    bq.push(true);
    bq.push(true);

    std::cout << "Initial size: " << bq.size() << std::endl;

    // Dequeue and print the bits
    std::cout << "Bits: ";
    while (!bq.empty()) {
        std::cout << bq.pop() << " ";
    }
    std::cout << std::endl;
    std::cout << "Final size: " << bq.size() << std::endl;

    // Enqueue a 10-bit value
    bq.push(0x2A, 10); // 0b0000101010
    std::cout << "\nEnqueued 10 bits (0x2A). Size: " << bq.size() << std::endl;

    // Dequeue the 10-bit value
    uint64_t val = bq.pop(10);
    std::cout << "Dequeued 10 bits: " << std::hex << "0x" << val << std::dec << std::endl;
    std::cout << "Final size: " << bq.size() << std::endl;

    return 0;
}
```

## API Reference

### `BitQueue()`

Default constructor.

### `void push(bool bit)`

Enqueues a single bit.

### `void push(uint64_t value, uint8_t count)`

Enqueues the `count` least significant bits from `value`.

### `bool pop()`

Dequeues and returns a single bit. Throws `std::out_of_range` if the queue is empty.

### `uint64_t pop(uint8_t count)`

Dequeues `count` bits and returns them as a `uint64_t`. Throws `std::out_of_range` if there are not enough bits in the queue.

### `bool front() const`

Returns the next bit in the queue without removing it. Throws `std::out_of_range` if the queue is empty.

### `size_t size() const`

Returns the number of bits in the queue.

### `bool empty() const`

Returns `true` if the queue is empty, `false` otherwise.

### `void clear()`

Removes all bits from the queue.

# DynamicBitset

## Overview

`DynamicBitset` is a C++ class that implements a bitset (also known as a bitarray or bitvector) whose size is determined at runtime. It provides efficient storage and manipulation of a sequence of bits. This is in contrast to `std::bitset` where the size must be a compile-time constant. `DynamicBitset` aims to offer a similar interface to `std::bitset` where applicable, along with the flexibility of dynamic sizing.

It is useful for representing a compact set of boolean flags, tracking states, or performing bitwise operations on data whose size isn't known until runtime.

## Features

*   **Dynamic Sizing**: The number of bits can be specified during construction.
*   **Efficient Storage**: Bits are packed into an array of unsigned integer blocks (typically `uint64_t`).
*   **Bit Manipulation**:
    *   Set, reset, and flip individual bits.
    *   Set, reset, and flip all bits in the set.
    *   Access individual bits using `operator[]` (returns a proxy object for assignment and direct read).
    *   Test individual bits using `test()`.
*   **Query Operations**:
    *   `size()`: Get the number of bits.
    *   `count()`: Get the number of bits that are set to true.
    *   `all()`: Check if all bits are set.
    *   `any()`: Check if any bit is set.
    *   `none()`: Check if no bits are set (i.e., all are false).
    *   `empty()`: Check if the bitset has a size of 0.
*   **Bitwise Operations**:
    *   `operator&=`, `operator|=`, `operator^=` for bitwise AND, OR, XOR with another `DynamicBitset` of the same size.
*   **Bounds Checking**: Accessing bits out of range will throw `std::out_of_range`. Operations on bitsets of mismatched sizes (for bitwise ops) will throw `std::invalid_argument`.

## Internal Representation

Bits are stored in a `std::vector<BlockType>`, where `BlockType` is `uint64_t`. The least significant bit of the first block corresponds to bit 0, and so on. Padding bits in the last block (if `num_bits_` is not a multiple of `kBitsPerBlock`) are always maintained as 0 to ensure correctness of operations like `count()`, `any()`, `none()`, and bitwise operations.

## Usage

### Include Header

```cpp
#include "DynamicBitset.h"
```

### Construction

```cpp
// Creates an empty bitset (size 0)
DynamicBitset bs1;

// Creates a bitset with 100 bits, all initialized to false (default)
DynamicBitset bs2(100);

// Creates a bitset with 64 bits, all initialized to true
DynamicBitset bs3(64, true);
```

### Accessing and Modifying Bits

```cpp
DynamicBitset bs(10); // 10 bits, all false

bs.set(0);        // Set bit 0 to true:  1000000000
bs.set(3, true);  // Set bit 3 to true:  1001000000
bs.set(5, false); // Set bit 5 to false (no change if already false)

bs[1] = true;     // Set bit 1 using proxy: 1101000000
bs[0].flip();     // Flip bit 0 using proxy: 0101000000

if (bs.test(1)) { // Check bit 1
    // ... bit 1 is true
}

bool val_at_2 = bs[2]; // Read bit 2 using proxy

bs.reset(1);      // Reset bit 1 to false: 0001000000
bs.flip(3);       // Flip bit 3: 0000000000
```

### Whole-Set Operations

```cpp
DynamicBitset bs(5); // 00000
bs.set();            // Set all bits: 11111
bs.reset();          // Reset all bits: 00000
bs.flip();           // Flip all bits: 11111
```

### Querying

```cpp
DynamicBitset bs(8);
bs.set(1); bs.set(3); bs.set(5); // 01010100

std::cout << "Size: " << bs.size();     // Output: Size: 8
std::cout << "Count: " << bs.count();   // Output: Count: 3
std::cout << "Any set? " << bs.any();   // Output: Any set? true
std::cout << "None set? " << bs.none(); // Output: None set? false
std::cout << "All set? " << bs.all();   // Output: All set? false
```

### Bitwise Operations

```cpp
DynamicBitset a(4, false); // 0000
a.set(0); a.set(2);        // a is 1010

DynamicBitset b(4, false); // 0000
b.set(1); b.set(2);        // b is 0110

a &= b; // a becomes 0010 (a is modified)
// a |= b;
// a ^= b;

// These require bitsets of the same size.
// DynamicBitset c(5);
// a &= c; // throws std::invalid_argument
```

## Future Considerations (Not Implemented)

*   `resize()`: Method to change the size of the bitset after construction.
*   `operator~()`: Bitwise NOT operator (returning a new bitset).
*   Conversion to/from string or integer types.
*   `find_first()`, `find_next()` for finding set bits.

This `DynamicBitset` provides a flexible and efficient way to work with sequences of bits when the size is not known at compile time.

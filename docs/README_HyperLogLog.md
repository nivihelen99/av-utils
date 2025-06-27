# HyperLogLog

## Overview

The `HyperLogLog` class is a C++ implementation of the HyperLogLog algorithm, a probabilistic data structure used for estimating the cardinality (number of unique elements) of a very large dataset. It is particularly useful when the dataset is too large to fit into memory for an exact count of unique items, or when a very precise count is not necessary and an approximation with known error bounds is acceptable.

HyperLogLog achieves remarkable accuracy using a small, fixed amount of memory, regardless of the number of items processed. The memory usage is determined by a `precision` parameter, and the typical relative error is around `1.04 / sqrt(m)`, where `m = 2^precision`.

## Features

-   **Approximate Cardinality Estimation:** Provides an estimate of the number of unique items added.
-   **Configurable Precision:** Allows balancing memory usage and accuracy via the `precision` parameter `p`.
    -   Number of internal registers `m = 2^p`.
    -   Memory usage is approximately `m` bytes.
    -   Typical `p` values range from 4 to 18.
-   **Templated Design:** Can be used with various data types (`T`) as long as a hash function is available.
-   **Custom Hash Functions:** Supports custom hash functions and specification of hash output width (32-bit or 64-bit).
-   **Mergeable:** Supports merging two HyperLogLog sketches to estimate the cardinality of the union of their underlying sets. This is useful for distributed counting scenarios.
-   **Corrections:** Implements standard small-range and large-range (for 32-bit hashes) corrections for improved accuracy.

## Template Parameters

```cpp
template <typename T, typename Hash = std::hash<T>, size_t HashBits = 32>
class HyperLogLog { ... };
```

-   `T`: The type of elements to be added.
-   `Hash`: The hash function to use for elements of type `T`. Defaults to `std::hash<T>`. The hash function must return a `size_t` (or any unsigned integer type that can be cast to `uint64_t`).
-   `HashBits`: The effective number of bits from the hash function's output to use. Can be `32` or `64`. This influences the maximum rank and the large-range correction behavior. Defaults to `32`.

## API Overview

### Constructor

```cpp
explicit HyperLogLog(uint8_t precision);
```

-   `precision` (`p`): Determines the number of registers `m = 2^p`. Must be between 4 and 18 (inclusive). Higher precision leads to better accuracy but uses more memory.

### Adding Elements

```cpp
void add(const T& item);
```

-   Adds an item to the sketch. The item is hashed, and its hash value is used to update one of the internal registers.

### Estimating Cardinality

```cpp
double estimate() const;
```

-   Returns the estimated number of unique items added to the sketch.

### Merging Sketches

```cpp
void merge(const HyperLogLog& other);
```

-   Merges another `HyperLogLog` sketch into this one. Both sketches must have the same `precision` and `HashBits` configuration. After merging, this sketch will represent the cardinality of the union of the items from both original sketches.

```cpp
void merge_registers(const std::vector<uint8_t>& other_registers);
```
- Merges register values from an external vector. This is useful for advanced scenarios like deserializing or combining sketches from a distributed system where only register data is available. The size of `other_registers` must match the current sketch's number of registers (`m`).

### Clearing the Sketch

```cpp
void clear();
```

-   Resets the sketch to its initial state (as if no items have been added). All registers are set to 0.

### Accessors

```cpp
uint8_t precision() const;         // Returns the precision p
size_t num_registers() const;      // Returns the number of registers m = 2^p
const std::vector<uint8_t>& get_registers() const; // Returns a const reference to the internal registers
```

## Usage Example

```cpp
#include "hyperloglog.h"
#include <iostream>
#include <string>

int main() {
    // Create a HyperLogLog sketch for strings with precision 10 (1024 registers)
    cpp_collections::HyperLogLog<std::string> hll(10);

    hll.add("apple");
    hll.add("banana");
    hll.add("orange");
    hll.add("apple"); // Duplicate, does not significantly change estimate

    std::cout << "Estimated unique items: " << hll.estimate() << std::endl; // Expected: approx 3

    // --- Merging ---
    cpp_collections::HyperLogLog<std::string> hll2(10);
    hll2.add("banana"); // Common
    hll2.add("grape");

    hll.merge(hll2);
    // Now hll contains information about {apple, banana, orange, grape}
    std::cout << "Estimated unique items after merge: " << hll.estimate() << std::endl; // Expected: approx 4

    return 0;
}
```

## Precision, Error Rate, and Memory

The `precision` parameter `p` is the primary knob to control the trade-off between accuracy and memory usage.

-   **Number of Registers (`m`):** `m = 2^p`
-   **Memory Usage:** Approximately `m` bytes (since each register stores a small integer, typically `uint8_t`).
-   **Standard Error (Relative Error):** Approximately `1.04 / sqrt(m)`.

| `p` (Precision) | `m` (Registers) | Memory (approx) | Standard Error (approx) |
|-----------------|-----------------|-----------------|-------------------------|
| 4               | 16              | 16 B            | 26.0%                   |
| 6               | 64              | 64 B            | 13.0%                   |
| 8               | 256             | 256 B           | 6.5%                    |
| 10              | 1024            | 1 KB            | 3.25%                   |
| 12              | 4096            | 4 KB            | 1.625%                  |
| 14              | 16384           | 16 KB           | 0.8125%                 |
| 16              | 65536           | 64 KB           | 0.40625%                |
| 18              | 262144          | 256 KB          | 0.203125%               |

**Choosing `p`:**
- For rough estimates where memory is very constrained, a small `p` (e.g., 4-6) might suffice.
- For many common applications, `p` values between 10 and 14 offer a good balance.
- Higher `p` values (e.g., 16-18) provide greater accuracy but consume more memory. The maximum `p` is typically limited by implementation details (e.g., how hash bits are split). This implementation supports `p` up to 18.

## Notes on Hashing

-   The quality of the hash function is important for HyperLogLog to work effectively. `std::hash` is used by default, which is generally good for standard types.
-   For custom types `T`, you must provide a specialization for `std::hash<T>` or supply a custom hash function as a template argument.
-   The `HashBits` template parameter (defaulting to 32) tells HyperLogLog whether to primarily consider the hash output as a 32-bit or 64-bit value. This affects the maximum possible rank derived from the hash and the application of large-range corrections (which are typically defined for 32-bit hash spaces). Using `HashBits = 64` can be beneficial if your hash function produces well-distributed 64-bit values and you are dealing with cardinalities that might approach or exceed the limits of a 32-bit hash space interpretation.

## Further Reading

-   Flajolet, P., Fusy, É., Gandouet, O., & Meunier, F. (2007). HyperLogLog: the analysis of a near-optimal cardinality estimation algorithm. *Discrete Mathematics & Theoretical Computer Science Proceedings*, ( DMTCS Proc. AH). ([Link to paper often found via search](https://hal.inria.fr/inria-00072722/document))
-   Google Research Blog: "HyperLogLog in Plain English" (A good conceptual overview).
```

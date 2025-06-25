# Count-Min Sketch (`count_min_sketch.h`)

## Overview

The `count_min_sketch.h` header provides a C++ implementation of a Count-Min Sketch, a probabilistic data structure used for estimating the frequency of items in a data stream. It is particularly useful when dealing with large amounts of data where storing exact frequencies for all items is infeasible due to memory constraints.

Key properties:
- **Frequency Estimation:** Provides an estimate of how many times an item has appeared.
- **Overestimation:** Estimates are always greater than or equal to the true frequency. The sketch never underestimates.
- **Error Bounds:** The amount of overestimation is bounded. With a user-specified probability (1 - `delta`), the estimated frequency `est_freq(item)` is within the range:
  `true_freq(item) <= est_freq(item) <= true_freq(item) + epsilon * N`
  where `N` is the sum of all counts inserted into the sketch (total number of items if all counts are 1).
- **Space-Efficient:** Uses sub-linear space relative to the number of distinct items or the domain size.
- **No Deletion (Standard):** This standard implementation does not support decreasing counts or removing items.

The implementation allows you to configure the sketch based on two parameters:
- `epsilon (ε)`: Controls the error factor. Smaller `epsilon` means the estimate is closer to the true frequency (less overestimation), but requires more memory (wider sketch).
- `delta (δ)`: Controls the probability that the error bound is met. Smaller `delta` means higher confidence in the error bound, but requires more hash functions (deeper sketch), increasing computation per operation and memory.

## Template Parameters

The `CountMinSketch` class is templated:

```cpp
template <typename T>
class CountMinSketch {
    // ...
};
```

- `T`: The type of items whose frequencies are to be estimated. The internal hashing mechanism is designed for fundamental types and `std::string`. For custom types, they should be trivially copyable or standard layout for the default FNV-1a based hashing to work on their byte representation.

## Constructor

```cpp
CountMinSketch(double epsilon, double delta);
```

- `epsilon (double)`: The desired maximum additive error factor relative to the total sum of counts in the sketch. For example, if `epsilon` is 0.01, the error will be at most 1% of the total sum of counts. Must be greater than 0.0 and less than 1.0.
- `delta (double)`: The desired probability that the error guarantee (as defined by `epsilon`) is *not* met. For example, if `delta` is 0.01, there is a 99% probability that the estimate adheres to the error bound. Must be greater than 0.0 and less than 1.0.

Throws `std::invalid_argument` if `epsilon` or `delta` are not within the range `(0.0, 1.0)`.

The constructor calculates the optimal `width` (number of counters per hash function, `w = ceil(e / epsilon)`) and `depth` (number of hash functions, `d = ceil(ln(1 / delta))`) for the sketch.

## Public Methods

### `void add(const T& item, unsigned int count = 1)`
Increments the estimated frequency of `item` by `count`. If `count` is 0, the operation has no effect. If the internal counters reach their maximum value (`std::numeric_limits<unsigned int>::max()`), they will be capped at that value.

### `unsigned int estimate(const T& item) const`
Returns the estimated frequency of `item`. This estimate is always greater than or equal to the true frequency.

### `size_t get_width() const`
Returns the width (`w`) of the sketch's internal counter table (number of counters associated with each hash function).

### `size_t get_depth() const`
Returns the depth (`d`) of the sketch (which is also the number of hash functions used).

### `double get_error_factor_epsilon() const`
Returns the `epsilon` value provided at construction.

### `double get_error_probability_delta() const`
Returns the `delta` value provided at construction.

## Usage Example

```cpp
#include "count_min_sketch.h"
#include <iostream>
#include <string>
#include <vector>

int main() {
    // Epsilon = 0.01 (error up to 1% of total counts)
    // Delta = 0.01 (99% confidence in this error bound)
    CountMinSketch<std::string> word_freq_sketch(0.01, 0.01);

    std::cout << "Sketch created with: width=" << word_freq_sketch.get_width()
              << ", depth=" << word_freq_sketch.get_depth() << std::endl;

    std::vector<std::string> words = {"hello", "world", "hello", "count", "min", "sketch", "hello", "world"};
    unsigned int total_counts = 0;
    for (const std::string& word : words) {
        word_freq_sketch.add(word, 1);
        total_counts++;
    }

    std::string test_word = "hello"; // True frequency is 3
    unsigned int estimated_freq = word_freq_sketch.estimate(test_word);
    std::cout << "Estimated frequency of \"" << test_word << "\": " << estimated_freq << std::endl;
    // Expected: estimate >= 3. With 99% prob, estimate <= 3 + 0.01 * total_counts.

    std::string absent_word = "CPlusPlus"; // True frequency is 0
    unsigned int estimated_absent_freq = word_freq_sketch.estimate(absent_word);
    std::cout << "Estimated frequency of \"" << absent_word << "\": " << estimated_absent_freq << std::endl;
    // Expected: estimate >= 0. With 99% prob, estimate <= 0 + 0.01 * total_counts.
    // For absent items, the estimate is often a small non-zero value due to collisions.

    return 0;
}
```

## Trade-offs and Considerations

- **Accuracy vs. Memory/Computation:**
    - Smaller `epsilon` (higher accuracy for error factor) leads to a larger `width`, increasing memory usage.
    - Smaller `delta` (higher probability of meeting the error bound) leads to a larger `depth`, increasing memory usage and the number of hash computations per `add`/`estimate` operation.
- **Total Sum of Counts (N):** The error bound `epsilon * N` depends on the total sum of all counts inserted. If `N` is very large, `epsilon` must be chosen to be very small to achieve a low absolute error.
- **Hashing:** The quality of hash functions is important. This implementation uses FNV-1a based hashing, which is generally good.
- **Counter Size:** The counters are `unsigned int`. If individual item counts or the sum of counts hitting a single counter cell are expected to exceed `std::numeric_limits<unsigned int>::max()`, this implementation will cap the count. For extremely high frequency items, a sketch with `uint64_t` counters might be necessary.
- **No Deletion:** This is a standard Count-Min Sketch. If decrementing counts or item deletion is required, variations like Count-Mean-Min Sketch or Conservative Update Count-Min Sketch would be needed, which are more complex.

## When to Use Count-Min Sketch

Count-Min Sketch is suitable for:
- Estimating frequencies of events in high-volume data streams.
- Identifying frequent items (heavy hitters) in a stream.
- Applications where some overestimation is acceptable and exact counts are too costly to maintain.
- Network traffic monitoring (e.g., estimating flows sizes).
- Database query optimization (e.g., estimating cardinalities).
- Web analytics (e.g., counting page views for popular pages).

It's a good alternative to exact frequency counting when memory is limited and probabilistic estimates are sufficient.
It complements structures like Bloom Filters: Bloom Filters tell you if an item *might* be in a set, while Count-Min Sketches estimate *how often* an item has appeared.

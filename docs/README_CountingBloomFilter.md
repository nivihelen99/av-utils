# Counting Bloom Filter

## Overview

A Counting Bloom Filter (CBF) is a probabilistic data structure used to test whether an element is a member of a set, similar to a standard Bloom Filter. The key advantage of a Counting Bloom Filter over a standard one is its ability to handle the **deletion** of elements. This is achieved by using an array of small counters instead of an array of bits.

When an element is added, the counters corresponding to its hash values are incremented. When an element is queried (`contains`), all corresponding counters are checked; if any counter is zero, the element is definitively not in the set. If all counters are non-zero, the element is *probably* in the set (with a certain false positive probability). When an element is removed, the corresponding counters are decremented.

## Features

*   **Approximate Membership Testing**: Like standard Bloom filters, CBFs can tell you if an item is *definitely not* in the set or *probably is* in the set.
*   **Element Deletion**: Supports removing elements, which is not possible with a standard Bloom filter.
*   **Configurable False Positive Rate**: You can tune the filter's size and number of hash functions based on the expected number of items and the desired false positive probability.
*   **Space Efficient (Relatively)**: More space-intensive than a standard Bloom filter due to counters instead of bits, but still more space-efficient than explicitly storing all elements for approximate set membership.

## When to Use

Counting Bloom Filters are useful in scenarios where:
*   You need approximate set membership with a low false positive rate.
*   Elements are frequently added *and removed* from the set.
*   Memory is a concern, but you can afford more than a standard Bloom filter to gain deletion capability.
*   Examples:
    *   Tracking recently seen items in network traffic where flows can end.
    *   Caching systems where items expire or are evicted.
    *   Databases to avoid disk lookups for non-existent keys that might have been recently deleted.

## `CountingBloomFilter` Class

The `cpp_utils::CountingBloomFilter<T, CounterType, Hasher>` class provides this functionality.

### Template Parameters

*   `T`: The type of elements to be stored in the filter.
*   `CounterType` (optional, defaults to `uint8_t`): The integral type used for the counters. Smaller types (`uint8_t`, `uint16_t`) save space but can saturate more quickly if items are added many times or if there are many hash collisions. `uint8_t` allows a counter to go up to 255. If an item is added more times than the counter can hold, the counter will saturate at its maximum value.
*   `Hasher` (optional, defaults to `cpp_utils::DefaultHash<T>`): The hash functor used. The default uses `std::hash` and a simple mixing strategy to generate multiple hash values. For critical applications, you might provide a custom hasher that implements `uint64_t operator()(const T& item, uint64_t seed) const`.

### Constructor

```cpp
CountingBloomFilter(size_t expected_insertions, double false_positive_rate);
```
*   `expected_insertions`: An estimate of the maximum number of unique items you expect to store in the filter simultaneously. This is used to calculate the optimal size of the counter array and the number of hash functions.
*   `false_positive_rate`: The desired probability (e.g., `0.01` for 1%) that `contains()` will return `true` for an item that has not been added.

### Public Methods

*   `void add(const T& item)`: Adds an item to the filter. Increments the counters at the item's hash locations. If a counter reaches its maximum value, it remains saturated.
*   `bool contains(const T& item) const`: Checks if an item might be in the filter. Returns `false` if the item is definitely not present. Returns `true` if the item is probably present (could be a false positive).
*   `bool remove(const T& item)`: Removes an item from the filter. Decrements the counters at the item's hash locations.
    *   Returns `true` if the item was potentially removed (i.e., all its associated counters were non-zero before decrementing).
    *   Returns `false` if the item was definitely not present (i.e., at least one of its associated counters was already zero).
    *   Counters are not decremented below zero.
*   `size_t numCounters() const`: Returns the number of counters in the filter array.
*   `size_t numHashFunctions() const`: Returns the number of hash functions used.
*   `size_t approxMemoryUsage() const`: Returns an estimate of the memory used by the filter in bytes.

## Basic Usage

```cpp
#include "include/counting_bloom_filter.h"
#include <iostream>
#include <string>

int main() {
    // Expecting ~1000 items, desire 1% false positive rate
    cpp_utils::CountingBloomFilter<std::string> cbf(1000, 0.01);

    cbf.add("apple");
    cbf.add("banana");
    cbf.add("apple"); // Add "apple" again

    std::cout << "Contains 'apple'? " << cbf.contains("apple") << std::endl; // Expected: 1 (true)
    std::cout << "Contains 'orange'? " << cbf.contains("orange") << std::endl; // Expected: 0 (false, unless FP)

    cbf.remove("banana");
    std::cout << "Contains 'banana' after remove? " << cbf.contains("banana") << std::endl; // Expected: 0 (false)

    cbf.remove("apple"); // First remove of "apple"
    std::cout << "Contains 'apple' after one remove? " << cbf.contains("apple") << std::endl; // Expected: 1 (true, was added twice)

    cbf.remove("apple"); // Second remove of "apple"
    std::cout << "Contains 'apple' after second remove? " << cbf.contains("apple") << std::endl; // Expected: 0 (false)

    return 0;
}
```

## Hashing

The quality of the hash functions is crucial for the performance and accuracy of the Bloom filter. The provided `DefaultHash` is a basic implementation. For production systems, consider using well-known non-cryptographic hash functions like MurmurHash3 or xxHash, and ensure your `Hasher` functor generates multiple distinct hash values effectively using the `seed` parameter. The current implementation uses the `seed` (which is the hash function index `0..k-1`) to perturb a base hash from `std::hash<T>`.

## Trade-offs

*   **Counter Size (`CounterType`)**:
    *   Smaller counters (e.g., `uint8_t` for 4-bit or 8-bit counters, though `uint8_t` is the smallest directly usable standard type) save space.
    *   Risk of saturation: If an item is added many times, or many distinct items hash to the same counter, the counter might reach its maximum value. If this happens, removing an item might not correctly reflect its absence if another item still requires that counter to be high.
    *   Larger counters (e.g., `uint16_t`, `uint32_t`) reduce saturation risk but increase space. Typically, 4-bit to 8-bit counters are common. `uint8_t` is a practical choice for many applications.
*   **False Positives**: The false positive rate is determined by the size of the counter array and the number of hash functions, which are derived from `expected_insertions` and the desired `false_positive_rate`. Deleting items can, over time, slightly alter the actual false positive rate compared to a static Bloom filter, as emptied slots become available.
*   **False Negatives**: Counting Bloom Filters, like standard Bloom filters, do *not* produce false negatives. If `contains()` returns `false`, the item is definitely not in the set.

This documentation should provide a good overview for users of the `CountingBloomFilter`.

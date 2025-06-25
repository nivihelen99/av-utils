# Bloom Filter (`bloom_filter.h`)

## Overview

The `bloom_filter.h` header provides a C++ implementation of a Bloom filter, a space-efficient probabilistic data structure used to test whether an element is a member of a set.

- **False positives are possible:** If `might_contain(item)` returns `true`, the item *might* be in the set, or it could be a false positive.
- **No false negatives:** If `might_contain(item)` returns `false`, the item is *definitely not* in the set.
- **Space-efficient:** Bloom filters use significantly less memory than storing all items in a traditional hash set, especially for large sets, at the cost of the aforementioned false positive probability.
- **No deletion:** Standard Bloom filters do not support removing items.

This implementation allows you to configure the filter based on the expected number of items you plan to insert and your desired false positive probability. Based on these parameters, it automatically calculates the optimal size of the bit array and the optimal number of hash functions to use.

## Template Parameters

The `BloomFilter` class is templated:

```cpp
template <typename T>
class BloomFilter {
    // ...
};
```

- `T`: The type of items to be stored in the filter. `std::hash<T>` must be available for the item type `T`, or you must provide a custom hash function (see Hashing section in `bloom_filter.h` for the internal `detail::BloomHash` struct if customization beyond `std::hash` is needed directly by the filter's internal mechanism, though typically specializing `std::hash` is sufficient).

## Constructor

```cpp
BloomFilter(size_t expected_items, double false_positive_probability);
```

- `expected_items (size_t)`: The number of items you expect to insert into the filter. This is used to optimize the filter's size and hash count. If you insert significantly more items than this, the false positive rate will increase. If 0 is provided, a minimal filter is created (1 bit, 1 hash function).
- `false_positive_probability (double)`: The desired probability of false positives (e.g., `0.01` for 1%, `0.001` for 0.1%). This value must be greater than 0.0 and less than 1.0. A lower probability requires more memory.

Throws `std::invalid_argument` if `false_positive_probability` is not within the range `(0.0, 1.0)`.

## Public Methods

### `void add(const T& item)`
Adds an item to the Bloom filter. This operation involves hashing the item multiple times and setting the corresponding bits in the internal bit array.

### `bool might_contain(const T& item) const`
Checks if an item might be in the Bloom filter.
- Returns `true`: The item *might* be in the filter (it was either added, or this is a false positive).
- Returns `false`: The item is *definitely not* in the filter.

### `size_t approximate_item_count() const`
Returns the number of times `add()` has been called. This is an approximate count of items and does not reflect unique items if the same item is added multiple times.

### `size_t bit_array_size() const`
Returns the size (number of bits, `m`) of the internal bit array used by the filter. This is calculated based on `expected_items` and `false_positive_probability`.

### `size_t number_of_hash_functions() const`
Returns the number of hash functions (`k`) used by the filter. This is also calculated based on `expected_items` and `false_positive_probability`.

### `size_t expected_items_capacity() const`
Returns the `expected_items` value provided at construction.

### `double configured_fp_probability() const`
Returns the `false_positive_probability` value provided at construction.

## Usage Example

```cpp
#include "bloom_filter.h"
#include <iostream>
#include <string>

int main() {
    // Create a Bloom filter for an expected 10,000 strings
    // with a desired false positive probability of 0.5% (0.005)
    BloomFilter<std::string> name_filter(10000, 0.005);

    std::cout << "Filter settings:" << std::endl;
    std::cout << "  Expected items: " << name_filter.expected_items_capacity() << std::endl;
    std::cout << "  Target FP prob: " << name_filter.configured_fp_probability() << std::endl;
    std::cout << "  Bit array size: " << name_filter.bit_array_size() << " bits" << std::endl;
    std::cout << "  Hash functions: " << name_filter.number_of_hash_functions() << std::endl;

    // Add some names
    name_filter.add("Alice");
    name_filter.add("Bob");
    name_filter.add("Charlie");

    // Check for names
    std::string name1 = "Alice";    // Added
    std::string name2 = "David";    // Not added
    std::string name3 = "Bob";      // Added

    std::cout << "\nChecking for '" << name1 << "': "
              << (name_filter.might_contain(name1) ? "Might be present" : "Definitely not present")
              << std::endl;

    std::cout << "Checking for '" << name2 << "': "
              << (name_filter.might_contain(name2) ? "Might be present (False Positive?)" : "Definitely not present")
              << std::endl;

    std::cout << "Checking for '" << name3 << "': "
              << (name_filter.might_contain(name3) ? "Might be present" : "Definitely not present")
              << std::endl;

    // Demonstrate the effect of adding more items than expected (increases FP rate)
    // For a real scenario, choose expected_items carefully.
    // Here, we add many more items to see the FP rate degrade if we were to test.
    for (int i = 0; i < 20000; ++i) {
        name_filter.add("user_" + std::to_string(i));
    }
    std::cout << "\nAdded 20,000 more items. Total additions: " << name_filter.approximate_item_count() << std::endl;
    std::cout << "The actual false positive rate will now likely be higher than the initial "
              << name_filter.configured_fp_probability() << std::endl;

    return 0;
}
```

## Trade-offs and Considerations

- **False Positive Rate vs. Memory:** Lowering the false positive probability requires a larger bit array (more memory) and potentially more hash functions (more computation per operation).
- **Number of Items:** The filter is optimized for the `expected_items`. If you add significantly more items, the false positive rate will increase beyond the initially configured probability. If you add fewer, the filter might be larger than necessary.
- **No Deletion:** This standard Bloom filter implementation does not support item deletion. Removing an item would require clearing bits in the array, which could inadvertently affect other items that hash to the same bits, leading to false negatives (which Bloom filters must not have). Variants like Counting Bloom Filters can support deletion at the cost of more space.
- **Hashing:** The quality of the hash functions is crucial for the performance and accuracy of the Bloom filter. The current implementation uses `std::hash` with seed perturbation. For very demanding applications, using more robust hash functions (e.g., MurmurHash3, xxHash) might be considered. Ensure `std::hash` is well-defined for your custom types.

## When to Use Bloom Filters

Bloom filters are useful in scenarios where:
- You need to quickly check if an item *might* be in a large set.
- A small chance of false positives is acceptable.
- False negatives are not acceptable.
- Memory usage is a concern.
- Item deletion is not required.

Common use cases include:
- **Caching:** Checking if a resource is in a cache to avoid expensive lookups (e.g., web browsers checking for malicious URLs).
- **Databases:** Reducing disk lookups for non-existent rows or keys.
- **Network Routers:** Filtering packets.
- **Spell Checkers:** Identifying misspelled words (a word not in the filter is likely misspelled).

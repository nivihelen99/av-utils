# WeightedSet Data Structure

## Table of Contents
1. [Overview](#overview)
2. [Purpose](#purpose)
3. [API Reference](#api-reference)
    - [Template Parameters](#template-parameters)
    - [Type Aliases](#type-aliases)
    - [Constructors](#constructors)
    - [Modifiers](#modifiers)
    - [Lookup](#lookup)
    - [Sampling](#sampling)
    - [Capacity](#capacity)
    - [Iterators](#iterators)
    - [Comparison](#comparison)
    - [Swap](#swap)
4. [Usage Example](#usage-example)
5. [Time Complexity](#time-complexity)
6. [Thread Safety](#thread-safety)

## Overview

`cpp_collections::WeightedSet<Key, WeightType, Compare>` is a C++ data structure that stores a collection of unique keys, each associated with a numerical weight. It allows for efficient addition, removal, and updating of key-weights, and most importantly, provides a mechanism to randomly sample keys based on their assigned weights. Keys with higher weights have a proportionally higher probability of being selected during sampling.

It is implemented as a header-only library and aims to be idiomatic C++17/C++20.

## Purpose

The `WeightedSet` is useful in scenarios where elements need to be chosen randomly but with a bias determined by their importance or frequency, as represented by their weights. Common use cases include:

-   **Probabilistic Sampling:** Implementing loot tables in games, selecting advertisements, A/B testing variations.
-   **Weighted Random Selection:** Choosing a server based on load or capacity, weighted selection in simulations.
-   **Data Analysis:** Sampling data points where each point has a different level of significance.

It provides an alternative to manually managing weights in a `std::vector` or `std::map` and performing complex calculations for weighted sampling.

## API Reference

```cpp
namespace cpp_collections {

template <
    typename Key,
    typename WeightType = double,
    typename Compare = std::less<Key>
>
class WeightedSet {
public:
    // ... public interface ...
};

} // namespace cpp_collections
```

### Template Parameters

-   `Key`: The type of the keys stored in the set.
-   `WeightType`: The numerical type used for weights (defaults to `double`). Must support arithmetic operations and comparison.
-   `Compare`: A comparison function object type that defines the ordering of keys (defaults to `std::less<Key>`). Used by the underlying `std::map`.

### Type Aliases

-   `key_type = Key`
-   `weight_type = WeightType`
-   `key_compare = Compare`
-   `value_type = std::pair<const Key, WeightType>`: Type of elements iterated over.
-   `map_type = std::map<Key, WeightType, Compare>`: Underlying map type.
-   `size_type = typename map_type::size_type`: Unsigned integer type for sizes.
-   `iterator = typename map_type::iterator`
-   `const_iterator = typename map_type::const_iterator`

### Constructors

-   `WeightedSet()`
    -   Default constructor. Creates an empty `WeightedSet`.
-   `explicit WeightedSet(const key_compare& comp)`
    -   Creates an empty `WeightedSet` with a custom key comparator.
-   `template<typename InputIt> WeightedSet(InputIt first, InputIt last, const key_compare& comp = key_compare())`
    -   Constructs with elements from an iterator range `[first, last)`. `InputIt` must dereference to a pair-like object (e.g., `std::pair<Key, WeightType>`).
-   `WeightedSet(std::initializer_list<value_type> ilist, const key_compare& comp = key_compare())`
    -   Constructs with elements from an initializer list.
-   `WeightedSet(const WeightedSet& other)`
    -   Copy constructor.
-   `WeightedSet(WeightedSet&& other) noexcept`
    -   Move constructor.

### Modifiers

-   `void add(const key_type& key, weight_type weight)`
-   `void add(key_type&& key, weight_type weight)`
    -   Adds `key` with the given `weight`. If `key` already exists, its weight is updated.
    -   If `weight` is less than or equal to `0`, the `key` is effectively removed from the set (if it exists) or not added.
    -   Invalidates internal sampling cache.
-   `bool remove(const key_type& key)`
    -   Removes `key` from the set. Returns `true` if the key was found and removed, `false` otherwise.
    -   Invalidates internal sampling cache if an item is removed.

### Lookup

-   `weight_type get_weight(const key_type& key) const`
    -   Returns the weight of `key`. If `key` is not found, returns `0`.
-   `bool contains(const key_type& key) const`
    -   Returns `true` if `key` is in the set (i.e., was added with a positive weight), `false` otherwise.

### Sampling

-   `const key_type& sample() const`
    -   Randomly samples a key from the set based on current weights.
    -   Keys with higher weights are more likely to be selected.
    -   Throws `std::out_of_range` if the set is empty or if the total positive weight is zero.
    -   This method is logically `const` but may modify internal mutable caches for sampling efficiency.

### Capacity

-   `bool empty() const noexcept`
    -   Returns `true` if the set contains no items, `false` otherwise.
-   `size_type size() const noexcept`
    -   Returns the number of unique keys in the set.
-   `weight_type total_weight() const noexcept`
    -   Returns the sum of all positive weights currently in the set.

### Iterators

The `WeightedSet` provides iterators that allow iterating over the `(key, weight)` pairs in the order defined by the `Compare` function (key-sorted by default).

-   `iterator begin() noexcept`
-   `const_iterator begin() const noexcept`
-   `const_iterator cbegin() const noexcept`
-   `iterator end() noexcept`
-   `const_iterator end() const noexcept`
-   `const_iterator cend() const noexcept`

### Comparison

-   `key_compare key_comp() const`
    -   Returns the key comparison function object.

### Swap

-   `void swap(WeightedSet& other) noexcept(...)`
    -   Exchanges the contents of this `WeightedSet` with `other`.
-   `template <typename K, typename W, typename C> void swap(WeightedSet<K, W, C>& lhs, WeightedSet<K, W, C>& rhs) noexcept(...)`
    -   Non-member swap function.

## Usage Example

```cpp
#include "weighted_set.h"
#include <iostream>
#include <string>
#include <map>

int main() {
    cpp_collections::WeightedSet<std::string, double> loot_table;

    loot_table.add("Sword", 10.0);
    loot_table.add("Shield", 10.0);
    loot_table.add("Potion", 25.0);
    loot_table.add("Gold Coin", 50.0);
    loot_table.add("Rare Gem", 5.0);

    std::cout << "Loot Table (Total Weight: " << loot_table.total_weight() << "):\n";
    for (const auto& item : loot_table) {
        std::cout << "  Item: " << item.first << ", Weight: " << item.second << "\n";
    }

    std::cout << "\nPerforming 10000 samples:\n";
    std::map<std::string, int> counts;
    if (!loot_table.empty() && loot_table.total_weight() > 0) {
        for (int i = 0; i < 10000; ++i) {
            try {
                counts[loot_table.sample()]++;
            } catch (const std::out_of_range& e) {
                std::cerr << "Error during sampling: " << e.what() << std::endl;
                break;
            }
        }
    }

    std::cout << "Sample counts:\n";
    for (const auto& pair : counts) {
        std::cout << "  " << pair.first << ": " << pair.second << " times\n";
    }

    return 0;
}
```

## Time Complexity

Let `N` be the number of items in the `WeightedSet`.
The underlying data structure is a `std::map`.

-   **`add(key, weight)`**: `O(log N)` due to `std::map::insert_or_assign`. Sets a flag indicating sampling data is stale.
-   **`remove(key)`**: `O(log N)` due to `std::map::erase`. Sets a flag indicating sampling data is stale.
-   **`get_weight(key)`**: `O(log N)` due to `std::map::find`.
-   **`contains(key)`**: `O(log N)` due to `std::map::count`.
-   **`sample()`**:
    -   If sampling data is up-to-date: `O(log N)` for binary search on the cumulative weight vector.
    -   If sampling data is stale: `O(N)` to rebuild the cumulative weight vector, then `O(log N)` for binary search. The rebuild happens only once after modifications until the next sample call.
    -   Amortized complexity depends on the frequency of modifications versus sampling. If sampling is done frequently after many modifications, each sample effectively costs `O(N)`. If modifications are rare, most samples are `O(log N)`.
-   **`empty()`**: `O(1)`.
-   **`size()`**: `O(1)`.
-   **`total_weight()`**: `O(N)` as it iterates through all items to sum positive weights.
-   **Iterators**: Standard `std::map` iterator complexity. Increment/decrement is typically amortized `O(log N)` or `O(1)` in some cases. Traversal of all elements is `O(N)`.

## Thread Safety

The `WeightedSet` class is **not** thread-safe for concurrent write operations or mixed read/write operations without external synchronization.
-   Concurrent calls to non-`const` methods (e.g., `add`, `remove`) from multiple threads must be synchronized.
-   Concurrent calls to `const` methods (e.g., `get_weight`, `contains`, `size`, `empty`, iterators) are generally safe if no other thread is modifying the `WeightedSet`.
-   The `sample()` method, although `const`, modifies internal mutable caches (`cumulative_items_`, `total_weight_for_sampling_`, `stale_sampling_data_`, `random_engine_`). If multiple threads call `sample()` concurrently, it can lead to data races on these mutable members. The `random_engine_` itself is not thread-safe for concurrent calls. Therefore, calls to `sample()` from multiple threads also require external synchronization, even on a `const WeightedSet` object.

For multi-threaded scenarios, appropriate locking mechanisms (e.g., `std::mutex`) should be employed to protect access to `WeightedSet` instances.

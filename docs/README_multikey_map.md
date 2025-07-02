# MultiKeyMap

## Overview

`MultiKeyMap` is a C++ data structure that provides a map-like interface where keys are composed of multiple heterogeneous values. It simplifies the use of composite keys by allowing users to interact with the map using individual key components directly, rather than manually managing `std::tuple` objects for every operation.

Internally, `MultiKeyMap` uses an `std::unordered_map` with `std::tuple` as the actual key. It provides a custom hash function (`TupleHash`) for `std::tuple` to enable its use with `std::unordered_map`.

This structure is useful when you need to map values based on a combination of several attributes, for example, `(int id, std::string type, double version) -> MyObject`.

## Features

*   **Variadic Key Handling**: Supports keys composed of a variable number of elements with different types.
*   **Type Safety**: Enforces type correctness for key components at compile time.
*   **Ergonomic API**: Offers methods like `insert`, `find`, `at`, `erase`, `contains` that accept individual key components directly.
*   **Tuple Compatibility**: Also allows operations using `std::tuple` objects directly (e.g., `insert_tuple`, `find_tuple`).
*   **Standard Map Interface**: Provides common map functionalities like `size()`, `empty()`, `clear()`, iterators, etc.
*   **Customizable Hashing**: Uses a robust default hash function for tuples, which can be extended or customized if needed (though the default `TupleHash` should cover most standard types).
*   **Header-Only**: Easy to integrate by including `multikey_map.h`.

## API Reference

### Template Parameters

```cpp
template <typename Value, typename... KeyTypes>
class MultiKeyMap;
```

*   `Value`: The type of the values stored in the map.
*   `KeyTypes...`: A variadic pack representing the types of the individual components that form the composite key.

### Key Type Alias

*   `KeyTuple`: Alias for `std::tuple<KeyTypes...>`.

### Constructors

*   `MultiKeyMap()`: Default constructor.
*   `MultiKeyMap(size_type bucket_count, const hasher& hash = hasher(), const key_equal& equal = key_equal())`: Constructor with initial bucket count and custom hasher/equality functors.
*   `MultiKeyMap(std::initializer_list<std::pair<KeyTuple, Value>> init)`: Constructor from an initializer list of key-tuple/value pairs.

### Core Operations (Variadic Keys)

These methods accept individual key components.

*   `std::pair<iterator, bool> insert(const KeyTypes&... keys, const Value& value)`
*   `std::pair<iterator, bool> insert(const KeyTypes&... keys, Value&& value)`
*   `template <typename... ArgsValue> std::pair<iterator, bool> emplace(const KeyTypes&... keys, ArgsValue&&... args_value)`
*   `template <typename... ArgsValue> std::pair<iterator, bool> try_emplace(const KeyTypes&... keys, ArgsValue&&... args_value)`
*   `iterator find(const KeyTypes&... keys)`
*   `const_iterator find(const KeyTypes&... keys) const`
*   `Value& at(const KeyTypes&... keys)`
*   `const Value& at(const KeyTypes&... keys) const`
*   `size_type erase(const KeyTypes&... keys)`
*   `bool contains(const KeyTypes&... keys) const`
*   `Value& operator()(const KeyTypes&... keys)`: Accesses element, creates if not exists (like `std::map::operator[]`).

### Core Operations (Tuple Keys)

These methods accept a `KeyTuple` (`std::tuple<KeyTypes...>`) directly.

*   `std::pair<iterator, bool> insert_tuple(const KeyTuple& key_tuple, const Value& value)` (and overloads for `Value&&`, `KeyTuple&&`)
*   `template <typename... ArgsValue> std::pair<iterator, bool> emplace_tuple(const KeyTuple& key_tuple, ArgsValue&&... args_value)` (and overload for `KeyTuple&&`)
*   `template <typename... ArgsValue> std::pair<iterator, bool> try_emplace_tuple(const KeyTuple& key_tuple, ArgsValue&&... args_value)` (and overload for `KeyTuple&&`)
*   `iterator find_tuple(const KeyTuple& key_tuple)`
*   `const_iterator find_tuple(const KeyTuple& key_tuple) const`
*   `Value& at_tuple(const KeyTuple& key_tuple)`
*   `const Value& at_tuple(const KeyTuple& key_tuple) const`
*   `size_type erase_tuple(const KeyTuple& key_tuple)`
*   `bool contains_tuple(const KeyTuple& key_tuple) const`
*   `Value& operator[](const KeyTuple& key_tuple)`
*   `Value& operator[](KeyTuple&& key_tuple)`

### Other Standard Methods

*   `empty() const noexcept -> bool`
*   `size() const noexcept -> size_type`
*   `max_size() const noexcept -> size_type`
*   `clear() noexcept`
*   `begin() noexcept -> iterator`
*   `end() noexcept -> iterator`
*   `cbegin() const noexcept -> const_iterator`
*   `cend() const noexcept -> const_iterator`
*   `swap(MultiKeyMap& other) noexcept`
*   Bucket interface methods (`bucket_count`, `bucket_size`, `bucket`, etc.)
*   Hash policy methods (`load_factor`, `max_load_factor`, `rehash`, `reserve`)
*   Observer methods (`hash_function`, `key_eq`)
*   Equality operators (`operator==`, `operator!=`)

## Usage Example

```cpp
#include "multikey_map.h"
#include <iostream>
#include <string>

int main() {
    // Map (int user_id, std::string item_category) to (double score)
    MultiKeyMap<double, int, std::string> userScores;

    // Insert using individual keys
    userScores.insert(101, "books", 85.5);
    userScores.emplace(102, "electronics", 92.0);
    userScores.insert(101, "music", 78.3);

    // Access using operator()
    userScores(103, "games") = 88.0; // Insert or assign

    // Check size
    std::cout << "Number of entries: " << userScores.size() << std::endl; // Output: 4

    // Find and print a score
    if (userScores.contains(101, "books")) {
        std::cout << "Score for (101, books): " << userScores.at(101, "books") << std::endl;
    }

    // Using operator() for access
    std::cout << "Score for (103, games) via operator(): " << userScores(103, "games") << std::endl;

    // Iterate over the map
    std::cout << "All scores:" << std::endl;
    for (const auto& entry : userScores) {
        // entry.first is std::tuple<int, std::string>
        // entry.second is double
        std::cout << "  User ID: " << std::get<0>(entry.first)
                  << ", Category: " << std::get<1>(entry.first)
                  << " -> Score: " << entry.second << std::endl;
    }

    // Erase an entry
    userScores.erase(101, "music");
    std::cout << "Number of entries after erase: " << userScores.size() << std::endl; // Output: 3

    // Using std::tuple with operator[]
    MultiKeyMap<std::string, int, int> map_tuple_keys;
    std::tuple<int, int> key_t(200, 300);
    map_tuple_keys[key_t] = "Value for (200,300)";
    std::cout << map_tuple_keys[key_t] << std::endl;


    return 0;
}
```

## Internal Implementation Details

`MultiKeyMap` internally wraps an `std::unordered_map<std::tuple<KeyTypes...>, Value, TupleHash<KeyTypes...>>`.

The `TupleHash<KeyTypes...>` struct provides a specialization for `std::hash` for `std::tuple`. It computes the hash by combining the hashes of individual elements within the tuple. This is achieved using `std::apply` and a `hash_combine` utility function, similar to `boost::hash_combine`.

For example:
```cpp
// Helper to combine hash values
template <typename T>
inline void hash_combine(std::size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// Hash functor for std::tuple
template <typename... KeyTypes>
struct TupleHash {
    std::size_t operator()(const std::tuple<KeyTypes...>& t) const {
        std::size_t seed = 0;
        std::apply([&seed](const auto&... args) {
            (hash_combine(seed, args), ...); // Fold expression
        }, t);
        return seed;
    }
};
```

This allows `std::tuple` to be used effectively as a key in `std::unordered_map`. The `MultiKeyMap` class then provides a more convenient variadic interface on top of this.

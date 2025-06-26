# `std_ext::pairwise`

## Overview

`std_ext::pairwise` is a utility that takes an iterable sequence and returns a view of successive overlapping pairs of elements from that sequence. If the input iterable is `[e1, e2, e3, ..., eN]`, the `pairwise` view will produce pairs `(e1, e2), (e2, e3), ..., (eN-1, eN)`.

This is similar to Python's `itertools.pairwise` (available since Python 3.10).

The utility consists of:
- A factory function `std_ext::pairwise(iterable)` that returns a view object.
- An iterator class `std_ext::pairwise_iterator` used by the view.
- A view class `std_ext::PairwiseIterView` that holds iterators to the original sequence.

## Features

- Works with any iterable that provides `begin()` and `end()` iterators (at least InputIterators).
- Handles empty sequences and sequences with a single element correctly (produces an empty pairwise view).
- Supports C-style arrays.
- Supports `const` iterables, yielding pairs of `const` references (or values, depending on iterator traits).
- The returned view is lightweight and does not copy the underlying data. It stores iterators to the original sequence.

## Usage

Include the header:
```cpp
#include "include/pairwise.h"
```

### Basic Example

```cpp
#include "include/pairwise.h"
#include <vector>
#include <iostream>

int main() {
    std::vector<int> numbers = {1, 2, 3, 4, 5};

    for (const auto& pair : std_ext::pairwise(numbers)) {
        // pair is std::pair<int, int> (or similar, based on iterator value_type)
        std::cout << "(" << pair.first << ", " << pair.second << ")" << std::endl;
    }
    // Output:
    // (1, 2)
    // (2, 3)
    // (3, 4)
    // (4, 5)

    return 0;
}
```

### With Different Container Types

`pairwise` can be used with various standard containers:

```cpp
#include <list>
#include <string>

std::list<std::string> words = {"hello", "world", "from", "C++"};
for (const auto& pair : std_ext::pairwise(words)) {
    std::cout << "\"" << pair.first << "\" -> \"" << pair.second << "\"" << std::endl;
}
// Output:
// "hello" -> "world"
// "world" -> "from"
// "from" -> "C++"
```

### With C-style Arrays

```cpp
int arr[] = {10, 20, 30};
for (const auto& pair : std_ext::pairwise(arr)) {
    std::cout << "Pair: " << pair.first << ", " << pair.second << std::endl;
}
// Output:
// Pair: 10, 20
// Pair: 20, 30
```

### Empty and Single-Element Sequences

If the input sequence has fewer than two elements, the `pairwise` view will be empty.

```cpp
std::vector<int> empty_vec;
for (const auto& pair : std_ext::pairwise(empty_vec)) {
    // This loop will not execute
}
std::cout << "Pairwise over empty_vec is empty: "
          << std_ext::pairwise(empty_vec).empty() << std::endl; // true

std::vector<int> single_vec = {100};
for (const auto& pair : std_ext::pairwise(single_vec)) {
    // This loop will not execute
}
std::cout << "Pairwise over single_vec is empty: "
          << std_ext::pairwise(single_vec).empty() << std::endl; // true
```

## Iterator Behavior

- The `pairwise_iterator` models at least a `std::input_iterator_tag`.
- Dereferencing the iterator (`operator*`) returns a `std::pair` of the current and next elements from the underlying sequence. This pair is constructed by value.
- Incrementing the iterator (`operator++`) advances to the next pair.
- The view returned by `pairwise` can be iterated over multiple times if the underlying iterable's iterators are at least ForwardIterators. If they are InputIterators (e.g., from an `std::istream_iterator`), the view can only be traversed once.

## Lifetime Management

**Important:** The view returned by `std_ext::pairwise` holds iterators (or pointers for C-style arrays) into the original sequence. It does **not** take ownership of the elements or make a copy of the sequence.

Therefore, the original sequence **must outlive** the `pairwise` view. Accessing a `pairwise` view after the underlying sequence has been destroyed will result in undefined behavior (typically dangling iterators).

**Safe Usage:**
```cpp
std::vector<int> data = {1, 2, 3};
auto view = std_ext::pairwise(data); // 'data' is an lvalue and outlives 'view'
for (const auto& p : view) { /* ... */ } // Safe
```

**Unsafe Usage (Dangling Iterators):**
```cpp
// Function that returns a temporary vector
std::vector<int> get_temp_vector() { return {10, 20, 30}; }

auto unsafe_view = std_ext::pairwise(get_temp_vector());
// The temporary vector returned by get_temp_vector() is destroyed here.
// 'unsafe_view' now holds dangling iterators.

// for (const auto& p : unsafe_view) { /* Undefined Behavior! */ }
```
To use `pairwise` with a temporary, you must first bind it to an lvalue whose lifetime is appropriate:
```cpp
auto my_data = get_temp_vector(); // my_data now owns the elements
auto safe_view = std_ext::pairwise(my_data); // Safe, my_data outlives safe_view
for (const auto& p : safe_view) { /* ... */ }
```

## API Reference

### `std_ext::pairwise(Iterable&& iterable)`
- **Template Parameters:**
    - `Iterable`: The type of the input sequence.
- **Parameters:**
    - `iterable`: An lvalue reference or rvalue reference to an iterable sequence.
- **Returns:** An instance of `std_ext::PairwiseIterView` which provides `begin()` and `end()` methods for iteration.
- **Note on rvalues:** As mentioned in "Lifetime Management", passing an rvalue temporary directly can lead to dangling iterators if the returned view outlives the scope of the full expression that created the temporary.

### `std_ext::pairwise(T (&array)[N])`
- **Template Parameters:**
    - `T`: The type of elements in the array.
    - `N`: The size of the array (deduced).
- **Parameters:**
    - `array`: A reference to a C-style array.
- **Returns:** An instance of `std_ext::PairwiseIterView<T*>` for iterating over the array.

### `std_ext::PairwiseIterView<IteratorType>`
- A view class returned by `pairwise`.
- **Member Functions:**
    - `begin() const`: Returns a `pairwise_iterator` to the beginning of the pairs.
    - `end() const`: Returns a `pairwise_iterator` to the end of the pairs.
    - `empty() const`: Returns `true` if no pairs can be formed (i.e., input sequence has < 2 elements), `false` otherwise.
    - `value_type`: The type of the elements yielded by the iterator (typically `std::pair<UnderlyingValueType, UnderlyingValueType>`).

### `std_ext::pairwise_iterator<IteratorType>`
- An iterator that yields pairs.
- **Member Types:** `iterator_category`, `value_type`, `difference_type`, `pointer`, `reference`.
- **Operations:** `operator*`, `operator++`, `operator==`, `operator!=`.
- `operator*()` returns `value_type` (a `std::pair`) by value. It throws `std::logic_error` if a non-dereferenceable iterator (e.g., an end iterator) is dereferenced.
This README provides an overview, feature list, usage examples, and important notes on lifetime management, along with a brief API reference. It should be sufficient for users to understand and use the `pairwise` utility.

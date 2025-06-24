# Peekable Iterator Wrapper (`peekable.h`)

## Overview

The `Peekable<Iterator>` class template, found in `utils/peekable.h`, provides a wrapper around an existing iterator (from any C++ standard container or custom iterators) that allows you to "peek" at the next element in the sequence without advancing the iterator, and then "next" (consume) it when ready. This is particularly useful for parsing, lookahead algorithms, or any situation where decisions need to be made based on the upcoming data before processing it.

The `Peekable` wrapper works with any iterator type that conforms to at least the InputIterator concept. For iterators that are ForwardIterators or stronger (e.g., BidirectionalIterator, RandomAccessIterator), an additional `peek_n(size_t n)` method is available to look ahead multiple steps.

## Features

- **Peek at Next Element**: `peek()` returns an `std::optional<ValueType>` of the next element without consuming it.
- **Consume Next Element**: `next()` returns an `std::optional<ValueType>` of the next element and advances the internal iterator.
- **Check for More Elements**: `has_next()` returns `true` if there are more elements to process.
- **Consume Without Return**: `consume()` advances the iterator without returning the element.
- **Iterator-like Interface**: Supports `operator*`, `operator++` (pre and post), `operator==`, and `operator!=` for iterator-style usage.
- **Peek Ahead (Conditional)**: `peek_n(size_t n)` allows looking `n` elements ahead. This is available only for ForwardIterators and stronger. Use `has_peek_n()` (static constexpr bool) to check if the current `Peekable` instantiation supports `peek_n`.
- **Underlying Iterator Access**: `base()` returns the current underlying iterator.
- **Convenience Factory**: `utils::make_peekable(container)` creates a `Peekable` from a container.
- **Range Support**: `utils::peekable_range(container)` can be used to adapt a container for use in scenarios expecting a range, though direct iteration on `Peekable` objects is more common.

## How to Use

### Include Header
```cpp
#include "peekable.h"
#include <vector>
#include <string>
#include <iostream>
```

### Basic Usage with `make_peekable`

```cpp
std::vector<int> data = {10, 20, 30};
auto p = utils::make_peekable(data);

while (p.has_next()) {
    std::optional<int> next_val = p.peek();
    if (next_val) {
        std::cout << "Peeking: " << *next_val << std::endl;
        // Make a decision based on *next_val
        if (*next_val == 20) {
            std::cout << "Found 20, consuming it." << std::endl;
        }
    }

    std::optional<int> consumed_val = p.next();
    if (consumed_val) {
        std::cout << "Consumed: " << *consumed_val << std::endl;
    }
}
// Output:
// Peeking: 10
// Consumed: 10
// Peeking: 20
// Found 20, consuming it.
// Consumed: 20
// Peeking: 30
// Consumed: 30
```

### Using with Different Iterator Types

`Peekable` can be constructed directly with begin and end iterators.

```cpp
std::string s = "hello";
utils::Peekable<std::string::iterator> p_str(s.begin(), s.end());

// Works with stream iterators too
std::istringstream iss("word1 word2");
utils::Peekable<std::istream_iterator<std::string>> p_stream(
    std::istream_iterator<std::string>(iss),
    std::istream_iterator<std::string>()
);
```

### Peeking Ahead with `peek_n` (for ForwardIterators and stronger)

```cpp
std::vector<int> nums = {1, 2, 3, 4, 5};
auto p_nums = utils::make_peekable(nums);

if (p_nums.has_next()) {
    std::cout << "Current: " << *p_nums.peek() << std::endl; // Current: 1

    if constexpr (decltype(p_nums)::has_peek_n()) { // Check if peek_n is available
        std::optional<int> next_plus_1 = p_nums.peek_n(1); // Peeks at '2'
        std::optional<int> next_plus_2 = p_nums.peek_n(2); // Peeks at '3'

        if (next_plus_1) {
            std::cout << "Next+1: " << *next_plus_1 << std::endl; // Next+1: 2
        }
        if (next_plus_2) {
            std::cout << "Next+2: " << *next_plus_2 << std::endl; // Next+2: 3
        }
    }
}
// Output:
// Current: 1
// Next+1: 2
// Next+2: 3
```
**Note**: `peek_n(0)` is equivalent to `peek()`. `peek_n(k)` peeks at the element `k` positions *after* the one that `peek()` would return (i.e., it skips `k` elements from the one `peek()` would look at).

### Iterator-Style Usage

```cpp
std::vector<std::string> words = {"one", "two", "three"};
auto p_words = utils::make_peekable(words);
auto p_end = utils::Peekable<std::vector<std::string>::iterator>(words.end(), words.end()); // End sentinel

for (; p_words != p_end; ++p_words) {
    std::cout << "Word: " << *p_words << std::endl; // *p_words calls peek()
}
// Output:
// Word: one
// Word: two
// Word: three
```
**Important**: When using iterator style, ensure your loop condition correctly checks against an "end" `Peekable` or use `has_next()` if more appropriate for the loop structure.

## API Summary

`Peekable<Iterator>`
- `Peekable(Iterator begin, Iterator end)`: Constructor.
- `bool has_next() const`: Checks if more elements are available.
- `std::optional<ValueType> peek() const`: Peeks at the next element.
- `std::optional<ValueType> next()`: Consumes and returns the next element.
- `void consume()`: Consumes the next element without returning it.
- `Iterator base() const`: Returns the underlying iterator to the *next* position to be processed.
- `ValueType operator*() const`: Dereferences (peeks).
- `Peekable& operator++()`: Pre-increment (consumes).
- `Peekable operator++(int)`: Post-increment (consumes).
- `bool operator==(const Peekable& other) const`: Compares based on underlying iterators.
- `bool operator!=(const Peekable& other) const`: Inverse of `operator==`.
- `static constexpr bool has_peek_n()`: Compile-time check if `peek_n` is available.
- `std::optional<ValueType> peek_n(size_t n) const`: (Conditional) Peeks `n` elements ahead.

Helper Functions (in `utils` namespace):
- `template<typename Container> auto make_peekable(Container& container)`
- `template<typename Container> auto make_peekable(const Container& container)`
- `template<typename Container> auto peekable_range(Container& container)`

## Considerations

- **Iterator Invalidation**: `Peekable` holds iterators to the original container. If the container is modified in a way that invalidates its iterators while a `Peekable` is active on it, the behavior is undefined.
- **Input Iterators**: For input iterators (e.g., `std::istream_iterator`), once an element is read (either by `peek()` or `next()`), it cannot be re-read. `peek()` buffers the element, so subsequent `peek()` calls return the buffered value until `next()` or `consume()` is called. `peek_n` is not available for input iterators.
- **Performance**: `peek()` and `next()` operations are generally efficient (O(1) amortized time after the first peek/next which might buffer). `peek_n(k)` might take O(k) for iterators that are not random access.
- **Return Type**: `peek()`, `next()`, and `peek_n()` return `std::optional<ValueType>`. Always check if the optional contains a value before dereferencing it.

## Use Cases

- **Parsers**: Lexers or parsers often need to look at the next token(s) to decide parsing rules.
- **Lookahead Algorithms**: Algorithms that make decisions based on upcoming data.
- **Data Processing Pipelines**: When processing a stream of data where an item might affect how subsequent items are handled.
- **Finite State Machines**: Transitioning states based on the current and next input symbols.
```

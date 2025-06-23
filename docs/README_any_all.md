# `any_of`, `all_of`, `none_of` Utility Functions

## Overview

The `any_all.h` header provides a set of utility functions (`any_of`, `all_of`, `none_of`) that operate on containers. These functions are similar in spirit to the standard C++ library algorithms (`std::any_of`, `std::all_of`, `std::none_of` found in `<algorithm>`) but offer specialized behavior for their single-argument versions.

The primary distinction of the single-argument versions in this header is their definition of "truthiness":
- For elements that have an `.empty()` method (e.g., `std::string`), an element is considered "truthy" if `!element.empty()` is true.
- For other types, an element is considered "truthy" if `static_cast<bool>(element)` is true (e.g., non-zero for numbers, non-null for pointers).

The two-argument versions (which take a predicate) behave identically to their standard library counterparts.

## Functions

### `any_of`

-   **`template<typename Container> bool any_of(const Container& container)`**
    -   Returns `true` if at least one element in the `container` is "truthy" according to the rules defined above.
    -   Returns `false` if the container is empty or all elements are "falsy".

-   **`template<typename Container, typename Predicate> bool any_of(const Container& container, Predicate pred)`**
    -   Returns `true` if the predicate `pred` returns `true` for at least one element in the `container`.
    -   Equivalent to `std::any_of(container.begin(), container.end(), pred)`.

### `all_of`

-   **`template<typename Container> bool all_of(const Container& container)`**
    -   Returns `true` if all elements in the `container` are "truthy" according to the rules defined above.
    -   Returns `true` if the container is empty.
    -   Returns `false` if at least one element is "falsy".

-   **`template<typename Container, typename Predicate> bool all_of(const Container& container, Predicate pred)`**
    -   Returns `true` if the predicate `pred` returns `true` for all elements in the `container`.
    -   Equivalent to `std::all_of(container.begin(), container.end(), pred)`.

### `none_of`

-   **`template<typename Container> bool none_of(const Container& container)`**
    -   Returns `true` if no elements in the `container` are "truthy" (i.e., all elements are "falsy") according to the rules defined above.
    -   Returns `true` if the container is empty.

-   **`template<typename Container, typename Predicate> bool none_of(const Container& container, Predicate pred)`**
    -   Returns `true` if the predicate `pred` returns `false` for all elements in the `container`.
    -   Equivalent to `std::none_of(container.begin(), container.end(), pred)`.

## Helper Traits

The implementation uses the following SFINAE helper traits:
- `has_empty_method<T>`: Detects if type `T` has an `empty()` member function.

## Usage Examples

(Based on `examples/any_all_example.cpp`)

### Basic Usage (Implicit Truthiness)

```cpp
#include "any_all.h" // Assumes this header is available
#include <iostream>
#include <vector>
#include <string>

int main() {
    // With integers (0 is falsy, non-zero is truthy)
    std::vector<int> numbers1 = {0, 0, 0};
    std::vector<int> numbers2 = {0, 1, 0};
    std::vector<int> numbers3 = {1, 2, 3};
    std::vector<int> empty_numbers = {};

    std::cout << "any_of({0,0,0}): " << std::boolalpha << any_of(numbers1) << std::endl; // false
    std::cout << "all_of({0,0,0}): " << std::boolalpha << all_of(numbers1) << std::endl; // false
    std::cout << "none_of({0,0,0}): " << std::boolalpha << none_of(numbers1) << std::endl; // true

    std::cout << "any_of({0,1,0}): " << std::boolalpha << any_of(numbers2) << std::endl; // true
    std::cout << "all_of({0,1,0}): " << std::boolalpha << all_of(numbers2) << std::endl; // false

    std::cout << "any_of({1,2,3}): " << std::boolalpha << any_of(numbers3) << std::endl; // true
    std::cout << "all_of({1,2,3}): " << std::boolalpha << all_of(numbers3) << std::endl; // true

    std::cout << "any_of({}): " << std::boolalpha << any_of(empty_numbers) << std::endl; // false
    std::cout << "all_of({}): " << std::boolalpha << all_of(empty_numbers) << std::endl; // true
    std::cout << "none_of({}): " << std::boolalpha << none_of(empty_numbers) << std::endl; // true

    // With strings (empty string is falsy, non-empty is truthy)
    std::vector<std::string> words1 = {"", "", ""};
    std::vector<std::string> words2 = {"hello", "", "world"};
    std::vector<std::string> words3 = {"hello", "world"};

    std::cout << "any_of({\"\", \"\", \"\"}): " << std::boolalpha << any_of(words1) << std::endl; // false
    std::cout << "all_of({\"\", \"\", \"\"}): " << std::boolalpha << all_of(words1) << std::endl; // false

    std::cout << "any_of({\"hello\", \"\", \"world\"}): " << std::boolalpha << any_of(words2) << std::endl; // true
    std::cout << "all_of({\"hello\", \"\", \"world\"}): " << std::boolalpha << all_of(words2) << std::endl; // false

    std::cout << "any_of({\"hello\", \"world\"}): " << std::boolalpha << any_of(words3) << std::endl; // true
    std::cout << "all_of({\"hello\", \"world\"}): " << std::boolalpha << all_of(words3) << std::endl; // true
}
```

### Usage with Predicates

```cpp
#include "any_all.h"
#include <iostream>
#include <vector>
#include <string>

int main() {
    std::vector<int> numbers = {1, 2, 3, 4, 5, 6};
    auto is_even = [](int x) { return x % 2 == 0; };
    auto is_large = [](int x) { return x > 10; };

    std::cout << "any_of(numbers, is_even): " << std::boolalpha << any_of(numbers, is_even) << std::endl; // true
    std::cout << "all_of(numbers, is_even): " << std::boolalpha << all_of(numbers, is_even) << std::endl; // false
    std::cout << "none_of(numbers, is_even): " << std::boolalpha << none_of(numbers, is_even) << std::endl; // false

    std::cout << "any_of(numbers, is_large): " << std::boolalpha << any_of(numbers, is_large) << std::endl; // false
    std::cout << "none_of(numbers, is_large): " << std::boolalpha << none_of(numbers, is_large) << std::endl; // true

    std::vector<std::string> words = {"apple", "banana", "kiwi"};
    auto has_letter_a = [](const std::string& s){ return s.find('a') != std::string::npos; };

    std::cout << "any_of(words, has_letter_a): " << std::boolalpha << any_of(words, has_letter_a) << std::endl; // true
    std::cout << "all_of(words, has_letter_a): " << std::boolalpha << all_of(words, has_letter_a) << std::endl; // false (kiwi)
}

```

## Dependencies
- `<iostream>` (used in commented-out demo code in header, not strictly by functions)
- `<vector>` (used in commented-out demo code and examples)
- `<algorithm>` (for `std::any_of`, `std::all_of`, `std::none_of`)
- `<functional>` (not directly used by these functions, but often useful with predicates)
- `<type_traits>` (for `std::true_type`, `std::false_type`, `std::declval`, `std::void_t`)

These utilities provide a convenient way to check conditions across container elements, especially when a more nuanced definition of "truthiness" (like treating empty strings as false) is desired for the single-argument versions.

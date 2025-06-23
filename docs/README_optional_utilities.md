# `pipeline::optional` - Standalone Optional Utilities

## Overview

The `optional_utilities.h` header file provides a collection of standalone utility functions (combinators) designed for working with `std::optional` values in a functional style. These functions facilitate common operations such as transforming the value within an optional, chaining operations that may themselves return an optional, filtering, providing default values, and more.

These utilities return callable objects (typically lambdas) that take an `std::optional` as input and produce a result, often another `std::optional`. They are the building blocks that can be used for manual composition or with a separate pipelining mechanism.

**Note:** A related header, `optional_pipeline.h`, provides a more comprehensive set of these utilities, often with performance optimizations and additional features, along with a `pipe().then()` chaining mechanism. `optional_utilities.h` appears to contain a subset or earlier versions of these combinators.

## Namespace
All utilities are within the `pipeline::optional` namespace.

## Key Utility Functions

These functions typically return a lambda that expects an `std::optional` as its argument.

### Transformations & Chaining
-   **`map(F&& func)`**:
    -   If the input `std::optional<T>` has a value, applies `func(value)` and returns `std::optional<ResultType>`.
    -   If input is `std::nullopt`, returns `std::nullopt`.
    -   `ResultType` is `std::invoke_result_t<F, T>`.
-   **`and_then(F&& func)`**:
    -   If input `std::optional<T>` has a value, applies `func(value)`. `func` must return a `std::optional<U>`.
    -   Propagates `std::nullopt`. Useful for chaining operations where each step can fail.
-   **`flatten(const std::optional<std::optional<T>>& nested_opt)`**:
    -   Converts `std::optional<std::optional<T>>` to `std::optional<T>`.

### Filtering & Value Access
-   **`filter(F&& predicate)`**:
    -   If input `std::optional<T>` has a value and `predicate(value)` is true, returns the original optional.
    -   Otherwise, returns `std::nullopt`.
-   **`value_or(DefaultType&& default_value)`**:
    -   If input `std::optional<T>` has a value, returns `T value`.
    -   Otherwise, returns `default_value` (converted to `T`).
-   **`expect<Exception = std::runtime_error>(const std::string& message)`**:
    -   If input `std::optional<T>` has a value, returns `T value`.
    -   Otherwise, throws an `Exception` (default `std::runtime_error`) with the given `message`.

### Creation & Side Effects
-   **`some(T&& value)`**: Creates `std::optional<std::decay_t<T>>{value}`.
-   **`none<T>()`**: Creates `std::optional<T>{std::nullopt}`.
-   **`tap(F&& func)`**:
    -   If input `std::optional<T>` has a value, applies `func(value)` (typically for side effects like logging).
    -   Returns the original input optional, regardless of what `func` returns.

### Combination & Lifting
-   **`lift(F&& func)`**:
    -   "Lifts" a regular function `func` (that takes non-optional arguments) to operate on `std::optional` arguments.
    -   If all input optionals to the lifted function have values, applies `func` to these values and returns `std::optional` of the result.
    -   Otherwise, returns `std::nullopt`.
-   **`zip_with(F&& func)`**:
    -   A curried function. `zip_with(binary_func)(opt1)(opt2)` applies `binary_func` to the values of `opt1` and `opt2` if both have values, returning an `std::optional` of the result. Otherwise `std::nullopt`.

### Specialized Utilities
-   **`safe_divide(double denominator)`**: Returns a lambda `(double numerator) -> std::optional<double>`. Returns `std::nullopt` if `denominator` is zero.
-   **`safe_parse<T = int>()`**: Returns a lambda `(const std::string& s) -> std::optional<T>`. Attempts to parse `s` into type `T` (supports `int`, `long`, `float`, `double`). Returns `std::nullopt` on failure.

### Validation
-   **`validate(F&& predicate, const std::string& error_msg = "Validation failed")`**:
    -   If input `value` satisfies `predicate(value)`, returns `std::optional<value>`.
    -   Otherwise, returns `std::nullopt`. (Note: `error_msg` is not used by this version to throw an exception, it just returns nullopt on failure).
-   **`validate_range(T min_val, T max_val)`**: Validates if a numeric value is within `[min_val, max_val]`.
-   **`validate_non_empty()`**: Validates if a string is not empty.
-   **`validate_email()`**: Basic check for '@' and '.' in a string.

### Error Handling & Control Flow
-   **`try_optional(F&& func)`**:
    -   Wraps a call to `func(args...)`. If `func` executes successfully, returns `std::optional` of its result.
    -   If `func` throws any exception, returns `std::nullopt`.
-   **`match(OnSome&& on_some, OnNone&& on_none)`**:
    -   Takes two callables. If the input optional has a value, calls `on_some(value)`. Otherwise, calls `on_none()`.
    -   Both `on_some` and `on_none` must return values of the same type.

## Usage Examples

Since this header provides the combinator functions but not the `pipe().then()` mechanism directly, examples show manual application or simple chaining.

### `map` and `value_or`

```cpp
#include "optional_utilities.h" // Adjust path
#include <iostream>
#include <string>
#include <optional>

int main() {
    using namespace pipeline::optional;

    std::optional<int> opt_num = 42;
    auto doubler = map([](int x) { return x * 2; });
    auto stringifier = map([](int x) { return std::to_string(x); });
    auto get_default_str = value_or(std::string("N/A"));

    std::optional<int> doubled_opt = doubler(opt_num);          // optional<84>
    std::optional<std::string> str_opt = stringifier(doubled_opt); // optional<"84">
    std::string final_val = get_default_str(str_opt);           // "84"
    std::cout << "Result: " << final_val << std::endl;

    std::optional<int> empty_opt = std::nullopt;
    final_val = get_default_str(stringifier(doubler(empty_opt))); // "N/A"
    std::cout << "Result for empty: " << final_val << std::endl;
}
```

### `and_then` for Failable Chain

```cpp
#include "optional_utilities.h"
#include <iostream>
#include <string>
#include <optional>
#include <cmath> // For std::sqrt

// Function that might fail (returns optional)
std::optional<double> safe_sqrt(double x) {
    if (x >= 0) return std::sqrt(x);
    return std::nullopt;
}

std::optional<double> ensure_positive(double x) {
    if (x > 0) return x;
    return std::nullopt;
}

int main() {
    using namespace pipeline::optional;

    std::optional<double> input1 = 16.0;
    auto processing_chain = and_then(safe_sqrt);
    auto positive_check = and_then(ensure_positive);

    std::optional<double> result1 = positive_check(processing_chain(input1)); // sqrt(16) = 4; 4 > 0 is true. result1 = optional<4.0>
    if(result1) std::cout << "Chain for 16.0: " << *result1 << std::endl;

    std::optional<double> input2 = -9.0;
    std::optional<double> result2 = positive_check(processing_chain(input2)); // safe_sqrt(-9) is nullopt. result2 = nullopt
    if(!result2) std::cout << "Chain for -9.0 resulted in nullopt." << std::endl;

    std::optional<double> input3 = 0.0; // sqrt(0) = 0. ensure_positive(0) is nullopt.
    std::optional<double> result3 = positive_check(processing_chain(input3));
    if(!result3) std::cout << "Chain for 0.0 resulted in nullopt (due to ensure_positive)." << std::endl;
}
```

### `validate` and `safe_parse`

```cpp
#include "optional_utilities.h"
#include <iostream>
#include <string>
#include <optional>

int main() {
    using namespace pipeline::optional;

    auto parse_int = safe_parse<int>();
    auto validate_positive_int = validate([](int x){ return x > 0; });

    std::string s_num = "123";
    std::optional<int> parsed_opt = parse_int(s_num);        // optional<123>
    std::optional<int> validated_opt = validate_positive_int(*parsed_opt); // optional<123>

    if (validated_opt) {
        std::cout << "Validated positive number: " << *validated_opt << std::endl;
    }

    s_num = "abc";
    parsed_opt = parse_int(s_num); // nullopt
    // validated_opt = validate_positive_int(*parsed_opt); // Would crash if parsed_opt is nullopt
    if (!parsed_opt) {
        std::cout << "'abc' could not be parsed to int." << std::endl;
    }
}
```

## Dependencies
- `<optional>`, `<functional>`, `<type_traits>`, `<string>`, `<stdexcept>`, `<regex>` (though regex is not used in this specific header's version of `validate_email`).

These utilities provide fundamental building blocks for robustly handling optional values in C++, promoting a more functional and explicit style of error and state management.

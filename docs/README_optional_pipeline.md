# `pipeline::optional` - Optional Chaining Utilities

## Overview

The `optional_pipeline.h` header provides a suite of utilities designed for fluent, functional-style manipulation of `std::optional` values. It allows developers to chain operations on optionals in a way that gracefully handles the empty (`std::nullopt`) state, similar to monadic operations found in functional programming languages or features in libraries like C# LINQ or JavaScript Promises.

The core idea is to start a "pipeline" with an optional value using `pipeline::optional::pipe()` and then chain various transforming, filtering, or handling functions using the `.then()` method.

## Namespace
All utilities related to optional pipelining are within the `pipeline::optional` namespace.

## Core Pipelining Mechanism

-   **`pipeline_wrapper<T>`**: A simple internal wrapper for the current value in the pipeline (usually an `std::optional<...>`).
-   **`pipe(T&& value)`**: The entry point function. It takes an initial value (e.g., `std::optional<int>`) and wraps it to start a pipeline.
-   **`.then(F&& func)`**: A method of `pipeline_wrapper`. It applies the function `func` to the currently wrapped value and returns a new `pipeline_wrapper` containing the result. `func` is typically one of the combinator functions provided by this header (like `map`, `and_then`, `filter`, etc.).
-   **`.get()`**: A method of `pipeline_wrapper` to extract the final value from the pipeline. Can also implicitly convert to the underlying type.

## Key Combinator Functions

These functions are designed to be passed to the `.then()` method of a pipeline. They typically return a lambda that operates on an `std::optional`.

### Transformations
-   **`map(F&& func)`**: If the input optional has a value, applies `func` to it and returns `std::optional<ResultType>`. If input is `std::nullopt` or `func` throws, returns `std::nullopt`. Handles `void` return from `func` by returning `std::optional<std::monostate>`.
-   **`and_then(F&& func)`**: If input optional has a value, applies `func` (which must return a `std::optional<U>`) to the value and returns its result. Propagates `std::nullopt` or exceptions from `func` as `std::nullopt`.
-   **`and_then_lazy(F&& func)`**: Similar to `and_then`, but returns a callable that, when invoked, performs the operation. Useful for deferring expensive computations.
-   **`flatten(std::optional<std::optional<T>>&& nested_opt)`**: Converts `std::optional<std::optional<T>>` to `std::optional<T>`.

### Filtering & Value Access
-   **`filter(F&& predicate)`**: If input optional has value and `predicate(value)` is true, returns the optional. Otherwise, returns `std::nullopt`.
-   **`value_or(T&& default_value)`**: If input optional has value, returns it. Otherwise, returns `default_value`.
-   **`expect<Exception = std::runtime_error>(std::string_view message)`**: If input optional has value, returns it. Otherwise, throws `Exception` with `message`.

### Creation & Side Effects
-   **`some(T&& value)`**: Creates `std::optional<std::decay_t<T>>{value}`.
-   **`none<T>()`**: Creates `std::optional<T>{std::nullopt}`.
-   **`tap(F&& func)`**: If input optional has value, applies `func` to it (for side effects) and returns the original optional.

### Combination & Lifting
-   **`lift(F&& func)`**: "Lifts" a regular function `func` (taking non-optional args) to operate on `std::optional` args. If all input optionals have values, applies `func` and returns `std::optional` of the result; else `std::nullopt`.
-   **`zip_with(F&& func)`**: Curried function. `zip_with(binary_func)(opt1)(opt2)` applies `binary_func` if `opt1` and `opt2` both have values.

### Specialized Utilities
-   **`safe_divide(T denominator)`**: Returns a lambda for safe division.
-   **`safe_parse<T>()`**: Returns a lambda for safe string-to-numeric parsing.

### Validation
-   **`validate(F&& predicate, std::string_view error_msg)`**: Generic validation.
-   **`validate_range(T min_val, T max_val)`**: Checks if a numeric value is in a range.
-   **`validate_non_empty()`**: Checks if a string is not empty.
-   **`validate_email()` / `validate_url()`**: Regex-based validation for emails/URLs.

### Advanced
-   **`try_optional<F, ...ExceptionTypes>(F&& func)`**: Wraps `func` call. Returns `std::optional` of result, or `std::nullopt` if specified exceptions (or any if none specified) are caught.
-   **`match(OnSome&& on_some, OnNone&& on_none)`**: Pattern matching on optional; executes one of two functions based on state. Both must return same type.
-   **`transform_if(Predicate&& pred, Transform&& transform)`**: Conditional mapping.
-   **`collect<Container>()`**: Collects values from multiple optionals into a container.

## Usage Examples

(Based on `examples/optional_pipeline_use.cpp`)

### Basic Chaining with `map` and `value_or`

```cpp
#include "optional_pipeline.h" // Adjust path as needed
#include <iostream>
#include <string>
#include <optional>

int main() {
    using namespace pipeline::optional;

    std::optional<int> opt_val = 42;

    std::string result = pipe(opt_val)
        .then(map([](int x) { return x / 2; }))       // opt_val becomes optional<21>
        .then(map([](int x) { return std::to_string(x); })) // opt_val becomes optional<"21">
        .then(value_or(std::string("N/A")))        // Extracts "21"
        .get();                                     // Gets the final string

    std::cout << "Result 1: " << result << std::endl; // Output: Result 1: 21

    std::optional<int> empty_opt = std::nullopt;
    result = pipe(empty_opt)
        .then(map([](int x) { return std::to_string(x); })) // Skipped
        .then(value_or(std::string("Default Value")))      // Uses "Default Value"
        .get();

    std::cout << "Result 2: " << result << std::endl; // Output: Result 2: Default Value
}
```

### Using `and_then` and `filter`

```cpp
#include "optional_pipeline.h"
#include <iostream>
#include <string>
#include <optional>
#include <cmath> // For std::sqrt

// Function that returns an optional
std::optional<double> safe_sqrt(double x) {
    if (x >= 0) {
        return std::sqrt(x);
    }
    return std::nullopt;
}

int main() {
    using namespace pipeline::optional;

    std::optional<double> input = 16.0;

    std::optional<double> output = pipe(input)
        .then(and_then(safe_sqrt)) // Input: optional<16.0> -> Output of safe_sqrt: optional<4.0>
        .then(filter([](double x) { return x > 3.0; })) // 4.0 > 3.0 is true, so optional<4.0> passes
        .then(map([](double x) { return x * 10.0; }))   // optional<4.0> -> optional<40.0>
        .get();

    if (output) {
        std::cout << "Processed output: " << *output << std::endl; // Output: 40.0
    }

    input = -9.0;
    output = pipe(input)
        .then(and_then(safe_sqrt)) // safe_sqrt(-9.0) returns nullopt
        .then(filter([](double x) { return x > 3.0; })) // Skipped
        .get();

    if (!output) {
        std::cout << "Processing -9.0 resulted in nullopt, as expected." << std::endl;
    }
}
```

### Validation Pipeline

```cpp
#include "optional_pipeline.h"
#include <iostream>
#include <string>
#include <optional>

int main() {
    using namespace pipeline::optional;
    std::string email_to_validate = "test@example.com";

    std::optional<std::string> validated_email = pipe(some(email_to_validate))
        .then(validate_non_empty())    // Check if not empty
        .then(validate_email())        // Check basic email format
        .then(map([](const std::string& s) { // Transform if valid
            std::string lower_s = s;
            std::transform(lower_s.begin(), lower_s.end(), lower_s.begin(), ::tolower);
            return lower_s;
        }))
        .get();

    if (validated_email) {
        std::cout << "Valid and processed email: " << *validated_email << std::endl;
    } else {
        std::cout << "Email '" << email_to_validate << "' is invalid or empty." << std::endl;
    }

    email_to_validate = "invalid";
    validated_email = pipe(some(email_to_validate))
        .then(validate_email())
        .get();
    if (!validated_email) {
        std::cout << "Email '" << email_to_validate << "' correctly identified as invalid." << std::endl;
    }
}
```

## Dependencies
- `<optional>`, `<functional>`, `<type_traits>`, `<string>`, `<stdexcept>`, `<regex>`, `<concepts>` (C++20, used for some internal concepts, but library aims for C++17 compatibility broadly).

The `pipeline::optional` utilities provide a powerful and expressive way to work with `std::optional`, reducing boilerplate and improving the clarity of code that deals with potentially missing values.

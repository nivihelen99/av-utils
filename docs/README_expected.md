# `aos_utils::Expected<T, E>`

## Overview

The `expected.h` header provides the `aos_utils::Expected<T, E>` class template, a type used to represent the outcome of an operation that can either succeed with a value of type `T` or fail with an error of type `E`. This is similar in spirit and functionality to `std::expected` (standardized in C++23) and offers a robust way to handle errors without relying on exceptions for expected failure conditions or using special return values like null pointers or error codes.

It can hold either an expected value (`T`) or an unexpected error (`E`). A common default for `E` is `std::string`. A specialization `Expected<void, E>` is provided for operations that don't return a value on success but can still fail.

## Core Components

-   **`aos_utils::Unexpected<E>`**: A wrapper class for error types. It's used to explicitly construct an `Expected` in its error state.
    -   Factory: `aos_utils::make_unexpected(error_value)`

-   **`aos_utils::Expected<T, E>`**: The main class template.
    -   If `T` is not `void`, it internally uses `std::variant<T, E>`.
    -   If `T` is `void`, it uses a specialized implementation storing either nothing (for success) or an `E` (for error).

## Features

-   **Explicit Error Handling:** Makes success and error paths explicit in function signatures and call sites.
-   **Value or Error Storage:** Safely stores either the expected value or an error object.
-   **State Checking:** `has_value()` and `explicit operator bool()` to check if it holds a value or an error.
-   **Safe Value/Error Access:**
    -   `value()`: Accesses the contained value; throws `std::bad_variant_access` (or similar for `void` specialization) if it holds an error.
    -   `error()`: Accesses the contained error; throws if it holds a value.
    -   `operator*()` and `operator->()`: Provide direct (unchecked) access to the value. Behavior is undefined if it holds an error.
    -   `value_or(default_val)`: Returns the value if present, otherwise a provided default.
-   **Monadic Operations:** Supports functional composition of failable operations:
    -   `map(Func f)`: Applies `f` to the contained value if present, returning a new `Expected` with the transformed value or propagating the error.
    -   `and_then(Func f)`: If it holds a value, applies `f` (which must return another `Expected`) to the value and returns the result. Propagates errors.
    -   `or_else(Func f)`: If it holds an error, applies `f` (which must return another `Expected`) to the error and returns the result. Propagates values.
    -   `map_error(Func f)`: If it holds an error, applies `f` to the error, returning a new `Expected` with the transformed error or propagating the value.
-   **Exception Wrapping:** `aos_utils::make_expected(Func&& func)` factory function can execute a callable and wrap its normal return in an `Expected` or catch exceptions and return them as an `Expected` error.
-   **STL Compatibility:** Provides type aliases, comparison operators, and a swap function.

## Public Interface Highlights (`Expected<T, E>`)

### Construction
```cpp
Expected<int, std::string> ex_val = 42; // From value
Expected<int, std::string> ex_err = make_unexpected("An error occurred"); // From Unexpected
Expected<MyType, MyError> ex_emplace(std::in_place, arg1, arg2); // In-place value
Expected<MyType, MyError> ex_err_emplace(std::in_place_type_t<Unexpected<MyError>>{}, err_arg); // In-place error
Expected<void, std::string> ex_void_ok(std::in_place); // For void success
```

### State & Access
-   **`bool has_value() const noexcept` / `explicit operator bool() const noexcept`**
-   **`T& value()` / `const T& value() const`** (throws if error)
-   **`E& error()` / `const E& error() const`** (throws if value)
-   **`T& operator*()` / `const T& operator*() const`** (unchecked value access)
-   **`T* operator->()` / `const T* operator->() const`** (unchecked value access)
-   **`T value_or(U&& default_value) const`**

### Monadic Interface
-   **`map(Func f)`**
-   **`and_then(Func f)`**
-   **`or_else(Func f)`**
-   **`map_error(Func f)`**

### Comparison & Swap
-   `operator==`, `operator!=` (with other `Expected`, values, or `Unexpected`)
-   `swap(Expected& other)` (member and non-member)

## Usage Examples

(Based on `examples/use_expected.cpp`)

### Basic Success and Error Handling

```cpp
#include "expected.h"
#include <iostream>
#include <string>

using namespace aos_utils;

Expected<int, std::string> parse_number(const std::string& s) {
    try {
        return std::stoi(s);
    } catch (const std::exception& e) {
        return make_unexpected(std::string("Parse error: ") + e.what());
    }
}

int main() {
    Expected<int, std::string> result1 = parse_number("123");
    if (result1) { // or result1.has_value()
        std::cout << "Parsed value: " << *result1 << std::endl; // 123
    }

    Expected<int, std::string> result2 = parse_number("abc");
    if (!result2) {
        std::cout << "Error: " << result2.error() << std::endl; // Parse error...
    }
    std::cout << "Value or default for result2: " << result2.value_or(0) << std::endl; // 0
}
```

### Chaining Operations with Monadic Methods

```cpp
#include "expected.h"
#include <iostream>
#include <string>
#include <cmath> // For std::sqrt

using namespace aos_utils;

// Assume parse_number from previous example

Expected<double, std::string> safe_sqrt(double val) {
    if (val < 0) {
        return make_unexpected("Cannot take sqrt of negative number");
    }
    return std::sqrt(val);
}

int main() {
    std::string input = "16";

    auto computation = parse_number(input) // Expected<int, std::string>
        .and_then([](int n) {
            std::cout << "Parsed to: " << n << std::endl;
            return safe_sqrt(static_cast<double>(n)); // Returns Expected<double, std::string>
        })
        .map([](double d) {
            std::cout << "Sqrt is: " << d << std::endl;
            return d * 10.0; // Returns Expected<double, std::string>
        });

    if (computation) {
        std::cout << "Final result: " << *computation << std::endl; // 40.0
    } else {
        std::cout << "Computation failed: " << computation.error() << std::endl;
    }

    // Example with an error path
    input = "-9";
    auto error_computation = parse_number(input) // Expected<int, std::string> (success with -9)
        .and_then(safe_sqrt)                     // Fails here: safe_sqrt(-9) returns error
        .map([](double d) { return d * 10.0; }); // This map is skipped

    if (!error_computation) {
        std::cout << "Computation with '-9' failed: " << error_computation.error() << std::endl;
    }
}
```

### `Expected<void, E>`

```cpp
#include "expected.h"
#include <iostream>
#include <fstream> // For std::ofstream

using namespace aos_utils;

Expected<void, std::string> write_to_file(const std::string& filename, const std::string& content) {
    std::ofstream ofs(filename);
    if (!ofs) {
        return make_unexpected("Failed to open file: " + filename);
    }
    ofs << content;
    if (!ofs) {
        return make_unexpected("Failed to write to file: " + filename);
    }
    return Expected<void, std::string>(std::in_place); // Indicate success
}

int main() {
    auto result = write_to_file("output.txt", "Hello, Expected<void>!");
    if (result) {
        std::cout << "Successfully wrote to file." << std::endl;
    } else {
        std::cout << "File write error: " << result.error() << std::endl;
    }
}
```

## Dependencies
- `<variant>` (for `Expected<T,E>`), `<stdexcept>`, `<string>`, `<type_traits>`, `<utility>`, `<functional>`, `<ostream>` (for example usage/debugging in header), `<map>` (unused, possibly leftover), `<iostream>` (unused directly by core logic).

`aos_utils::Expected` provides a powerful and modern approach to error handling in C++, promoting clarity and robustness by making potential failures an explicit part of a function's return type.

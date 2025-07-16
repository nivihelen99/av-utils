# `value_or_error<T, E>`

The `cxxds::value_or_error<T, E>` is a template class that holds either a value of type `T` or an error of type `E`. It is useful for functions that can fail, as it allows them to return either a result or a descriptive error without resorting to exceptions or out-of-band error signaling.

## Usage

To use `value_or_error`, include the header `value_or_error.h`.

```cpp
#include "value_or_error.h"
```

### Creating a `value_or_error`

You can create a `value_or_error` with either a value or an error:

```cpp
cxxds::value_or_error<int, std::string> success(42);
cxxds::value_or_error<int, std::string> failure("An error occurred");
```

### Checking for a value or an error

You can check whether a `value_or_error` contains a value or an error using the `has_value()` and `has_error()` methods:

```cpp
if (success.has_value()) {
    // ...
}

if (failure.has_error()) {
    // ...
}
```

### Accessing the value or error

You can access the value or error using the `value()` and `error()` methods. These methods will throw a `std::logic_error` if you try to access a value when there is an error, or vice versa.

```cpp
int v = success.value();
std::string e = failure.error();
```

## Example

Here is an example of a function that uses `value_or_error` to parse an integer from a string:

```cpp
#include "value_or_error.h"
#include <iostream>
#include <string>

cxxds::value_or_error<int, std::string> parse_int(const std::string& s) {
    try {
        return std::stoi(s);
    } catch (const std::invalid_argument& e) {
        return "Invalid argument";
    } catch (const std::out_of_range& e) {
        return "Out of range";
    }
}

int main() {
    auto result1 = parse_int("123");
    if (result1.has_value()) {
        std::cout << "Parsed integer: " << result1.value() << std::endl;
    } else {
        std::cout << "Error: " << result1.error() << std::endl;
    }

    auto result2 = parse_int("abc");
    if (result2.has_value()) {
        std::cout << "Parsed integer: " << result2.value() << std::endl;
    } else {
        std::cout << "Error: " << result2.error() << std::endl;
    }

    return 0;
}
```

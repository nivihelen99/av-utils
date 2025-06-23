# `enum_reflect` - Enum Reflection Utilities

## Overview

The `enum_reflect.h` header provides a suite of utilities for performing compile-time and runtime reflection on C++ enumeration types. This allows developers to:

-   Convert enum values to their string representations (names).
-   Convert string names back to enum values.
-   Iterate over all defined values of an enum.
-   Get an array of all enum value names.
-   Get the count of defined enum values.
-   Stream enum values directly to output streams (e.g., `std::cout`).

The core mechanism relies on compiler-specific intrinsics (like `__PRETTY_FUNCTION__` or `__FUNCSIG__`) to extract enum names, making many operations `constexpr` where possible.

**Note:** This implementation typically inspects underlying enum values in a predefined range (e.g., 0-255) to discover enumerators. This means it works best for enums whose values fall within this range and may not correctly reflect all enumerators if their values are outside this range or if the enum is not densely packed (though it attempts to filter valid names).

## Namespace
All utilities are within the `enum_reflect` namespace.

## Core Components

### `enum_reflect::enum_info<E>`
A class template that provides static methods for reflecting on enum type `E`.
-   `static constexpr std::string_view name(E value)`: Get the string name of an enum value.
-   `template<E V> static constexpr std::string_view name()`: Get the string name of a compile-time enum value.
-   `static constexpr std::optional<E> from_name(std::string_view name)`: Get an enum value from its string name.
-   `static constexpr auto values()`: Returns a `std::array<E, count>` of all valid enum values.
-   `static constexpr auto names()`: Returns a `std::array<std::string_view, count>` of all valid enum names.
-   `static constexpr std::size_t size()`: Returns the count of valid enum values.
-   `static constexpr bool is_valid(E value)`: Checks if an enum value has a valid reflected name.
-   `to_string(E value)` / `from_string(std::string_view str)`: Aliases for `name()` and `from_name()`.

### Convenience Functions
Free functions that wrap `enum_info<E>` methods:
-   `enum_reflect::enum_name(E value)`
-   `enum_reflect::enum_from_name<E>(std::string_view name)`
-   `enum_reflect::enum_values<E>()`
-   `enum_reflect::enum_names<E>()`
-   `enum_reflect::enum_size<E>()`
-   `enum_reflect::is_valid_enum(E value)`

### Iteration Support
-   **`enum_reflect::enum_range<E>()`**: Returns a range object that can be used in a range-based for loop to iterate over the values of enum `E`.
    ```cpp
    for (MyEnum val : enum_reflect::enum_range<MyEnum>()) {
        // ... use val ...
    }
    ```

### Stream Output
-   **`std::ostream& operator<<(std::ostream& os, E value)`**: Allows direct printing of enum values to output streams, which will output their string names.

## Usage Examples

(Adapted from the example within `enum_reflect.h`)

```cpp
#include "enum_reflect.h" // Or your actual include path
#include <iostream>
#include <string>
#include <optional>

// Define some enums
enum class Status {
    PENDING = 0,
    RUNNING = 1,
    COMPLETE = 2,
    FAILED = 10 // Example of a non-contiguous value
    // Assumes reflection checks up to a certain underlying value (e.g., 255)
};

enum class LogLevel : char { // Example with a different underlying type
    DEBUG = 'D',
    INFO = 'I',
    WARNING = 'W',
    ERROR = 'E'
    // Note: Current reflection in header might be limited to underlying values 0-255.
    // This LogLevel example might not work perfectly with the 0-255 range assumption
    // if char values are outside this or if not mapped as integers.
    // For the provided header, it's best to use enums with default underlying types
    // or ensure underlying values are small positive integers.
};

// A more typical enum for the provided reflection utilities
enum class Color {
    RED,
    GREEN,
    BLUE,
    YELLOW
    // No explicit COUNT needed if relying on the iteration up to 255.
    // If a COUNT was used by other utilities like EnumMap, it would be here.
};


int main() {
    using namespace enum_reflect;

    // 1. Get enum value name
    std::cout << "Name of Status::RUNNING: " << enum_name(Status::RUNNING) << std::endl; // "RUNNING"
    std::cout << "Name of Status::FAILED: " << enum_name(Status::FAILED) << std::endl;   // "FAILED"

    // For Color enum
    std::cout << "Name of Color::GREEN: " << enum_name(Color::GREEN) << std::endl; // "GREEN"


    // 2. Convert string name to enum value
    std::optional<Status> status_val = enum_from_name<Status>("COMPLETE");
    if (status_val) {
        std::cout << "String \"COMPLETE\" to Status: "
                  << static_cast<int>(*status_val) << std::endl; // Outputs underlying int value
    } else {
        std::cout << "String \"COMPLETE\" not found in Status." << std::endl;
    }

    std::optional<Status> invalid_status = enum_from_name<Status>("UNKNOWN_STATUS");
    if (!invalid_status) {
        std::cout << "String \"UNKNOWN_STATUS\" correctly not found." << std::endl;
    }

    // 3. Iterate over enum values using enum_range
    std::cout << "\nIterating over Status enum values:" << std::endl;
    for (Status s : enum_range<Status>()) {
        std::cout << "  Value: " << enum_name(s)
                  << " (Underlying: " << static_cast<int>(s) << ")" << std::endl;
    }
    // Note: This will iterate values 0,1,2,... up to the internal limit (e.g. 255)
    // and `enum_name` will return "UNKNOWN" for values not explicitly named or if `has_valid_name` fails.
    // The `enum_range` in conjunction with `enum_values` is designed to iterate only valid, named values.

    std::cout << "\nIterating over Color enum values:" << std::endl;
    for (Color c : enum_range<Color>()) {
        std::cout << "  Color: " << enum_name(c)
                  << " (Underlying: " << static_cast<int>(c) << ")" << std::endl;
    }

    // 4. Get all enum values and names as arrays
    constexpr auto all_status_values = enum_values<Status>();
    std::cout << "\nAll Status values array (size " << all_status_values.size() << "):" << std::endl;
    for (Status s : all_status_values) {
        std::cout << "  " << enum_name(s) << std::endl;
    }

    constexpr auto all_color_names = enum_names<Color>();
    std::cout << "\nAll Color names array (size " << all_color_names.size() << "):" << std::endl;
    for (std::string_view sv : all_color_names) {
        std::cout << "  " << sv << std::endl;
    }

    // 5. Get size (count of valid enumerators found)
    std::cout << "\nSize of Status enum (reflected): " << enum_size<Status>() << std::endl;
    std::cout << "Size of Color enum (reflected): " << enum_size<Color>() << std::endl;

    // 6. Stream output
    std::cout << "\nDirect stream output of Status::PENDING: " << Status::PENDING << std::endl; // "PENDING"
    std::cout << "Direct stream output of Color::BLUE: " << Color::BLUE << std::endl;     // "BLUE"

    // 7. Check validity
    Status s_test = Status::RUNNING;
    Status s_invalid = static_cast<Status>(100); // Assuming 100 is not a defined Status

    std::cout << "Is " << enum_name(s_test) << " valid? " << std::boolalpha << is_valid_enum(s_test) << std::endl;
    std::cout << "Is Status(100) valid? " << std::boolalpha << is_valid_enum(s_invalid)
              << " (Name: " << enum_name(s_invalid) << ")" << std::endl;


    return 0;
}
```

## Dependencies
- `<array>`, `<string_view>`, `<type_traits>`, `<algorithm>`, `<optional>`, `<stdexcept>`
- Requires C++17 or later. Some features (like C++20 concepts) might be conditionally compiled.

This reflection utility provides powerful introspection capabilities for enums, bridging the gap between compile-time enum definitions and runtime string manipulations or iterations.

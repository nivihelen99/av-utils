# `NamedStruct` - Compile-Time Named Fields

## Overview

The `named_struct.h` header provides a C++ utility, `NamedStruct`, for creating struct-like types where fields are defined with compile-time string names. This allows accessing fields by name or by index, with compile-time checks for name validity and index bounds. It aims to offer some of the convenience of Python's `dataclasses` or `namedtuple`.

Fields can be declared as mutable or immutable. The utility also provides helpers for definition, pretty-printing, basic JSON serialization, comparison, and hashing.

**⚠️ Important Note on Structured Bindings ⚠️**

Direct structured bindings (e.g., `auto [x, y] = my_named_struct_instance;`) have shown unreliability with this `NamedStruct` implementation on certain compilers (e.g., g++ 13.1.0). This is likely due to complexities in template argument deduction and interaction with the structured binding mechanism.

**Recommended alternatives for field access/decomposition:**
1.  **Direct Member Access (Safest & Preferred):**
    ```cpp
    my_instance.get<"field_name">()
    my_instance.get<index>()
    ```
2.  **Using `as_tuple()`:**
    ```cpp
    auto t = my_instance.as_tuple();
    auto x = std::get<0>(t); // Access elements from the resulting std::tuple
    // Note: Structured binding on the tuple from as_tuple() might also face issues
    // on affected compilers in some contexts. Direct std::get is safer.
    ```
Custom `std::tuple_size`, `std::tuple_element`, and ADL `get()` functions for `NamedStruct` that would enable direct structured bindings have been removed from this implementation due to these compiler issues.

## Core Components

-   **`StringLiteral<N>`**: A helper struct to enable passing string literals as non-type template parameters (NTTPs) for field names.
-   **`FieldMutability`**: An enum (`Mutable`, `Immutable`) to specify if a field can be modified after construction.
-   **`Field<StringLiteral Name, typename Type, FieldMutability Mut>`**: A template struct representing a single named field with its type and mutability.
-   **`NamedStruct<typename... Fields>`**: The main class template. It inherits from a variadic pack of `Field` types. Each `Field` base contributes its `value` member.

## Features

-   **Named Fields:** Define structs with fields accessible by compile-time string names.
-   **Indexed Access:** Fields can also be accessed by their 0-based index.
-   **Compile-Time Safety:** Field name and index validity are checked at compile time.
-   **Mutability Control:** Fields can be declared `Mutable` (default) or `Immutable`. Attempts to modify immutable fields result in compile-time errors.
-   **Helper Macros:**
    -   `NAMED_STRUCT(StructName, ...)`: For defining a `NamedStruct` type.
    -   `FIELD("name", Type)` or `MUTABLE_FIELD("name", Type)`: For mutable fields.
    -   `IMMUTABLE_FIELD("name", Type)`: For immutable fields.
-   **Tuple Conversion:** `as_tuple()` method to get a `std::tuple` of references to field values.
-   **Utilities:**
    -   Pretty-printing via `operator<<`.
    -   Basic JSON serialization via `to_json()`.
    -   Comparison operators (`==`, `!=`, `<`, etc.) based on field values.
    -   `std::hash` specialization for use with unordered containers.

## Public Interface Highlights

(Assuming `MyStruct` is defined via `NAMED_STRUCT(MyStruct, FIELD("x", int), IMMUTABLE_FIELD("id", const char*));`)

### Definition
```cpp
// Define a NamedStruct type
NAMED_STRUCT(Point,
    FIELD("x", int),
    FIELD("y", int)
);

NAMED_STRUCT(UserRecord,
    IMMUTABLE_FIELD("userID", unsigned long),
    FIELD("username", std::string),
    FIELD("isActive", bool)
);
```

### Construction
```cpp
Point p1; // Default construction (fields default-initialized)
Point p2{10, 20}; // Positional initialization: p2.x=10, p2.y=20

UserRecord ur{12345UL, "testuser", true};
```

### Field Access (Get)
-   **By Index:**
    ```cpp
    int x_val = p2.get<0>();
    const char* id_val = ur.get<0>(); // For IMMUTABLE_FIELD, get() returns const ref
    ```
-   **By Name:**
    ```cpp
    p2.get<"x">() = 100; // If "x" is mutable
    std::string name_val = ur.get<"username">();
    ```

### Field Modification (Set - for mutable fields)
-   **By Index:**
    ```cpp
    p2.set<1>(200); // Sets p2.y = 200 (if field at index 1 is mutable)
    ```
-   **By Name:**
    ```cpp
    ur.set<"username">("new_user"); // Sets ur.username (if "username" is mutable)
    // ur.set<"userID">(2UL); // Compile-time error if "userID" is IMMUTABLE_FIELD
    ```

### Metadata and Utilities
-   **`my_instance.size()`**: Number of fields.
-   **`MyStructType::field_name<I>()`**: Get name of field at index `I`.
-   **`MyStructType::is_mutable<I>()` / `MyStructType::is_mutable<"name">()`**: Check field mutability.
-   **`my_instance.as_tuple()`**: Get `std::tuple` of field value references.
-   `std::cout << my_instance;`: Pretty print.
-   `std::string json_str = to_json(my_instance);`: Basic JSON.
-   Comparison operators (`==`, `!=`, `<`, etc.) and `std::hash` are available.

## Usage Examples

### Defining and Using a Mutable `NamedStruct`

```cpp
#include "named_struct.h" // Adjust path as needed
#include <iostream>
#include <string>

// Define a Point struct with mutable fields 'x' and 'y'
NAMED_STRUCT(Point,
    FIELD("x", int),
    FIELD("y", int)
);

int main() {
    Point p1{10, 20};
    std::cout << "Initial p1: " << p1 << std::endl; // { x: 10, y: 20 }

    p1.get<"x">() = 15;
    p1.set<1>(25); // Set field at index 1 (which is "y")

    std::cout << "Modified p1: { x: " << p1.get<0>()
              << ", y: " << p1.get<"y">() << " }" << std::endl; // { x: 15, y: 25 }

    // Using as_tuple (carefully, due to structured binding notes)
    auto p1_tuple = p1.as_tuple();
    std::cout << "Tuple from p1: (" << std::get<0>(p1_tuple)
              << ", " << std::get<1>(p1_tuple) << ")" << std::endl; // (15, 25)
}
```

### Defining and Using Mixed/Immutable `NamedStruct`

```cpp
#include "named_struct.h"
#include <iostream>
#include <string>
#include <unordered_set> // For std::hash example

NAMED_STRUCT(Config,
    IMMUTABLE_FIELD("host", std::string),
    MUTABLE_FIELD("port", int),
    IMMUTABLE_FIELD("readonly_flag", bool)
);

int main() {
    Config cfg{"localhost", 8080, true};
    std::cout << "Config: " << cfg << std::endl;
    // Output: { host: localhost, port: 8080, readonly_flag: true }

    // cfg.get<"host">() = "example.com"; // Compile error: "host" is immutable
    // cfg.set<0>("example.com");       // Compile error: field at index 0 ("host") is immutable

    cfg.set<"port">(9090); // OK, "port" is mutable
    std::cout << "Updated Config: " << cfg << std::endl;
    // Output: { host: localhost, port: 9090, readonly_flag: true }

    std::cout << "Is 'port' mutable? " << std::boolalpha << Config::is_mutable<"port">() << std::endl; // true
    std::cout << "Is 'host' mutable? " << Config::is_mutable<0>() << std::endl; // false

    // Hashing
    std::unordered_set<Config> config_set;
    config_set.insert(cfg);
    Config cfg2{"localhost", 9090, true};
    std::cout << "cfg == cfg2: " << (cfg == cfg2) << std::endl; // true
    std::cout << "Set contains cfg2? " << config_set.count(cfg2) << std::endl; // 1
}
```

## Dependencies
- `<iostream>`, `<string>`, `<tuple>`, `<utility>`, `<functional>`, `<string_view>`, `<type_traits>`, `<concepts>` (C++20), `<algorithm>`, `<set>` (for `std::hash` example), `<stdexcept>` (not directly used, but good practice for similar libs).

`NamedStruct` offers a way to create more descriptive and type-safe structures in C++, though current compiler limitations regarding structured bindings should be considered.

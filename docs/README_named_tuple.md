# `cpp_utils::named_tuple<IsMutable, Fields...>`

## Overview

The `named_tuple.h` header provides the `cpp_utils::named_tuple` class template, which aims to offer a C++ equivalent to Python's `namedtuple`. It allows creating tuple-like structures where elements can be accessed not only by their compile-time index but also by a "name." In this C++ implementation, these "names" are represented by unique types (typically empty structs defined via a helper macro).

This utility provides a way to group related data like a struct, but with field access semantics that are checked at compile time using these type-based names. The mutability of the tuple's fields (whether they can be changed after construction) is controlled by a boolean template parameter.

## Core Concepts

-   **Field Tags as Types:** Instead of using string literals for field names at runtime, `named_tuple` uses distinct C++ types (often empty structs) as "tags" or "names" for its fields. These tag types are used as template arguments to the `get()` method for named access.
-   **`DEFINE_NAMED_TUPLE_FIELD` Macro:** A helper macro to easily define these field tag types. For a field intended to be named `MyField` holding a `MyType`, you would define:
    ```cpp
    DEFINE_NAMED_TUPLE_FIELD(MyField, MyType);
    // This creates: struct MyField { using type = MyType; };
    ```
-   **Underlying `std::tuple`:** The actual data is stored in a `std::tuple`, whose element types are derived from the `type` alias within each provided field tag struct.

## Template Parameters

-   `IsMutable` (boolean): If `true`, the fields of the `named_tuple` can be modified after creation via non-const `get()` methods. If `false`, fields are effectively const after construction (only const `get()` methods are available for modification).
-   `Fields...`: A variadic parameter pack of the field tag types (structs defined using `DEFINE_NAMED_TUPLE_FIELD`). The order of these types determines the order of elements in the underlying tuple.

## Features

-   **Named Access:** Access fields using their corresponding tag type: `my_tuple.get<FieldNameType>()`.
-   **Indexed Access:** Access fields using their 0-based index: `my_tuple.get<0>()`.
-   **Compile-Time Safety:** Field access (both by name-type and index) is checked at compile time. Using an incorrect field type or out-of-bounds index results in a compilation error.
-   **Mutability Control:** The `IsMutable` template parameter determines if the tuple's fields can be modified.
-   **`std::tuple` Backend:** Leverages `std::tuple` for storage and some operations.
-   **Helper Macro:** `DEFINE_NAMED_TUPLE_FIELD` simplifies the definition of field tag types.

## Public Interface Highlights

### Defining Fields and `named_tuple` Types
```cpp
// In some namespace or globally
DEFINE_NAMED_TUPLE_FIELD(UserId, int);
DEFINE_NAMED_TUPLE_FIELD(UserName, std::string);

// Define a mutable named_tuple type
using UserTuple = cpp_utils::named_tuple<true, // Mutable
                                         UserId,
                                         UserName>;

// Define an immutable named_tuple type
DEFINE_NAMED_TUPLE_FIELD(XCoord, double);
DEFINE_NAMED_TUPLE_FIELD(YCoord, double);
using PointTuple = cpp_utils::named_tuple<false, // Immutable
                                          XCoord,
                                          YCoord>;
```

### Construction
```cpp
UserTuple user1{101, "Alice"};
PointTuple point1{1.0, 2.5};
UserTuple default_user; // Default constructs elements if possible
```

### Field Access (`get`)
-   **By Field Tag Type:**
    ```cpp
    std::cout << user1.get<UserName>() << std::endl; // "Alice"
    user1.get<UserName>() = "Alicia"; // OK if UserTuple is mutable

    double x = point1.get<XCoord>();
    // point1.get<XCoord>() = 0.5; // Compile error if PointTuple is immutable
    ```
-   **By Index:**
    ```cpp
    std::cout << user1.get<0>() << std::endl; // 101 (UserId)
    user1.get<0>() = 102; // OK if UserTuple is mutable
    ```

### Other Methods
-   **`constexpr std::size_t size() const`**: Returns the number of fields.
-   **`const std::tuple<...>& to_tuple() const`**: Returns a const reference to the underlying `std::tuple` object.

## Usage Examples

(Based on `examples/use_named_tuple.cpp`)

### Defining and Using a `Person` Named Tuple

```cpp
#include "named_tuple.h" // Adjust path as needed
#include <iostream>
#include <string>

// Define field tags for Person
namespace person_fields {
    DEFINE_NAMED_TUPLE_FIELD(Id, int);
    DEFINE_NAMED_TUPLE_FIELD(Name, std::string);
    DEFINE_NAMED_TUPLE_FIELD(Age, int);
}

// Create an alias for a mutable Person named_tuple
using Person = cpp_utils::named_tuple<true, // Mutable
                                      person_fields::Id,
                                      person_fields::Name,
                                      person_fields::Age>;

int main() {
    Person person1{1, "John Doe", 30};

    // Access by field tag type
    std::cout << "ID: " << person1.get<person_fields::Id>() << std::endl;
    std::cout << "Name: " << person1.get<person_fields::Name>() << std::endl;
    std::cout << "Age: " << person1.get<person_fields::Age>() << std::endl;

    // Access by index
    std::cout << "Name (by index 1): " << person1.get<1>() << std::endl;

    // Modify fields (since it's mutable)
    person1.get<person_fields::Age>() = 31;
    person1.get<0>() = 2; // Modifying ID by index

    std::cout << "Updated Age: " << person1.get<person_fields::Age>() << std::endl;
    std::cout << "Updated ID: " << person1.get<person_fields::Id>() << std::endl;

    std::cout << "Number of fields: " << person1.size() << std::endl;

    // Get underlying tuple
    const auto& underlying_std_tuple = person1.to_tuple();
    std::cout << "Underlying tuple's first element (ID): "
              << std::get<0>(underlying_std_tuple) << std::endl;
}
```

## Dependencies
- `<tuple>`
- `<string>` (commonly used for field types)
- `<type_traits>`
- `<utility>`
- `<iostream>` (for examples)

The `cpp_utils::named_tuple` offers a way to create tuple-like objects with more descriptive, type-safe field access compared to relying solely on numerical indices with `std::tuple`.

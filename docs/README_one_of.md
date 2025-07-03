# `OneOf<Ts...>` - Runtime-Safe Variant Container

## 1. Overview

`OneOf<T1, T2, ..., TN>` is a C++17 header-only utility that provides a type-erased, runtime-polymorphic container. It is designed to hold exactly one value from a predefined set of types (`Ts...`) at any given time. It supports safe runtime dispatching via a visitor pattern, offering a more dynamic alternative to `std::variant` in certain scenarios.

This utility is particularly useful in systems where types might not be fully known at compile-time across all modules (e.g., plugin systems, interpreters) or where a simpler, runtime-based visitation mechanism is preferred over `std::visit`'s stricter compile-time requirements.

## 2. Motivation

While `std::variant` is excellent for type-safe unions, its reliance on compile-time type information and the sometimes complex nature of `std::visit` can be limiting in highly dynamic environments. `OneOf<Ts...>` aims to address these by providing:

*   **Runtime Type Introspection**: Ability to query the currently held type.
*   **Type-Safe Storage**: Ensures only one of the specified types is active.
*   **Simplified Visitation**: A runtime dispatch mechanism for visitors.
*   **Dynamic Integration**: Easier to use with systems involving plugins or dynamically typed values.

## 3. Key Features

*   **Header-Only**: Easy to integrate (`#include "one_of.h"`).
*   **C++17**: Leverages modern C++ features.
*   **Stack-Based Storage**: Uses `std::byte[]` (similar to `std::aligned_storage`) to store the active object on the stack. No dynamic memory allocation by `OneOf` itself for its storage.
*   **Runtime Visitation**: Supports `visit(Visitor&& visitor)` for safe, dynamic dispatch.
*   **Introspection**: Provides `has<T>()`, `get_if<T>()`, `index()`, and `type()` (using `typeid`).
*   **Value Management**: Supports `set()`, `emplace()`, and `reset()`.
*   **Copy and Move Semantics**: Full support for copy and move construction/assignment.

## 4. API Sketch

```cpp
template<typename... Ts>
class OneOf {
public:
    // --- Constructors ---
    OneOf() noexcept; // Default: valueless state

    template<typename T, /* SFINAE to check T is in Ts... */>
    OneOf(T&& val);    // Construct with a value

    OneOf(const OneOf& other);
    OneOf(OneOf&& other) noexcept;

    // --- Assignment ---
    OneOf& operator=(const OneOf& other);
    OneOf& operator=(OneOf&& other) noexcept;

    template<typename T, /* SFINAE */>
    void set(T&& val); // Sets a new value, destroying the old

    template<typename T, typename... Args, /* SFINAE */>
    std::decay_t<T>& emplace(Args&&... args); // Emplaces a new value

    // --- Introspection & Access ---
    bool has_value() const noexcept;

    template<typename T>
    bool has() const noexcept; // Checks if it holds type T

    template<typename T>
    T* get_if() noexcept;       // Returns pointer to value if type T, else nullptr

    template<typename T>
    const T* get_if() const noexcept;

    std::size_t index() const noexcept; // 0-based index of active type, or std::variant_npos
    std::type_index type() const;       // std::type_index of active type (throws if valueless)

    // --- Visitation ---
    template<typename Visitor>
    decltype(auto) visit(Visitor&& visitor);

    template<typename Visitor>
    decltype(auto) visit(Visitor&& visitor) const;

    // --- State Management ---
    void reset() noexcept; // Destroys held value, becomes valueless
};
```

## 5. Usage Example

```cpp
#include "one_of.h"
#include <iostream>
#include <string>

struct MyInt { int val; void print() const { std::cout << "MyInt: " << val << std::endl; } };
struct MyStr { std::string val; void print() const { std::cout << "MyStr: " << val << std::endl; } };

struct PrintVisitor {
    void operator()(const MyInt& mi) const { mi.print(); }
    void operator()(const MyStr& ms) const { ms.print(); }
    // Add overloads for other types in OneOf
};

int main() {
    OneOf<MyInt, MyStr, double> var;

    // Emplace a MyInt
    var.emplace<MyInt>(123);
    var.visit(PrintVisitor{}); // Calls PrintVisitor::operator()(const MyInt&)

    if (var.has<MyInt>()) {
        MyInt* mi_ptr = var.get_if<MyInt>();
        if (mi_ptr) {
            std::cout << "Got MyInt ptr, value: " << mi_ptr->val << std::endl;
        }
    }
    std::cout << "Current type index: " << var.index() << std::endl; // Should be 0

    // Set to MyStr
    var.set(MyStr{"Hello"});
    var.visit(PrintVisitor{}); // Calls PrintVisitor::operator()(const MyStr&)
    std::cout << "Current type index: " << var.index() << std::endl; // Should be 1

    // Reset
    var.reset();
    std::cout << "Has value after reset? " << std::boolalpha << var.has_value() << std::endl; // false

    try {
        var.visit(PrintVisitor{}); // Throws std::bad_variant_access
    } catch (const std::bad_variant_access& e) {
        std::cout << "Caught: " << e.what() << std::endl;
    }
}
```

## 6. Building and Testing

The `OneOf` utility is header-only. To use it, simply include `one_of.h`.

The provided `CMakeLists.txt` files can be used to build the example (`one_of_example`) and run the unit tests (`one_of_test`).

### Building Examples and Tests:

A typical CMake workflow:

```bash
mkdir build
cd build
cmake ..
make
# Run example
./examples/one_of_example # Or path from build directory, e.g., ./one_of_example
# Run tests
./tests/one_of_test       # Or path from build directory, e.g., ./one_of_test
# Or using CTest
ctest
```

## 7. Design Notes

*   **Storage**: `OneOf` uses a `alignas(...) std::byte storage_[MAX_SIZE]` array to hold the data. `MAX_SIZE` is determined by the largest `sizeof(T)` among `Ts...`, and alignment is handled by `alignas` with the largest `alignof(T)`.
*   **Type Dispatch**:
    *   Internal operations (like destruction, copy, move) use a fold expression over `std::index_sequence` to dispatch to the correct type.
    *   The public `visit` method uses a recursive template helper function (`call_visitor_recursively`) with `if constexpr` to achieve runtime dispatch while allowing `decltype(auto)` for return type deduction. This handles various visitor return types effectively.
*   **Exception Safety**:
    *   If a type's constructor throws during `emplace` or `set` (after `reset`), `OneOf` remains in a valueless state (basic guarantee).
    *   If copy/move of a type throws during `OneOf` copy/move construction/assignment, the state of `OneOf` depends on when the exception occurred. The assignment operators aim for a basic guarantee by resetting first.
    *   Accessing a valueless `OneOf` via `type()` or `visit()` throws `std::bad_variant_access`.
*   **Move-Only Types**: `OneOf` supports holding move-only types for operations that involve moving (e.g., construction from rvalue, `emplace`, `set` with rvalue, move construction/assignment of `OneOf` itself). Copy construction or copy assignment of a `OneOf` containing a move-only type will result in a compile-time error, as expected.

## 8. Limitations and Considerations

*   **RTTI for `type()`**: The `type()` method returns `std::type_index` which relies on `typeid`. This means RTTI must be enabled for this specific feature to work. If RTTI is disabled, calls to `type()` may lead to compilation or runtime issues. All other features work without RTTI.
*   **Visitor Return Types**: While `decltype(auto)` is used for `visit`, if a visitor has overloads that return fundamentally incompatible types that cannot be unified into a common return type by the compiler, it may lead to compilation errors within the dispatch mechanism. Users should ensure their visitor overloads return types that are either all `void` or can be implicitly converted to a common type if a single return value is expected from `visit`.
*   **No `std::hash` Specialization**: A `std::hash<OneOf<Ts...>>` specialization is not provided by default.
*   **No `get<T>()` or `get<Idx>()`**: Unlike `std::variant`, `OneOf` does not provide `get<T>()` or `get<index>()` that throw on type mismatch. Access is primarily through `get_if<T>()` (returns `nullptr` on mismatch) or `visit()`.

This README provides a basic guide to `OneOf`. For detailed API usage and behavior, refer to the source code (`include/one_of.h`), the example (`examples/one_of_example.cpp`), and the unit tests (`tests/one_of_test.cpp`).

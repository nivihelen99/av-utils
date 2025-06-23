# Heterogeneous Vector Utilities (`variant_vector.h`)

## Overview

The `variant_vector.h` header provides C++ classes for storing collections of heterogeneous objects (objects of different types) in a vector-like manner. It offers two main approaches, both utilizing a **Structure of Arrays (SoA)** memory layout:

1.  **`static_variant_vector<Types...>`**: For when the set of possible types is known at compile time.
2.  **`dynamic_variant_vector`**: For when types can be registered and used at runtime.

The SoA layout means that objects of the same type are stored contiguously in their own dedicated `std::vector`. An additional mapping structure (`index_map`) maintains the overall sequence of elements and links each "global" index to the correct typed vector and the element's local index within that vector.

This approach can offer performance benefits (due to improved cache locality) and potentially reduced memory overhead compared to a `std::vector<std::variant<Types...>>` (Array of Structures, AoS) for certain access patterns, especially when iterating over all elements of a specific type.

## 1. `static_variant_vector<Types...>`

A vector that can store a sequence of objects, where each object's type must be one of those specified in the `Types...` template parameter pack.

### Template Parameters
-   `Types...`: A variadic template parameter pack listing all possible types that can be stored.

### Features
-   **Compile-Time Type Safety:** The set of storable types is fixed at compile time.
-   **Structure of Arrays (SoA):** Internally uses a `std::tuple` of `std::vector<Type>` for each type in `Types...`.
-   **Global Indexing:** Maintains an `index_map` to translate a global, heterogeneous index to a specific type's vector and local index.
-   **Efficient Typed Iteration:** Accessing all elements of a single known type is efficient via `get_type_vector<T>()`.
-   **Variant Access:** Elements can be retrieved as a `std::variant<Types...>` via `operator[]` or `at()`.
-   **Direct Typed Access:** `get_typed<T>(global_idx)` provides faster access if the type `T` at `global_idx` is known (throws `std::bad_variant_access` if type mismatches).

### Public Interface Highlights
-   **`static_variant_vector()`**: Default constructor.
-   **`void reserve(size_t capacity)`**: Reserves space (distributes capacity among type vectors).
-   **`void push_back(T&& value)`**: Adds an element of type `T` (must be one of `Types...`).
-   **`template<typename T, typename... Args> void emplace_back(Args&&... args)`**: Emplaces an element of type `T`.
-   **`variant_type at(size_t global_idx) const`**: Returns `std::variant<Types...>`; bounds-checked.
-   **`variant_type operator[](size_t global_idx) const`**: Returns `std::variant<Types...>`; no bounds check in release.
-   **`template<typename T> const T& get_typed(size_t global_idx) const`**: Direct typed access; throws if type mismatch.
-   **`size_t size() const` / `bool empty() const` / `void clear()` / `void pop_back()`**.
-   **`template<typename T> const std::vector<T>& get_type_vector() const`**: Get direct access to the vector storing all elements of type `T`.
-   **`uint8_t get_type_index(size_t global_idx) const`**: Get the 0-based type index (in `Types...`) for the element at `global_idx`.
-   **`size_t memory_usage() const`**: Reports approximate memory usage.

## 2. `dynamic_variant_vector`

A vector that can store objects of different types, where types can be registered at runtime using `std::type_index`.

### Features
-   **Runtime Type Flexibility:** Types can be added dynamically.
-   **Structure of Arrays (SoA) with Type Erasure:** Uses a base class `type_storage_base` and templated derived classes `type_storage<T>` (each holding a `std::vector<T>`) to store data for different types.
-   **Type Registration:** Types generally need to be registered via `register_type<T>()` before use, though `push_back`/`emplace_back` can auto-register.
-   **Global Indexing:** Similar `index_map` as `static_variant_vector`, but `storage_idx` points into a vector of type-erased storage managers.
-   **`std::any` Access:** `at(global_idx)` returns a `std::any` holding the element.
-   **Direct Typed Access:** `get_typed<T>(global_idx)` provides access if type `T` is known.

### Public Interface Highlights
-   **`dynamic_variant_vector()`**: Default constructor.
-   **`template<typename T> void register_type()`**: Registers a type `T` for storage.
-   **`void reserve(size_t capacity)`**.
-   **`template<typename T> void push_back(T&& value)`**: Adds an element of type `T` (registers type if new).
-   **`template<typename T, typename... Args> void emplace_back(Args&&... args)`**: Emplaces an element of type `T`.
-   **`std::any at(size_t global_idx) const`**: Returns `std::any`; bounds-checked.
-   **`template<typename T> const T& get_typed(size_t global_idx) const` / `template<typename T> T& get_typed(size_t global_idx)`**: Direct typed access.
-   **`size_t size() const` / `bool empty() const` / `void clear()` / `void pop_back()`**.
-   **`template<typename T> const std::vector<T>& get_type_vector() const`**: Get direct access to the vector storing all elements of type `T`. Throws if type not registered.
-   **`size_t memory_usage() const`**.

## Usage Examples

(Based on `tests/variant_vector_test.cpp` and `tests/variant_vector_usage_test.cpp`)

### `static_variant_vector` Example

```cpp
#include "variant_vector.h" // Adjust path
#include <iostream>
#include <string>
#include <variant>

struct TypeA { int id; std::string name; };
struct TypeB { double value; };

int main() {
    static_variant_vector<TypeA, TypeB, int> s_vec;

    s_vec.emplace_back<TypeA>(1, "Hello");
    s_vec.emplace_back<int>(42);
    s_vec.emplace_back<TypeB>(3.14);
    s_vec.push_back(TypeA{2, "World"});

    std::cout << "Static Variant Vector (size " << s_vec.size() << "):" << std::endl;
    for (size_t i = 0; i < s_vec.size(); ++i) {
        std::cout << "  Index " << i << ": ";
        std::visit([](const auto& val) {
            // Note: TypeA and TypeB would need operator<< or specific handling here
            if constexpr (std::is_same_v<std::decay_t<decltype(val)>, TypeA>) {
                std::cout << "TypeA(id=" << val.id << ", name=" << val.name << ")";
            } else if constexpr (std::is_same_v<std::decay_t<decltype(val)>, TypeB>) {
                std::cout << "TypeB(value=" << val.value << ")";
            } else if constexpr (std::is_same_v<std::decay_t<decltype(val)>, int>) {
                std::cout << "int(" << val << ")";
            }
        }, s_vec[i]); // Using operator[] to get std::variant
        std::cout << std::endl;
    }

    // Accessing all TypeA objects efficiently
    std::cout << "\nAll TypeA objects:" << std::endl;
    const auto& type_a_elements = s_vec.get_type_vector<TypeA>();
    for (const auto& ta : type_a_elements) {
        std::cout << "  TypeA(id=" << ta.id << ", name=" << ta.name << ")" << std::endl;
    }
     // Direct typed access
    std::cout << "Element at global index 0 (TypeA): " << s_vec.get_typed<TypeA>(0).name << std::endl;
}
```

### `dynamic_variant_vector` Example

```cpp
#include "variant_vector.h" // Adjust path
#include <iostream>
#include <string>
#include <any> // For std::any_cast

struct MyInt { int val; MyInt(int v): val(v){} };
struct MyString { std::string str; MyString(std::string s): str(std::move(s)){} };

int main() {
    dynamic_variant_vector d_vec;

    // Types are registered automatically on first push/emplace of that type,
    // or can be pre-registered with d_vec.register_type<MyInt>();

    d_vec.emplace_back<MyInt>(100);
    d_vec.push_back(MyString("Dynamic Test"));
    d_vec.emplace_back<MyInt>(200);

    std::cout << "\nDynamic Variant Vector (size " << d_vec.size() << "):" << std::endl;
    for (size_t i = 0; i < d_vec.size(); ++i) {
        std::cout << "  Index " << i << " (type: " << d_vec.at(i).type().name() << "): ";
        std::any current_val = d_vec.at(i);
        if (current_val.type() == typeid(MyInt)) {
            std::cout << "MyInt(" << std::any_cast<MyInt>(current_val).val << ")";
        } else if (current_val.type() == typeid(MyString)) {
            std::cout << "MyString(\"" << std::any_cast<MyString>(current_val).str << "\")";
        }
        std::cout << std::endl;
    }

    // Accessing all MyInt objects
    std::cout << "\nAll MyInt objects:" << std::endl;
    // Need to register type before get_type_vector if not already pushed
    // d_vec.register_type<MyInt>();
    const auto& my_int_elements = d_vec.get_type_vector<MyInt>();
    for (const auto& mi : my_int_elements) {
        std::cout << "  MyInt(" << mi.val << ")" << std::endl;
    }
}
```

## Dependencies
- `<vector>`, `<variant>` (for `static_variant_vector`), `<type_traits>`, `<array>`, `<tuple>`, `<algorithm>`, `<cassert>`, `<memory>`, `<unordered_map>`, `<typeindex>`, `<stdexcept>`, `<any>` (for `dynamic_variant_vector`).

These variant vector implementations offer alternatives to `std::vector<std::variant>` with a Structure of Arrays layout, potentially providing performance and memory advantages for specific use patterns involving heterogeneous collections.

# `EnumMap<TEnum, TValue, N>`

## Overview

The `EnumMap<TEnum, TValue, N>` class (`enum_map.h`) is a specialized associative container that uses an enumeration type `TEnum` as its keys. It is implemented using `std::array` internally, providing highly efficient O(1) time complexity for element access.

This container is ideal when you have a fixed set of keys known at compile time (represented by an enum) and you want to map each enumerator to a value. It requires the enum `TEnum` to have a special enumerator, typically named `COUNT`, which indicates the total number of enumerators in the enum. This `COUNT` value is used to determine the size of the underlying array.

## Template Parameters

-   `TEnum`: The enumeration type to be used as keys. It must be a C++ enum type (`std::is_enum<TEnum>::value` must be true).
-   `TValue`: The type of the values stored in the map.
-   `N`: The size of the map, automatically deduced as `static_cast<std::size_t>(TEnum::COUNT)`. This parameter can also be specified explicitly if needed.

## Features

-   **O(1) Performance:** Element access (`operator[]`, `at()`) is O(1) due to direct array indexing.
-   **Type Safety:** Uses strong enum typing for keys.
-   **Fixed Size:** The size of the map is determined at compile time by `TEnum::COUNT`.
-   **STL-like Interface:** Provides common map operations like `at()`, `operator[]`, `size()`, `empty()`, `contains()`, iterators, etc.
-   **Custom Iterators:** Iterators (`begin()`, `end()`) yield `std::pair<const TEnum, TValue&>`-like objects (via a proxy), allowing iteration over key-value pairs where the key is reconstructed from the iterator's current index. These iterators support random access.
-   **Value-Only Iterators:** `value_begin()`, `value_end()` provide direct iteration over the stored values.
-   **Default Initialization:** The default constructor default-initializes all values in the map if `TValue` is default-constructible.
-   **`clear()` and `erase()` Semantics:** These methods "reset" values to their default-constructed state. They require `TValue` to be default-constructible.

## Convention for `TEnum`

The enum type `TEnum` is expected to have its last enumerator be `COUNT`. This `COUNT` value determines the number of elements in the map. For example:

```cpp
enum class MyEnum {
    FIRST_KEY,
    SECOND_KEY,
    THIRD_KEY,
    COUNT // Indicates 3 valid keys (0, 1, 2)
};
// EnumMap<MyEnum, SomeValue> will have a size of 3.
```

## Public Interface Highlights

### Constructors
-   **`EnumMap()`**: Default constructor. If `TValue` is default-constructible, values are default-initialized.
-   **`EnumMap(std::initializer_list<std::pair<TEnum, TValue>> init)`**: Initializes with specified key-value pairs. Any unspecified enum keys will retain their default-initialized values.
-   Copy/Move constructors and assignment operators are defaulted.

### Element Access
-   **`TValue& operator[](TEnum key)` / `const TValue& operator[](TEnum key) const`**: Direct access (no bounds check).
-   **`TValue& at(TEnum key)` / `const TValue& at(TEnum key) const`**: Bounds-checked access; throws `std::out_of_range` if `key` is invalid.

### Iterators
-   **Key-Value Iterators**:
    -   `iterator begin() / end()`
    -   `const_iterator cbegin() / cend()`
    -   These are random access iterators. Dereferencing yields a proxy object behaving like `std::pair<const TEnum, TValue&>` (or const version).
-   **Value-Only Iterators** (iterating directly over the underlying `std::array<TValue, N>`):
    -   `value_iterator value_begin() / value_end()`
    -   `const_value_iterator const_value_cbegin() / const_value_cend()`

### Capacity
-   **`constexpr std::size_t size() const noexcept`**: Returns `N`.
-   **`constexpr bool empty() const noexcept`**: Returns `N == 0`.
-   **`constexpr std::size_t max_size() const noexcept`**: Returns `N`.

### Modifiers
-   **`void fill(const TValue& value)`**: Assigns `value` to all elements.
-   **`void clear()`**: Resets all values to their default-constructed state. Requires `TValue` to be default-constructible.
-   **`bool erase(TEnum key)`**: Resets the value at `key` to its default-constructed state. Returns `false` if `key` is invalid. Requires `TValue` to be default-constructible.
-   **`void swap(EnumMap& other) noexcept`**: Swaps contents.

### Lookup
-   **`bool contains(TEnum key) const noexcept`**: Checks if `key` is a valid enumerator within the map's range.

### Direct Data Access
-   **`std::array<TValue, N>& data() noexcept` / `const std::array<TValue, N>& data() const noexcept`**: Access to the underlying `std::array`.

## Usage Examples

(Based on `examples/enum_map_example.cpp`)

### Basic Usage with State Machine

```cpp
#include "enum_map.h"
#include <iostream>
#include <string>

enum class AppState {
    IDLE,
    LOADING,
    RUNNING,
    ERROR,
    COUNT // Defines the size of the map
};

int main() {
    EnumMap<AppState, std::string> state_descriptions = {
        {AppState::IDLE, "System is idle."},
        {AppState::LOADING, "System is loading data..."},
        {AppState::RUNNING, "System is currently running."},
        {AppState::ERROR, "An error has occurred."}
    };

    std::cout << "Current state (LOADING): " << state_descriptions[AppState::LOADING] << std::endl;

    state_descriptions[AppState::IDLE] = "Waiting for user input.";
    std::cout << "Updated IDLE: " << state_descriptions.at(AppState::IDLE) << std::endl;

    std::cout << "\nAll State Descriptions:" << std::endl;
    for (const auto& pair : state_descriptions) { // Uses key-value iterator
        std::cout << "  Enum Value " << static_cast<int>(pair.first)
                  << " (" << typeid(pair.first).name() << ")" // Shows enum type
                  << " -> " << pair.second << std::endl;
    }
}
```

### Dispatch Table with Function Pointers or `std::function`

```cpp
#include "enum_map.h"
#include <iostream>
#include <functional> // For std::function

enum class Operation {
    ADD,
    SUBTRACT,
    MULTIPLY,
    COUNT
};

void perform_add() { std::cout << "Addition performed." << std::endl; }
void perform_subtract() { std::cout << "Subtraction performed." << std::endl; }

int main() {
    EnumMap<Operation, std::function<void()>> operations;
    operations[Operation::ADD] = perform_add;
    operations[Operation::SUBTRACT] = perform_subtract;
    // operations[Operation::MULTIPLY] would be default-constructed (e.g., empty std::function)

    if (operations.contains(Operation::ADD) && operations[Operation::ADD]) {
        operations[Operation::ADD](); // Calls perform_add
    }

    if (operations.contains(Operation::MULTIPLY) && operations[Operation::MULTIPLY]) {
        operations[Operation::MULTIPLY]();
    } else {
        std::cout << "Multiply operation not defined." << std::endl;
    }
}
```

## Dependencies
- `<array>`
- `<initializer_list>`
- `<type_traits>`
- `<utility>`
- `<stdexcept>` (for `at()`)

`EnumMap` is a highly efficient and type-safe alternative to `std::map` or `std::unordered_map` when keys are known, fixed enumerations.

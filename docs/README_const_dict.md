# `ConstDict<Key, Value, UnderlyingMap>`

## Overview

The `ConstDict` class (`const_dict.h`) provides a read-only, immutable wrapper around standard C++ map types (such as `std::unordered_map` or `std::map`). Once a `ConstDict` is created, its contents cannot be modified through its public interface. This is useful for representing fixed collections of key-value pairs, like configurations, lookup tables, or any data that should remain constant after initialization.

`ConstDict` uses `std::shared_ptr<const UnderlyingMap>` internally, allowing multiple `ConstDict` instances to efficiently share the same underlying immutable map data. Copying a `ConstDict` is therefore a lightweight operation.

## Template Parameters

-   `Key`: The type of the keys in the dictionary.
-   `Value`: The type of the values in the dictionary.
-   `UnderlyingMap`: The type of the map container being wrapped. Defaults to `std::unordered_map<Key, Value>`. You can specify other map types like `std::map<Key, Value>` if, for example, ordered iteration is required.

## Features

-   **Immutability:** All mutating operations (insert, erase, clear, etc.) are explicitly deleted, ensuring read-only access at compile time.
-   **Standard Map Interface:** Provides a familiar, `const`-correct subset of the `std::map` / `std::unordered_map` interface (e.g., `at()`, `operator[]`, `find()`, `contains()`, `begin()`, `end()`, `size()`, `empty()`).
-   **Safe `operator[]`:** Unlike `std::map::operator[]`, `ConstDict::operator[]` throws `std::out_of_range` if the key is not found, preventing accidental insertion into a supposedly immutable structure.
-   **Shared Ownership:** Uses `std::shared_ptr` internally, making copies cheap and allowing efficient sharing of the underlying data.
-   **Flexible Construction:** Can be created from initializer lists, existing maps (by copy or move), or an existing `std::shared_ptr<const UnderlyingMap>`.
-   **Type Aliases & Factories:** Includes convenience aliases (`ConstUnorderedDict`, `ConstOrderedDict`) and factory functions (`make_const_dict`) for ease of use.
-   **C++17 Deduction Guides:** Simplifies template argument deduction during construction.

## Public Interface Highlights

### Constructors
```cpp
// From initializer list
ConstDict(std::initializer_list<std::pair<const Key, Value>> init);

// From an existing map (copies or moves)
explicit ConstDict(const UnderlyingMap& map);
explicit ConstDict(UnderlyingMap&& map);

// From a shared_ptr to a const map
explicit ConstDict(std::shared_ptr<const UnderlyingMap> map_ptr);

// Copy/Move constructors/assignments are defaulted (use shared_ptr semantics)
```

### Accessors (Read-Only)
-   **`const_reference at(const Key& key) const`**: Access element by `key`; throws `std::out_of_range` if `key` not found.
-   **`const_reference operator[](const Key& key) const`**: Same as `at(key)`. **Note:** This is different from `std::map::operator[]` as it does not insert if `key` is not found.
-   **`const_iterator find(const Key& key) const`**: Finds element by `key`.
-   **`bool contains(const Key& key) const`**: Checks if `key` exists. (Uses C++20 `map::contains` if available, else `find`).
-   **`size_type count(const Key& key) const`**: Counts elements with `key` (0 or 1 for maps).

### Iterators (Read-Only)
-   **`const_iterator begin() const noexcept` / `const_iterator cbegin() const noexcept`**
-   **`const_iterator end() const noexcept` / `const_iterator cend() const noexcept`**

### Size & Capacity
-   **`size_type size() const noexcept`**
-   **`bool empty() const noexcept`**
-   **`size_type max_size() const noexcept`**

### Comparison
-   **`bool operator==(const ConstDict& other) const`**
-   **`bool operator!=(const ConstDict& other) const`**

### Utility
-   **`std::shared_ptr<const UnderlyingMap> get_underlying_map() const`**: Get the shared pointer to the underlying const map.

### Deleted Operations
Mutating operations like `insert`, `emplace`, `erase`, `clear`, `swap` are `delete`d.

## Convenience Aliases and Factories
```cpp
// Aliases
template<typename Key, typename Value>
using ConstUnorderedDict = ConstDict<Key, Value, std::unordered_map<Key, Value>>;

template<typename Key, typename Value>
using ConstOrderedDict = ConstDict<Key, Value, std::map<Key, Value>>;

// Factory functions
template<typename Key, typename Value>
auto make_const_dict(std::initializer_list<std::pair<const Key, Value>> init);

template<typename UnderlyingMap>
auto make_const_dict(const UnderlyingMap& map);

template<typename UnderlyingMap>
auto make_const_dict(UnderlyingMap&& map);
```

## Usage Examples

(Based on `examples/const_dict_example.cpp`)

### Basic Usage

```cpp
#include "const_dict.h" // Or your specific path
#include <iostream>
#include <string>

int main() {
    // Create from initializer list (infers std::unordered_map by default)
    ConstDict<std::string, int> config_params = {
        {"timeout", 30},
        {"retries", 3},
        {"port", 8080}
    };

    std::cout << "Port: " << config_params.at("port") << std::endl;
    std::cout << "Retries: " << config_params["retries"] << std::endl;

    if (config_params.contains("host")) {
        std::cout << "Host: " << config_params.at("host") << std::endl;
    } else {
        std::cout << "Host not specified." << std::endl;
    }

    std::cout << "Configuration size: " << config_params.size() << std::endl;

    std::cout << "Iterating config:" << std::endl;
    for (const auto& pair : config_params) { // or `for (const auto& [key, value] : config_params)` in C++17
        std::cout << "  " << pair.first << ": " << pair.second << std::endl;
    }
}
```

### Using `ConstOrderedDict` for Ordered Iteration

```cpp
#include "const_dict.h"
#include <iostream>
#include <string>

int main() {
    // Using std::map as the underlying type for ordered iteration
    ConstOrderedDict<int, std::string> error_codes = {
        {500, "Internal Server Error"},
        {404, "Not Found"},
        {401, "Unauthorized"},
        {200, "OK"}
    };

    std::cout << "Error Codes (ordered by key):" << std::endl;
    for (const auto& entry : error_codes) {
        std::cout << "  " << entry.first << ": " << entry.second << std::endl;
    }
    // Output will be sorted by error code (key)
}
```

### Error Handling for Missing Keys

```cpp
#include "const_dict.h"
#include <iostream>
#include <string>
#include <stdexcept>

int main() {
    ConstDict<std::string, std::string> settings = {{"theme", "dark"}};

    try {
        std::cout << "Theme: " << settings.at("theme") << std::endl;
        // The following line will throw std::out_of_range
        std::cout << "Font: " << settings["font"] << std::endl;
    } catch (const std::out_of_range& e) {
        std::cerr << "Error accessing setting: " << e.what() << std::endl;
    }

    // Safer access using find or contains
    auto it = settings.find("font");
    if (it != settings.end()) {
        std::cout << "Font (found): " << it->second << std::endl;
    } else {
        std::cout << "Font setting not found." << std::endl;
    }
}
```

## Dependencies
- `<unordered_map>` (default underlying map)
- `<map>` (alternative underlying map)
- `<memory>` (for `std::shared_ptr`)
- `<initializer_list>`
- `<stdexcept>` (for `std::out_of_range`)
- `<utility>` (for `std::pair`, `std::move`)

`ConstDict` is a valuable tool for ensuring data integrity by providing a truly immutable map interface, suitable for configurations, constant lookup tables, and scenarios where read-only shared data is beneficial.

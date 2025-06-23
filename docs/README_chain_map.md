# `ChainMap<K, V, MapType>`

## Overview

The `ChainMap<K, V, MapType>` class (`chain_map.h`) provides a mechanism to link multiple map-like objects (dictionaries) together into a single, unified view. When performing a lookup, the ChainMap searches through the underlying maps in the order they were added to the chain, returning the value from the first map in which the key is found.

Modifications (insertions, updates via `operator[]` or `insert` methods, and deletions via `erase`) typically operate only on the *first* map in the chain. This makes ChainMap useful for managing contexts, configurations, or scopes where defaults can be overridden by more specific settings.

## Template Parameters

-   `K`: The key type for the maps.
-   `V`: The value type for the maps.
-   `MapType`: The type of the underlying map objects. Defaults to `std::unordered_map<K, V>`. Can be changed to other map-like types (e.g., `std::map<K, V>`) that provide a compatible interface.

## Features

-   **Chained Lookups:** Searches for keys sequentially through a chain of underlying maps.
-   **Scoped Modifications:** Insertions, updates (via `operator[]` and `insert`), and deletions (`erase`) primarily affect the first map in the chain. `at()` can modify values in-place in whichever map the key is found.
-   **Dynamic Chain:** Maps can be added to the front (`prepend_layer`) or back (`add_layer`) of the chain.
-   **Unified Iteration:** Provides iterators that traverse all unique key-value pairs visible in the ChainMap, respecting precedence (keys from earlier maps in the chain hide those in later maps).
-   **View Generation:** Methods to get collections of all unique visible `keys()`, `values()`, or `items()` (key-value pairs).
-   **STL Compatibility:** Designed to work with STL map interfaces and iterators.

## Core Concepts

-   **`maps_`:** An internal `std::vector<MapType*>` stores pointers to the constituent maps. The order in this vector defines the lookup precedence.
-   **`ChainMapIterator`:** A custom forward iterator that handles iterating over unique keys across the chained maps. It ensures each key is reported only once from its highest-priority map.
-   **Writable Map:** Most mutation operations target the first map in the `maps_` vector. If the chain is empty, such operations may throw `std::logic_error`.

## Public Interface Highlights

### Constructors
-   **`ChainMap()`**: Default constructor (empty chain).
-   **`explicit ChainMap(map_pointer first_map)`**: From a single map pointer.
-   **`ChainMap(std::initializer_list<map_pointer> maps_list)`**: From an initializer list of map pointers.
-   **`template <typename... MapArgs> explicit ChainMap(MapArgs&... maps_args)`**: Variadic constructor taking multiple map references.

### Layer Management
-   **`void prepend_layer(map_pointer new_map)`**: Adds `new_map` to the front (highest priority).
-   **`void add_layer(map_pointer new_map)`**: Adds `new_map` to the end (lowest priority).

### Iteration & Views
-   **`iterator begin()` / `iterator end()`**: For iterating unique visible items.
-   **`const_iterator begin() const` / `const_iterator end() const`**: Const versions.
-   **`const_iterator cbegin() const` / `const_iterator cend() const`**.
-   **`std::vector<K> keys() const`**: Returns unique visible keys.
-   **`std::vector<V> values() const`**: Returns values for unique visible keys.
-   **`std::vector<std::pair<const K, V>> items() const`**: Returns unique visible key-value pairs.

### Access & Lookup
-   **`mapped_type& operator[](const key_type& key)`**:
    -   If `key` is found, returns a reference to its value (in the map where found).
    -   If `key` is not found, inserts `key` (with a default-constructed value) into the *first* map and returns a reference to it. Throws `std::logic_error` if chain is empty.
-   **`const mapped_type& at(const key_type& key) const`**: Returns value for `key`; throws `std::out_of_range` if not found.
-   **`mapped_type& at(const key_type& key)`**: Non-const version of `at()`; allows modification of the value in the map where `key` is found.
-   **`const mapped_type& get(const key_type& key) const`**: Same as `at(key) const`.
-   **`bool contains(const key_type& key) const`**: Checks if `key` exists in any map.

### Modification (Primarily on First Map)
-   **`void insert(const key_type& key, const mapped_type& value)`** (and rvalue overloads): Inserts or updates `key` with `value` in the *first* map.
-   **`size_t erase(const key_type& key)`**: Erases `key` from the *first* map. Returns 1 if erased, 0 otherwise.

### State & Utility
-   **`const std::vector<map_pointer>& get_maps() const`**: Access to the underlying map pointers.
-   **`bool empty() const noexcept`**: True if no maps are in the chain.
-   **`size_t size() const`**: Number of unique keys visible in the ChainMap.
-   **`void clear()`**: Removes all maps from the chain.

## Usage Examples

(Based on `examples/chain_map_example.cpp`)

### Basic Chaining and Lookup

```cpp
#include "chain_map.h"
#include <iostream>
#include <string>
#include <unordered_map> // Default MapType
#include <map>           // Example with std::map

int main() {
    std::map<std::string, std::string> user_config = {{"color", "blue"}, {"font", "Arial"}};
    std::map<std::string, std::string> system_config = {{"font", "Times New Roman"}, {"debug", "false"}};
    std::map<std::string, std::string> default_config = {{"debug", "true"}, {"timeout", "30"}};

    // ChainMap using std::map as the MapType
    ChainMap<std::string, std::string, std::map<std::string, std::string>> config;
    config.add_layer(&user_config);    // Highest priority
    config.add_layer(&system_config);
    config.add_layer(&default_config); // Lowest priority

    std::cout << "Color: " << config.get("color") << std::endl;          // "blue" (from user_config)
    std::cout << "Font: " << config.get("font") << std::endl;            // "Arial" (from user_config)
    std::cout << "Debug: " << config.get("debug") << std::endl;          // "false" (from system_config)
    std::cout << "Timeout: " << config.get("timeout") << std::endl;        // "30" (from default_config)

    if (config.contains("font_size")) {
        std::cout << "Font Size: " << config.get("font_size") << std::endl;
    } else {
        std::cout << "Font Size: not set" << std::endl;
    }
}
```

### Modification and Iteration

```cpp
#include "chain_map.h"
#include <iostream>
#include <string>
#include <unordered_map>

int main() {
    std::unordered_map<std::string, int> m1 = {{"a", 1}, {"b", 2}};
    std::unordered_map<std::string, int> m2 = {{"b", 20}, {"c", 3}}; // "b" in m1 will hide this

    ChainMap<std::string, int> cm; // Uses default std::unordered_map
    cm.add_layer(&m1); // m1 is now the first (writable) map
    cm.add_layer(&m2);

    // operator[] for access and insertion into the first map (m1)
    std::cout << "cm[\"a\"]: " << cm["a"] << std::endl; // 1 (from m1)
    cm["d"] = 4; // Inserts {"d", 4} into m1
    std::cout << "m1 now contains d: " << std::boolalpha << m1.count("d") << std::endl; // true
    std::cout << "cm[\"d\"]: " << cm["d"] << std::endl; // 4 (from m1)

    // 'insert' also modifies the first map
    cm.insert("b", 22); // Updates "b" in m1 to 22
    std::cout << "cm[\"b\"] after insert: " << cm["b"] << std::endl; // 22 (from m1)
    std::cout << "m2[\"b\"] (unchanged): " << m2["b"] << std::endl; // 20

    // Erase from the first map
    cm.erase("a");
    std::cout << "cm.contains(\"a\") after erase: " << cm.contains("a") << std::endl; // false

    std::cout << "\nIterating unique items in ChainMap (size " << cm.size() << "):" << std::endl;
    // Expected order: b (from m1), d (from m1), c (from m2)
    // Actual order of b,d from m1 depends on unordered_map's internal order.
    for (const auto& pair : cm) {
        std::cout << "  " << pair.first << " -> " << pair.second << std::endl;
    }

    std::cout << "\nKeys: ";
    for(const auto& key : cm.keys()) std::cout << key << " ";
    std::cout << std::endl;
}
```

## Dependencies
- `<vector>`, `<string>`, `<unordered_map>`, `<map>`, `<stdexcept>`, `<algorithm>`, `<set>`, `<memory>`, `<list>`

The `ChainMap` provides a flexible way to manage layered configurations or scoped contexts, drawing inspiration from similar structures in other languages like Python.

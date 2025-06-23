# JSON FieldMask Library (`json_fieldmask.h`)

## Table of Contents
1. [Overview](#overview)
2. [Key Features](#key-features)
3. [Core Concepts](#core-concepts)
    - [FieldMask Class](#fieldmask-class)
    - [JSON Pointer Paths](#json-pointer-paths)
4. [Main Operations](#main-operations)
    - [Creating and Manipulating FieldMasks](#creating-and-manipulating-fieldmasks)
    - [Calculating Differences (Diffing)](#calculating-differences-diffing)
    - [Applying Masked Updates](#applying-masked-updates)
    - [Extracting Data by Mask](#extracting-data-by-mask)
    - [Pruning Redundant Paths](#pruning-redundant-paths)
    - [Inverting Masks](#inverting-masks)
    - [Path Utilities](#path-utilities)
5. [Basic Usage Examples](#basic-usage-examples)
    - [Creating a FieldMask](#creating-a-fieldmask)
    - [Diffing two JSON objects](#diffing-two-json-objects)
    - [Applying an update using a mask](#applying-an-update-using-a-mask)
    - [Extracting specific fields](#extracting-specific-fields)
6. [Building and Running](#building-and-running)
    - [Prerequisites](#prerequisites)
    - [Building](#building)
    - [Running Examples](#running-examples)
    - [Running Tests](#running-tests)

## Overview

The `json_fieldmask.h` library provides utilities for working with field masks on JSON objects, specifically using the `nlohmann/json` library. A field mask is a set of paths that specify which fields in a JSON object should be considered for an operation, such as updating, extracting, or diffing. This is particularly useful for implementing partial updates (sparse updates) or for understanding the specific differences between two JSON structures.

Paths are represented as strings following the JSON Pointer syntax (RFC 6901), e.g., `/config/name` or `/users/0/email`.

## Key Features

*   **FieldMask Representation**: A `FieldMask` class to store and manage a set of field paths.
*   **JSON Diffing**: Generate a `FieldMask` that describes the differences between two JSON objects.
*   **Masked Updates**: Apply changes from one JSON object to another, but only for the fields specified in a `FieldMask`.
*   **Data Extraction**: Create a new JSON object containing only the fields specified in a `FieldMask` from a source JSON object.
*   **Path Manipulation**: Utilities for building, splitting, and escaping JSON Pointer path components.
*   **Mask Utilities**: Functions for pruning redundant paths from a mask and inverting masks.
*   **Header-Only**: The core library is header-only, simplifying integration.

## Core Concepts

### FieldMask Class

The `fieldmask::FieldMask` class is central to the library. It internally uses a `std::set<std::string>` to store unique, sorted field paths.

Key methods:
*   `add_path(const std::string& path)`: Adds a path to the mask.
*   `contains(const std::string& path)`: Checks if the exact path is in the mask.
*   `contains_prefix(const std::string& prefix)`: Checks if any path in the mask starts with the given prefix.
*   `empty() const`: Returns true if the mask has no paths.
*   `clear()`: Removes all paths from the mask.
*   `merge(const FieldMask& other)`: Adds all paths from another mask into this one.
*   `get_paths() const`: Returns a const reference to the set of paths.
*   `to_string() const`: Returns a string representation of the mask for debugging.

### JSON Pointer Paths

All paths used by this library adhere to the JSON Pointer specification (RFC 6901).
*   Paths start with `/`.
*   Segments are separated by `/`.
*   Special characters `~` and `/` within a segment are escaped as `~0` and `~1` respectively.
*   Array elements are referenced by their zero-based index, e.g., `/array/0`, `/array/1`.

## Main Operations

### Creating and Manipulating FieldMasks

```cpp
#include "json_fieldmask.h" // Or <your_project_include_path/json_fieldmask.h>

fieldmask::FieldMask mask;
mask.add_path("/user/name");
mask.add_path("/user/contact/email");

if (mask.contains("/user/name")) {
    // ...
}
```

### Calculating Differences (Diffing)

The `diff_fields(const json& a, const json& b)` function compares two JSON objects (`a` and `b`) and returns a `FieldMask` containing the paths where they differ.

```cpp
json old_config = {{"host", "localhost"}, {"port", 8080}};
json new_config = {{"host", "example.com"}, {"port", 8080}, {"timeout", 5000}};

fieldmask::FieldMask differences = fieldmask::diff_fields(old_config, new_config);
// differences will contain "/host" and "/timeout"
```

### Applying Masked Updates

The `apply_masked_update(json& target, const json& src, const FieldMask& mask)` function updates the `target` JSON object with values from the `src` JSON object, but only for the paths specified in the `mask`. If paths in the mask do not exist in `target`, they (and their parent objects/arrays) will be created.

```cpp
json current_settings = {{"feature_a", true}, {"feature_b", false}};
json update_payload = {{"feature_a", true}, {"feature_b", true}, {"feature_c", 123}};

fieldmask::FieldMask update_mask;
update_mask.add_path("/feature_b"); // Only update feature_b

fieldmask::apply_masked_update(current_settings, update_payload, update_mask);
// current_settings is now {{"feature_a", true}, {"feature_b", true}}
// feature_c from update_payload is ignored.
```

### Extracting Data by Mask

The `extract_by_mask(const json& src, const FieldMask& mask)` function creates a new JSON object containing only the data from `src` at the paths specified in the `mask`.

```cpp
json user_data = {
    {"id", "123"},
    {"name", "John Doe"},
    {"email", "john.doe@example.com"},
    {"profile", {{"theme", "dark"}, {"notifications", true}}}
};

fieldmask::FieldMask public_fields_mask;
public_fields_mask.add_path("/name");
public_fields_mask.add_path("/profile/theme");

json public_profile = fieldmask::extract_by_mask(user_data, public_fields_mask);
// public_profile is {{"name", "John Doe"}, {"profile", {{"theme", "dark"}}}}
```

### Pruning Redundant Paths

The `prune_redundant_paths(const FieldMask& mask)` function takes a mask and returns a new mask where redundant child paths have been removed if their parent path is already included. For example, if a mask contains both `/config` and `/config/port`, the pruned mask would only contain `/config`.

```cpp
fieldmask::FieldMask my_mask;
my_mask.add_path("/system");
my_mask.add_path("/system/logging/level");

fieldmask::FieldMask pruned = fieldmask::prune_redundant_paths(my_mask);
// pruned will contain only "/system"
```

### Inverting Masks

The `invert_mask(const json& a, const json& b)` function creates a mask representing fields that are the *same* between JSON objects `a` and `b`. It first calculates all possible paths in `a` and `b`, then removes the paths that are different (obtained via `diff_fields`).

```cpp
json obj_v1 = {{"a", 1}, {"b", 2}, {"c", 3}};
json obj_v2 = {{"a", 1}, {"b", 99}, {"c", 3}}; // only 'b' changed

fieldmask::FieldMask same_fields_mask = fieldmask::invert_mask(obj_v1, obj_v2);
// same_fields_mask will contain "/a", "/c", and parent paths like "/"
```

### Path Utilities

The `fieldmask::path_utils` namespace provides helper functions:
*   `escape_component(const std::string& component)`: Escapes `~` and `/` in a single path component.
*   `build_path(const std::vector<std::string>& components)`: Constructs a JSON pointer from a vector of unescaped components.
*   `split_path(const std::string& path)`: Splits a JSON pointer path into its (escaped) components.
*   `get_parent_path(const std::string& path)`: Returns the parent path of a given path.

## Basic Usage Examples

### Creating a FieldMask
```cpp
#include "json_fieldmask.h"
#include <iostream>

int main() {
    fieldmask::FieldMask mask;
    mask.add_path("/configuration/host");
    mask.add_path("/configuration/port");
    std::cout << "Mask: " << mask.to_string() << std::endl;
    return 0;
}
```

### Diffing two JSON objects
```cpp
#include "json_fieldmask.h"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

int main() {
    json j1 = {{"name", "Alice"}, {"age", 30}};
    json j2 = {{"name", "Bob"}, {"age", 30}};
    fieldmask::FieldMask diff = fieldmask::diff_fields(j1, j2);
    std::cout << "Differences: " << diff.to_string() << std::endl;
    // Output: Differences: FieldMask{"/name"}
    return 0;
}
```

### Applying an update using a mask
```cpp
#include "json_fieldmask.h"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

int main() {
    json target = {{"name", "Alice"}, {"role", "user"}};
    json source = {{"name", "Alice Wonderland"}, {"role", "admin"}, {"location", "virtual"}};
    
    fieldmask::FieldMask update_mask;
    update_mask.add_path("/role"); // Only update the role
    
    fieldmask::apply_masked_update(target, source, update_mask);
    std::cout << "Updated target: " << target.dump(2) << std::endl;
    // Output:
    // {
    //   "name": "Alice",
    //   "role": "admin"
    // }
    return 0;
}
```

### Extracting specific fields
```cpp
#include "json_fieldmask.h"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

int main() {
    json data = {
        {"id", 7}, 
        {"content", "secret data"}, 
        {"public_info", "available to all"}
    };
    fieldmask::FieldMask mask;
    mask.add_path("/id");
    mask.add_path("/public_info");
    
    json extracted_data = fieldmask::extract_by_mask(data, mask);
    std::cout << "Extracted data: " << extracted_data.dump(2) << std::endl;
    // Output:
    // {
    //   "id": 7,
    //   "public_info": "available to all"
    // }
    return 0;
}
```

## Building and Running

This library is part of a larger C++ utilities project built with CMake.

### Prerequisites
*   CMake (version 3.10 or higher recommended)
*   A C++17 (or higher, C++20 for this project) compatible compiler (e.g., GCC, Clang, MSVC)
*   Git (for fetching dependencies like nlohmann/json and GoogleTest)

### Building
The project uses a root `CMakeLists.txt` that manages dependencies and subdirectories. To build the entire project, including examples and tests:

```bash
# From the root directory of the project
mkdir build
cd build
cmake ..
make # or cmake --build .
```

### Running Examples
Example executables are built into the build directory (typically `build/` or `build/examples/` depending on CMake configuration, for this project it's the root of the build directory).
After building, you can run the `json_fieldmask_example`:

```bash
# From the build directory
./json_fieldmask_example
```

### Running Tests
Tests are built using GoogleTest and can be run using CTest or by directly executing the test binary.
After building:

```bash
# From the build directory
ctest # To run all tests discovered by CTest

# Or run the specific test executable for json_fieldmask
./json_fieldmask_test # (Or tests/json_fieldmask_test if configured differently)
```
The specific test executable for this library will be `json_fieldmask_test` located in the build directory.

# JSON Patch Library (`jsonpatch.h`)

## Overview

`jsonpatch.h` is a header-only C++ library for working with JSON Patch (RFC 6902). It allows you to:

*   **Generate patches**: Create a sequence of operations (a JSON Patch) that describes the differences between two JSON documents.
*   **Apply patches**: Modify a JSON document according to a given JSON Patch.
*   **Invert patches**: Generate an inverse patch that can revert the changes made by a forward patch.

The library uses the `nlohmann/json` library for JSON representation and manipulation.

## Features

*   **Header-only**: Easy to integrate, just include `jsonpatch.h`.
*   **RFC 6902 Compliant**: Implements standard JSON Patch operations:
    *   `add`
    *   `remove`
    *   `replace`
    *   `move`
    *   `copy`
    *   `test`
*   **Diff Generation**: Automatically computes the differences between two JSON documents.
*   **Patch Application**: Safely applies patches to documents.
*   **Patch Inversion**: Can create patches to undo previous patches.
*   **Serialization**: Patches can be serialized to and deserialized from JSON.
*   **Error Handling**: Throws `JsonPatchException` for invalid operations or errors during application.

## Integration

1.  **Include `json.hpp`**: Ensure the `nlohmann/json` library's header (`json.hpp` or `nlohmann/json.hpp`) is available in your include paths. This library is a dependency.
    ```cpp
    #include "json.hpp" // Or <nlohmann/json.hpp>
    using json = nlohmann::json;
    ```
2.  **Include `jsonpatch.h`**:
    ```cpp
    #include "jsonpatch.h"
    ```
3.  **Compile**: Since it's header-only, there's no separate library to link against for `jsonpatch.h` itself. Just compile your C++ source files.

## API Reference

### `json` (alias for `nlohmann::json`)

This is the type used for all JSON documents and values.

### `struct JsonPatchOperation`

Represents a single JSON Patch operation.

*   **Enum `JsonPatchOperation::Type`**: `ADD`, `REMOVE`, `REPLACE`, `MOVE`, `COPY`, `TEST`.
*   **Members**:
    *   `Type op`: The type of operation.
    *   `std::string path`: The JSON Pointer path for the operation.
    *   `std::string from`: (For `move` and `copy`) The source JSON Pointer path.
    *   `json value`: (For `add`, `replace`, `test`) The value for the operation.
*   **Constructors**:
    *   `JsonPatchOperation(Type op_type, const std::string& path_str)`: For `remove`.
    *   `JsonPatchOperation(Type op_type, const std::string& path_str, const json& val)`: For `add`, `replace`, `test`.
    *   `JsonPatchOperation(Type op_type, const std::string& from_path, const std::string& to_path)`: For `move`, `copy`.
*   **Methods**:
    *   `json to_json() const`: Serializes the operation to its JSON representation.
    *   `static JsonPatchOperation from_json(const json& j)`: Deserializes an operation from its JSON representation.

### `struct JsonDiffOptions`

Configuration for generating diffs.

*   **Members**:
    *   `bool detect_moves = false`: (Currently not fully implemented in basic diff) Tries to detect move operations in arrays.
    *   `bool use_test_operations = false`: (Currently not implemented in basic diff) Includes `test` operations for validation.
    *   `bool compact_patches = true`: Merges redundant operations (basic compaction currently).
*   **Default Constructor**: `JsonDiffOptions() = default;`

### `class JsonPatch`

Represents a JSON Patch (a list of operations).

*   **Constructors**:
    *   `JsonPatch() = default;`
    *   `explicit JsonPatch(std::vector<JsonPatchOperation> ops);`
*   **Static Methods**:
    *   `static JsonPatch diff(const json& from, const json& to, const JsonDiffOptions& options = JsonDiffOptions{})`: Generates a patch to transform `from` into `to`.
    *   `static JsonPatch from_json(const json& j)`: Creates a patch from its JSON array representation.
*   **Member Functions**:
    *   `json apply(const json& document) const`: Applies the patch to the document and returns the modified document. Throws `JsonPatchException` on error.
    *   `bool dry_run(const json& document) const`: Checks if the patch can be applied without errors. Returns `true` if successful, `false` otherwise.
    *   `json to_json() const`: Converts the patch to its JSON array representation.
    *   `JsonPatch invert(const json& original_document) const`: Generates an inverse patch. `original_document` is the document *before* the forward patch was applied.
    *   `bool has_conflict(const json& document) const`: Equivalent to `!dry_run(document)`.
    *   `JsonPatch compact() const`: Returns a new patch with redundant operations potentially compacted (basic implementation).
    *   `size_t size() const`: Returns the number of operations in the patch.
    *   `bool empty() const`: Checks if the patch has no operations.
    *   `auto begin() const`, `auto end() const`: Iterators over the operations.

### `JsonPatchException`

*   `class JsonPatchException : public std::runtime_error`: Thrown for errors during patch processing.

### Convenience Function

*   `JsonPatch diff(const json& from, const json& to, const JsonDiffOptions& options = JsonDiffOptions{})`: Same as `JsonPatch::diff`.

## Usage Examples

### Generating a Diff and Applying a Patch

```cpp
#include "jsonpatch.h"
#include "json.hpp" // nlohmann/json
#include <iostream>

using json = nlohmann::json;

int main() {
    json doc1 = {
        {"name", "Alice"},
        {"age", 30}
    };

    json doc2 = {
        {"name", "Bob"},
        {"age", 30},
        {"city", "New York"}
    };

    // Generate a patch to transform doc1 to doc2
    JsonPatch patch = JsonPatch::diff(doc1, doc2);

    std::cout << "Generated Patch:\n" << patch.to_json().dump(2) << std::endl;
    // Expected output might be:
    // [
    //   {
    //     "op": "replace",
    //     "path": "/name",
    //     "value": "Bob"
    //   },
    //   {
    //     "op": "add",
    //     "path": "/city",
    //     "value": "New York"
    //   }
    // ]

    // Apply the patch to doc1
    try {
        json patched_doc = patch.apply(doc1);
        std::cout << "Patched Document:\n" << patched_doc.dump(2) << std::endl;
        // Expected output:
        // {
        //   "name": "Bob",
        //   "age": 30,
        //   "city": "New York"
        // }
        assert(patched_doc == doc2);
    } catch (const JsonPatchException& e) {
        std::cerr << "Patch application failed: " << e.what() << std::endl;
    }

    return 0;
}
```

### Inverting a Patch

```cpp
#include "jsonpatch.h"
#include "json.hpp" // nlohmann/json
#include <iostream>
#include <cassert>


using json = nlohmann::json;

int main() {
    json original_doc = {{"value", 10}};
    json target_doc = {{"value", 20}, {"new_field", "added"}};

    JsonPatch forward_patch = JsonPatch::diff(original_doc, target_doc);
    json modified_doc = forward_patch.apply(original_doc);
    assert(modified_doc == target_doc);

    JsonPatch inverse_patch = forward_patch.invert(original_doc);
    json reverted_doc = inverse_patch.apply(modified_doc);

    std::cout << "Original Document:\n" << original_doc.dump(2) << std::endl;
    std::cout << "Modified Document:\n" << modified_doc.dump(2) << std::endl;
    std::cout << "Reverted Document:\n" << reverted_doc.dump(2) << std::endl;

    assert(reverted_doc == original_doc);
    return 0;
}
```

## Building and Running Examples

The example code is located in `examples/jsonpatch_example.cpp`. To build and run it (assuming a CMake setup):

1.  **Configure CMake**:
    ```bash
    cmake -S . -B build -DENABLE_EXAMPLES=ON # Assuming an option to enable examples
    ```
2.  **Build**:
    ```bash
    cmake --build build
    ```
3.  **Run**: The executable might be found in `build/examples/jsonpatch_example` or similar, depending on your CMake configuration.
    ```bash
    ./build/examples/jsonpatch_example
    ```

## Building and Running Tests

The tests for `jsonpatch.h` are located in `tests/jsonpatch_test.cpp` and use the Google Test framework.

1.  **Configure CMake** (ensure tests are enabled, which is typical):
    ```bash
    cmake -S . -B build
    ```
2.  **Build**:
    ```bash
    cmake --build build
    ```
3.  **Run Tests**: Tests can be run via CTest or by directly executing the test binary.
    *   Using CTest (from the `build` directory):
        ```bash
        ctest
        # Or for verbose output:
        # ctest -V
        # To run a specific test (if its name is known, e.g., jsonpatch_test):
        # ctest -R jsonpatch_test
        ```
    *   Directly (the executable path might vary, e.g., `build/tests/jsonpatch_test`):
        ```bash
        ./build/tests/jsonpatch_test
        ```

This assumes the main `CMakeLists.txt` and `tests/CMakeLists.txt` are configured to build examples and tests, and that `nlohmann/json` and GTest are correctly fetched or found by CMake.
The specific CMake option `-DENABLE_EXAMPLES=ON` is a guess; refer to the project's main `CMakeLists.txt` or build instructions for actual options.
The test executable name (`jsonpatch_test`) is based on the filename `jsonpatch_test.cpp` as per the `tests/CMakeLists.txt` logic.
```

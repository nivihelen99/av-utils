# Quotient Filter

## Overview

A Quotient Filter is a probabilistic data structure used to determine whether an element *may be* in a set, or is *definitively not* in the set. It is a space-efficient alternative to Bloom filters, often offering better cache performance and the ability to be resized or merged (though this specific implementation might not support all advanced operations like dynamic resizing without careful consideration).

Key characteristics:
- **Probabilistic**: Returns either "possibly in set" or "definitively not in set". False positives are possible, but false negatives are not.
- **Space Efficient**: Uses a compact representation for stored elements.
- **Good Locality of Reference**: Can offer better performance than traditional Bloom filters in some scenarios due to how data is stored and accessed.

This implementation `QuotientFilter.h` provides a template class `QuotientFilter<T>` that can store elements of type `T`, assuming `std::hash<T>` is available.

## Building the Project

The project uses CMake to manage the build process for examples and tests.

### Prerequisites
- A C++ compiler supporting C++20 (e.g., GCC, Clang, MSVC)
- CMake (version 3.10 or higher)

### Build Steps

1.  **Create a build directory**:
    It's good practice to build out-of-source.
    ```bash
    mkdir build
    cd build
    ```

2.  **Configure the project using CMake**:
    From the `build` directory:
    ```bash
    cmake ..
    ```
    This will detect your compiler and set up the build system (e.g., Makefiles on Linux/macOS, Visual Studio solution on Windows).

3.  **Compile the project**:
    Still in the `build` directory:
    ```bash
    # For Makefiles (Linux/macOS)
    make

    # For Visual Studio (Windows - run from a developer command prompt)
    # cmake --build .
    # Or open the generated .sln file in Visual Studio and build.

    # For Ninja (if configured)
    # ninja
    ```
    This will compile all examples (including `quotient_filter_example`) and all tests (including `test_quotient_filter`).

### Running Examples and Tests

-   **Examples**: Executables for examples will be placed in the `build` directory (or a subdirectory like `build/examples/` depending on CMake configuration, but typically directly in `build` for this project structure).
    ```bash
    ./quotient_filter_example  # Assuming you are in the build directory
    ```

-   **Tests**: Test executables will also be in the `build` directory (often under `build/tests/` or directly in `build`). You can run them individually:
    ```bash
    ./test_quotient_filter   # Assuming it's in build/ or build/tests/
    ```
    Alternatively, you can use CTest to run all discovered tests from the `build` directory:
    ```bash
    ctest
    # For more verbose output:
    # ctest -V
    ```

## Basic Usage

Here's a simple example of how to use the `QuotientFilter`:

```cpp
#include "QuotientFilter.h"
#include <iostream>
#include <string>
#include <vector>

int main() {
    // Create a Quotient Filter for approximately 1000 string items
    // with an expected false positive rate of 0.5% (0.005).
    QuotientFilter<std::string> qf(1000, 0.005);

    // Add some items
    qf.add("hello");
    qf.add("world");
    qf.add("example");

    std::cout << "Filter size: " << qf.size() << std::endl; // Expected: 3

    // Check for items
    std::string item1 = "hello";
    if (qf.might_contain(item1)) {
        std::cout << "\"" << item1 << "\" might be in the filter." << std::endl;
    } else {
        std::cout << "\"" << item1 << "\" is definitely NOT in the filter." << std::endl;
    }

    std::string item2 = "test";
    if (qf.might_contain(item2)) {
        std::cout << "\"" << item2 << "\" might be in the filter (could be a false positive)." << std::endl;
    } else {
        std::cout << "\"" << item2 << "\" is definitely NOT in the filter." << std::endl;
    }

    // Adding an item that might already be present (due to hash collision or actual presence)
    // The `add` method typically checks `might_contain` first. If true, it might not add again
    // or re-insert, depending on the specific implementation details to avoid increasing size count
    // for logical duplicates or to handle hash collisions appropriately.
    // The current implementation's `add` will not increment size if `might_contain` is true.
    qf.add("hello");
    std::cout << "Filter size after adding 'hello' again: " << qf.size() << std::endl; // Expected: still 3

    return 0;
}

```

### Key Parameters for Construction

`QuotientFilter<T>(size_t expected_items, double fp_rate)`:
-   `expected_items`: The number of unique items you expect to insert into the filter. This is used to calculate the optimal size of the filter's internal structures.
-   `fp_rate`: The desired false positive probability (e.g., `0.01` for 1%). This also influences the size and configuration of the filter. The actual false positive rate may vary slightly.

Ensure that `expected_items` is greater than 0 and `fp_rate` is between 0.0 (exclusive) and 1.0 (exclusive). Invalid parameters will typically cause `std::invalid_argument` to be thrown.
The filter may become "full" if you attempt to insert significantly more items than its calculated capacity (which is related to `expected_items` and an internal load factor). Adding to a full filter will throw `std::runtime_error`.

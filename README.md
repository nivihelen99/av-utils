This repository contains a collection of various C++ data structures and libraries. Many of these components are header-only, allowing for easy integration into your projects. The repository also includes examples and tests to demonstrate usage and verify functionality.

## Getting Started

Since most components in this repository are header-only, integrating them into your C++ project is straightforward.

1.  **Include the desired header:** Simply include the relevant `.h` file from the `include/` directory in your C++ source code. For example, to use the `bimap` component:

    ```cpp
    #include "bimap.h"
    ```
    *(Assuming `bimap.h` is accessible via your include paths.)*

2.  **Configure Include Paths:** Ensure that your build system (e.g., CMake, Makefile, Visual Studio project settings) is configured to find the header files in the `include/` directory of this repository.
    *   If you've cloned this repository directly into your project or as a submodule, you might add `path_to_this_repo/include` to your compiler's include search paths.
    *   For CMake-based projects, you can use `target_include_directories()`:
        ```cmake
        # Assuming this library is located at "libs/cpp_utils" relative to your CMakeLists.txt
        target_include_directories(YourTargetName PRIVATE libs/cpp_utils/include)
        ```

The `examples/` directory contains various code samples that demonstrate how to integrate and use these components. These examples can serve as a practical guide.

## Highlighted Categories/Components

This section provides an overview of some of the key categories and components available in this repository.

### Core Data Structures
This category includes fundamental and advanced data structures for efficient data storage and retrieval.
It features:
*   **Tree and List Structures:** `skiplist.h`, `trie.h`, `fenwick_tree.h`, `interval_tree.h` (maps intervals to values).
*   **Associative Containers:** `bimap.h` (bi-directional map), `dict.h` (Python-style dictionary), `const_dict.h` (compile-time constant dictionary), `flatMap.h` (sorted vector-based map), `chain_map.h` (view over multiple maps), `delta_map.h` (tracks changes to a map).
*   **Set-like Structures:** `ordered_set.h` (preserves insertion order), `bounded_set.h` (fixed-capacity with FIFO eviction), `disjoint_set_union.h` (DSU, for tracking disjoint sets).
*   **Specialized Arrays and Storage:** `persist_array.h` (memory-mapped array), `SlotMap.h` (stable keyed storage with O(1) operations).
*   **Queues:** `heap_queue.h` (priority queue), `round_robin_queue.h`, `unique_queue.h` (elements are unique), `spsc.h` (single-producer, single-consumer).
*   **Specialized Maps:** `interval_map.h` (maps values to intervals, distinct from `interval_tree.h`), `enum_map.h` (map keyed by enum values).
*   **Sorted Lists:** `sorted_list.h` (elements kept sorted).

### Caching Libraries
These components offer various caching mechanisms to improve performance by storing frequently accessed data.
Examples include:
*   `lru_cache.h`: Implements a Least Recently Used (LRU) cache.
*   `expiring_containers.h`: Provides `TimeStampedQueue` and `ExpiringDict` where entries auto-expire based on TTL.
*   `retain_latest.h`: Cache that retains the latest N items.

### Networking Utilities
This category includes utilities specifically designed for networking applications.
Key components:
*   MAC address manipulation (`MacAddress.h`, `mac_parse.h`).
*   ARP and ND cache implementations (`arp_cache.h`, `nd_cache.h`).
*   Topic-based filtering (`topic_filter.h`).
*   Policy-based routing and TCAM (`policy_radix.h` - radix tree for routing policies, `tcam.h` - Ternary Content-Addressable Memory).
*   Distributed ID allocation (`redis_id_allocator.h` - uses Redis backend, `IDAllocator.h` - general unique ID allocation).

### Language/Utility Extensions
These components extend C++ with useful general-purpose utilities and language-like features.
This includes:
*   **Containers with Special Properties:** `default_dict.h` (default values for missing keys), `counter.h` (frequency counting).
*   **Value/Error Handling:** `expected.h` (Rust-like Result type), `value_or_error.h` (similar to expected, for simpler cases).
*   **Metaprogramming and Reflection:** `named_struct.h`, `named_tuple.h`, `enum_reflect.h` (compile-time enum reflection), `type_name.h` (get string representation of a type).
*   **Concurrency, Timing, and Asynchronous Operations:**
    *   Queues: `AsyncEventQueue.h` (thread-safe event queue), `call_queue.h` (queue for function calls).
    *   Execution Control: `delayed_call.h` (deferred execution), `retry.h` (retry logic for operations), `run_once.h` (ensure a function runs only once).
    *   Timing: `timer_wheel.h` (efficient timer management), `timer_queue.h`, `scoped_timer.h` (measure execution time).
    *   Synchronization: `named_lock.h` (named global locks).
    *   Threading: `thread_pool.h`, `worker_pool.h`.
*   **Resource Management:** `context_mgr.h` (RAII context managers, scope guards), `scoped_flag.h` (RAII boolean flag setter), `delayed_init.h` (delay object initialization).
*   **Functional Programming & Pipelines:** `partial.h` (function argument binding), `function_pipeline.h`, `optional_pipeline.h` (chain operations on optionals).
*   **Iterator and Algorithm Utilities:** `any_all.h` (truth testing for containers), `group_by_consecutive.h` (groups consecutive items), `lazy_sorted_merger.h` (merges sorted sequences lazily), `zip_view.h` (Python-like `zip` and `enumerate`), `peekable.h` (iterator with `peek()` capability), `split_view.h` (view over a string split by a delimiter).
*   **JSON Utilities:** `json_fieldmask.h` (apply field masks to JSON), `jsonpatch.h` (apply JSON patches).
*   **String Utilities:** `string_utils.h` (various string manipulation helpers), `duration_parser.h` (parse string to duration).
*   **Miscellaneous Utilities:** `undo.h` (undo/redo functionality), `sliding_window.h` (generic sliding window), `sliding_window_minmax.h` (tracks min/max in a sliding window), `simple_moving_average.h`, `optional_utilities.h` (helpers for `std::optional`), `weighted_round_robin.h` (weighted round-robin selection).

## Documentation

Many components have dedicated README files in the `docs/` directory, providing more detailed explanations, usage examples, and API references. Users are encouraged to consult these for in-depth information on specific libraries.

For example, to learn more about `AsyncEventQueue.h`, you can read `docs/README_AsyncEventQueue.md`.

## Building and Testing Examples and Tests

The examples and tests included in this repository are built using CMake. The `build_and_test.sh` script in the root directory automates this process.

### Prerequisites

*   A C++17 compliant compiler (e.g., GCC, Clang, MSVC).
*   CMake (version 3.10 or higher).

### Steps

1.  **Navigate to the repository root.**

2.  **Run the build script:**
    ```bash
    ./build_and_test.sh
    ```
    This script will:
    *   Create a `build` directory (if it doesn't exist).
    *   Run CMake to configure the project.
    *   Compile all examples and tests using `make`.
    *   Run all tests using `ctest`.

    Alternatively, you can perform these steps manually:

    a.  Create a build directory and navigate into it:
        ```bash
        mkdir -p build
        cd build
        ```

    b.  Configure the project using CMake:
        ```bash
        cmake ..
        ```
        *   You may need to specify your C++ compiler if it's not the system default (e.g., `cmake .. -DCMAKE_C_COMPILER=gcc-10 -DCMAKE_CXX_COMPILER=g++-10`).
        *   The `CMakeLists.txt` uses `FetchContent` to download Google Test and nlohmann/json if they are not found.

    c.  Compile the project:
        ```bash
        make -j$(nproc)
        ```
        (or simply `make` if `nproc` is not available or you prefer a single-threaded build).

### Running Examples

After a successful build, the example executables will be located in the `build` directory (or `build/examples` if `CMAKE_RUNTIME_OUTPUT_DIRECTORY` was set, though the current `CMakeLists.txt` places them directly in `build`).

For example, to run `bimap_example`:
```bash
./build/bimap_example
```
*(Adjust path if your build directory or executable location differs)*

### Running Tests

The `build_and_test.sh` script automatically runs all tests using CTest.

To run tests manually after building:
1.  Navigate to the `build` directory:
    ```bash
    cd build
    ```
2.  Run CTest:
    ```bash
    ctest --output-on-failure
    ```
    This command will execute all registered tests and show output only for failing tests.

## Contributing and License

### Contributing

Contributions are welcome! Please feel free to open an issue to report bugs, suggest features, or ask questions. If you'd like to contribute code, please fork the repository and submit a pull request with your changes.

### License

This project is currently unlicensed. This means you are free to use, modify, and distribute the code, but there are no warranty protections or formal permissions granted.

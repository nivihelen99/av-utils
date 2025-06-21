This repository contains a collection of various C++ data structures and libraries. Many of these components are header-only, allowing for easy integration into your projects. The repository also includes examples and tests to demonstrate usage and verify functionality.

## Highlighted Categories/Components

This section provides an overview of some of the key categories and components available in this repository.

### Core Data Structures
This category includes fundamental and advanced data structures for efficient data storage and retrieval.
It features:
*   Tree and list structures (e.g., `skiplist.h`, `skiplist_std.h` - concurrent skip list, `trie.h`, `fenwick_tree.h`).
*   Associative containers (e.g., `bimap.h`, `dict.h`, `flatMap.h` - sorted vector-based map, `chain_map.h` - view over multiple maps).
*   Set-like structures (e.g., `ordered_set.h` - preserves insertion order, `bounded_set.h` - fixed-capacity with FIFO eviction).
*   Specialized arrays and vectors (e.g., `persist_array.h`, `variant_vector.h` - stores elements of mixed types).
*   Queues (e.g., `heap_queue.h` - priority queue with custom key/comparison).
*   Specialized maps (e.g., `interval_map.h` - maps values to intervals, `slot_map_new.h` - stable storage with O(1) ops via keys).
*   Union-Find structures (e.g., `disjoint_set_union.h` - generic and integer-optimized DSU).
*   Sorted lists (e.g., `sorted_list_bisect.h` - elements kept sorted).

### Caching Libraries
These components offer various caching mechanisms to improve performance by storing frequently accessed data.
Examples include:
*   `lru_cache.h`: Implements a Least Recently Used (LRU) cache.
*   `expiring_containers.h`: Provides `TimeStampedQueue` and `ExpiringDict` where entries auto-expire based on TTL.

### Networking Utilities
This category includes utilities specifically designed for networking applications.
Key components:
*   MAC address manipulation (`MacAddress.h`, `mac_parse.h`).
*   ARP and ND cache implementations (`arp_cache.h`, `nd_cache.h`).
*   Topic-based filtering (`topic_filter.h`).
*   Policy-based routing and TCAM (`policy_radix.h` - radix tree for routing policies, `tcam.h` - Ternary Content-Addressable Memory).
*   Distributed ID allocation (`redis_id_allocator.h` - uses Redis backend).

### Language/Utility Extensions
These components extend C++ with useful general-purpose utilities and language-like features.
This includes:
*   Containers with special properties (e.g., `default_dict.h` - default values for missing keys, `counter.h` - frequency counting).
*   Value/error type handling (e.g., `expected.h`).
*   Metaprogramming and reflection (e.g., `named_struct.h`, `named_tuple.h`, `enum_reflect.h` - compile-time enum reflection).
*   Concurrency and timing (e.g., `spsc.h` - single-producer/consumer queue, `AsyncEventQueue.h` - thread-safe event queue, `delayed_call.h` - deferred execution, `timer_wheel.h` - efficient timer management).
*   Resource management (e.g., `context_mgr.h` - RAII context managers, scope guards).
*   Functional programming utilities (e.g., `partial.h` - function argument binding).
*   Iterator and algorithm utilities (e.g., `any_all.h` - truth testing for containers, `group_by_consecutive.h` - groups consecutive items, `lazy_sorted_merger.h` - merges sorted sequences lazily, `zip_view.h` - Python-like `zip` and `enumerate`).
*   Miscellaneous utilities (e.g., `undo.h` - undo/redo functionality, `IDAllocator.h` - unique ID allocation, `sliding_window_minmax.h` - tracks min/max in a sliding window).

## General Usage

Most components in this repository are header-only. To use them, simply include the relevant `.h` file in your C++ source code. For example, to use the `bimap` component, you would add:

```cpp
#include "bimap.h"
```
Ensure that your build system (e.g., CMake, Makefile) is configured to correctly resolve the include paths for these headers. For example, you might add the directory containing these headers to your include search paths.

The `examples/` directory contains various code samples that demonstrate how to integrate and use these components. These examples can serve as a practical guide for getting started.

## Building and Testing

The examples and tests included in this repository are built using CMake.

### Prerequisites

*   A C++17 compliant compiler (e.g., GCC, Clang, MSVC).
*   CMake (version 3.10 or higher recommended).

### Building Examples and Tests

Follow these steps to configure and build the project:

1.  Create a build directory (e.g., `build`) and navigate into it:
    ```bash
    mkdir build
    cd build
    ```

2.  Configure the project using CMake:
    ```bash
    cmake ..
    ```
    *Note: You might need to specify your compiler if it's not the system default, e.g., `cmake .. -DCMAKE_C_COMPILER=gcc-10 -DCMAKE_CXX_COMPILER=g++-10`.*

3.  Compile the project:
    ```bash
    cmake --build .
    ```
    Alternatively, on Unix-like systems, you can use `make`:
    ```bash
    make
    ```

### Running Examples and Tests

After a successful build, the compiled executables for examples and tests will be located in the `build` directory (or a subdirectory within it, depending on the CMake configuration).

*   **Running Examples**: Navigate to the directory containing the example executable and run it directly, for example:
    ```bash
    ./examples/example_bimap
    ```
    *(The exact path and name will depend on how examples are structured and named in the `CMakeLists.txt` file.)*

*   **Running Tests**: If the project uses CTest (CMake's testing framework), you can run all tests from the build directory:
    ```bash
    ctest
    ```
    Alternatively, individual test executables might be produced, which can be run directly:
    ```bash
    ./tests/run_all_tests
    ```
    *(The exact command will depend on how tests are defined in the `CMakeLists.txt` file.)*

## Contributing and License

### Contributing

Contributions are welcome! Please feel free to open an issue to report bugs, suggest features, or ask questions. If you'd like to contribute code, please fork the repository and submit a pull request with your changes.

### License

This project is currently unlicensed. This means you are free to use, modify, and distribute the code, but there are no warranty protections or formal permissions granted.

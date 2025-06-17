This repository contains a collection of various C++ data structures and libraries. Many of these components are header-only, allowing for easy integration into your projects. The repository also includes examples and tests to demonstrate usage and verify functionality.

## Highlighted Categories/Components

This section provides an overview of some of the key categories and components available in this repository.

### Core Data Structures
This category includes fundamental and advanced data structures for efficient data storage and retrieval. It features various tree and list structures (e.g., `skiplist.h`, `trie.h`, `fenwick_tree.h`), associative containers (e.g., `bimap.h`, `dict.h`), and specialized arrays (e.g., `persist_array.h`).

### Caching Libraries
These components offer various caching mechanisms to improve performance by storing frequently accessed data. A key example is the `lru_cache.h`, which implements a Least Recently Used (LRU) cache.

### Networking Utilities
This category includes utilities specifically designed for networking applications, such as MAC address manipulation (`MacAddress.h`, `mac_parse.h`), ARP and ND cache implementations (`arp_cache.h`, `nd_cache.h`), and topic-based filtering (`topic_filter.h`).

### Language/Utility Extensions
These components extend C++ with useful general-purpose utilities and language-like features. This includes containers with default values (`default_dict.h`), value/error type handling (`expected.h`), named struct/tuple utilities (`named_struct.h`, `named_tuple.h`), thread-safe queues (`spsc.h`), and undo/redo functionality (`undo.h`).

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

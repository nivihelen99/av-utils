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
*   **Tree and List Structures:** [BTree](docs/README_btree.md), `fenwick_tree.h` ([FenwickTree](docs/README_fenwick_tree.md)), [Graph](docs/README_Graph.md), `interval_tree.h`, `radix_tree.h`, [RedBlackTree](docs/README_RedBlackTree.md), [ScapegoatTree](docs/README_ScapegoatTree.md), `segment_tree.h` ([SegmentTree](docs/README_SegmentTree.md)), `SkipList.h` ([SkipList](docs/README_SkipList.md)), `treap.h`, `trie.h`.
*   **Associative Containers:** `bimap.h` ([Bimap](docs/README_bimap.md)), `chain_map.h` ([ChainMap](docs/README_chain_map.md)), `const_dict.h` ([ConstDict](docs/README_const_dict.md)), `delta_map.h` ([DeltaMap](docs/README_delta_map.md)), `dict.h` ([Dict](docs/README_dict.md)), `flat_map.h` ([FlatMap](docs/README_flat_map.md)), `flatMap.h` ([FlatMap (alt)](docs/README_flatMap.md)), [FrozenCounter](docs/README_FrozenCounter.md), [FrozenDict](docs/README_FrozenDict.md), [OrderedDict](docs/README_OrderedDict.md), `sharded_map.h`.
*   **Set-like Structures:** `bounded_set.h` ([BoundedSet](docs/README_bounded_set.md)), `disjoint_set_union.h` ([DisjointSetUnion](docs/README_disjoint_set_union.md)), [FrozenSet](docs/README_FrozenSet.md), `ordered_set.h`, [UnorderedMultiset](docs/README_UnorderedMultiset.md).
*   **Specialized Arrays and Storage:** `chrono_ring.h` ([ChronoRing](docs/README_chrono_ring.md)), [DynamicBitset](docs/README_DynamicBitset.md), `instrumented_ring_buffer.h`, `persist_array.h`, `ring_buffer.h`, `SlotMap.h` ([SlotMap](docs/README_SlotMap.md)), `small_vector.h`, [SuffixArray](docs/README_SuffixArray.md).
*   **Queues:** `deque.h` ([Deque](docs/README_Deque.md)), `heap_queue.h`, `mpsc_queue.h`, `PriorityQueueMap.h` ([PriorityQueueMap](docs/README_PriorityQueueMap.md)), `round_robin_queue.h`, `spsc_queue.h`, `unique_queue.h`.
*   **Specialized Maps:** `enum_map.h` ([EnumMap](docs/README_enum_map.md)), `interval_map.h`, `inverted_index.h`.
*   **Sorted Lists:** `sorted_list.h`.
*   **Probabilistic Data Structures:** `bloom_filter.h` ([BloomFilter](docs/README_bloom_filter.md)), `count_min_sketch.h` ([CountMinSketch](docs/README_count_min_sketch.md)), `counting_bloom_filter.h` ([CountingBloomFilter](docs/README_CountingBloomFilter.md)), [HyperLogLog](docs/README_HyperLogLog.md), `QuotientFilter.h`.

### Caching Libraries
These components offer various caching mechanisms to improve performance by storing frequently accessed data.
Examples include:
*   [CachedProperty](docs/README_CachedProperty.md), `expiring_containers.h` ([ExpiringContainers](docs/README_expiring_containers.md)), `lru_cache.h`, `retain_latest.h`.

### Networking Utilities
This category includes utilities specifically designed for networking applications.
Key components:
*   MAC address manipulation (`MacAddress.h` ([MacAddress](docs/README_MacAddress.md)), `mac_parse.h`).
*   ARP and ND cache implementations (`arp_cache.h` ([ArpCache](docs/README_arp_cache.md)), `nd_cache.h`).
*   Distributed ID allocation (`IDAllocator.h` ([IDAllocator](docs/README_IDAllocator.md)), `redis_id_allocator.h`).
*   `prometheus_client.h`.
*   Policy-based routing and TCAM (`policy_radix.h`, `tcam.h`).
*   Topic-based filtering (`topic_filter.h`).


### Language/Utility Extensions
These components extend C++ with useful general-purpose utilities and language-like features.
This includes:
*   **Containers with Special Properties:** `counter.h` ([Counter](docs/README_counter.md)), `default_dict.h` ([DefaultDict](docs/README_default_dict.md)), `dict_wrapper.h` ([DictWrapper](docs/README_dict_wrapper.md)), `multiset_counter.hpp`.
*   **Value/Error Handling:** `expected.h` ([Expected](docs/README_expected.md)), `tagged_variant.h`, `value_or_error.h`.
*   **Metaprogramming and Reflection:** `enum_reflect.h` ([EnumReflect](docs/README_enum_reflect.md)), `named_struct.h`, `named_tuple.h`, `type_name.h`.
*   **Concurrency, Timing, and Asynchronous Operations:**
    *   Queues: `AsyncEventQueue.h` ([AsyncEventQueue](docs/README_AsyncEventQueue.md)), `call_queue.h` ([CallQueue](docs/README_callq.md)).
    *   Execution Control: `delayed_call.h` ([DelayedCall](docs/README_delayed_call.md)), `rate_limiter.h`, `retry.h`, `run_once.h`, `token_bucket.h`.
    *   Timing: `scoped_timer.h`, `stopwatch.h`, `timer_queue.h`, `timer_wheel.h`.
    *   Synchronization: `named_lock.h`, `semaphore.h`.
    *   Threading: `thread_pool.h`, `worker_pool.h`.
*   **Resource Management:** `context_mgr.h` ([ContextMgr](docs/README_context_mgr.md)), `delayed_init.h` ([DelayedInit](docs/README_delayed_init.md)), `memory_mapped_file.h`, `object_pool.h`, `observer_ptr.h`, `scoped_flag.h`, `weak_ref.h`.
*   **Functional Programming & Pipelines:** `function_pipeline.h`, `optional_pipeline.h`, `partial.h`, `unique_function.h`.
*   **Iterator and Algorithm Utilities:** `any_all.h` ([AnyAll](docs/README_any_all.md)), `group_by_consecutive.h`, `lazy_sorted_merger.h`, `peekable.h`, `split_view.h`, `zip_view.h`.
*   **JSON Utilities:** `json_fieldmask.h`, `jsonpatch.h`.
*   **String Utilities:** `cord.h` ([Cord](docs/README_Cord.md)), `duration_parser.h`, `interning_pool.h`, `parse_utils.h`, `random_string.h`, `string_interner.h`, `string_utils.h`.
*   **Miscellaneous Utilities:** `batcher.h` ([Batcher](docs/README_batcher.md)), `id_pool.h`, `interval_counter.h`, `optional_utilities.h`, `pretty_print.h`, `safe_numerics.h`, `signal_handler.h`, `simple_moving_average.h`, `singleton.h`, `sliding_window_minmax.h`, `sliding_window.h`, `type_safe_id.h`, `undo.h`, `varint.h`, `version.h`, `WeightedRandomList.h`, `weighted_round_robin.h`.

## Documentation

Many components have dedicated README files in the `docs/` directory, providing more detailed explanations, usage examples, and API references. Users are encouraged to consult these for in-depth information on specific libraries.

For example, to learn more about `AsyncEventQueue.h`, you can read `docs/README_AsyncEventQueue.md`.
Similarly, details for the Bloom Filter can be found in `docs/README_bloom_filter.md`.
Information on the Treap data structure is available in `docs/README_treap.md`.

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

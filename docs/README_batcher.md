# Batcher Utility (`batcher.h`)

## Overview

The `batcher.h` header provides a C++ utility for iterating over elements of a container in fixed-size batches (or chunks). This is useful in scenarios where data needs to be processed in smaller, manageable segments, such as sending data over a network in packets, processing large datasets in parts to manage memory, or parallelizing work.

The utility consists of:
- `BatchIterator`: An iterator that yields a `std::vector` representing the current batch of elements.
- `BatchView`: A view class that wraps a container and provides `begin()` and `end()` methods returning `BatchIterator`s. This enables easy use with range-based for loops.
- `batcher()`: A factory function to conveniently create `BatchView` objects.

It is designed to be generic and work with any container type that provides at least forward iterators.

## Features

-   **Generic Container Support:** Works with `std::vector`, `std::list`, `std::deque`, and other STL-like containers.
-   **Easy Integration:** Uses standard iterator patterns and is easily usable with range-based for loops.
-   **Const Correctness:** Handles both mutable and `const` containers correctly.
-   **Customizable Batch Size:** The desired size of each batch can be specified. The last batch may be smaller if the total number of elements is not an exact multiple of the batch size.
-   **Informative View:** `BatchView` provides methods to get the number of batches (`size()`), the configured `chunk_size()`, and check if the underlying container is `empty()`.

## Core Components

### `batcher_utils::BatchIterator<Iterator>`
-   A forward iterator.
-   When dereferenced (`operator*`), it returns a `std::vector<OriginalValueType>` containing the elements of the current batch.

### `batcher_utils::BatchView<Container>`
-   Wraps a reference to the original container.
-   Provides `begin()` and `end()` methods that return `BatchIterator`s, allowing it to be used in range-based for loops.
-   Has a specialization for `const Container` to ensure const-correct iteration.

### `batcher(Container& container, std::size_t chunk_size)`
### `batcher(const Container& container, std::size_t chunk_size)`
-   Factory functions that are the primary way to use this utility. They construct and return a `BatchView`.

## Usage Examples

(Based on `examples/batcher_example.cpp`)

### Basic Usage with `std::vector`

```cpp
#include "batcher.h" // Assuming batcher.h is in the include path
#include <iostream>
#include <vector>

int main() {
    std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::size_t batch_sz = 3;

    std::cout << "Original numbers: ";
    for(int n : numbers) std::cout << n << " ";
    std::cout << "\n\nProcessing in batches of " << batch_sz << ":" << std::endl;

    for (const auto& batch : batcher(numbers, batch_sz)) {
        std::cout << "Current batch: ";
        for (int value : batch) {
            std::cout << value << " ";
        }
        std::cout << "(batch size: " << batch.size() << ")" << std::endl;
        // Process the 'batch' vector here
    }
    // Expected output:
    // Current batch: 1 2 3 (batch size: 3)
    // Current batch: 4 5 6 (batch size: 3)
    // Current batch: 7 8 9 (batch size: 3)
    // Current batch: 10 (batch size: 1)
}
```

### With Different Container Types (e.g., `std::list`)

```cpp
#include "batcher.h"
#include <iostream>
#include <list>
#include <string>

int main() {
    std::list<std::string> words = {"alpha", "beta", "gamma", "delta", "epsilon", "zeta"};
    std::size_t desired_batch_size = 2;

    for (const auto& word_batch : batcher(words, desired_batch_size)) {
        std::cout << "Processing batch: ";
        for (const std::string& word : word_batch) {
            std::cout << "\"" << word << "\" ";
        }
        std::cout << std::endl;
    }
    // Expected output:
    // Processing batch: "alpha" "beta"
    // Processing batch: "gamma" "delta"
    // Processing batch: "epsilon" "zeta"
}
```

### Handling `const` Containers

```cpp
#include "batcher.h"
#include <iostream>
#include <vector>

int main() {
    const std::vector<double> const_data = {1.1, 2.2, 3.3, 4.4, 5.5};
    std::size_t chunk = 2;

    std::cout << "Iterating over a const vector in chunks of " << chunk << ":" << std::endl;
    for (const auto& batch : batcher(const_data, chunk)) {
        std::cout << "  Const Batch: ";
        for (double val : batch) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }
    // Expected output:
    // Const Batch: 1.1 2.2
    // Const Batch: 3.3 4.4
    // Const Batch: 5.5
}
```

### Edge Cases (Empty Container, Small Container)

```cpp
#include "batcher.h"
#include <iostream>
#include <vector>

int main() {
    std::vector<int> empty_vector;
    std::cout << "Batches from empty vector (chunk size 3):" << std::endl;
    for (const auto& batch : batcher(empty_vector, 3)) {
        // This loop body will not execute
        std::cout << "This should not print." << std::endl;
    }
    std::cout << "(No batches printed, as expected)" << std::endl;

    std::vector<int> small_vector = {100, 200};
    std::cout << "\nBatches from vector {100, 200} (chunk size 3):" << std::endl;
    for (const auto& batch : batcher(small_vector, 3)) {
        std::cout << "  Batch: ";
        for (int val : batch) {
            std::cout << val << " ";
        }
        std::cout << "(size: " << batch.size() << ")" << std::endl;
    }
    // Expected output:
    // Batch: 100 200 (size: 2)
}
```

### Using `BatchView` Information

```cpp
#include "batcher.h"
#include <iostream>
#include <vector>

int main() {
    std::vector<int> my_data = {0, 1, 2, 3, 4, 5, 6, 7};
    auto view = batcher(my_data, 3);

    std::cout << "Underlying data size: " << my_data.size() << std::endl;
    std::cout << "Configured chunk size: " << view.chunk_size() << std::endl;
    std::cout << "Number of batches expected: " << view.size() << std::endl; // (8 + 3 - 1) / 3 = 3
    std::cout << "Is BatchView empty? " << std::boolalpha << view.empty() << std::endl;

    int i = 0;
    for (const auto& batch : view) {
        std::cout << "Batch #" << ++i << " has " << batch.size() << " elements." << std::endl;
    }
}
```

## Dependencies
- `<vector>` (used by `BatchIterator` for its `value_type`)
- `<iterator>` (for `std::iterator_traits`, `std::forward_iterator_tag`, `std::distance`)
- `<type_traits>` (not directly used in the provided snippet, but common in such utilities)
- `<cassert>` (for `assert`)

This utility simplifies the common task of processing container elements in batches, promoting cleaner and more readable code.

# C++ Data Structures: Skip List and Trie

## Introduction
This repository provides header-only C++ implementations of two advanced data structures: a highly concurrent Skip List and a feature-rich Radix Trie.
Designed for educational purposes, performance exploration, and practical application in scenarios requiring efficient data management and complex search capabilities.

## Features Overview
- **Templated Design**: The Skip List (`SkipList<T>`) is fully templated, allowing storage of various data types, including key-value pairs.
- **Concurrency (Skip List)**: The Skip List is designed with lock-free principles using `std::atomic` and Compare-And-Swap operations. (Important concurrency considerations and current limitations are detailed in its section).
- **Efficient Operations**: Both structures offer logarithmic or better average-case time complexity for search, insertion, and deletion.
- **Advanced Search (Trie)**: The Radix Trie supports exact match, prefix search, suffix search, wildcard search (`?`, `*`), and fuzzy search (Levenshtein distance).
- **Radix Trie Implementation**: The Trie compresses paths by storing strings on edges, making it space-efficient for certain datasets.
- **Iterators**: Both data structures provide STL-compatible forward iterators for easy traversal, including support for range-based for loops.
- **Memory Management (Skip List)**: Includes a custom memory pool for efficient node allocation and reuse in the Skip List.
- **File I/O (Trie)**: The Trie implementation supports saving to and loading from files.

## How to Compile & Run Examples
The data structures are header-only. You just need to include `skiplist.h` or `trie.h` in your C++ project.
Example usage files are provided (`use_skip.cpp`, `example.cpp` which might use Trie, or a dedicated `use_trie.cpp` if created).

To compile an example (e.g., `use_skip.cpp`) with g++:
```bash
g++ -std=c++17 -o skip_example use_skip.cpp -pthread
```
(The `-pthread` flag is recommended for `skip_example` due to the Skip List's use of `std::atomic` and other concurrency-related features. It ensures proper linking of threading support if required by the system's C++ standard library implementation, though often not strictly needed for `std::atomic` itself on modern compilers.)

The `example.cpp` file contains comprehensive examples and tests for the Trie. To compile it:
```bash
g++ -std=c++17 -o trie_example example.cpp
```

To run the compiled examples:
```bash
./skip_example
./trie_example
```
Ensure your compiler supports C++17 for features like `if constexpr` and `std::atomic` used in the implementations.

## Skip List (`skiplist.h`)

A Skip List is a probabilistic data structure that allows for efficient search, insertion, and deletion operations, typically with an average time complexity of O(log n). It uses multiple levels of linked lists to "skip" over elements, achieving performance comparable to balanced trees while being conceptually simpler and often more amenable to concurrent access.

### Features & API

- **Templated `SkipList<T>`**: Can store various data types. For custom types, ensure comparison operators (`<`, `==`) are defined, or use `std::pair` for key-based comparison.
- **Key-Value Storage**: Natively supports `std::pair<const Key, Value>` as `T`. Comparisons are automatically performed on `Key`. `Key` and `Value` types must be default-constructible if used with the memory pool's default node initialization.
- **Core Operations**:
    - `void insert(T value)`: Inserts a value (or key-value pair).
    - `bool search(T value)`: Searches for a value (or by key if `T` is a pair).
    - `bool remove(T value)`: Removes a value (or by key if `T` is a pair).
    - `int size() const`: Returns the number of elements.
- **Utility Functions**:
    - `T kthElement(int k) const`: Finds the k-th smallest element (0-indexed). Throws `std::out_of_range` or `std::invalid_argument`.
    - `std::vector<T> rangeQuery(T minVal, T maxVal) const`: Returns elements in the range `[minVal, maxVal]` (inclusive, comparison by key if `T` is a pair).
- **Iterators**:
    - Provides STL-compatible forward iterators:
        - `iterator begin()`, `iterator end()`
        - `const_iterator cbegin() const`, `const_iterator cend() const`
        - `const_iterator begin() const`, `const_iterator end() const`
    - Supports range-based for loops for easy traversal.
- **Bulk Operations**:
    - `void insert_bulk(const std::vector<T>& values)`: Inserts multiple elements. Values are sorted internally first.
    - `size_t remove_bulk(const std::vector<T>& values)`: Removes multiple elements.
    - **Note**: These bulk operations are *not atomic* for the entire set of values; they perform individual insertions/deletions.
- **Performance Optimizations**:
    - **Memory Pool**: A custom memory manager pre-allocates nodes in blocks and reuses them via a lock-free stack and thread-local caches to reduce allocation overhead.
    - **Finger Search**: A `thread_local` hint (`thread_local_finger`) remembers the last search position to potentially speed up subsequent localized searches.

### Usage Examples

#### Basic Integer List

```cpp
#include "skiplist.h"
#include <iostream>

int main() {
    SkipList<int> sl;
    sl.insert(30);
    sl.insert(10);
    sl.insert(20);
    sl.insert(15);

    sl.display(); // Shows the layered structure

    std::cout << "Search 20: " << (sl.search(20) ? "Found" : "Not found") << std::endl;
    std::cout << "Search 25: " << (sl.search(25) ? "Found" : "Not found") << std::endl;

    sl.remove(20);
    std::cout << "Search 20 after removal: " << (sl.search(20) ? "Found" : "Not found") << std::endl;
    sl.printValues(); // Prints all values in sorted order
    return 0;
}
```

#### Iterator Usage

```cpp
#include "skiplist.h"
#include <iostream>
#include <vector>

int main() {
    SkipList<int> sl;
    std::vector<int> data = {50, 10, 80, 30, 60};
    for (int x : data) {
        sl.insert(x);
    }

    std::cout << "Iterating with iterators: ";
    for (SkipList<int>::iterator it = sl.begin(); it != sl.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    std::cout << "Iterating with range-based for: ";
    for (int val : sl) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
    return 0;
}
```

#### Key-Value Pair Usage

```cpp
#include "skiplist.h"
#include <iostream>
#include <string>
#include <utility> // For std::pair

int main() {
    SkipList<std::pair<const int, std::string>> kv_sl;

    kv_sl.insert({10, "apple"});
    kv_sl.insert({5, "banana"});
    kv_sl.insert({20, "cherry"});

    // Search by key (value part of pair can be dummy for search/remove)
    std::cout << "Search key 5: ";
    if (kv_sl.search({5, ""})) {
        // To get the actual value, you'd iterate or use a more complex find method
        // (Currently, search only returns bool. For full K-V, you might adapt search or use iterators)
        // For now, let's iterate to find it if simple search confirms existence
        for (const auto& p : kv_sl) {
            if (p.first == 5) {
                std::cout << "Found, value: " << p.second;
                break;
            }
        }
        std::cout << std::endl;
    } else {
        std::cout << "Not found" << std::endl;
    }

    std::cout << "Iterating through key-value pairs: ";
    for (const auto& p : kv_sl) {
        std::cout << "<" << p.first << ":" << p.second << "> ";
    }
    std::cout << std::endl;

    kv_sl.display(); // Shows structure with <key:value> format
    return 0;
}
```

### Concurrency Notes

The Skip List implementation in `skiplist.h` is designed with concurrency in mind, utilizing `std::atomic` operations and Compare-And-Swap (CAS) loops for its core logic (`insert()`, `remove()`, `search()`). This aims for a lock-free behavior where threads can operate simultaneously without acquiring traditional locks for most parts of the data structure traversal and modification.

- **Atomic Operations**: Node pointers (`forward`) and the list's `currentLevel` are managed using `std::atomic`.
- **Random Level Generation**: The `randomLevel()` method, crucial for determining a new node's height, uses `thread_local` random number generators to avoid mutex contention, enhancing scalability.
- **Memory Pool**: The custom memory pool uses a lock-free stack (`atomic_free_list_head_`) for managing freed nodes, contributing to concurrent performance.

**IMPORTANT WARNINGS & CONSIDERATIONS:**

1.  **Safe Memory Reclamation (SMR)**:
    - The current implementation **lacks a proper Safe Memory Reclamation (SMR) mechanism** (e.g., Hazard Pointers, Epoch-Based Reclamation, Quiescent State-Based Reclamation).
    - In the `remove()` operation, once a node is unlinked, its memory is returned to the pool by calling its destructor and then reconstructing it as a "dummy" node for reuse.
    - **Risk**: If other threads are concurrently traversing the list and hold pointers to this node *just before or during its unlinking and deallocation*, they might subsequently access freed or altered memory. This can lead to use-after-free errors, data corruption, or crashes.
    - **Status**: Without SMR, the `remove()` operation (and by extension `remove_bulk()`) is **not fully safe** for general concurrent use where nodes are frequently removed and memory is reused. It's safer in scenarios with infrequent removals or where external synchronization guarantees no thread is accessing a node during its removal.

2.  **ABA Problem**:
    - The Compare-And-Swap (CAS) loops used for linking and unlinking nodes are susceptible to the ABA problem. If a node's pointer `A` is read, then the node is removed, its memory recycled, and a new node allocated at the same address `A` which then becomes part of the list structure again, a CAS operation might incorrectly succeed thinking the state hasn't changed.
    - SMR techniques or tagged pointers are typical solutions to mitigate the ABA problem, which are not implemented here.

This Skip List is an excellent tool for understanding concurrent data structures but use in production systems with high concurrency on deletion paths would require integrating a robust SMR strategy.

## Trie (`trie.h`)

A Trie, also known as a prefix tree, is a tree-like data structure that stores a dynamic set of strings, typically used for efficient retrieval of words based on their prefixes. This repository implements a **Radix Trie** (also similar to a Patricia Trie), where edges can represent sequences of characters rather than just single characters. This compresses paths and makes the Trie more space-efficient for certain datasets.

### Features & API

- **Radix Trie Structure**: Edges represent strings, compressing common non-branching paths.
- **Case Sensitivity**: Configurable via the constructor `Trie(bool caseSensitive = true)`. If `false`, all words are normalized to lowercase internally.
- **Core Operations**:
    - `void insert(const std::string& word)`: Inserts a word. Handles splitting and merging edges as necessary for the Radix structure.
    - `bool search(const std::string& word) const`: Checks if an exact word exists in the Trie.
    - `bool deleteWord(const std::string& word)`: Deletes a word, including merging edges to maintain Radix properties if a node becomes redundant.
- **Prefix-based Operations**:
    - `bool startsWith(const std::string& prefix) const`: Checks if any word in the Trie starts with the given prefix.
    - `std::vector<std::string> getWordsWithPrefix(const std::string& prefix) const`: Returns all words that start with the given prefix.
- **Suffix-based Operations (via an internal reversed Trie)**:
    - `bool endsWith(const std::string& suffix) const`: Checks if any word ends with the given suffix.
    - `std::vector<std::string> getWordsEndingWith(const std::string& suffix) const`: Returns all words ending with the given suffix. This is efficiently implemented by querying an internal, automatically maintained reversed Trie.
- **Advanced Search Capabilities**:
    - `std::vector<std::string> wildcardSearch(const std::string& pattern) const`: Performs a wildcard search. Supports `?` to match any single character and `*` to match any sequence of characters (including empty).
    - `std::vector<std::pair<std::string, int>> fuzzySearch(const std::string& query_word, int max_k) const`: Finds words within a specified Levenshtein distance (`max_k`) of the `query_word`. Returns pairs of `(word, distance)`.
- **Iterators**:
    - Provides STL-compatible forward iterators:
        - `Iterator begin() const`, `Iterator end() const`
    - Supports range-based for loops for lexicographical traversal of all words.
- **File I/O**:
    - `bool saveToFile(const std::string& filename) const`: Saves all words and their frequencies (if tracked) to a file.
    - `bool loadFromFile(const std::string& filename)`: Loads words (and their frequencies) from a file into the Trie. Note: The `frequency` member in `TrieNode` is updated on `insert`, but `getWordFrequency()` is currently marked as non-functional in the source. The save/load functions do attempt to preserve frequency.
- **Word Frequency**:
    - `TrieNode` includes a `frequency` member, incremented on `insert`. `saveToFile` stores this. `loadFromFile` attempts to restore it. (As noted, `getWordFrequency(word)` is not fully implemented).

### Usage Examples

#### Basic Trie Operations

```cpp
#include "trie.h"
#include <iostream>
#include <vector>

int main() {
    Trie t(false); // Case-insensitive Trie

    t.insert("apple");
    t.insert("apricot");
    t.insert("application");
    t.insert("banana");

    std::cout << "Search 'apple': " << (t.search("Apple") ? "Found" : "Not found") << std::endl;
    std::cout << "Search 'apply': " << (t.search("apply") ? "Found" : "Not found") << std::endl;
    std::cout << "Starts with 'app': " << (t.startsWith("App") ? "Yes" : "No") << std::endl;

    std::cout << "Words starting with 'app':" << std::endl;
    std::vector<std::string> words = t.getWordsWithPrefix("app");
    for (const std::string& word : words) {
        std::cout << "- " << word << std::endl;
    }

    t.deleteWord("apple");
    std::cout << "Search 'apple' after deletion: " << (t.search("apple") ? "Found" : "Not found") << std::endl;
    return 0;
}
```

#### Iterator Usage

```cpp
#include "trie.h"
#include <iostream>

int main() {
    Trie t;
    t.insert("cat");
    t.insert("catch");
    t.insert("catalog");
    t.insert("car");

    std::cout << "Words in Trie (using iterators): ";
    for (const std::string& word : t) { // Range-based for loop
        std::cout << word << " ";
    }
    std::cout << std::endl;
    return 0;
}
```

#### Advanced Search Examples

```cpp
#include "trie.h"
#include <iostream>
#include <vector>

int main() {
    Trie t;
    t.insert("apple");
    t.insert("apply");
    t.insert("apricot");
    t.insert("banana");
    t.insert("bandana");
    t.insert("boron");

    std::cout << "Wildcard search 'ap?le':" << std::endl;
    std::vector<std::string> wildcard_results = t.wildcardSearch("ap?le");
    for (const std::string& word : wildcard_results) {
        std::cout << "- " << word << std::endl;
    }

    std::cout << "Wildcard search 'ba*a':" << std::endl;
    wildcard_results = t.wildcardSearch("ba*a");
    for (const std::string& word : wildcard_results) {
        std::cout << "- " << word << std::endl;
    }

    std::cout << "Fuzzy search for 'aple' (k=1):" << std::endl;
    std::vector<std::pair<std::string, int>> fuzzy_results = t.fuzzySearch("aple", 1);
    for (const auto& p : fuzzy_results) {
        std::cout << "- " << p.first << " (distance " << p.second << ")" << std::endl;
    }
    return 0;
}
```

#### Suffix and File I/O Examples

```cpp
#include "trie.h"
#include <iostream>
#include <vector>

int main() {
    Trie t;
    t.insert("application");
    t.insert("education");
    t.insert("creation");

    std::cout << "Ends with 'tion': " << (t.endsWith("tion") ? "Yes" : "No") << std::endl;
    std::vector<std::string> suffix_words = t.getWordsEndingWith("tion");
    std::cout << "Words ending with 'tion':" << std::endl;
    for (const std::string& word : suffix_words) {
        std::cout << "- " << word << std::endl;
    }

    // File I/O
    std::string filename = "trie_data.txt";
    std::cout << "Saving trie to " << filename << std::endl;
    t.saveToFile(filename);

    Trie loaded_trie;
    std::cout << "Loading trie from " << filename << std::endl;
    loaded_trie.loadFromFile(filename);

    std::cout << "Words in loaded trie:" << std::endl;
    for (const std::string& word : loaded_trie) {
        std::cout << "- " << word << std::endl;
    }
    return 0;
}
```

### Notes

- The Radix Trie structure optimizes space by compressing chains of single-child nodes into single edges labeled with strings.
- The suffix search capability is facilitated by an internal, automatically managed reversed version of the Trie, providing efficient `endsWith()` and `getWordsEndingWith()` queries.
- While `TrieNode` tracks `frequency`, the direct `getWordFrequency(word)` method is noted in source as incomplete for Radix Trie. However, frequencies are inserted and saved/loaded.

## Contributing

Contributions to this repository are welcome! If you find any issues or have suggestions for improvements, please feel free to open an issue or submit a pull request.

## License

This project is currently unlicensed. Please refer back for updates on licensing information.

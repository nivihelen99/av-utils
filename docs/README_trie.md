# `Trie` (Radix Trie / Patricia Trie)

## Overview

The `trie.h` header provides a C++ implementation of a Trie data structure, specifically a **Radix Trie** (also known as a Patricia Trie or compressed trie). Tries are tree-like data structures primarily used for storing and searching a dynamic set of strings (like words in a dictionary). In a Radix Trie, edges can represent sequences of characters rather than single characters, leading to a more compressed structure, especially when keys share long common prefixes.

This implementation supports:
-   Insertion, deletion, and search of words.
-   Prefix-based searches (e.g., "find all words starting with 'pre'").
-   Suffix-based searches (e.g., "find all words ending with 'ing'") via an internal reversed Trie.
-   Wildcard searches (using `?` for single characters and `*` for zero or more characters).
-   Fuzzy searches (finding words within a certain Levenshtein distance).
-   Case-sensitive and case-insensitive modes.
-   Iteration over all stored words in lexicographical order.
-   Serialization to and from a file, including word frequencies.

## Core Components

-   **`TrieNode`**: Represents a node in the Radix Trie.
    -   `edges`: A `std::map<char, std::pair<std::string, std::unique_ptr<TrieNode>>>`. The `char` key is the first character of an outgoing edge. The `std::string` in the pair is the (potentially multi-character) label of that edge. The `std::unique_ptr<TrieNode>` points to the child node.
    -   `isEndOfWord`: A boolean flag indicating if the path to this node forms a complete word.
    -   `frequency`: An integer to store the frequency of the word ending at this node.
-   **`Trie`**: The main class.
    -   Manages a `root` node (`std::unique_ptr<TrieNode>`).
    -   Handles case sensitivity via a boolean flag.
    -   Maintains an internal `reversed_trie_` (another `Trie` instance storing words in reverse) to efficiently support suffix-based operations.
    -   Provides custom iterators for lexicographical traversal.

## Features

-   **Radix Trie Structure:** Edges can represent sequences of characters, making the trie more compact.
-   **Case Sensitivity:** Operations can be performed in either case-sensitive (default) or case-insensitive mode.
-   **Standard Operations:** `insert()`, `search()`, `deleteWord()`.
-   **Prefix Operations:** `startsWith()`, `getWordsWithPrefix()`.
-   **Suffix Operations:** `endsWith()`, `getWordsEndingWith()` (implemented using an internal reversed trie).
-   **Advanced Searches:**
    -   `wildcardSearch(pattern)`: Supports `?` (any single char) and `*` (any sequence of chars, including empty).
    -   `fuzzySearch(query_word, max_k)`: Finds words within `max_k` Levenshtein distance from `query_word`.
-   **Iteration:** Provides forward iterators (`begin()`, `end()`) to traverse all stored words lexicographically.
-   **Word Frequencies:** Stores and can serialize/deserialize word frequencies (though `getWordFrequency` method is noted as potentially broken for Radix in header comments).
-   **Serialization:** `saveToFile(filename)` and `loadFromFile(filename)` methods.

## Public Interface Highlights

### Constructor
-   **`Trie(bool caseSensitive = true)`**: Creates a Trie.

### Basic Operations
-   **`void insert(const std::string& word)`**: Inserts a word.
-   **`bool search(const std::string& word) const`**: Checks if a word exists.
-   **`bool deleteWord(const std::string& word)`**: Deletes a word. Returns `true` if found and deleted.

### Prefix & Suffix Operations
-   **`bool startsWith(const std::string& prefix) const`**
-   **`std::vector<std::string> getWordsWithPrefix(const std::string& prefix) const`**
-   **`bool endsWith(const std::string& suffix) const`**
-   **`std::vector<std::string> getWordsEndingWith(const std::string& suffix) const`**

### Advanced Searches
-   **`std::vector<std::string> wildcardSearch(const std::string& pattern) const`**
-   **`std::vector<std::pair<std::string, int>> fuzzySearch(const std::string& query_word, int max_k) const`**: Returns pairs of (word, distance).

### Iterators
-   **`Iterator begin() const` / `Iterator end() const`**: For range-based for loops and manual iteration.
    ```cpp
    // Example:
    // for (const std::string& word : my_trie) {
    //     std::cout << word << std::endl;
    // }
    ```

### Serialization & Utility
-   **`bool saveToFile(const std::string& filename) const`**
-   **`bool loadFromFile(const std::string& filename)`**
-   **`void printAllWords() const`**: Debug utility (may use older non-Radix logic).
-   **`int getWordFrequency(const std::string& word) const`**: (Note: Header comments suggest this might be broken for the Radix version).

## Usage Examples

(Based on `tests/trie_test.cpp`)

### Basic Trie Operations

```cpp
#include "trie.h" // Adjust path as needed
#include <iostream>
#include <vector>
#include <string>

int main() {
    Trie dictionary(false); // Case-insensitive Trie

    dictionary.insert("Apple");
    dictionary.insert("apply");
    dictionary.insert("apricot");
    dictionary.insert("Banana");

    std::cout << "Search 'apple': " << std::boolalpha << dictionary.search("apple") << std::endl;     // true
    std::cout << "Search 'Apply': " << dictionary.search("Apply") << std::endl;     // true
    std::cout << "Search 'apricots': " << dictionary.search("apricots") << std::endl; // false

    std::cout << "\nWords starting with 'ap':" << std::endl;
    std::vector<std::string> ap_words = dictionary.getWordsWithPrefix("ap");
    for (const std::string& word : ap_words) {
        std::cout << "  " << word << std::endl; // apple, apply, apricot (order may vary for unordered output)
    }

    dictionary.deleteWord("Apply");
    std::cout << "\nSearch 'apply' after delete: " << dictionary.search("apply") << std::endl; // false
}
```

### Wildcard and Fuzzy Search

```cpp
#include "trie.h"
#include <iostream>
#include <vector>
#include <string>

int main() {
    Trie t;
    t.insert("catalog");
    t.insert("category");
    t.insert("cast");
    t.insert("catch");

    std::cout << "Wildcard 'cat*':" << std::endl;
    for (const auto& word : t.wildcardSearch("cat*")) {
        std::cout << "  " << word << std::endl; // catalog, category, catch
    }

    std::cout << "\nWildcard 'ca?t':" << std::endl;
    for (const auto& word : t.wildcardSearch("ca?t")) {
        std::cout << "  " << word << std::endl; // cast, catch (if '?' matches one char)
    }

    std::cout << "\nFuzzy search for 'caterogy' (max_k=1):" << std::endl;
    // Expected: "category" with distance 1
    for (const auto& pair : t.fuzzySearch("caterogy", 1)) {
        std::cout << "  Found: " << pair.first << " (distance " << pair.second << ")" << std::endl;
    }
}
```

### Iteration and Suffix Search

```cpp
#include "trie.h"
#include <iostream>
#include <string>

int main() {
    Trie words_db;
    words_db.insert("programming");
    words_db.insert("gaming");
    words_db.insert("running");
    words_db.insert("king");

    std::cout << "All words in Trie (lexicographical order):" << std::endl;
    for (const std::string& word : words_db) {
        std::cout << "  " << word << std::endl;
    }

    std::cout << "\nWords ending with 'ing':" << std::endl;
    for (const std::string& word : words_db.getWordsEndingWith("ing")) {
        std::cout << "  " << word << std::endl; // programming, gaming, running, king
    }
}
```

## Dependencies
- Standard C++ libraries: `<iostream>`, `<string>`, `<vector>`, `<memory>`, `<map>`, `<cctype>`, `<algorithm>`, `<fstream>`, `<sstream>`, `<iterator>`, `<stack>`, `<utility>`.

This Radix Trie implementation provides a powerful set of tools for string storage and complex pattern matching.

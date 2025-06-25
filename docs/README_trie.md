# Trie Data Structure

## Overview

A Trie (also known as a prefix tree or digital tree) is a tree-like data structure used to store a dynamic set of strings. Tries are commonly used for operations like string searching, prefix matching, and autocomplete features.

Each node in a Trie represents a single character of a string. A path from the root to a particular node represents a prefix, and if that node is marked as the end of a word, it represents a complete word.

## Features

The implemented Trie supports the following operations:

- **Insert**: Adds a word to the Trie.
- **Search**: Checks if a specific word exists in the Trie.
- **StartsWith**: Checks if there is any word in the Trie that starts with a given prefix.

## Usage Example

```cpp
#include "trie.h"
#include <iostream>

int main() {
    Trie trie;

    // Insert words
    trie.insert("apple");
    trie.insert("app");
    trie.insert("apricot");
    trie.insert("banana");

    // Search for words
    std::cout << "Search 'apple': " << (trie.search("apple") ? "Found" : "Not Found") << std::endl; // Found
    std::cout << "Search 'app': " << (trie.search("app") ? "Found" : "Not Found") << std::endl;     // Found
    std::cout << "Search 'apples': " << (trie.search("apples") ? "Found" : "Not Found") << std::endl; // Not Found
    std::cout << "Search 'ban': " << (trie.search("ban") ? "Found" : "Not Found") << std::endl;     // Not Found

    // Check for prefixes
    std::cout << "StartsWith 'ap': " << (trie.startsWith("ap") ? "Yes" : "No") << std::endl;   // Yes
    std::cout << "StartsWith 'b': " << (trie.startsWith("b") ? "Yes" : "No") << std::endl;    // Yes
    std::cout << "StartsWith 'appo': " << (trie.startsWith("appo") ? "Yes" : "No") << std::endl; // No
    std::cout << "StartsWith 'xyz': " << (trie.startsWith("xyz") ? "Yes" : "No") << std::endl;  // No

    return 0;
}

```

## Complexity

- **Insert**: O(L), where L is the length of the word.
- **Search**: O(L), where L is the length of the word.
- **StartsWith**: O(P), where P is the length of the prefix.

The space complexity of a Trie is O(N*M), where N is the number of words and M is the average length of the words, in the worst case where all words are distinct and share no common prefixes. In practice, if words share common prefixes, the space complexity can be lower.

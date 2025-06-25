#include "trie.h"
#include <iostream>
#include <vector>

int main() {
    Trie trie;

    std::cout << "Inserting words: 'apple', 'app', 'apricot', 'banana', 'bandana', 'band'" << std::endl;
    trie.insert("apple");
    trie.insert("app");
    trie.insert("apricot");
    trie.insert("banana");
    trie.insert("bandana");
    trie.insert("band");

    std::cout << "\n--- Search Operations ---" << std::endl;
    std::vector<std::string> searchWords = {"apple", "app", "apples", "apricot", "banana", "band", "application", ""};
    for (const auto& word : searchWords) {
        std::cout << "Search '" << (word.empty() ? "[empty string]" : word) << "': "
                  << (trie.search(word) ? "Found" : "Not Found") << std::endl;
    }

    trie.insert(""); // Insert empty string
    std::cout << "Search '[empty string]' after insertion: "
              << (trie.search("") ? "Found" : "Not Found") << std::endl;


    std::cout << "\n--- StartsWith Operations ---" << std::endl;
    std::vector<std::string> prefixes = {"a", "ap", "app", "appl", "b", "ban", "banda", "bandanas", "xyz", ""};
    for (const auto& prefix : prefixes) {
        std::cout << "Prefix '" << (prefix.empty() ? "[empty string]" : prefix) << "': "
                  << (trie.startsWith(prefix) ? "Exists" : "Does Not Exist") << std::endl;
    }

    std::cout << "\n--- Demonstrating edge cases ---" << std::endl;
    Trie emptyTrie;
    std::cout << "Search 'hello' in empty trie: " << (emptyTrie.search("hello") ? "Found" : "Not Found") << std::endl;
    std::cout << "StartsWith 'a' in empty trie: " << (emptyTrie.startsWith("a") ? "Exists" : "Does Not Exist") << std::endl;
    std::cout << "StartsWith '' in empty trie: " << (emptyTrie.startsWith("") ? "Exists" : "Does Not Exist") << std::endl;
    emptyTrie.insert("test");
    std::cout << "StartsWith '' in trie with 'test': " << (emptyTrie.startsWith("") ? "Exists" : "Does Not Exist") << std::endl;


    std::cout << "\nExample finished." << std::endl;

    return 0;
}

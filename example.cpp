#include "trie.h"
// Example usage and test cases
int main() {
    Trie trie;
    
    // Insert some words
    trie.insert("cat");
    trie.insert("car");
    trie.insert("card");
    trie.insert("care");
    trie.insert("careful");
    trie.insert("dog");
    trie.insert("dodge");
    
    std::cout << "=== Basic Operations ===" << std::endl;
    
    // Test search
    std::cout << "Search 'car': " << (trie.search("car") ? "Found" : "Not found") << std::endl;
    std::cout << "Search 'care': " << (trie.search("care") ? "Found" : "Not found") << std::endl;
    std::cout << "Search 'caring': " << (trie.search("caring") ? "Found" : "Not found") << std::endl;
    
    // Test prefix search
    std::cout << "\nPrefix 'car' exists: " << (trie.startsWith("car") ? "Yes" : "No") << std::endl;
    std::cout << "Prefix 'caring' exists: " << (trie.startsWith("caring") ? "Yes" : "No") << std::endl;
    
    // Get words with prefix
    std::cout << "\n=== Words with prefix 'car': ===" << std::endl;
    auto carWords = trie.getWordsWithPrefix("car");
    for (const auto& word : carWords) {
        std::cout << word << " ";
    }
    std::cout << std::endl;
    
    std::cout << "\n=== Words with prefix 'do': ===" << std::endl;
    auto doWords = trie.getWordsWithPrefix("do");
    for (const auto& word : doWords) {
        std::cout << word << " ";
    }
    std::cout << std::endl;
    
    // Print all words
    std::cout << "\n=== All words in trie: ===" << std::endl;
    trie.printAllWords();
    
    // Test deletion
    std::cout << "\n=== Testing deletion ===" << std::endl;
    trie.deleteWord("care");
    std::cout << "After deleting 'care':" << std::endl;
    trie.printAllWords();
    
    return 0;
}

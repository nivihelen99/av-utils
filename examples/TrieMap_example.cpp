#include <iostream>
#include "TrieMap.h"

int main() {
    TrieMap<int> trie;

    trie.insert("apple", 1);
    trie.insert("apply", 2);
    trie.insert("apples", 3);
    trie.insert("orange", 4);

    std::cout << "Value for 'apple': " << trie.find("apple").value_or(-1) << std::endl;
    std::cout << "Value for 'apply': " << trie.find("apply").value_or(-1) << std::endl;
    std::cout << "Value for 'apples': " << trie.find("apples").value_or(-1) << std::endl;
    std::cout << "Value for 'orange': " << trie.find("orange").value_or(-1) << std::endl;
    std::cout << "Value for 'app': " << trie.find("app").value_or(-1) << std::endl;

    std::cout << "Contains 'apple': " << trie.contains("apple") << std::endl;
    std::cout << "Contains 'app': " << trie.contains("app") << std::endl;

    std::cout << "Starts with 'app': " << trie.starts_with("app") << std::endl;
    std::cout << "Starts with 'ora': " << trie.starts_with("ora") << std::endl;
    std::cout << "Starts with 'xyz': " << trie.starts_with("xyz") << std::endl;

    std::cout << "Keys with prefix 'app':" << std::endl;
    for (const auto& key : trie.get_keys_with_prefix("app")) {
        std::cout << "  " << key << std::endl;
    }

    return 0;
}

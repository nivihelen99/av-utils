#pragma once

#include <string>
#include <unordered_map>
#include <memory>

// Define the TrieNode structure
struct TrieNode {
    std::unordered_map<char, std::unique_ptr<TrieNode>> children;
    bool isEndOfWord;

    TrieNode() : isEndOfWord(false) {}
};

class Trie {
public:
    Trie() : root(std::make_unique<TrieNode>()) {}

    // Inserts a word into the trie
    void insert(const std::string& word) {
        TrieNode* current = root.get();
        for (char ch : word) {
            if (current->children.find(ch) == current->children.end()) {
                current->children[ch] = std::make_unique<TrieNode>();
            }
            current = current->children[ch].get();
        }
        current->isEndOfWord = true;
    }

    // Searches for a word in the trie
    bool search(const std::string& word) const {
        const TrieNode* current = root.get();
        for (char ch : word) {
            if (current->children.find(ch) == current->children.end()) {
                return false;
            }
            current = current->children.at(ch).get();
        }
        return current != nullptr && current->isEndOfWord;
    }

    // Checks if there is any word in the trie that starts with the given prefix
    bool startsWith(const std::string& prefix) const {
        const TrieNode* current = root.get();
        for (char ch : prefix) {
            if (current->children.find(ch) == current->children.end()) {
                return false;
            }
            current = current->children.at(ch).get();
        }
        return current != nullptr;
    }

private:
    std::unique_ptr<TrieNode> root;
};

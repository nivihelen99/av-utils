#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

class TrieNode {
public:
    std::unordered_map<char, std::unique_ptr<TrieNode>> children;
    bool isEndOfWord;
    
    TrieNode() : isEndOfWord(false) {}
};

class Trie {
private:
    std::unique_ptr<TrieNode> root;
    
public:
    Trie() : root(std::make_unique<TrieNode>()) {}
    
    // Insert a word into the trie
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
    
    // Search for a word in the trie
    bool search(const std::string& word) const {
        TrieNode* current = root.get();
        
        for (char ch : word) {
            if (current->children.find(ch) == current->children.end()) {
                return false;
            }
            current = current->children[ch].get();
        }
        return current->isEndOfWord;
    }
    
    // Check if any word starts with the given prefix
    bool startsWith(const std::string& prefix) const {
        TrieNode* current = root.get();
        
        for (char ch : prefix) {
            if (current->children.find(ch) == current->children.end()) {
                return false;
            }
            current = current->children[ch].get();
        }
        return true;
    }
    
    // Get all words with a given prefix
    std::vector<std::string> getWordsWithPrefix(const std::string& prefix) const {
        std::vector<std::string> result;
        TrieNode* current = root.get();
        
        // Navigate to the prefix node
        for (char ch : prefix) {
            if (current->children.find(ch) == current->children.end()) {
                return result; // Empty vector if prefix doesn't exist
            }
            current = current->children[ch].get();
        }
        
        // DFS to collect all words from this node
        collectWords(current, prefix, result);
        return result;
    }
    
    // Delete a word from the trie
    bool deleteWord(const std::string& word) {
        return deleteHelper(root.get(), word, 0);
    }
    
    // Print all words in the trie (for debugging)
    void printAllWords() const {
        std::vector<std::string> allWords;
        collectWords(root.get(), "", allWords);
        
        std::cout << "Words in trie: ";
        for (const auto& word : allWords) {
            std::cout << word << " ";
        }
        std::cout << std::endl;
    }

private:
    // Helper function to collect all words from a given node
    void collectWords(TrieNode* node, const std::string& currentWord, 
                     std::vector<std::string>& result) const {
        if (node->isEndOfWord) {
            result.push_back(currentWord);
        }
        
        for (const auto& pair : node->children) {
            collectWords(pair.second.get(), currentWord + pair.first, result);
        }
    }
    
    // Helper function for deletion
    bool deleteHelper(TrieNode* node, const std::string& word, int index) {
        if (index == word.length()) {
            if (!node->isEndOfWord) {
                return false; // Word doesn't exist
            }
            node->isEndOfWord = false;
            // Return true if node has no children (can be deleted)
            return node->children.empty();
        }
        
        char ch = word[index];
        auto it = node->children.find(ch);
        if (it == node->children.end()) {
            return false; // Word doesn't exist
        }
        
        bool shouldDeleteChild = deleteHelper(it->second.get(), word, index + 1);
        
        if (shouldDeleteChild) {
            node->children.erase(it);
            // Return true if node can be deleted (no children and not end of word)
            return node->children.empty() && !node->isEndOfWord;
        }
        
        return false;
    }
};


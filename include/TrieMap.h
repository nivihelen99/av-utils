#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <optional>
#include <vector>

template <typename Value>
class TrieMap {
private:
    struct TrieNode {
        std::unordered_map<char, std::unique_ptr<TrieNode>> children;
        std::optional<Value> value;

        TrieNode() : value(std::nullopt) {}
    };

    std::unique_ptr<TrieNode> root;

public:
    TrieMap() : root(std::make_unique<TrieNode>()) {}

    void insert(const std::string& key, const Value& value) {
        TrieNode* current = root.get();
        for (char ch : key) {
            if (current->children.find(ch) == current->children.end()) {
                current->children[ch] = std::make_unique<TrieNode>();
            }
            current = current->children[ch].get();
        }
        current->value = value;
    }

    std::optional<Value> find(const std::string& key) const {
        const TrieNode* current = root.get();
        for (char ch : key) {
            if (current->children.find(ch) == current->children.end()) {
                return std::nullopt;
            }
            current = current->children.at(ch).get();
        }
        return current->value;
    }

    bool contains(const std::string& key) const {
        const TrieNode* current = root.get();
        for (char ch : key) {
            if (current->children.find(ch) == current->children.end()) {
                return false;
            }
            current = current->children.at(ch).get();
        }
        return current->value.has_value();
    }

    bool starts_with(const std::string& prefix) const {
        const TrieNode* current = root.get();
        for (char ch : prefix) {
            if (current->children.find(ch) == current->children.end()) {
                return false;
            }
            current = current->children.at(ch).get();
        }
        return true;
    }

    void get_keys_with_prefix_recursive(const TrieNode* node, const std::string& current_prefix, std::vector<std::string>& keys) const {
        if (node->value.has_value()) {
            keys.push_back(current_prefix);
        }

        for (const auto& pair : node->children) {
            get_keys_with_prefix_recursive(pair.second.get(), current_prefix + pair.first, keys);
        }
    }

    std::vector<std::string> get_keys_with_prefix(const std::string& prefix) const {
        std::vector<std::string> keys;
        const TrieNode* current = root.get();
        for (char ch : prefix) {
            if (current->children.find(ch) == current->children.end()) {
                return keys;
            }
            current = current->children.at(ch).get();
        }

        get_keys_with_prefix_recursive(current, prefix, keys);
        return keys;
    }
};

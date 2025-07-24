#pragma once

#include <string>
#include <memory>
#include <vector>
#include <stdexcept>

class Rope {
private:
    struct Node {
        std::unique_ptr<Node> left = nullptr;
        std::unique_ptr<Node> right = nullptr;
        std::string data;
        size_t length;

        Node(std::string str) : data(std::move(str)), length(data.length()) {}
        Node(std::unique_ptr<Node> l, std::unique_ptr<Node> r) : left(std::move(l)), right(std::move(r)), length(0) {
            if (left) length += left->length;
            if (right) length += right->length;
        }
    };

    std::unique_ptr<Node> root = nullptr;

public:
    Rope() = default;
    Rope(const std::string& str);
    void append(const std::string& str);
    char at(size_t index) const;
    std::string to_string() const;
    size_t size() const;

private:
    void to_string_helper(const Node* node, std::string& result) const;
};

Rope::Rope(const std::string& str) {
    if (!str.empty()) {
        root = std::make_unique<Node>(str);
    }
}

void Rope::append(const std::string& str) {
    if (str.empty()) return;
    auto new_node = std::make_unique<Node>(str);
    if (!root) {
        root = std::move(new_node);
    } else {
        auto old_root = std::move(root);
        root = std::make_unique<Node>(std::move(old_root), std::move(new_node));
    }
}

char Rope::at(size_t index) const {
    if (!root || index >= root->length) {
        throw std::out_of_range("Index out of range");
    }
    const Node* current = root.get();
    while (current->left) {
        if (index < current->left->length) {
            current = current->left.get();
        } else {
            index -= current->left->length;
            current = current->right.get();
        }
    }
    return current->data[index];
}

std::string Rope::to_string() const {
    std::string result;
    if (root) {
        to_string_helper(root.get(), result);
    }
    return result;
}

size_t Rope::size() const {
    return root ? root->length : 0;
}

void Rope::to_string_helper(const Node* node, std::string& result) const {
    if (!node) return;
    if (node->left) {
        to_string_helper(node->left.get(), result);
        to_string_helper(node->right.get(), result);
    } else {
        result += node->data;
    }
}

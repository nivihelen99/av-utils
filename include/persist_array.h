#pragma once

#include <memory>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <cassert>

template<typename T>
class PersistentArray {
private:
    struct Node {
        std::vector<T> data;
        mutable int ref_count;
        
        Node(size_t size) : data(size), ref_count(1) {}
        Node(const std::vector<T>& vec) : data(vec), ref_count(1) {}
        Node(std::vector<T>&& vec) : data(std::move(vec)), ref_count(1) {}
        
        // Copy constructor for copy-on-write
        Node(const Node& other) : data(other.data), ref_count(1) {}
    };
    
    mutable std::shared_ptr<Node> root;
    
    // Ensure we have a unique copy for modification
    void ensure_unique() const {
        if (!root || root.use_count() > 1) {
            root = std::make_shared<Node>(*root);
        }
    }
    
public:
    // Constructors
    PersistentArray() : root(std::make_shared<Node>(0)) {}
    
    explicit PersistentArray(size_t size) 
        : root(std::make_shared<Node>(size)) {}
    
    PersistentArray(size_t size, const T& value) 
        : root(std::make_shared<Node>(std::vector<T>(size, value))) {}
    
    PersistentArray(std::initializer_list<T> init) 
        : root(std::make_shared<Node>(std::vector<T>(init))) {}
    
    // Copy constructor (shallow copy - shares data)
    PersistentArray(const PersistentArray& other) : root(other.root) {}
    
    // Assignment operator (shallow copy - shares data)
    PersistentArray& operator=(const PersistentArray& other) {
        if (this != &other) {
            root = other.root;
        }
        return *this;
    }
    
    // Move constructor
    PersistentArray(PersistentArray&& other) noexcept : root(std::move(other.root)) {}
    
    // Move assignment
    PersistentArray& operator=(PersistentArray&& other) noexcept {
        if (this != &other) {
            root = std::move(other.root);
        }
        return *this;
    }
    
    // Access operations (const)
    const T& operator[](size_t index) const {
        if (index >= size()) {
            throw std::out_of_range("Index out of range");
        }
        return root->data[index];
    }
    
    const T& at(size_t index) const {
        if (index >= size()) {
            throw std::out_of_range("Index out of range");
        }
        return root->data[index];
    }
    
    // Modification operations (return new version)
    PersistentArray set(size_t index, const T& value) const {
        if (index >= size()) {
            throw std::out_of_range("Index out of range");
        }
        
        PersistentArray new_version(*this);
        new_version.ensure_unique();
        new_version.root->data[index] = value;
        return new_version;
    }
    
    PersistentArray push_back(const T& value) const {
        PersistentArray new_version(*this);
        new_version.ensure_unique();
        new_version.root->data.push_back(value);
        return new_version;
    }
    
    PersistentArray pop_back() const {
        if (empty()) {
            throw std::runtime_error("Cannot pop from empty array");
        }
        
        PersistentArray new_version(*this);
        new_version.ensure_unique();
        new_version.root->data.pop_back();
        return new_version;
    }
    
    PersistentArray insert(size_t index, const T& value) const {
        if (index > size()) {
            throw std::out_of_range("Index out of range");
        }
        
        PersistentArray new_version(*this);
        new_version.ensure_unique();
        new_version.root->data.insert(new_version.root->data.begin() + index, value);
        return new_version;
    }
    
    PersistentArray erase(size_t index) const {
        if (index >= size()) {
            throw std::out_of_range("Index out of range");
        }
        
        PersistentArray new_version(*this);
        new_version.ensure_unique();
        new_version.root->data.erase(new_version.root->data.begin() + index);
        return new_version;
    }
    
    // In-place modification operations (modifies current version)
    void set_inplace(size_t index, const T& value) {
        if (index >= size()) {
            throw std::out_of_range("Index out of range");
        }
        ensure_unique();
        root->data[index] = value;
    }
    
    void push_back_inplace(const T& value) {
        ensure_unique();
        root->data.push_back(value);
    }
    
    void pop_back_inplace() {
        if (empty()) {
            throw std::runtime_error("Cannot pop from empty array");
        }
        ensure_unique();
        root->data.pop_back();
    }
    
    // Utility methods
    size_t size() const {
        return root ? root->data.size() : 0;
    }
    
    bool empty() const {
        return size() == 0;
    }
    
    void clear() {
        root = std::make_shared<Node>(0);
    }
    
    // Iterator support
    typename std::vector<T>::const_iterator begin() const {
        return root->data.begin();
    }
    
    typename std::vector<T>::const_iterator end() const {
        return root->data.end();
    }
    
    // Debug information
    long use_count() const {
        return root.use_count();
    }
    
    void print_debug_info() const {
        std::cout << "Array size: " << size() 
                  << ", Reference count: " << use_count() << std::endl;
    }
    
    // Comparison operators
    bool operator==(const PersistentArray& other) const {
        return root->data == other.root->data;
    }
    
    bool operator!=(const PersistentArray& other) const {
        return !(*this == other);
    }
};

// Demonstration and test code
void demonstrate_persistent_array() {
    std::cout << "=== Persistent Array Demonstration ===" << std::endl;
    
    // Create initial array
    PersistentArray<int> v1{1, 2, 3, 4, 5};
    std::cout << "v1 created with values: ";
    for (const auto& val : v1) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
    v1.print_debug_info();
    
    // Create version 2 by modifying v1
    auto v2 = v1.set(2, 100);
    std::cout << "\nv2 = v1.set(2, 100):" << std::endl;
    std::cout << "v1: ";
    for (const auto& val : v1) {
        std::cout << val << " ";
    }
    std::cout << " (unchanged)" << std::endl;
    std::cout << "v2: ";
    for (const auto& val : v2) {
        std::cout << val << " ";
    }
    std::cout << " (modified)" << std::endl;
    
    v1.print_debug_info();
    v2.print_debug_info();
    
    // Create version 3 by adding to v2
    auto v3 = v2.push_back(200);
    std::cout << "\nv3 = v2.push_back(200):" << std::endl;
    std::cout << "v2: ";
    for (const auto& val : v2) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
    std::cout << "v3: ";
    for (const auto& val : v3) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
    
    // Show that copying shares data initially
    auto v4 = v3;
    std::cout << "\nv4 = v3 (copy):" << std::endl;
    v3.print_debug_info();
    v4.print_debug_info();
    
    // Modify v4 to trigger copy-on-write
    v4.set_inplace(0, 999);
    std::cout << "\nAfter v4.set_inplace(0, 999):" << std::endl;
    std::cout << "v3: ";
    for (const auto& val : v3) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
    std::cout << "v4: ";
    for (const auto& val : v4) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
    v3.print_debug_info();
    v4.print_debug_info();
    
    // Demonstrate undo functionality
    std::cout << "\n=== Undo Functionality Demo ===" << std::endl;
    std::vector<PersistentArray<int>> history;
    auto current = PersistentArray<int>{10, 20};
    history.push_back(current);
    
    std::cout << "Initial: ";
    for (const auto& val : current) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
    
    // Perform operations and save history
    current = current.push_back(30);
    history.push_back(current);
    std::cout << "After push_back(30): ";
    for (const auto& val : current) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
    
    current = current.set(1, 200);
    history.push_back(current);
    std::cout << "After set(1, 200): ";
    for (const auto& val : current) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
    
    // Undo operations
    std::cout << "\nUndo operations:" << std::endl;
    for (int i = history.size() - 2; i >= 0; --i) {
        std::cout << "Undo to state " << i << ": ";
        for (const auto& val : history[i]) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }
}


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
        
        Node(size_t size) : data(size) {}
        Node(const std::vector<T>& vec) : data(vec) {}
        Node(std::vector<T>&& vec) : data(std::move(vec)) {}
        
        // Copy constructor for copy-on-write
        Node(const Node& other) : data(other.data) {}
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
       if (!root) { // Should not happen with current constructors but good for safety
           throw std::out_of_range("Array is uninitialized");
       }
       if (index >= root->data.size()) {
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
    
    // Iterator invalidation:
    // - Iterators are invalidated by any operation that modifies the array *in-place*
    //   and causes a reallocation or element shift (e.g., push_back_inplace, erase_inplace if they existed and behaved like std::vector).
    // - For copy-on-write operations (set, push_back, etc.), iterators to the *original* array remain valid
    //   as the original data is unchanged. Iterators to the *new* version are valid for that new version.
    // - Iterators are essentially std::vector<T>::const_iterator for a specific version's data.
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


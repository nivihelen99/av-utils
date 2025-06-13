#pragma once

#include <unordered_map>
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <vector>

template<typename Left, typename Right>
class BiMap {
private:
    std::unordered_map<Left, Right> left_to_right_;
    std::unordered_map<Right, Left> right_to_left_;

public:
    // Type aliases for STL compatibility
    using left_value_type = std::pair<const Left, Right>;
    using right_value_type = std::pair<const Right, Left>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    
    // Iterator types
    using left_iterator = typename std::unordered_map<Left, Right>::iterator;
    using left_const_iterator = typename std::unordered_map<Left, Right>::const_iterator;
    using right_iterator = typename std::unordered_map<Right, Left>::iterator;
    using right_const_iterator = typename std::unordered_map<Right, Left>::const_iterator;
    
    // Combined iterator class for both views
    template<bool IsLeft>
    class bimap_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = typename std::conditional<IsLeft, left_value_type, right_value_type>::type;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;
        
    private:
        using internal_iterator = typename std::conditional<
            IsLeft, 
            typename std::unordered_map<Left, Right>::const_iterator,
            typename std::unordered_map<Right, Left>::const_iterator
        >::type;
        
        internal_iterator it_;
        
    public:
        bimap_iterator() = default;
        explicit bimap_iterator(internal_iterator it) : it_(it) {}
        
        reference operator*() const { return *it_; }
        pointer operator->() const { return &(*it_); }
        
        bimap_iterator& operator++() { ++it_; return *this; }
        bimap_iterator operator++(int) { bimap_iterator tmp = *this; ++it_; return tmp; }
        
        bool operator==(const bimap_iterator& other) const { return it_ == other.it_; }
        bool operator!=(const bimap_iterator& other) const { return it_ != other.it_; }
        
        // Allow conversion from non-const to const
        operator bimap_iterator<IsLeft>() const { return bimap_iterator<IsLeft>(it_); }
    };
    
    using left_view_iterator = bimap_iterator<true>;
    using right_view_iterator = bimap_iterator<false>;
    
    // Nested view classes for clean syntax
    class left_view {
    private:
        const BiMap* parent_;
    public:
        explicit left_view(const BiMap* parent) : parent_(parent) {}
        
        left_view_iterator begin() const { 
            return left_view_iterator(parent_->left_to_right_.begin()); 
        }
        left_view_iterator end() const { 
            return left_view_iterator(parent_->left_to_right_.end()); 
        }
        left_view_iterator cbegin() const { return begin(); }
        left_view_iterator cend() const { return end(); }
        
        size_type size() const { return parent_->size(); }
        bool empty() const { return parent_->empty(); }
        
        const Right& at(const Left& key) const { return parent_->at_left(key); }
        const Right* find(const Left& key) const { return parent_->find_left(key); }
        bool contains(const Left& key) const { return parent_->contains_left(key); }
    };
    
    class right_view {
    private:
        const BiMap* parent_;
    public:
        explicit right_view(const BiMap* parent) : parent_(parent) {}
        
        right_view_iterator begin() const { 
            return right_view_iterator(parent_->right_to_left_.begin()); 
        }
        right_view_iterator end() const { 
            return right_view_iterator(parent_->right_to_left_.end()); 
        }
        right_view_iterator cbegin() const { return begin(); }
        right_view_iterator cend() const { return end(); }
        
        size_type size() const { return parent_->size(); }
        bool empty() const { return parent_->empty(); }
        
        const Left& at(const Right& key) const { return parent_->at_right(key); }
        const Left* find(const Right& key) const { return parent_->find_right(key); }
        bool contains(const Right& key) const { return parent_->contains_right(key); }
    };

    // Insert a pair - returns true if successful, false if either key already exists
    bool insert(const Left& left, const Right& right) {
        if (left_to_right_.find(left) != left_to_right_.end() ||
            right_to_left_.find(right) != right_to_left_.end()) {
            return false;
        }
        
        left_to_right_[left] = right;
        right_to_left_[right] = left;
        return true;
    }
    
    // Insert with pair
    bool insert(const std::pair<Left, Right>& pair) {
        return insert(pair.first, pair.second);
    }
    
    // Insert range
    template<typename InputIt>
    void insert(InputIt first, InputIt last) {
        for (auto it = first; it != last; ++it) {
            insert(*it);
        }
    }
    
    // Insert initializer list
    void insert(std::initializer_list<std::pair<Left, Right>> ilist) {
        insert(ilist.begin(), ilist.end());
    }
    
    // Insert or update a pair
    void insert_or_assign(const Left& left, const Right& right) {
        auto left_it = left_to_right_.find(left);
        if (left_it != left_to_right_.end()) {
            right_to_left_.erase(left_it->second);
        }
        
        auto right_it = right_to_left_.find(right);
        if (right_it != right_to_left_.end()) {
            left_to_right_.erase(right_it->second);
        }
        
        left_to_right_[left] = right;
        right_to_left_[right] = left;
    }
    
    // Access methods
    const Right& at_left(const Left& left) const {
        auto it = left_to_right_.find(left);
        if (it == left_to_right_.end()) {
            throw std::out_of_range("Left key not found");
        }
        return it->second;
    }
    
    const Left& at_right(const Right& right) const {
        auto it = right_to_left_.find(right);
        if (it == right_to_left_.end()) {
            throw std::out_of_range("Right key not found");
        }
        return it->second;
    }
    
    const Right* find_left(const Left& left) const {
        auto it = left_to_right_.find(left);
        return (it != left_to_right_.end()) ? &(it->second) : nullptr;
    }
    
    const Left* find_right(const Right& right) const {
        auto it = right_to_left_.find(right);
        return (it != right_to_left_.end()) ? &(it->second) : nullptr;
    }
    
    bool contains_left(const Left& left) const {
        return left_to_right_.find(left) != left_to_right_.end();
    }
    
    bool contains_right(const Right& right) const {
        return right_to_left_.find(right) != right_to_left_.end();
    }
    
    // Erase methods
    bool erase_left(const Left& left) {
        auto it = left_to_right_.find(left);
        if (it == left_to_right_.end()) {
            return false;
        }
        
        right_to_left_.erase(it->second);
        left_to_right_.erase(it);
        return true;
    }
    
    bool erase_right(const Right& right) {
        auto it = right_to_left_.find(right);
        if (it == right_to_left_.end()) {
            return false;
        }
        
        left_to_right_.erase(it->second);
        right_to_left_.erase(it);
        return true;
    }
    
    // Erase by iterator
    left_iterator erase_left(left_const_iterator pos) {
        if (pos != left_to_right_.end()) {
            right_to_left_.erase(pos->second);
            return left_to_right_.erase(pos);
        }
        return left_to_right_.end();
    }
    
    right_iterator erase_right(right_const_iterator pos) {
        if (pos != right_to_left_.end()) {
            left_to_right_.erase(pos->second);
            return right_to_left_.erase(pos);
        }
        return right_to_left_.end();
    }
    
    void clear() {
        left_to_right_.clear();
        right_to_left_.clear();
    }
    
    size_type size() const {
        return left_to_right_.size();
    }
    
    bool empty() const {
        return left_to_right_.empty();
    }
    
    // View accessors
    left_view left() const { return left_view(this); }
    right_view right() const { return right_view(this); }
    
    // STL-compatible iterators for left view (default)
    left_view_iterator begin() const { return left().begin(); }
    left_view_iterator end() const { return left().end(); }
    left_view_iterator cbegin() const { return left().cbegin(); }
    left_view_iterator cend() const { return left().cend(); }
    
    // Direct access to underlying iterators (for advanced use)
    left_iterator left_begin() { return left_to_right_.begin(); }
    left_iterator left_end() { return left_to_right_.end(); }
    left_const_iterator left_begin() const { return left_to_right_.begin(); }
    left_const_iterator left_end() const { return left_to_right_.end(); }
    left_const_iterator left_cbegin() const { return left_to_right_.cbegin(); }
    left_const_iterator left_cend() const { return left_to_right_.cend(); }
    
    right_iterator right_begin() { return right_to_left_.begin(); }
    right_iterator right_end() { return right_to_left_.end(); }
    right_const_iterator right_begin() const { return right_to_left_.begin(); }
    right_const_iterator right_end() const { return right_to_left_.end(); }
    right_const_iterator right_cbegin() const { return right_to_left_.cbegin(); }
    right_const_iterator right_cend() const { return right_to_left_.cend(); }
};


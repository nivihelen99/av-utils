#pragma once

#include <unordered_map>
#include <stdexcept>
#include <utility> // For std::move, std::swap
#include <iostream>
#include <algorithm>
#include <vector>

template<typename Left, typename Right>
class BiMap {
private:
    std::unordered_map<Left, Right> left_to_right_;
    std::unordered_map<Right, Left> right_to_left_;

public:
    // Rule of 5/3
    BiMap() noexcept = default; // Default constructor
    ~BiMap() = default; // Destructor

    // Copy constructor
    BiMap(const BiMap& other)
        : left_to_right_(other.left_to_right_), right_to_left_(other.right_to_left_) {}

    // Copy assignment operator
    BiMap& operator=(const BiMap& other) {
        if (this == &other) {
            return *this;
        }
        left_to_right_ = other.left_to_right_;
        right_to_left_ = other.right_to_left_;
        return *this;
    }

    // Move constructor
    BiMap(BiMap&& other) noexcept
        : left_to_right_(std::move(other.left_to_right_)), right_to_left_(std::move(other.right_to_left_)) {}

    // Move assignment operator
    BiMap& operator=(BiMap&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        left_to_right_ = std::move(other.left_to_right_);
        right_to_left_ = std::move(other.right_to_left_);
        // It's important to leave 'other' in a valid but unspecified state.
        // Clearing is a simple way to achieve this for the BiMap's invariants,
        // though the underlying map moves should already handle this for their own state.
        // other.left_to_right_.clear(); // Not strictly necessary as map move assignment should handle it
        // other.right_to_left_.clear(); // Not strictly necessary
        return *this;
    }

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
        // Since internal_iterator is always a const_iterator, these must be const too.
        using pointer = const value_type*;
        using reference = const value_type&;
        
    private:
        using internal_iterator = typename std::conditional<
            IsLeft, 
            typename std::unordered_map<Left, Right>::const_iterator,
            typename std::unordered_map<Right, Left>::const_iterator
        >::type;
        
        internal_iterator it_;
        
    public:
        bimap_iterator() noexcept = default;
        explicit bimap_iterator(internal_iterator it) noexcept : it_(it) {}

        bimap_iterator(const bimap_iterator& other) noexcept = default;
        bimap_iterator& operator=(const bimap_iterator& other) noexcept = default;
        bimap_iterator(bimap_iterator&& other) noexcept = default;
        bimap_iterator& operator=(bimap_iterator&& other) noexcept = default;
        
        reference operator*() const noexcept { return *it_; }
        pointer operator->() const noexcept { return &(*it_); }
        
        bimap_iterator& operator++() noexcept { ++it_; return *this; }
        bimap_iterator operator++(int) noexcept { bimap_iterator tmp = *this; ++it_; return tmp; }
        
        bool operator==(const bimap_iterator& other) const noexcept { return it_ == other.it_; }
        bool operator!=(const bimap_iterator& other) const noexcept { return it_ != other.it_; }
        // Redundant self-conversion operator removed.
        // operator bimap_iterator<IsLeft>() const noexcept { return bimap_iterator<IsLeft>(it_); }
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
        
        size_type size() const noexcept { return parent_->size(); }
        bool empty() const noexcept { return parent_->empty(); }
        
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
        
        size_type size() const noexcept { return parent_->size(); }
        bool empty() const noexcept { return parent_->empty(); }
        
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

    // Rvalue overloads for insert
    bool insert(Left&& left, Right&& right) {
        if (contains_left(left) || contains_right(right)) {
            return false;
        }
        // Capture values for the second map before they are moved.
        Left l_key_copy = left;
        Right r_val_copy = right;

        left_to_right_.emplace(std::forward<Left>(left), std::forward<Right>(right));
        right_to_left_.emplace(std::move(r_val_copy), std::move(l_key_copy));
        return true;
    }

    bool insert(const Left& left, Right&& right) {
        if (contains_left(left) || contains_right(right)) {
            return false;
        }
        Right r_val_copy = right; // Capture rvalue before move

        left_to_right_.emplace(left, std::forward<Right>(right));
        right_to_left_.emplace(std::move(r_val_copy), left); // Use copy for key, const Left& for value
        return true;
    }

    bool insert(Left&& left, const Right& right) {
        if (contains_left(left) || contains_right(right)) {
            return false;
        }
        Left l_key_copy = left; // Capture rvalue before move

        left_to_right_.emplace(std::forward<Left>(left), right);
        right_to_left_.emplace(right, std::move(l_key_copy)); // Use const Right& for key, copy for value
        return true;
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

    // Rvalue overloads for insert_or_assign
    void insert_or_assign(Left&& left, Right&& right) {
        // Capture original values of rvalue arguments for lookups and for the reverse map.
        Left l_key_original = left;
        Right r_key_original = right;

        // Check if the original 'left' value is already a key in left_to_right_
        auto left_it_old = left_to_right_.find(l_key_original);
        if (left_it_old != left_to_right_.end()) {
            // If so, 'l_key_original' was mapped to 'old_R_val'.
            // Remove 'old_R_val' from right_to_left_ map.
            right_to_left_.erase(left_it_old->second);
        }

        // Check if the original 'right' value is already a key in right_to_left_
        auto right_it_old = right_to_left_.find(r_key_original);
        if (right_it_old != right_to_left_.end()) {
            // If so, 'r_key_original' was mapped to 'old_L_val'.
            // Remove 'old_L_val' from left_to_right_ map,
            // unless 'old_L_val' is the same as the 'l_key_original' we are about to insert/update.
            if (right_it_old->second != l_key_original) {
                 left_to_right_.erase(right_it_old->second);
            }
        }

        // Perform the assignment, moving from input arguments.
        left_to_right_[std::forward<Left>(left)] = std::forward<Right>(right);
        // Update the reverse map using the (now moved-from, if applicable) original values.
        right_to_left_[std::move(r_key_original)] = std::move(l_key_original);
    }

    void insert_or_assign(const Left& left, Right&& right) {
        // `left` is const lvalue, `right` is rvalue.
        Right r_key_original = right; // Capture original value of rvalue 'right'.

        auto left_it_old = left_to_right_.find(left);
        if (left_it_old != left_to_right_.end()) {
            right_to_left_.erase(left_it_old->second);
        }

        auto right_it_old = right_to_left_.find(r_key_original);
        if (right_it_old != right_to_left_.end()) {
            if (right_it_old->second != left) {
                 left_to_right_.erase(right_it_old->second);
            }
        }

        left_to_right_[left] = std::forward<Right>(right);
        right_to_left_[std::move(r_key_original)] = left;
    }

    void insert_or_assign(Left&& left, const Right& right) {
        // `left` is rvalue, `right` is const lvalue.
        Left l_key_original = left; // Capture original value of rvalue 'left'.

        auto left_it_old = left_to_right_.find(l_key_original);
        if (left_it_old != left_to_right_.end()) {
            right_to_left_.erase(left_it_old->second);
        }

        auto right_it_old = right_to_left_.find(right);
        if (right_it_old != right_to_left_.end()) {
             if (right_it_old->second != l_key_original) {
                 left_to_right_.erase(right_it_old->second);
             }
        }

        left_to_right_[std::forward<Left>(left)] = right;
        right_to_left_[right] = std::move(l_key_original);
    }

    // Emplace method - attempts to construct a std::pair<const Left, Right> in place
    // Returns std::pair<left_iterator, bool>
    // bool is true if insertion took place, false otherwise.
    // If bool is true, iterator points to the newly inserted element in the left_to_right_ map.
    // If bool is false, iterator points to the element that prevented insertion in left_to_right_,
    // or to left_to_right_.end() if the failure was due to the right_to_left_ map.
    template <class... Args>
    std::pair<left_iterator, bool> emplace(Args&&... args) {
        // Try to emplace into the left_to_right_ map
        // The arguments args... are forwarded to construct a std::pair<const Left, Right>
        auto result_ltr = left_to_right_.emplace(std::forward<Args>(args)...);

        if (!result_ltr.second) {
            // Emplacement into left_to_right_ failed (e.g., key already exists)
            return result_ltr; // Iterator points to existing element, bool is false
        }

        // Emplacement into left_to_right_ succeeded. Now try to emplace into right_to_left_.
        // The new element is pointed to by result_ltr.first.
        // Key for right_to_left_ is result_ltr.first->second (the Right value)
        // Value for right_to_left_ is result_ltr.first->first (the Left value)
        auto result_rtl = right_to_left_.emplace(result_ltr.first->second, result_ltr.first->first);

        if (!result_rtl.second) {
            // Emplacement into right_to_left_ failed (e.g., the Right value already mapped)
            // This means we need to roll back the insertion into left_to_right_.
            left_to_right_.erase(result_ltr.first);
            // Return an iterator to end() and false, indicating failure.
            // The original result_ltr.first is now dangling or invalid.
            return { left_to_right_.end(), false };
        }

        // Both emplacements succeeded
        return result_ltr; // Iterator points to new element in left_to_right_, bool is true
    }

    // try_emplace_left: If key k is not found in left_to_right_, constructs a Right value from args.
    // If the constructed Right value is not found in right_to_left_, inserts (k, Right(args...)).
    // Otherwise, does nothing. Returns {iterator_to_element_with_key_k, bool_success}.
    template <class... Args>
    std::pair<left_iterator, bool> try_emplace_left(const Left& k, Args&&... args) {
        auto it_ltr = left_to_right_.find(k);
        if (it_ltr != left_to_right_.end()) {
            // Key k already exists in left_to_right_
            return {it_ltr, false};
        }

        // Key k does not exist in left_to_right_. Construct Right value.
        Right r_val(std::forward<Args>(args)...);

        // Check if constructed r_val exists in right_to_left_
        if (right_to_left_.count(r_val)) {
            // Constructed Right value already exists in right_to_left_, cannot insert.
            return {left_to_right_.end(), false};
        }

        // Proceed with emplacing into both maps
        // k is const Left&, r_val is local and can be moved.
        auto emplace_res_ltr = left_to_right_.emplace(k, std::move(r_val));
        // emplace_res_ltr.first->first is k, emplace_res_ltr.first->second is the moved r_val
        right_to_left_.emplace(emplace_res_ltr.first->second, emplace_res_ltr.first->first);

        return {emplace_res_ltr.first, true};
    }

    template <class... Args>
    std::pair<left_iterator, bool> try_emplace_left(Left&& k, Args&&... args) {
        auto it_ltr = left_to_right_.find(k);
        if (it_ltr != left_to_right_.end()) {
            // Key k already exists in left_to_right_
            // Note: k (rvalue) is not moved if it already exists.
            return {it_ltr, false};
        }

        // Key k does not exist in left_to_right_. Construct Right value.
        Right r_val(std::forward<Args>(args)...);

        // Check if constructed r_val exists in right_to_left_
        if (right_to_left_.count(r_val)) {
            // Constructed Right value already exists in right_to_left_, cannot insert.
            // k (rvalue) is not moved.
            return {left_to_right_.end(), false};
        }

        // Proceed with emplacing into both maps
        // k is Left&& and can be moved. r_val is local and can be moved.
        // Need to capture k's value for right_to_left before it's moved if Left and Right are same type.
        // However, emplace takes Key&& for the key.
        // The value of k for right_to_left will be taken from the emplaced element in left_to_right.
        auto emplace_res_ltr = left_to_right_.emplace(std::forward<Left>(k), std::move(r_val));
        right_to_left_.emplace(emplace_res_ltr.first->second, emplace_res_ltr.first->first);

        return {emplace_res_ltr.first, true};
    }

    // try_emplace_right: If key k is not found in right_to_left_, constructs a Left value from args.
    // If the constructed Left value is not found in left_to_right_, inserts (Left(args...), k).
    // Otherwise, does nothing. Returns {iterator_to_element_with_key_k, bool_success}.
    template <class... Args>
    std::pair<right_iterator, bool> try_emplace_right(const Right& k, Args&&... args) {
        auto it_rtl = right_to_left_.find(k);
        if (it_rtl != right_to_left_.end()) {
            return {it_rtl, false};
        }

        Left l_val(std::forward<Args>(args)...);
        if (left_to_right_.count(l_val)) {
            return {right_to_left_.end(), false};
        }

        auto emplace_res_rtl = right_to_left_.emplace(k, std::move(l_val));
        left_to_right_.emplace(emplace_res_rtl.first->second, emplace_res_rtl.first->first);

        return {emplace_res_rtl.first, true};
    }

    template <class... Args>
    std::pair<right_iterator, bool> try_emplace_right(Right&& k, Args&&... args) {
        auto it_rtl = right_to_left_.find(k); // Find with k before potential move
        if (it_rtl != right_to_left_.end()) {
            return {it_rtl, false};
        }

        Left l_val(std::forward<Args>(args)...);
        if (left_to_right_.count(l_val)) {
            return {right_to_left_.end(), false};
        }

        auto emplace_res_rtl = right_to_left_.emplace(std::forward<Right>(k), std::move(l_val));
        left_to_right_.emplace(emplace_res_rtl.first->second, emplace_res_rtl.first->first);

        return {emplace_res_rtl.first, true};
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
    
    const Right* find_left(const Left& left) const noexcept {
        auto it = left_to_right_.find(left);
        return (it != left_to_right_.end()) ? &(it->second) : nullptr;
    }
    
    const Left* find_right(const Right& right) const noexcept {
        auto it = right_to_left_.find(right);
        return (it != right_to_left_.end()) ? &(it->second) : nullptr;
    }
    
    bool contains_left(const Left& left) const noexcept {
        return left_to_right_.find(left) != left_to_right_.end();
    }
    
    bool contains_right(const Right& right) const noexcept {
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
    
    void clear() noexcept { // Assuming std::unordered_map::clear() is noexcept
        left_to_right_.clear();
        right_to_left_.clear();
    }
    
    size_type size() const noexcept {
        return left_to_right_.size();
    }
    
    bool empty() const noexcept {
        return left_to_right_.empty();
    }

    void swap(BiMap& other) noexcept {
        using std::swap; // Enable ADL
        swap(left_to_right_, other.left_to_right_);
        swap(right_to_left_, other.right_to_left_);
    }

    // Comparison operators
    bool operator==(const BiMap& other) const {
        // If left_to_right_ maps are equal, and both BiMaps maintain invariants,
        // then right_to_left_ maps should also be equal.
        return left_to_right_ == other.left_to_right_;
    }

    bool operator!=(const BiMap& other) const {
        return !(*this == other);
    }
    
    // View accessors
    left_view left() const noexcept { return left_view(this); }
    right_view right() const noexcept { return right_view(this); }
    
    // STL-compatible iterators for left view (default)
    left_view_iterator begin() const { return left().begin(); }
    left_view_iterator end() const { return left().end(); }
    left_view_iterator cbegin() const { return left().cbegin(); }
    left_view_iterator cend() const { return left().cend(); }
    
    // Direct access to underlying iterators (for advanced use)
    // Non-const versions now return const_iterators for safety.
    left_const_iterator left_begin() { return left_to_right_.cbegin(); }
    left_const_iterator left_end() { return left_to_right_.cend(); }
    left_const_iterator left_begin() const { return left_to_right_.cbegin(); } // Changed to cbegin for consistency
    left_const_iterator left_end() const { return left_to_right_.cend(); }   // Changed to cend for consistency
    left_const_iterator left_cbegin() const { return left_to_right_.cbegin(); }
    left_const_iterator left_cend() const { return left_to_right_.cend(); }
    
    // Non-const versions now return const_iterators for safety.
    right_const_iterator right_begin() { return right_to_left_.cbegin(); }
    right_const_iterator right_end() { return right_to_left_.cend(); }
    right_const_iterator right_begin() const { return right_to_left_.cbegin(); } // Changed to cbegin for consistency
    right_const_iterator right_end() const { return right_to_left_.cend(); }   // Changed to cend for consistency
    right_const_iterator right_cbegin() const { return right_to_left_.cbegin(); }
    right_const_iterator right_cend() const { return right_to_left_.cend(); }
};

// Non-member swap for BiMap
template<typename Left, typename Right>
void swap(BiMap<Left, Right>& lhs, BiMap<Left, Right>& rhs) noexcept {
    lhs.swap(rhs);
}

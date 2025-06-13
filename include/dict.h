#pragma once

#include <unordered_map>
#include <vector>
#include <utility>
#include <stdexcept>
#include <functional>
#include <memory>
#include <type_traits>
#include <initializer_list>
#include <iostream>
#include <optional>

namespace pydict {

template<typename Key, typename Value, 
         typename Hash = std::hash<Key>,
         typename KeyEqual = std::equal_to<Key>,
         typename Allocator = std::allocator<std::pair<const Key, Value>>>
class dict {
public:
    // Type aliases for STL compatibility
    // Iterator invalidation:
    // - General: Iterators are invalidated if a rehash occurs. A rehash can be triggered by insertions
    //   (insert, emplace, operator[], setdefault) that increase the size beyond the current capacity times load_factor.
    //   Explicit calls to rehash() or reserve() can also cause invalidation.
    // - erase(key): Invalidates iterators and references to the erased element.
    //   Additionally, because erase uses a swap-and-pop mechanism on insertion_order_ to maintain O(1) complexity,
    //   an iterator pointing to the element that was previously at the end of the insertion order and got swapped
    //   into the erased element's position will now point to a different element (the one it was swapped with).
    //   Effectively, iterators to the erased element, the (previously) last element (if it was moved), and end()
    //   iterators might be invalidated or changed.
    // - clear(): Invalidates all iterators.
    // - insert/emplace/operator[]/setdefault: If a rehash occurs, all iterators are invalidated.
    //   If no rehash occurs, iterators are not invalidated. References might be invalidated if
    //   insertion_order_ (std::vector) reallocates. Since new elements are added to the end of
    //   insertion_order_, this can cause reallocation.

    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<const Key, Value>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using hasher = Hash;
    using key_equal = KeyEqual;
    using allocator_type = Allocator;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = typename std::allocator_traits<Allocator>::pointer;
    using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

private:
    // Internal storage combining hash map for O(1) access and vector for insertion order
    using storage_type = std::unordered_map<Key, std::pair<Value, size_type>, Hash, KeyEqual, Allocator>;
    using order_type = std::vector<Key>;
    
    storage_type storage_;
    order_type insertion_order_;
    
    // Helper to maintain insertion order
    void add_to_order(const Key& key) {
        insertion_order_.push_back(key);
        storage_[key].second = insertion_order_.size() - 1;
    }
    
    void remove_from_order(const Key& key) {
        auto it = storage_.find(key);
        if (it != storage_.end()) {
            size_type index = it->second.second;
            // Remove from order vector - swap with last element for O(1) removal
            if (index < insertion_order_.size() - 1) {
                Key last_key = insertion_order_.back();
                insertion_order_[index] = last_key;
                storage_[last_key].second = index;
            }
            insertion_order_.pop_back();
        }
    }

public:
    // Iterator implementation for ordered traversal
    // Note: For iterator invalidation rules, see comments at the top of the dict class definition.
    class iterator {
    private:
        dict* dict_ptr_;
        size_type index_;
        
        // Proxy class to handle the const Key issue
        class reference_proxy {
        private:
            const Key& key_;
            Value& value_;
            
        public:
            reference_proxy(const Key& k, Value& v) : key_(k), value_(v) {}
            
            // Allow structured binding and member access
            const Key& first() const { return key_; }
            Value& second() { return value_; }
            const Value& second() const { return value_; }
            
            // Conversion to pair for compatibility
            operator std::pair<Key, Value>() const {
                return {key_, value_};
            }
            
            // For arrow operator support
            reference_proxy* operator->() { return this; }
            const reference_proxy* operator->() const { return this; }
        };
        
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<const Key, Value>;
        using difference_type = std::ptrdiff_t;
        using pointer = reference_proxy;
        using reference = reference_proxy;
        
        iterator(dict* d, size_type idx) : dict_ptr_(d), index_(idx) {}
        
        reference operator*() {
            const Key& key = dict_ptr_->insertion_order_[index_];
            Value& value = dict_ptr_->storage_[key].first;
            return reference_proxy(key, value);
        }
        
        pointer operator->() { 
            const Key& key = dict_ptr_->insertion_order_[index_];
            Value& value = dict_ptr_->storage_[key].first;
            return reference_proxy(key, value);
        }
        
        iterator& operator++() {
            ++index_;
            return *this;
        }
        
        iterator operator++(int) {
            iterator temp = *this;
            ++index_;
            return temp;
        }
        
        bool operator==(const iterator& other) const {
            return dict_ptr_ == other.dict_ptr_ && index_ == other.index_;
        }
        
        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }
    };
    
    class const_iterator {
    private:
        // Note: For iterator invalidation rules, see comments at the top of the dict class definition.
        const dict* dict_ptr_;
        size_type index_;
        
        // Const proxy class
        class const_reference_proxy {
        private:
            const Key& key_;
            const Value& value_;
            
        public:
            const_reference_proxy(const Key& k, const Value& v) : key_(k), value_(v) {}
            
            // Allow structured binding and member access
            const Key& first() const { return key_; }
            const Value& second() const { return value_; }
            
            // Conversion to pair for compatibility
            operator std::pair<Key, Value>() const {
                return {key_, value_};
            }
            
            // For arrow operator support
            const const_reference_proxy* operator->() const { return this; }
        };
        
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<const Key, Value>;
        using difference_type = std::ptrdiff_t;
        using pointer = const_reference_proxy;
        using reference = const_reference_proxy;
        
        const_iterator(const dict* d, size_type idx) : dict_ptr_(d), index_(idx) {}
        
        reference operator*() const {
            const Key& key = dict_ptr_->insertion_order_[index_];
            const Value& value = dict_ptr_->storage_.at(key).first;
            return const_reference_proxy(key, value);
        }
        
        pointer operator->() const { 
            const Key& key = dict_ptr_->insertion_order_[index_];
            const Value& value = dict_ptr_->storage_.at(key).first;
            return const_reference_proxy(key, value);
        }
        
        const_iterator& operator++() {
            ++index_;
            return *this;
        }
        
        const_iterator operator++(int) {
            const_iterator temp = *this;
            ++index_;
            return temp;
        }
        
        bool operator==(const const_iterator& other) const {
            return dict_ptr_ == other.dict_ptr_ && index_ == other.index_;
        }
        
        bool operator!=(const const_iterator& other) const {
            return !(*this == other);
        }
    };

    // Constructors
    dict() = default;
    
    explicit dict(const Allocator& alloc) : storage_(alloc) {}
    
    dict(std::initializer_list<value_type> init, const Allocator& alloc = Allocator()) 
        : storage_(alloc) {
        for (const auto& pair : init) {
            insert(pair);
        }
    }
    
    template<typename InputIt>
    dict(InputIt first, InputIt last, const Allocator& alloc = Allocator()) 
        : storage_(alloc) {
        for (auto it = first; it != last; ++it) {
            insert(*it);
        }
    }
    
    // Copy and move constructors
    dict(const dict& other) = default;
    // Move constructor: noexcept if underlying moves are noexcept.
    // std::unordered_map and std::vector move operations are noexcept if their allocators are noexcept
    // or propagate on move assignment. Default allocator moves are noexcept.
    dict(dict&& other) noexcept(
        std::is_nothrow_move_constructible<storage_type>::value &&
        std::is_nothrow_move_constructible<order_type>::value
    ) = default;
    
    // Assignment operators
    dict& operator=(const dict& other) = default;
    // Move assignment operator: similar noexcept conditions as move constructor.
    dict& operator=(dict&& other) noexcept(
        std::is_nothrow_move_assignable<storage_type>::value &&
        std::is_nothrow_move_assignable<order_type>::value
    ) = default;
    dict& operator=(std::initializer_list<value_type> init) {
        clear();
        for (const auto& pair : init) {
            insert(pair);
        }
        return *this;
    }
    
    // Destructor
    ~dict() = default;
    
    // Iterators
    iterator begin() noexcept { return iterator(this, 0); }
    const_iterator begin() const noexcept { return const_iterator(this, 0); }
    const_iterator cbegin() const noexcept { return const_iterator(this, 0); }
    
    iterator end() noexcept { return iterator(this, insertion_order_.size()); }
    const_iterator end() const noexcept { return const_iterator(this, insertion_order_.size()); }
    const_iterator cend() const noexcept { return const_iterator(this, insertion_order_.size()); }
    
    // Capacity
    bool empty() const noexcept { return storage_.empty(); }
    size_type size() const noexcept { return storage_.size(); }
    size_type max_size() const noexcept { return storage_.max_size(); }
    
    // Element access
    Value& operator[](const Key& key) {
        auto it = storage_.find(key);
        if (it == storage_.end()) {
            add_to_order(key);
            return storage_[key].first;
        }
        return it->second.first;
    }
    
    Value& operator[](Key&& key) {
        auto it = storage_.find(key);
        if (it == storage_.end()) {
            add_to_order(key);
            return storage_[std::move(key)].first;
        }
        return it->second.first;
    }
    
    Value& at(const Key& key) {
        auto it = storage_.find(key);
        if (it == storage_.end()) {
            throw std::out_of_range("dict::at: key not found");
        }
        return it->second.first;
    }
    
    const Value& at(const Key& key) const {
        auto it = storage_.find(key);
        if (it == storage_.end()) {
            throw std::out_of_range("dict::at: key not found");
        }
        return it->second.first;
    }
    
    // Python-like get method with default value
    Value get(const Key& key, const Value& default_value = Value{}) const {
        auto it = storage_.find(key);
        return (it != storage_.end()) ? it->second.first : default_value;
    }
    
    std::optional<Value> get_optional(const Key& key) const {
        auto it = storage_.find(key);
        return (it != storage_.end()) ? std::make_optional(it->second.first) : std::nullopt;
    }
    
    // Modifiers
    // clear: Invalidates all iterators.
    void clear() noexcept {
        storage_.clear();
        insertion_order_.clear();
    }
    
    // insert: If a rehash occurs, all iterators are invalidated.
    // If no rehash occurs, iterators are not invalidated. References might be invalidated if
    // insertion_order_ (std::vector) reallocates.
    std::pair<iterator, bool> insert(const value_type& value) {
        auto it = storage_.find(value.first);
        if (it != storage_.end()) {
            return {iterator(this, it->second.second), false};
        }
        
        add_to_order(value.first);
        storage_[value.first].first = value.second;
        return {iterator(this, insertion_order_.size() - 1), true};
    }
    
    // insert (move version): Same invalidation rules as const value_type& version.
    std::pair<iterator, bool> insert(value_type&& value) {
        auto it = storage_.find(value.first);
        if (it != storage_.end()) {
            return {iterator(this, it->second.second), false};
        }
        
        Key key = value.first;  // Copy key before moving
        add_to_order(key);
        storage_[std::move(key)].first = std::move(value.second);
        return {iterator(this, insertion_order_.size() - 1), true};
    }
    
    // emplace: Same invalidation rules as insert.
    template<typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        // Note: This implementation of emplace creates a temporary pair and then moves it.
        // A more direct emplace would construct the value in place in the map and then add key to order.
        // However, the current structure with separate map and vector makes direct emplacement complex.
        auto temp_pair = value_type(std::forward<Args>(args)...); // This might throw if pair construction throws
        return insert(std::move(temp_pair));
    }
    
    // erase: Invalidates iterators and references to the erased element.
    // Also invalidates iterators to the element that was swapped with the erased element (the previously last element).
    size_type erase(const Key& key) {
        auto it = storage_.find(key);
        if (it == storage_.end()) {
            return 0;
        }
        
        remove_from_order(key);
        storage_.erase(it);
        return 1;
    }
    
    // Python-like pop method
    Value pop(const Key& key) {
        auto it = storage_.find(key);
        if (it == storage_.end()) {
            throw std::out_of_range("dict::pop: key not found");
        }
        
        Value value = std::move(it->second.first); // This move could potentially throw for complex Value types, though typically not for std::move
        remove_from_order(key);
        storage_.erase(it);
        return value;
    }
    
    Value pop(const Key& key, const Value& default_value) {
        auto it = storage_.find(key);
        if (it == storage_.end()) {
            return default_value;
        }
        
        Value value = std::move(it->second.first);  // Potential throw
        remove_from_order(key);
        storage_.erase(it);
        return value;
    }

    // popitem: Removes and returns the last inserted item (LIFO).
    // Throws std::out_of_range if the dictionary is empty.
    // Invalidates iterators to the removed element and the end() iterator.
    std::pair<Key, Value> popitem() {
        if (insertion_order_.empty()) {
            throw std::out_of_range("dict::popitem: dictionary is empty");
        }

        Key key_to_pop = insertion_order_.back();

        auto storage_it = storage_.find(key_to_pop);
        // This check should ideally not be needed if insertion_order_ and storage_ are always consistent.
        // However, it's a good safeguard.
        if (storage_it == storage_.end()) {
            // This indicates an internal inconsistency. For robustness, handle it.
            // Though, with current logic, this path should not be hit if dict is not empty.
            insertion_order_.pop_back(); // Try to correct insertion_order_
            throw std::runtime_error("dict::popitem: internal inconsistency, key not found in storage");
        }

        Value value_to_pop = std::move(storage_it->second.first); // Potential throw

        storage_.erase(storage_it);
        insertion_order_.pop_back(); // This invalidates end iterators and iterators to the popped element.

        return {key_to_pop, std::move(value_to_pop)}; // Key is copied, value is moved.
    }
    
    // Python-like setdefault method
    // setdefault: If an element is inserted, same invalidation rules as insert. Otherwise, no invalidation.
    Value& setdefault(const Key& key, const Value& default_value = Value{}) {
        auto it = storage_.find(key);
        if (it != storage_.end()) {
            return it->second.first;
        }
        
        add_to_order(key);
        storage_[key].first = default_value;
        return storage_[key].first;
    }
    
    // Python-like update method
    void update(const dict& other) {
        for (const auto& key : other.insertion_order_) {
            const auto& value = other.storage_.at(key).first;
            (*this)[key] = value;
        }
    }
    
    void update(std::initializer_list<value_type> init) {
        for (const auto& pair : init) {
            (*this)[pair.first] = pair.second;
        }
    }
    
    // Lookup
    size_type count(const Key& key) const {
        return storage_.count(key);
    }
    
    iterator find(const Key& key) {
        auto it = storage_.find(key);
        if (it == storage_.end()) {
            return end();
        }
        return iterator(this, it->second.second);
    }
    
    const_iterator find(const Key& key) const {
        auto it = storage_.find(key);
        if (it == storage_.end()) {
            return end();
        }
        return const_iterator(this, it->second.second);
    }
    
    bool contains(const Key& key) const {
        return storage_.find(key) != storage_.end();
    }
    
    // Python-like in operator (using contains)
    bool operator()(const Key& key) const {
        return contains(key);
    }
    
    // Hash policy
    float load_factor() const noexcept { return storage_.load_factor(); }
    float max_load_factor() const noexcept { return storage_.max_load_factor(); }
    void max_load_factor(float ml) { storage_.max_load_factor(ml); } // std::unordered_map::max_load_factor(float) is not noexcept
    // rehash: Invalidates all iterators if rehash occurs.
    void rehash(size_type count) { storage_.rehash(count); } // Can throw (e.g. std::bad_alloc)
    // reserve: Invalidates all iterators if rehash occurs.
    void reserve(size_type count) { storage_.reserve(count); } // Can throw (e.g. std::bad_alloc)
    
    // Observers
    hasher hash_function() const noexcept { return storage_.hash_function(); }
    key_equal key_eq() const noexcept { return storage_.key_eq(); }
    allocator_type get_allocator() const noexcept { return storage_.get_allocator(); }
    
    // Python-like methods for getting keys, values, items
    std::vector<Key> keys() const {
        return insertion_order_;
    }
    
    std::vector<Value> values() const {
        std::vector<Value> result;
        result.reserve(insertion_order_.size());
        for (const auto& key : insertion_order_) {
            result.push_back(storage_.at(key).first);
        }
        return result;
    }
    
    std::vector<std::pair<Key, Value>> items() const {
        std::vector<std::pair<Key, Value>> result;
        result.reserve(insertion_order_.size());
        for (const auto& key : insertion_order_) {
            result.emplace_back(key, storage_.at(key).first);
        }
        return result;
    }
    
    // Comparison operators
    bool operator==(const dict& other) const {
        if (size() != other.size()) return false;
        for (const auto& key : insertion_order_) {
            auto it = other.storage_.find(key);
            if (it == other.storage_.end() || it->second.first != storage_.at(key).first) {
                return false;
            }
        }
        return true;
    }
    
    bool operator!=(const dict& other) const {
        return !(*this == other);
    }
    
    // Stream output operator
    friend std::ostream& operator<<(std::ostream& os, const dict& d) { // Not typically noexcept due to stream operations
        os << "{";
        bool first = true;
        for (const auto& key : d.insertion_order_) {
            if (!first) os << ", ";
            os << key << ": " << d.storage_.at(key).first;
            first = false;
        }
        os << "}";
        return os;
    }

    // Friend declaration for non-member swap
    friend void swap<>(dict<Key, Value, Hash, KeyEqual, Allocator>& lhs, dict<Key, Value, Hash, KeyEqual, Allocator>& rhs)
        noexcept(noexcept(std::swap(lhs.storage_, rhs.storage_)) &&
                 noexcept(std::swap(lhs.insertion_order_, rhs.insertion_order_)));
};

// Deduction guides for C++17
template<typename InputIt>
dict(InputIt, InputIt) -> dict<
    typename std::iterator_traits<InputIt>::value_type::first_type,
    typename std::iterator_traits<InputIt>::value_type::second_type
>; // This deduction guide might need SFINAE or concepts to be more robust for non-pair value_types

// Swaps two dict objects.
// noexcept if the allocator supports noexcept swap and propagates on swap,
// and if Hash and KeyEqual are swappable.
// For standard types, this generally means noexcept.
template<typename K, typename V, typename H, typename KE, typename A>
void swap(dict<K, V, H, KE, A>& lhs, dict<K, V, H, KE, A>& rhs)
    noexcept(noexcept(std::swap(lhs.storage_, rhs.storage_)) &&
             noexcept(std::swap(lhs.insertion_order_, rhs.insertion_order_))) {
    using std::swap;
    swap(lhs.storage_, rhs.storage_);
    swap(lhs.insertion_order_, rhs.insertion_order_);
}

template<typename Key, typename Value>
dict(std::initializer_list<std::pair<Key, Value>>) -> dict<Key, Value>; // Similarly, might need constraints

} // namespace pydict


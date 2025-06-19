#ifndef SLOTMAP_H
#define SLOTMAP_H

#include <vector>
#include <cstdint>
#include <limits>
#include <cassert>
#include <type_traits>
#include <utility>
#include <algorithm>
#include <stdexcept>

namespace utils {

template <typename T>
class SlotMap {
public:
    using size_type = std::size_t;
    using value_type = T;
    
    struct Key {
        uint32_t index;
        uint32_t generation;

        constexpr bool operator==(const Key& other) const noexcept = default;
        constexpr bool operator!=(const Key& other) const noexcept = default;
        
        // Add comparison operators for use in containers
        constexpr bool operator<(const Key& other) const noexcept {
            return index < other.index || (index == other.index && generation < other.generation);
        }
        
        // Check if key is valid (not default-constructed)
        constexpr bool is_valid() const noexcept {
            return index != INVALID_INDEX;
        }
    };

    static constexpr uint32_t INVALID_INDEX = std::numeric_limits<uint32_t>::max();
    static constexpr Key INVALID_KEY = Key{INVALID_INDEX, 0};

    // Constructors
    SlotMap() = default;
    explicit SlotMap(size_type reserve_size) {
        reserve(reserve_size);
        slot_active_.reserve(reserve_size);
    }
    
    // Move-only semantics for better performance
    SlotMap(const SlotMap&) = delete;
    SlotMap& operator=(const SlotMap&) = delete;
    SlotMap(SlotMap&&) = default;
    SlotMap& operator=(SlotMap&&) = default;

    // Capacity management
    void reserve(size_type capacity) {
        data_.reserve(capacity);
        generations_.reserve(capacity);
        slot_active_.reserve(capacity);
    }
    
    void shrink_to_fit() {
        data_.shrink_to_fit();
        generations_.shrink_to_fit();
        free_list_.shrink_to_fit();
        slot_active_.shrink_to_fit();
    }

    // Element access and modification
    template<typename... Args>
    Key emplace(Args&&... args) {
        static_assert(std::is_constructible_v<T, Args...>, 
                     "T must be constructible from provided arguments");
        
        uint32_t index;
        if (!free_list_.empty()) {
            index = free_list_.back();
            free_list_.pop_back();
            
            // The object at data_[index] was destroyed by erase.
            // Construct the new object in place.
            new (std::addressof(data_[index])) T(std::forward<Args>(args)...);
            slot_active_[index] = true;
        } else {
            if (data_.size() >= INVALID_INDEX) {
                throw std::runtime_error("SlotMap capacity exceeded");
            }
            
            index = static_cast<uint32_t>(data_.size());
            data_.emplace_back(std::forward<Args>(args)...);
            generations_.push_back(0);
            slot_active_.push_back(true);
        }
        
        return Key{index, generations_[index]};
    }
    
    Key insert(T value) {
        return emplace(std::move(value));
    }

    bool erase(Key key) noexcept {
        if (!is_valid_key(key)) {
            return false;
        }

        // Destroy the object properly
        if constexpr (std::is_destructible_v<T>) {
            data_[key.index].~T();
        }
        
        // Add to free list
        free_list_.push_back(key.index);

        // Increment generation with overflow handling
        increment_generation(key.index);
        slot_active_[key.index] = false;
        
        return true;
    }

    // Access methods with bounds checking
    T* get(Key key) noexcept {
        return is_valid_key(key) ? &data_[key.index] : nullptr;
    }
    
    const T* get(Key key) const noexcept {
        return is_valid_key(key) ? &data_[key.index] : nullptr;
    }
    
    // Checked access - throws on invalid key
    T& at(Key key) {
        if (!is_valid_key(key)) {
            throw std::out_of_range("Invalid SlotMap key");
        }
        return data_[key.index];
    }
    
    const T& at(Key key) const {
        if (!is_valid_key(key)) {
            throw std::out_of_range("Invalid SlotMap key");
        }
        return data_[key.index];
    }
    
    // Unchecked access - faster but unsafe
    T& operator[](Key key) noexcept {
        assert(is_valid_key(key) && "Invalid SlotMap key");
        return data_[key.index];
    }
    
    const T& operator[](Key key) const noexcept {
        assert(is_valid_key(key) && "Invalid SlotMap key");
        return data_[key.index];
    }

    bool contains(Key key) const noexcept {
        return is_valid_key(key);
    }

    // Size and capacity
    size_type size() const noexcept {
        return data_.size() - free_list_.size();
    }
    
    size_type capacity() const noexcept {
        return data_.capacity();
    }
    
    bool empty() const noexcept {
        return size() == 0;
    }
    
    size_type max_size() const noexcept {
        return std::min(data_.max_size(), static_cast<size_type>(INVALID_INDEX));
    }

    // Clear all elements
    void clear() noexcept {
        data_.clear();
        generations_.clear();
        free_list_.clear();
        slot_active_.clear();
    }

    // Iterator support for range-based for loops
    class const_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<Key, const T&>;
        using difference_type = std::ptrdiff_t;
        using pointer = void; // Not meaningful for this iterator
        using reference = value_type;

        const_iterator(const SlotMap* map, size_type index) : map_(map), index_(index) {
            skip_invalid_slots();
        }

        reference operator*() const {
            return {Key{static_cast<uint32_t>(index_), map_->generations_[index_]},
                    map_->data_[index_]};
        }

        const_iterator& operator++() {
            ++index_;
            skip_invalid_slots();
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const const_iterator& other) const {
            return map_ == other.map_ && index_ == other.index_;
        }

        bool operator!=(const const_iterator& other) const {
            return !(*this == other);
        }

    private:
        void skip_invalid_slots() {
            while (index_ < map_->data_.size() &&
                   !map_->is_slot_active(static_cast<uint32_t>(index_))) {
                ++index_;
            }
        }

        const SlotMap* map_;
        size_type index_;
    };

    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<Key, T&>;
        using difference_type = std::ptrdiff_t;
        using pointer = void; // Not meaningful for this iterator
        using reference = value_type;

        iterator(SlotMap* map, size_type index) : map_(map), index_(index) {
            skip_invalid_slots();
        }

        reference operator*() const {
            return {Key{static_cast<uint32_t>(index_), map_->generations_[index_]}, 
                    map_->data_[index_]};
        }

        iterator& operator++() {
            ++index_;
            skip_invalid_slots();
            return *this;
        }

        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const iterator& other) const {
            return map_ == other.map_ && index_ == other.index_;
        }

        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }

    private:
        void skip_invalid_slots() {
            while (index_ < map_->data_.size() && 
                   !map_->is_slot_active(static_cast<uint32_t>(index_))) {
                ++index_;
            }
        }

        SlotMap* map_;
        size_type index_;
    };

    iterator begin() { return iterator(this, 0); }
    iterator end() { return iterator(this, data_.size()); }

    const_iterator begin() const { return cbegin(); }
    const_iterator end() const { return cend(); }

    const_iterator cbegin() const { return const_iterator(this, 0); }
    const_iterator cend() const { return const_iterator(this, data_.size()); }

private:
    std::vector<T> data_;
    std::vector<uint32_t> generations_;
    std::vector<uint32_t> free_list_; // Changed to uint32_t for consistency
    std::vector<bool> slot_active_;

    bool is_valid_index(uint32_t index) const noexcept {
        return index < data_.size();
    }
    
    bool is_valid_key(Key key) const noexcept {
        return key.index != INVALID_INDEX && 
               is_valid_index(key.index) && 
               generations_[key.index] == key.generation;
    }
    
    void increment_generation(uint32_t index) noexcept {
        if (generations_[index] == std::numeric_limits<uint32_t>::max()) {
            generations_[index] = 1; // Skip 0 to avoid confusion with new slots
        } else {
            generations_[index]++;
        }
    }
    
    // More efficient method to check if a slot is active (opposite of free)
    bool is_slot_active(uint32_t index) const noexcept {
        // A slot is active if its index is valid and its active flag is true.
        // We also still need to check the generation for key validity,
        // but for iteration purposes, slot_active_ is enough.
        return index < slot_active_.size() && slot_active_[index];
    }
};

} // namespace utils

#endif // SLOTMAP_H

#pragma once

#include <unordered_map>
#include <map>
#include <memory>
#include <initializer_list>
#include <stdexcept>
#include <utility>

/**
 * @brief ConstDict - A read-only wrapper around standard map types
 * 
 * Provides immutable dictionary semantics by wrapping an underlying map
 * and exposing only const operations. Designed for C++17 compatibility.
 * 
 * @tparam Key The key type (must be hashable for unordered_map or orderable for map)
 * @tparam Value The value type
 * @tparam UnderlyingMap The underlying map type to wrap
 */
template <typename Key, typename Value, typename UnderlyingMap = std::unordered_map<Key, Value>>
class ConstDict {
public:
    // Type aliases for STL compatibility
    using key_type = typename UnderlyingMap::key_type;
    using mapped_type = typename UnderlyingMap::mapped_type;
    using value_type = typename UnderlyingMap::value_type;
    using size_type = typename UnderlyingMap::size_type;
    using const_iterator = typename UnderlyingMap::const_iterator;
    using const_reference = const Value&;

    /**
     * @brief Construct from initializer list
     * @param init Initializer list of key-value pairs
     */
    ConstDict(std::initializer_list<std::pair<const Key, Value>> init)
        : map_(std::make_shared<const UnderlyingMap>(init)) {}

    /**
     * @brief Construct by copying an existing map
     * @param map The map to copy
     */
    explicit ConstDict(const UnderlyingMap& map)
        : map_(std::make_shared<const UnderlyingMap>(map)) {}

    /**
     * @brief Construct by moving an existing map
     * @param map The map to move
     */
    explicit ConstDict(UnderlyingMap&& map)
        : map_(std::make_shared<const UnderlyingMap>(std::move(map))) {}

    /**
     * @brief Construct from a shared pointer to const map
     * @param map_ptr Shared pointer to the underlying map
     */
    explicit ConstDict(std::shared_ptr<const UnderlyingMap> map_ptr)
        : map_(std::move(map_ptr)) {}

    // Copy constructor
    ConstDict(const ConstDict&) = default;

    // Move constructor
    ConstDict(ConstDict&&) = default;

    // Copy assignment
    ConstDict& operator=(const ConstDict&) = default;

    // Move assignment
    ConstDict& operator=(ConstDict&&) = default;

    // Destructor
    ~ConstDict() = default;

    /**
     * @brief Access element with bounds checking
     * @param key The key to look up
     * @return const reference to the value
     * @throws std::out_of_range if key is not found
     */
    const_reference at(const Key& key) const {
        return map_->at(key);
    }

    /**
     * @brief Access element (read-only)
     * @param key The key to look up
     * @return const reference to the value
     * @throws std::out_of_range if key is not found
     * 
     * Note: This operator[] throws on missing keys unlike std::map's operator[]
     * to maintain immutability (std::map's operator[] can insert new elements)
     */
    const_reference operator[](const Key& key) const {
        return at(key);
    }

    /**
     * @brief Get iterator to beginning
     * @return const iterator to first element
     */
    const_iterator begin() const noexcept {
        return map_->begin();
    }

    /**
     * @brief Get iterator to end
     * @return const iterator to past-the-end element
     */
    const_iterator end() const noexcept {
        return map_->end();
    }

    /**
     * @brief Get const iterator to beginning
     * @return const iterator to first element
     */
    const_iterator cbegin() const noexcept {
        return map_->cbegin();
    }

    /**
     * @brief Get const iterator to end
     * @return const iterator to past-the-end element
     */
    const_iterator cend() const noexcept {
        return map_->cend();
    }

    /**
     * @brief Get number of elements
     * @return number of elements in the dictionary
     */
    size_type size() const noexcept {
        return map_->size();
    }

    /**
     * @brief Check if dictionary is empty
     * @return true if empty, false otherwise
     */
    bool empty() const noexcept {
        return map_->empty();
    }

    /**
     * @brief Count elements with specific key
     * @param key The key to count
     * @return number of elements with key (0 or 1 for maps)
     */
    size_type count(const Key& key) const {
        return map_->count(key);
    }

    /**
     * @brief Find element with specific key
     * @param key The key to find
     * @return const iterator to element or end() if not found
     */
    const_iterator find(const Key& key) const {
        return map_->find(key);
    }

    /**
     * @brief Check if key exists in dictionary
     * @param key The key to check
     * @return true if key exists, false otherwise
     */
    bool contains(const Key& key) const {
        if constexpr (requires { map_->contains(key); }) {
            // Use contains() if available (C++20)
            return map_->contains(key);
        } else {
            // Fallback for C++17
            return map_->find(key) != map_->end();
        }
    }

    /**
     * @brief Get maximum possible size
     * @return maximum possible size
     */
    size_type max_size() const noexcept {
        return map_->max_size();
    }

    /**
     * @brief Equality comparison
     * @param other Another ConstDict to compare with
     * @return true if dictionaries are equal, false otherwise
     */
    bool operator==(const ConstDict& other) const {
        return *map_ == *other.map_;
    }

    /**
     * @brief Inequality comparison
     * @param other Another ConstDict to compare with
     * @return true if dictionaries are not equal, false otherwise
     */
    bool operator!=(const ConstDict& other) const {
        return !(*this == other);
    }

    /**
     * @brief Get underlying map as shared_ptr<const>
     * @return shared pointer to the underlying const map
     */
    std::shared_ptr<const UnderlyingMap> get_underlying_map() const {
        return map_;
    }

    // Explicitly delete mutation operations to prevent accidental use
    template<typename... Args>
    void insert(Args&&...) = delete;
    
    template<typename... Args>
    void emplace(Args&&...) = delete;
    
    template<typename... Args>
    void erase(Args&&...) = delete;
    
    void clear() = delete;
    void swap(ConstDict&) = delete;

private:
    std::shared_ptr<const UnderlyingMap> map_;
};

// Convenience type aliases
template<typename Key, typename Value>
using ConstUnorderedDict = ConstDict<Key, Value, std::unordered_map<Key, Value>>;

template<typename Key, typename Value>
using ConstOrderedDict = ConstDict<Key, Value, std::map<Key, Value>>;

// Deduction guides for C++17
template<typename Key, typename Value>
ConstDict(std::initializer_list<std::pair<const Key, Value>>) 
    -> ConstDict<Key, Value, std::unordered_map<Key, Value>>;

template<typename UnderlyingMap>
ConstDict(const UnderlyingMap&) 
    -> ConstDict<typename UnderlyingMap::key_type, 
                 typename UnderlyingMap::mapped_type, 
                 UnderlyingMap>;

template<typename UnderlyingMap>
ConstDict(UnderlyingMap&&) 
    -> ConstDict<typename UnderlyingMap::key_type, 
                 typename UnderlyingMap::mapped_type, 
                 UnderlyingMap>;

/**
 * @brief Factory function to create ConstDict from initializer list
 * @param init Initializer list of key-value pairs
 * @return ConstDict instance
 */
template<typename Key, typename Value>
auto make_const_dict(std::initializer_list<std::pair<const Key, Value>> init) {
    return ConstDict<Key, Value>(init);
}

/**
 * @brief Factory function to create ConstDict from existing map
 * @param map The map to wrap
 * @return ConstDict instance
 */
template<typename UnderlyingMap>
auto make_const_dict(const UnderlyingMap& map) {
    return ConstDict<typename UnderlyingMap::key_type, 
                     typename UnderlyingMap::mapped_type, 
                     UnderlyingMap>(map);
}

/**
 * @brief Factory function to create ConstDict from moved map
 * @param map The map to wrap (moved)
 * @return ConstDict instance
 */
template<typename UnderlyingMap>
auto make_const_dict(UnderlyingMap&& map) {
    return ConstDict<typename UnderlyingMap::key_type, 
                     typename UnderlyingMap::mapped_type, 
                     UnderlyingMap>(std::forward<UnderlyingMap>(map));
}

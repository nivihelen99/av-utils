#ifndef FLAT_MAP_H
#define FLAT_MAP_H

#include <vector>
#include <utility> // For std::pair
#include <functional> // For std::less
#include <stdexcept> // For std::out_of_range
#include <algorithm> // For std::lower_bound, std::sort (potentially if needed later)

template<typename Key, typename Value, typename Compare = std::less<Key>>
class FlatMap {
public:
    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<Key, Value>;
    using key_compare = Compare;
    using reference = value_type&;
    using const_reference = const value_type&;
    // Iterators will be based on std::vector's iterators
    using iterator = typename std::vector<value_type>::iterator;
    using const_iterator = typename std::vector<value_type>::const_iterator;
    using size_type = typename std::vector<value_type>::size_type;

    // Constructors
    FlatMap() = default;
    explicit FlatMap(const Compare& comp) : comp_(comp) {}

    // Modifiers
    bool insert(const Key& key, const Value& value) {
        auto it = std::lower_bound(data_.begin(), data_.end(), key,
            [this](const value_type& element, const Key& k) {
                return comp_(element.first, k);
            });

        if (it != data_.end() && !comp_(key, it->first)) { // Key already exists
            it->second = value; // Update value
            return false; // No new element inserted
        } else { // Key does not exist, insert new element
            data_.insert(it, value_type{key, value});
            return true; // New element inserted
        }
    }

    Value& operator[](const Key& key) {
        auto it = std::lower_bound(data_.begin(), data_.end(), key,
            [this](const value_type& element, const Key& k) {
                return comp_(element.first, k);
            });

        if (it != data_.end() && !comp_(key, it->first)) { // Key already exists
            return it->second;
        } else { // Key does not exist, insert new element with default-constructed value
            // data_.insert will return an iterator to the inserted element
            auto inserted_it = data_.insert(it, value_type{key, Value{}});
            return inserted_it->second;
        }
    }

    bool erase(const Key& key) {
        auto it = std::lower_bound(data_.begin(), data_.end(), key,
            [this](const value_type& element, const Key& k) {
                return comp_(element.first, k);
            });

        if (it != data_.end() && !comp_(key, it->first)) { // Key found
            data_.erase(it);
            return true;
        }
        return false; // Key not found
    }

    // Capacity
    size_type size() const noexcept {
        return data_.size();
    }

    bool empty() const noexcept {
        return data_.empty();
    }

    // Iterators
    iterator begin() noexcept {
        return data_.begin();
    }

    const_iterator begin() const noexcept {
        return data_.begin();
    }

    iterator end() noexcept {
        return data_.end();
    }

    const_iterator end() const noexcept {
        return data_.end();
    }

    const_iterator cbegin() const noexcept {
        return data_.cbegin();
    }

    const_iterator cend() const noexcept {
        return data_.cend();
    }

    // More methods will be added here in subsequent steps

    // Lookup
    Value* find(const Key& key) {
        auto it = std::lower_bound(data_.begin(), data_.end(), key,
            [this](const value_type& element, const Key& k) {
                return comp_(element.first, k);
            });
        if (it != data_.end() && !comp_(key, it->first)) { // Found and keys are equivalent
            return &(it->second);
        }
        return nullptr;
    }

    const Value* find(const Key& key) const {
        auto it = std::lower_bound(data_.begin(), data_.end(), key,
            [this](const value_type& element, const Key& k) {
                return comp_(element.first, k);
            });
        if (it != data_.end() && !comp_(key, it->first)) { // Found and keys are equivalent
            return &(it->second);
        }
        return nullptr;
    }

    bool contains(const Key& key) const {
        return find(key) != nullptr;
    }

    Value& at(const Key& key) {
        Value* val_ptr = find(key);
        if (val_ptr) {
            return *val_ptr;
        }
        throw std::out_of_range("FlatMap::at: key not found");
    }

    const Value& at(const Key& key) const {
        const Value* val_ptr = find(key);
        if (val_ptr) {
            return *val_ptr;
        }
        throw std::out_of_range("FlatMap::at: key not found");
    }

private:
    std::vector<value_type> data_;
    Compare comp_;
};

#endif // FLAT_MAP_H

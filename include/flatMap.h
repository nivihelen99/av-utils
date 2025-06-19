#ifndef FLAT_MAP_H
#define FLAT_MAP_H

#include <vector>
#include <utility> // For std::pair
#include <algorithm> // For std::lower_bound, std::sort, std::find_if
#include <stdexcept> // For std::out_of_range

template <typename Key, typename Value>
class FlatMap {
private:
    std::vector<std::pair<Key, Value>> data_;

    // Helper to find an iterator to an element (or where it would be)
    // Returns data_.end() if key is greater than all keys in map
    // Otherwise, returns iterator to first element >= key
    typename std::vector<std::pair<Key, Value>>::iterator find_iterator(const Key& key) {
        return std::lower_bound(data_.begin(), data_.end(), key,
            [](const std::pair<Key, Value>& element, const Key& k) {
                return element.first < k;
            });
    }

    typename std::vector<std::pair<Key, Value>>::const_iterator find_iterator(const Key& key) const {
        return std::lower_bound(data_.begin(), data_.end(), key,
            [](const std::pair<Key, Value>& element, const Key& k) {
                return element.first < k;
            });
    }

public:
    FlatMap() = default;

    // Insert or update (sorted insert)
    void insert(const Key& key, const Value& value) {
        auto it = find_iterator(key);
        if (it != data_.end() && it->first == key) {
            // Key already exists, update value
            it->second = value;
        } else {
            // Key does not exist, insert new pair
            data_.insert(it, {key, value});
        }
    }

    // Erase by key
    bool erase(const Key& key) {
        auto it = find_iterator(key);
        if (it != data_.end() && it->first == key) {
            data_.erase(it);
            return true;
        }
        return false; // Key not found
    }

    // Lookup by key (binary search)
    Value* find(const Key& key) {
        auto it = find_iterator(key);
        if (it != data_.end() && it->first == key) {
            return &it->second;
        }
        return nullptr;
    }

    const Value* find(const Key& key) const {
        auto it = find_iterator(key);
        if (it != data_.end() && it->first == key) {
            return &it->second;
        }
        return nullptr;
    }

    // Check if a key exists
    bool contains(const Key& key) const {
        auto it = find_iterator(key);
        return (it != data_.end() && it->first == key);
    }

    // Size and status
    size_t size() const {
        return data_.size();
    }

    bool empty() const {
        return data_.empty();
    }

    // Optional: read/write operator[]
    // Inserts default if not present
    Value& operator[](const Key& key) {
        auto it = find_iterator(key);
        if (it != data_.end() && it->first == key) {
            return it->second;
        } else {
            // Key not found, insert default-constructed value
            // and return a reference to it.
            auto inserted_it = data_.insert(it, {key, Value{}});
            return inserted_it->second;
        }
    }

    // Throws if key not found
    const Value& at(const Key& key) const {
        const Value* val_ptr = find(key);
        if (val_ptr) {
            return *val_ptr;
        }
        throw std::out_of_range("FlatMap::at: key not found");
    }

    // Iterator access
    auto begin() const {
        return data_.begin();
    }

    auto end() const {
        return data_.end();
    }

    // Adding non-const iterators as well, which might be useful
    // although the requirement doc only specified const iterators.
    // These are often expected for map-like containers.
    auto begin() {
        return data_.begin();
    }

    auto end() {
        return data_.end();
    }
};

#endif // FLAT_MAP_H

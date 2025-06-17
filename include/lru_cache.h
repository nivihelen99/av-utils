#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <list>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <optional> // For std::optional
#include <utility>  // For std::pair
#include <stdexcept> // For std::runtime_error (though not strictly needed by spec, good for constructor)

template <typename Key, typename Value>
class LRUCache {
public:
    using EvictCallback = std::function<void(const Key&, const Value&)>;

    explicit LRUCache(size_t max_size, EvictCallback on_evict = nullptr)
        : max_size_(max_size), on_evict_(on_evict) {
        if (max_size == 0) {
            // Or handle as a special case where cache is always empty / disabled
            // For now, let's consider it an invalid argument.
            throw std::invalid_argument("LRUCache max_size must be greater than 0");
        }
    }

    // Returns true if the key is found in the cache, false otherwise.
    // The value corresponding to the key is copied into the 'value' parameter.
    // This operation marks the accessed item as most recently used.
    std::optional<Value> get(const Key& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_items_map_.find(key);
        if (it == cache_items_map_.end()) {
            return std::nullopt;
        }
        // Move accessed item to the front of the list (most recently used)
        cache_items_list_.splice(cache_items_list_.begin(), cache_items_list_, it->second);
        return it->second->second; // Value part of std::pair<Key, Value>
    }

    // Inserts a key-value pair into the cache.
    // If the key already exists, its value is updated, and it's marked as most recently used.
    // If the cache is full, the least recently used item is evicted.
    void put(const Key& key, const Value& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_items_map_.find(key);

        if (it != cache_items_map_.end()) {
            // Key exists, update value and move to front (MRU)
            it->second->second = value;
            cache_items_list_.splice(cache_items_list_.begin(), cache_items_list_, it->second);
            return;
        }

        // Key does not exist, insert new item
        if (cache_items_list_.size() == max_size_) {
            // Cache is full, evict LRU item (last element in the list)
            std::pair<Key, Value> lru_item = cache_items_list_.back();
            if (on_evict_) {
                on_evict_(lru_item.first, lru_item.second);
            }
            cache_items_map_.erase(lru_item.first);
            cache_items_list_.pop_back();
        }

        // Insert new item at the front (MRU)
        cache_items_list_.emplace_front(key, value);
        cache_items_map_[key] = cache_items_list_.begin();
    }

    // Checks if the cache contains the given key.
    // This operation does NOT affect the LRU order.
    bool contains(const Key& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_items_map_.count(key) > 0;
    }

    // Removes the entry associated with the given key from the cache.
    // Returns true if an element was removed, false otherwise.
    bool erase(const Key& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_items_map_.find(key);
        if (it == cache_items_map_.end()) {
            return false; // Key not found
        }
        cache_items_list_.erase(it->second);
        cache_items_map_.erase(it);
        return true;
    }

    // Removes all entries from the cache.
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_items_map_.clear();
        cache_items_list_.clear();
        // Note: Eviction callback is not typically called on clear(),
        // as this is a deliberate removal of all items, not capacity-triggered eviction.
    }

    // Returns the current number of items in the cache.
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_items_map_.size();
    }

    // Returns true if the cache is empty, false otherwise.
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_items_map_.empty();
    }

private:
    size_t max_size_;
    std::list<std::pair<Key, Value>> cache_items_list_; // Stores items. Front is MRU, back is LRU.
    std::unordered_map<Key, typename std::list<std::pair<Key, Value>>::iterator> cache_items_map_; // Maps keys to list iterators.
    EvictCallback on_evict_;
    mutable std::mutex mutex_; // Mutex for thread safety.
};

#endif // LRU_CACHE_H

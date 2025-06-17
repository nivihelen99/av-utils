#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <list>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <optional>
#include <utility>
#include <stdexcept>
#include <shared_mutex>

template <typename Key, typename Value>
class LRUCache {
public:
    using EvictCallback = std::function<void(const Key&, const Value&)>;
    using KeyValuePair = std::pair<Key, Value>;
    using ListIterator = typename std::list<KeyValuePair>::iterator;

    explicit LRUCache(size_t max_size, EvictCallback on_evict = nullptr)
        : max_size_(max_size), on_evict_(std::move(on_evict)) {
        if (max_size == 0) {
            throw std::invalid_argument("LRUCache max_size must be greater than 0");
        }
        cache_items_map_.reserve(max_size);
    }

    // Move constructor and assignment
    LRUCache(LRUCache&& other) noexcept
        : max_size_(other.max_size_),
          cache_items_list_(std::move(other.cache_items_list_)),
          cache_items_map_(std::move(other.cache_items_map_)),
          on_evict_(std::move(other.on_evict_)) {}

    LRUCache& operator=(LRUCache&& other) noexcept {
        if (this != &other) {
            std::unique_lock<std::shared_mutex> lock1(mutex_, std::defer_lock);
            std::unique_lock<std::shared_mutex> lock2(other.mutex_, std::defer_lock);
            std::lock(lock1, lock2);

            max_size_ = other.max_size_;
            cache_items_list_ = std::move(other.cache_items_list_);
            cache_items_map_ = std::move(other.cache_items_map_);
            on_evict_ = std::move(other.on_evict_);
        }
        return *this;
    }

    // Disable copy constructor and assignment
    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;

    std::optional<Value> get(const Key& key) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        auto it = cache_items_map_.find(key);
        if (it == cache_items_map_.end()) {
            ++stats_.misses;
            return std::nullopt;
        }
        // Move to front (MRU position)
        move_to_front(it->second);
        ++stats_.hits;
        return it->second->second;
    }

    // Template version for perfect forwarding
    template<typename K, typename V>
    void put(K&& key, V&& value) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        auto it = cache_items_map_.find(key);

        if (it != cache_items_map_.end()) {
            // Update existing value and move to front
            it->second->second = std::forward<V>(value);
            move_to_front(it->second);
            return;
        }

        // Handle eviction if necessary
        if (cache_items_list_.size() >= max_size_) {
            evict_lru();
        }

        // Insert new item at front
        cache_items_list_.emplace_front(std::forward<K>(key), std::forward<V>(value));
        cache_items_map_[cache_items_list_.front().first] = cache_items_list_.begin();
    }

    // Original put method for backward compatibility
    void put(const Key& key, const Value& value) {
        put<const Key&, const Value&>(key, value);
    }

    bool contains(const Key& key) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return cache_items_map_.find(key) != cache_items_map_.end();
    }

    bool erase(const Key& key) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        auto it = cache_items_map_.find(key);
        if (it == cache_items_map_.end()) {
            return false;
        }
        cache_items_list_.erase(it->second);
        cache_items_map_.erase(it);
        return true;
    }

    void clear() {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        cache_items_map_.clear();
        cache_items_list_.clear();
    }

    size_t size() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return cache_items_map_.size();
    }

    bool empty() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return cache_items_map_.empty();
    }

    size_t max_size() const {
        return max_size_;
    }

    // Get cache statistics
    struct Stats {
        size_t hits = 0;
        size_t misses = 0;
        size_t evictions = 0;
        double hit_rate() const {
            return (hits + misses) > 0 ? static_cast<double>(hits) / (hits + misses) : 0.0;
        }
    };

    Stats get_stats() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return stats_;
    }

    void reset_stats() {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        stats_ = Stats{};
    }

private:
    size_t max_size_;
    std::list<KeyValuePair> cache_items_list_;
    std::unordered_map<Key, ListIterator> cache_items_map_;
    EvictCallback on_evict_;
    mutable std::shared_mutex mutex_;
    mutable Stats stats_;

    void move_to_front(ListIterator it) {
        if (it != cache_items_list_.begin()) {
            cache_items_list_.splice(cache_items_list_.begin(), cache_items_list_, it);
        }
    }

    void evict_lru() {
        if (!cache_items_list_.empty()) {
            const auto& lru_item = cache_items_list_.back();
            if (on_evict_) {
                on_evict_(lru_item.first, lru_item.second);
            }
            cache_items_map_.erase(lru_item.first);
            cache_items_list_.pop_back();
            ++stats_.evictions;
        }
    }
};

// Simple decorator-like function cache for single argument functions
template<typename Key, typename Value>
class CachedFunction {
private:
    std::function<Value(Key)> func_;
    LRUCache<Key, Value> cache_;

public:
    CachedFunction(std::function<Value(Key)> func, size_t max_size = 128)
        : func_(std::move(func)), cache_(max_size) {}

    Value operator()(const Key& key) {
        if (auto cached = cache_.get(key)) {
            return *cached;
        }
        Value result = func_(key);
        cache_.put(key, result);
        return result;
    }

    void clear_cache() { cache_.clear(); }
    size_t cache_size() const { return cache_.size(); }
    auto cache_stats() const { return cache_.get_stats(); }
};

// Helper function to create cached functions
template<typename Key, typename Value>
auto make_cached(std::function<Value(Key)> func, size_t max_size = 128) {
    return CachedFunction<Key, Value>(std::move(func), max_size);
}

// Macro for easier syntax (optional)
#define CACHED_FUNCTION(name, key_type, value_type, max_size, body) \
    auto name = make_cached<key_type, value_type>([](key_type arg) -> value_type body, max_size)

#endif // LRU_CACHE_H

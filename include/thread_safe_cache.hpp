#ifndef THREAD_SAFE_CACHE_HPP
#define THREAD_SAFE_CACHE_HPP

#include <iostream> // TODO: remove this include, only for basic printing during development
#include <list>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <unordered_map>

namespace cpp_collections {

enum class EvictionPolicy {
    LRU,
    LFU,
    FIFO
};

template <typename Key, typename Value>
class ThreadSafeCache {
public:
    ThreadSafeCache(size_t capacity, EvictionPolicy policy = EvictionPolicy::LRU);

    void put(const Key& key, const Value& value);
    std::optional<Value> get(const Key& key);
    bool erase(const Key& key);
    void clear();
    size_t size() const;
    bool empty() const;

private:
    // General members
    size_t capacity_;
    EvictionPolicy policy_;
    mutable std::mutex mutex_;

    // Data storage: map of keys to values
    std::unordered_map<Key, Value> cache_data_;

    // --- LRU specific members ---
    // List of keys, most recently used at front, least recently used at back
    std::list<Key> lru_order_;
    // Map of keys to their iterators in lru_order_ for O(1) access
    std::unordered_map<Key, typename std::list<Key>::iterator> lru_key_to_iter_;

    // --- FIFO specific members ---
    // Queue of keys, oldest at front, newest at back
    std::queue<Key> fifo_order_;

    // --- LFU specific members ---
    // Structure to hold frequency and a list of keys with that frequency
    struct LfuFrequencyNode {
        size_t frequency;
        std::list<Key> keys; // Keys with this frequency, in LRU order within this frequency

        // Iterators for the main frequency list to allow O(1) removal/update of frequency nodes
        typename std::list<LfuFrequencyNode>::iterator self_iter_in_freq_list;
    };

    // List of frequency nodes, sorted by frequency (ascending).
    // Enables finding the LFU node quickly.
    std::list<LfuFrequencyNode> lfu_frequency_list_;

    // Map from key to its current frequency node's iterator in lfu_frequency_list_
    // and its iterator within that node's key list.
    // This allows O(1) update of a key's frequency.
    std::unordered_map<Key,
        std::pair<
            typename std::list<LfuFrequencyNode>::iterator, // Iterator to the LfuFrequencyNode in lfu_frequency_list_
            typename std::list<Key>::iterator               // Iterator to the key in LfuFrequencyNode::keys
        >
    > lfu_key_to_node_and_iter_;


    // Private helper methods
    void evict();

    // LRU helpers
    void record_access_lru(const Key& key);
    void evict_lru();

    // FIFO helpers
    void record_insertion_fifo(const Key& key);
    void evict_fifo();

    // LFU helpers
    void record_access_lfu(const Key& key);
    void record_insertion_lfu(const Key& key); // For new items
    void evict_lfu();
    void increment_frequency_lfu(const Key& key);

};

// Constructor
template <typename Key, typename Value>
ThreadSafeCache<Key, Value>::ThreadSafeCache(size_t capacity, EvictionPolicy policy)
    : capacity_(capacity), policy_(policy) {
    if (capacity == 0) {
        throw std::invalid_argument("Cache capacity must be greater than 0.");
    }
}

// Public methods
template <typename Key, typename Value>
void ThreadSafeCache<Key, Value>::put(const Key& key, const Value& value) {
    std::lock_guard<std::mutex> lock(mutex_);

    // If key already exists, update its value and handle policy-specific access recording
    auto it = cache_data_.find(key);
    if (it != cache_data_.end()) {
        it->second = value;
        if (policy_ == EvictionPolicy::LRU) {
            record_access_lru(key);
        } else if (policy_ == EvictionPolicy::LFU) {
            // LFU: Accessing an existing item increments its frequency
            increment_frequency_lfu(key);
        }
        // FIFO: No special action on updating an existing item's value regarding its position
        return;
    }

    // Key does not exist, check for capacity
    if (cache_data_.size() >= capacity_) {
        evict(); // Evict an item based on policy
    }

    // Insert the new item
    cache_data_[key] = value;
    if (policy_ == EvictionPolicy::LRU) {
        lru_order_.push_front(key);
        lru_key_to_iter_[key] = lru_order_.begin();
    } else if (policy_ == EvictionPolicy::FIFO) {
        record_insertion_fifo(key);
    } else if (policy_ == EvictionPolicy::LFU) {
        record_insertion_lfu(key);
    }
}

template <typename Key, typename Value>
std::optional<Value> ThreadSafeCache<Key, Value>::get(const Key& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = cache_data_.find(key);
    if (it == cache_data_.end()) {
        return std::nullopt; // Key not found
    }

    // Key found, record access based on policy
    if (policy_ == EvictionPolicy::LRU) {
        record_access_lru(key);
    } else if (policy_ == EvictionPolicy::LFU) {
        increment_frequency_lfu(key);
    }
    // FIFO: No special action on get

    return it->second;
}

template <typename Key, typename Value>
bool ThreadSafeCache<Key, Value>::erase(const Key& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = cache_data_.find(key);
    if (it == cache_data_.end()) {
        return false; // Key not found
    }

    // Key found, remove it from main data and policy-specific structures
    cache_data_.erase(it);

    if (policy_ == EvictionPolicy::LRU) {
        auto lru_it = lru_key_to_iter_.find(key);
        if (lru_it != lru_key_to_iter_.end()) {
            lru_order_.erase(lru_it->second);
            lru_key_to_iter_.erase(lru_it);
        }
    } else if (policy_ == EvictionPolicy::FIFO) {
        // FIFO: Removal is tricky as std::queue doesn't support arbitrary element removal.
        // For simplicity in this version, we might need a temporary queue or accept that
        // 'erase' might be slow for FIFO or that items are only removed by eviction.
        // A common approach is to mark as "erased" and fully remove on get/evict.
        // For now, we'll rebuild the queue if an item is erased. This is not efficient.
        // TODO: Optimize FIFO erase if it's a critical path.
        std::queue<Key> temp_q;
        while(!fifo_order_.empty()){
            Key current_key = fifo_order_.front();
            fifo_order_.pop();
            if(current_key != key) {
                temp_q.push(current_key);
            }
        }
        fifo_order_ = temp_q;

    } else if (policy_ == EvictionPolicy::LFU) {
        auto lfu_map_iter = lfu_key_to_node_and_iter_.find(key);
        if (lfu_map_iter != lfu_key_to_node_and_iter_.end()) {
            auto& freq_node_iter = lfu_map_iter->second.first;
            auto& key_in_node_iter = lfu_map_iter->second.second;

            freq_node_iter->keys.erase(key_in_node_iter);
            if (freq_node_iter->keys.empty()) {
                lfu_frequency_list_.erase(freq_node_iter);
            }
            lfu_key_to_node_and_iter_.erase(lfu_map_iter);
        }
    }
    return true;
}

template <typename Key, typename Value>
void ThreadSafeCache<Key, Value>::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_data_.clear();

    if (policy_ == EvictionPolicy::LRU) {
        lru_order_.clear();
        lru_key_to_iter_.clear();
    } else if (policy_ == EvictionPolicy::FIFO) {
        std::queue<Key> empty_q;
        fifo_order_.swap(empty_q); // Efficient way to clear a queue
    } else if (policy_ == EvictionPolicy::LFU) {
        lfu_frequency_list_.clear();
        lfu_key_to_node_and_iter_.clear();
    }
}

template <typename Key, typename Value>
size_t ThreadSafeCache<Key, Value>::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return cache_data_.size();
}

template <typename Key, typename Value>
bool ThreadSafeCache<Key, Value>::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return cache_data_.empty();
}

// Private helper methods
template <typename Key, typename Value>
void ThreadSafeCache<Key, Value>::evict() {
    // Assumes lock is already held by the caller (e.g., put)
    if (cache_data_.empty() || cache_data_.size() < capacity_) { // Should not happen if called from put correctly
        return;
    }

    if (policy_ == EvictionPolicy::LRU) {
        evict_lru();
    } else if (policy_ == EvictionPolicy::FIFO) {
        evict_fifo();
    } else if (policy_ == EvictionPolicy::LFU) {
        evict_lfu();
    }
}

// --- LRU helpers ---
template <typename Key, typename Value>
void ThreadSafeCache<Key, Value>::record_access_lru(const Key& key) {
    // Assumes lock is held and key exists in cache_data_ and lru_key_to_iter_
    auto it = lru_key_to_iter_.find(key);
    if (it != lru_key_to_iter_.end()) {
        // Move the accessed key to the front of the list
        lru_order_.splice(lru_order_.begin(), lru_order_, it->second);
        // Update the iterator in the map (it->second is now lru_order_.begin())
        // it->second = lru_order_.begin(); // No, splice invalidates the old iterator value in map if it's not already begin()
                                         // The iterator itself (the one stored in the list node) remains valid.
                                         // The map's value (it->second) needs to point to the new position.
        lru_key_to_iter_[key] = lru_order_.begin(); // Correctly update the map's iterator value
    }
}

template <typename Key, typename Value>
void ThreadSafeCache<Key, Value>::evict_lru() {
    // Assumes lock is held
    if (lru_order_.empty()) {
        return; // Nothing to evict
    }

    // The least recently used item is at the back of the list
    Key key_to_evict = lru_order_.back();
    lru_order_.pop_back();
    lru_key_to_iter_.erase(key_to_evict);
    cache_data_.erase(key_to_evict);
}

// --- FIFO helpers ---
template <typename Key, typename Value>
void ThreadSafeCache<Key, Value>::record_insertion_fifo(const Key& key) {
    // Assumes lock is held
    fifo_order_.push(key);
}

template <typename Key, typename Value>
void ThreadSafeCache<Key, Value>::evict_fifo() {
    // Assumes lock is held
    if (fifo_order_.empty()) {
        return; // Nothing to evict
    }
    Key key_to_evict = fifo_order_.front();
    fifo_order_.pop();
    cache_data_.erase(key_to_evict);
}

// --- LFU helpers ---
template <typename Key, typename Value>
void ThreadSafeCache<Key, Value>::record_insertion_lfu(const Key& key) {
    // Assumes lock is held, key is new to the cache
    // New items start with frequency 1.
    // Find or create the frequency node for frequency 1.
    typename std::list<LfuFrequencyNode>::iterator freq_1_node_iter;
    if (lfu_frequency_list_.empty() || lfu_frequency_list_.front().frequency != 1) {
        // No frequency 1 node exists, or the first node has a higher frequency.
        // Insert a new node for frequency 1 at the beginning.
        LfuFrequencyNode new_node;
        new_node.frequency = 1;
        lfu_frequency_list_.push_front(new_node);
        freq_1_node_iter = lfu_frequency_list_.begin();
        freq_1_node_iter->self_iter_in_freq_list = freq_1_node_iter; // Store self-iterator
    } else {
        // Frequency 1 node already exists at the front.
        freq_1_node_iter = lfu_frequency_list_.begin();
    }

    // Add the key to this frequency node's list (at the front for LRU within frequency)
    freq_1_node_iter->keys.push_front(key);
    typename std::list<Key>::iterator key_iter_in_node = freq_1_node_iter->keys.begin();

    // Store mapping from key to its frequency node and position within that node's list
    lfu_key_to_node_and_iter_[key] = {freq_1_node_iter, key_iter_in_node};
}


template <typename Key, typename Value>
void ThreadSafeCache<Key, Value>::increment_frequency_lfu(const Key& key) {
    // Assumes lock is held and key exists in cache_data_ and lfu_key_to_node_and_iter_
    auto it_map = lfu_key_to_node_and_iter_.find(key);
    if (it_map == lfu_key_to_node_and_iter_.end()) {
        // Should not happen if called correctly (e.g., after a successful find in cache_data_)
        return;
    }

    auto& current_freq_node_iter = it_map->second.first;
    auto& key_iter_in_current_node = it_map->second.second;
    size_t old_frequency = current_freq_node_iter->frequency;
    size_t new_frequency = old_frequency + 1;

    // 1. Remove key from its current frequency node's list
    current_freq_node_iter->keys.erase(key_iter_in_current_node);

    // 2. Find or create the new frequency node
    typename std::list<LfuFrequencyNode>::iterator next_freq_node_iter = std::next(current_freq_node_iter);

    // Check if the current frequency node is now empty and needs removal
    bool current_node_was_removed = false;
    if (current_freq_node_iter->keys.empty()) {
        lfu_frequency_list_.erase(current_freq_node_iter);
        current_node_was_removed = true;
        // `next_freq_node_iter` is still valid if current_freq_node_iter was not the last element.
        // If current_freq_node_iter was last, next_freq_node_iter would be end().
        // If current_freq_node_iter was erased, next_freq_node_iter is the element that followed it, or end().
        // The `next_freq_node_iter` obtained before erase is what we need to inspect or insert before.
    }

    typename std::list<LfuFrequencyNode>::iterator target_freq_node_iter;

    // Search for the new frequency node starting from `next_freq_node_iter`
    // (or from beginning if old node was removed and was the first one)
    // `next_freq_node_iter` points to the node that *was* after the current_freq_node_iter,
    // or lfu_frequency_list_.end()

    // If current_node_was_removed is true, current_freq_node_iter is invalid.
    // next_freq_node_iter is the correct starting point for search/insertion.
    // If current_node_was_removed is false, current_freq_node_iter is valid,
    // and next_freq_node_iter = std::next(current_freq_node_iter) is the correct starting point.

    if (next_freq_node_iter == lfu_frequency_list_.end() || next_freq_node_iter->frequency != new_frequency) {
        // New frequency node does not exist immediately after, or we are at the end.
        // Insert a new node *before* next_freq_node_iter.
        LfuFrequencyNode new_node_val;
        new_node_val.frequency = new_frequency;
        // The insertion point is `next_freq_node_iter`. If current_node_was_removed was false,
        // current_freq_node_iter is still valid and next_freq_node_iter is std::next(current_freq_node_iter).
        // We want to insert after current_freq_node_iter (if it still exists) or at the correct sorted position.
        // The list is sorted by frequency. We need to insert the new_frequency node in its sorted place.
        // `next_freq_node_iter` is the iterator to the first element with frequency > old_frequency.
        // So, inserting before `next_freq_node_iter` is correct.

        target_freq_node_iter = lfu_frequency_list_.insert(next_freq_node_iter, new_node_val);
        target_freq_node_iter->self_iter_in_freq_list = target_freq_node_iter;
    } else {
        // Node for new_frequency already exists at next_freq_node_iter
        target_freq_node_iter = next_freq_node_iter;
    }

    // 3. Add key to the new/found frequency node's list (at the front for LRU within frequency)
    target_freq_node_iter->keys.push_front(key);
    typename std::list<Key>::iterator key_iter_in_new_node = target_freq_node_iter->keys.begin();

    // 4. Update the map for the key
    lfu_key_to_node_and_iter_[key] = {target_freq_node_iter, key_iter_in_new_node};
}


template <typename Key, typename Value>
void ThreadSafeCache<Key, Value>::evict_lfu() {
    // Assumes lock is held
    if (lfu_frequency_list_.empty()) {
        return; // Nothing to evict
    }

    // The LFU item is in the first frequency node (lowest frequency),
    // and at the back of that node's key list (LRU within that frequency).
    typename std::list<LfuFrequencyNode>::iterator lfu_node_iter = lfu_frequency_list_.begin();
    if (lfu_node_iter->keys.empty()) {
        // This should ideally not happen if logic is correct, means an empty frequency node exists.
        // Potentially remove it and try next, or signal error. For now, let's be robust.
        lfu_frequency_list_.erase(lfu_node_iter); // remove empty node
        if (lfu_frequency_list_.empty()) return; // if that was the only one
        lfu_node_iter = lfu_frequency_list_.begin(); // try the new first
        if (lfu_node_iter->keys.empty()) return; // if it's still empty, something is wrong or cache is empty
    }

    Key key_to_evict = lfu_node_iter->keys.back();

    // Remove from LFU structures
    lfu_node_iter->keys.pop_back();
    lfu_key_to_node_and_iter_.erase(key_to_evict);

    // If the frequency node becomes empty, remove it from the frequency list
    if (lfu_node_iter->keys.empty()) {
        lfu_frequency_list_.erase(lfu_node_iter);
    }

    // Remove from main cache data
    cache_data_.erase(key_to_evict);
}


} // namespace cpp_collections

#endif // THREAD_SAFE_CACHE_HPP

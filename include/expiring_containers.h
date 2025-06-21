#pragma once

#include <chrono>
#include <deque>
#include <unordered_map>
#include <utility>

namespace expiring {

/**
 * @brief A FIFO queue that automatically expires entries after a specified TTL
 * @tparam T The type of elements stored in the queue
 */
template<typename T>
class TimeStampedQueue {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = std::chrono::milliseconds;

private:
    struct TimedEntry {
        T value;
        TimePoint inserted;
        
        TimedEntry(T val, TimePoint time) : value(std::move(val)), inserted(time) {}
    };

    std::deque<TimedEntry> queue_;
    Duration ttl_;

public:
    /**
     * @brief Construct a new TimeStampedQueue with specified TTL
     * @param ttl Time-to-live for entries
     */
    explicit TimeStampedQueue(Duration ttl) : ttl_(ttl) {}

    /**
     * @brief Add an item to the queue with current timestamp
     * @param item The item to add
     */
    void push(const T& item) {
        queue_.emplace_back(item, Clock::now());
    }

    /**
     * @brief Add an item to the queue with current timestamp (move version)
     * @param item The item to add
     */
    void push(T&& item) {
        queue_.emplace_back(std::move(item), Clock::now());
    }

    /**
     * @brief Remove and return the front item (oldest)
     * @return The front item
     * @throws std::runtime_error if queue is empty
     */
    T pop() {
        expire(); // Clean up expired entries first
        if (queue_.empty()) {
            throw std::runtime_error("Queue is empty");
        }
        T result = std::move(queue_.front().value);
        queue_.pop_front();
        return result;
    }

    /**
     * @brief Access the front item without removing it
     * @return Reference to the front item
     * @throws std::runtime_error if queue is empty
     */
    const T& front() {
        expire(); // Clean up expired entries first
        if (queue_.empty()) {
            throw std::runtime_error("Queue is empty");
        }
        return queue_.front().value;
    }

    /**
     * @brief Remove all expired entries
     */
    void expire() {
        auto now = Clock::now();
        while (!queue_.empty() && (now - queue_.front().inserted) > ttl_) {
            queue_.pop_front();
        }
    }

    /**
     * @brief Get the number of live (non-expired) elements
     * @return Number of live elements
     */
    size_t size() {
        expire();
        return queue_.size();
    }

    /**
     * @brief Check if the queue is empty (after expiration cleanup)
     * @return true if empty, false otherwise
     */
    bool empty() {
        expire();
        return queue_.empty();
    }

    /**
     * @brief Remove all items immediately
     */
    void clear() {
        queue_.clear();
    }

    /**
     * @brief Change the TTL for future expiration checks
     * @param new_ttl New time-to-live duration
     */
    void set_ttl(Duration new_ttl) {
        ttl_ = new_ttl;
    }

    /**
     * @brief Get the current TTL
     * @return Current TTL duration
     */
    Duration get_ttl() const {
        return ttl_;
    }
};

/**
 * @brief A key-value map where entries auto-expire based on insertion time
 * @tparam K Key type (must be hashable and equality-comparable)
 * @tparam V Value type
 */
template<typename K, typename V>
class ExpiringDict {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = std::chrono::milliseconds;

private:
    struct TimedValue {
        V value;
        TimePoint timestamp;
        
        TimedValue(V val, TimePoint time) : value(std::move(val)), timestamp(time) {}
    };

    std::unordered_map<K, TimedValue> map_;
    Duration ttl_;
    bool access_renews_;

    /**
     * @brief Check if a timed value has expired
     * @param timed_val The timed value to check
     * @return true if expired, false otherwise
     */
    bool is_expired(const TimedValue& timed_val) const {
        return (Clock::now() - timed_val.timestamp) > ttl_;
    }

public:
    /**
     * @brief Construct a new ExpiringDict with specified TTL
     * @param ttl Time-to-live for entries
     * @param access_renews Whether accessing an entry renews its TTL
     */
    explicit ExpiringDict(Duration ttl, bool access_renews = false) 
        : ttl_(ttl), access_renews_(access_renews) {}

    /**
     * @brief Insert or overwrite a key-value pair with current timestamp
     * @param key The key
     * @param value The value
     */
    void insert(const K& key, const V& value) {
        map_.insert_or_assign(key, TimedValue(value, Clock::now()));
    }

    /**
     * @brief Insert or overwrite a key-value pair with current timestamp (move version)
     * @param key The key
     * @param value The value
     */
    void insert(const K& key, V&& value) {
        map_.insert_or_assign(key, TimedValue(std::move(value), Clock::now()));
    }

    /**
     * @brief Find a value by key, returns pointer if live, nullptr if expired/missing
     * @param key The key to search for
     * @return Pointer to value if found and live, nullptr otherwise
     */
    V* find(const K& key) {
        auto it = map_.find(key);
        if (it == map_.end()) {
            return nullptr;
        }
        
        if (is_expired(it->second)) {
            map_.erase(it);
            return nullptr;
        }
        
        // Optionally renew TTL on access
        if (access_renews_) {
            it->second.timestamp = Clock::now();
        }
        
        return &(it->second.value);
    }

    /**
     * @brief Find a value by key (const version)
     * @param key The key to search for
     * @return Pointer to value if found and live, nullptr otherwise
     */
    const V* find(const K& key) const {
        auto it = map_.find(key);
        if (it == map_.end()) {
            return nullptr;
        }
        
        if (is_expired(it->second)) {
            // Note: We can't erase in const method, but we return nullptr
            return nullptr;
        }
        
        return &(it->second.value);
    }

    /**
     * @brief Check if key exists and is not expired
     * @param key The key to check
     * @return true if key exists and is live, false otherwise
     */
    bool contains(const K& key) {
        auto it = map_.find(key);
        if (it == map_.end()) {
            return false;
        }
        
        if (is_expired(it->second)) {
            map_.erase(it);
            return false;
        }
        
        // Optionally renew TTL on access
        if (access_renews_) {
            it->second.timestamp = Clock::now();
        }
        
        return true;
    }

    /**
     * @brief Remove entry manually
     * @param key The key to remove
     * @return true if key was found and removed, false otherwise
     */
    bool erase(const K& key) {
        return map_.erase(key) > 0;
    }

    /**
     * @brief Remove all expired entries
     */
    void expire() {
        auto it = map_.begin();
        while (it != map_.end()) {
            if (is_expired(it->second)) {
                it = map_.erase(it);
            } else {
                ++it;
            }
        }
    }

    /**
     * @brief Get the number of valid (non-expired) entries
     * @return Number of valid entries
     */
    size_t size() {
        expire();
        return map_.size();
    }

    /**
     * @brief Check if the dictionary is empty (after expiration cleanup)
     * @return true if empty, false otherwise
     */
    bool empty() {
        expire();
        return map_.empty();
    }

    /**
     * @brief Clear the map
     */
    void clear() {
        map_.clear();
    }

    /**
     * @brief Update value and refresh timestamp
     * @param key The key to update
     * @param value The new value
     * @return true if key existed, false if it was a new insertion
     */
    bool update(const K& key, const V& value) {
        bool existed = map_.find(key) != map_.end();
        insert(key, value);
        return existed;
    }

    /**
     * @brief Update value and refresh timestamp (move version)
     * @param key The key to update  
     * @param value The new value
     * @return true if key existed, false if it was a new insertion
     */
    bool update(const K& key, V&& value) {
        bool existed = map_.find(key) != map_.end();
        insert(key, std::move(value));
        return existed;
    }

    /**
     * @brief Change the TTL for future expiration checks
     * @param new_ttl New time-to-live duration
     */
    void set_ttl(Duration new_ttl) {
        ttl_ = new_ttl;
    }

    /**
     * @brief Get the current TTL
     * @return Current TTL duration
     */
    Duration get_ttl() const {
        return ttl_;
    }

    /**
     * @brief Set whether accessing entries renews their TTL
     * @param renews Whether to renew TTL on access
     */
    void set_access_renews(bool renews) {
        access_renews_ = renews;
    }

    /**
     * @brief Check if access renews TTL
     * @return true if access renews TTL, false otherwise
     */
    bool get_access_renews() const {
        return access_renews_;
    }

    /**
     * @brief Visit all live entries with a function
     * @tparam Func Function type that accepts (const K&, const V&)
     * @param func The visitor function
     */
    template<typename Func>
    void for_each(Func&& func) {
        expire(); // Clean up first
        for (const auto& pair : map_) {
            func(pair.first, pair.second.value);
        }
    }
};

// Convenience type aliases
using namespace std::chrono_literals;

} // namespace expiring

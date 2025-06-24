#pragma once

#include <functional>
#include <deque>
#include <mutex>
#include <unordered_map>
#include <string>

namespace callqueue {

/**
 * @brief Single-threaded queue of callable functions with FIFO semantics
 * 
 * Provides a lightweight mechanism to queue std::function<void()> objects
 * for deferred execution. All callbacks are executed sequentially in the
 * order they were added when drain_all() is called.
 */
class CallQueue {
public:
    using Task = std::function<void()>;

private:
    std::deque<Task> queue_;
    std::unordered_map<std::string, size_t> coalesce_map_; // key -> queue index
    size_t max_size_ = 0; // 0 means unlimited

public:
    /**
     * @brief Construct a new CallQueue
     * @param max_size Maximum queue size (0 for unlimited)
     */
    explicit CallQueue(size_t max_size = 0) : max_size_(max_size) {}

    /**
     * @brief Add a callback to the queue (move semantics)
     * @param fn Function to execute later
     * @return true if added successfully, false if queue is full
     */
    bool push(Task&& fn) {
        if (max_size_ > 0 && queue_.size() >= max_size_) {
            return false; // Queue is full
        }
        queue_.emplace_back(std::move(fn));
        return true;
    }

    /**
     * @brief Add a callback to the queue (copy semantics)
     * @param fn Function to execute later
     * @return true if added successfully, false if queue is full
     */
    bool push(const Task& fn) {
        if (max_size_ > 0 && queue_.size() >= max_size_) {
            return false; // Queue is full
        }
        queue_.push_back(fn);
        return true;
    }

    /**
     * @brief Store only the most recent callable for a given key
     * @param key Coalescing key - only one function per key is kept
     * @param fn Function to execute later
     * @return true if added/updated successfully, false if queue is full
     */
    bool coalesce(const std::string& key, Task&& fn) {
        auto it = coalesce_map_.find(key);
        if (it != coalesce_map_.end()) {
            // Replace existing function at the same position
            queue_[it->second] = std::move(fn);
            return true;
        }
        
        if (max_size_ > 0 && queue_.size() >= max_size_) {
            return false; // Queue is full
        }
        
        // Add new function and track its position
        coalesce_map_[key] = queue_.size();
        queue_.emplace_back(std::move(fn));
        return true;
    }

    /**
     * @brief Execute and remove all queued functions in FIFO order
     * 
     * Functions added during drain_all() execution will not be executed
     * in the current cycle - they will be queued for the next drain_all().
     */
    void drain_all() {
        // Swap to avoid reentrancy issues
        std::deque<Task> local_queue;
        local_queue.swap(queue_);
        coalesce_map_.clear(); // Clear coalescing map since queue is drained
        
        // Execute all functions
        for (auto& task : local_queue) {
            if (task) {
                task();
            }
        }
    }

    /**
     * @brief Execute only one function from the queue
     * @return true if a function was executed, false if queue was empty
     */
    bool drain_one() {
        if (queue_.empty()) {
            return false;
        }
        
        Task task = std::move(queue_.front());
        queue_.pop_front();
        
        // Update coalesce map indices after removing front element
        for (auto& pair : coalesce_map_) {
            if (pair.second > 0) {
                --pair.second;
            }
        }
        
        if (task) {
            task();
        }
        return true;
    }

    /**
     * @brief Remove a previously queued function by key (only works for coalesced items)
     * @param key The coalescing key to cancel
     * @return true if a function was cancelled, false if key not found
     */
    bool cancel(const std::string& key) {
        auto it = coalesce_map_.find(key);
        if (it == coalesce_map_.end()) {
            return false;
        }
        
        size_t index = it->second;
        queue_.erase(queue_.begin() + index);
        coalesce_map_.erase(it);
        
        // Update remaining indices
        for (auto& pair : coalesce_map_) {
            if (pair.second > index) {
                --pair.second;
            }
        }
        
        return true;
    }

    /**
     * @brief Check if the queue is empty
     * @return true if no functions are queued
     */
    bool empty() const {
        return queue_.empty();
    }

    /**
     * @brief Get the number of queued functions
     * @return Number of functions in the queue
     */
    size_t size() const {
        return queue_.size();
    }

    /**
     * @brief Get the maximum queue size
     * @return Maximum size (0 means unlimited)
     */
    size_t max_size() const {
        return max_size_;
    }

    /**
     * @brief Clear all queued functions without executing them
     */
    void clear() {
        queue_.clear();
        coalesce_map_.clear();
    }
};

/**
 * @brief Thread-safe version of CallQueue using mutex synchronization
 * 
 * All operations are thread-safe and can be called from multiple threads
 * concurrently. Uses internal locking to synchronize access.
 */
class ThreadSafeCallQueue {
public:
    using Task = std::function<void()>;

private:
    mutable std::mutex mutex_;
    CallQueue queue_;

public:
    /**
     * @brief Construct a new ThreadSafeCallQueue
     * @param max_size Maximum queue size (0 for unlimited)
     */
    explicit ThreadSafeCallQueue(size_t max_size = 0) : queue_(max_size) {}

    /**
     * @brief Thread-safe push operation
     */
    bool push(Task&& fn) {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.push(std::move(fn));
    }

    /**
     * @brief Thread-safe push operation (copy)
     */
    bool push(const Task& fn) {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.push(fn);
    }

    /**
     * @brief Thread-safe coalesce operation
     */
    bool coalesce(const std::string& key, Task&& fn) {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.coalesce(key, std::move(fn));
    }

    /**
     * @brief Thread-safe drain all operations
     * 
     * Uses swap_and_drain pattern to minimize lock duration.
     * Creates a local queue under lock, then executes without holding the lock.
     */
    void drain_all() {
        CallQueue local_queue;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            // Swap the internal queue with empty local queue to minimize lock time
            std::swap(queue_, local_queue);
        }
        // Execute without holding the lock
        local_queue.drain_all();
    }

    /**
     * @brief Thread-safe drain one operation
     */
    bool drain_one() {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.drain_one();
    }

    /**
     * @brief Thread-safe cancel operation
     */
    bool cancel(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.cancel(key);
    }

    /**
     * @brief Thread-safe empty check
     */
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    /**
     * @brief Thread-safe size check
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    /**
     * @brief Get maximum queue size
     */
    size_t max_size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.max_size();
    }

    /**
     * @brief Thread-safe clear operation
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.clear();
    }
};

} // namespace callqueue

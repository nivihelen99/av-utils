#pragma once

#include <mutex>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <optional>
#include <chrono>

template <typename T>
class NamedLock {
private:
    struct LockEntry {
        std::timed_mutex mtx;  // Changed from std::mutex to support try_lock_for
        std::atomic<size_t> refcount{0};
        
        LockEntry() = default;
        
        // Non-copyable, non-movable to ensure stable mutex addresses
        LockEntry(const LockEntry&) = delete;
        LockEntry& operator=(const LockEntry&) = delete;
        LockEntry(LockEntry&&) = delete;
        LockEntry& operator=(LockEntry&&) = delete;
    };
    
    mutable std::mutex global_mutex_;
    std::unordered_map<T, std::shared_ptr<LockEntry>> lock_map_;

public:
    class Scoped {
    private:
        std::shared_ptr<LockEntry> entry_;
        std::unique_lock<std::timed_mutex> lock_;  // Changed to timed_mutex
        
        friend class NamedLock<T>;
        
        // Private constructor - only NamedLock can create Scoped instances
        Scoped(std::shared_ptr<LockEntry> entry, std::unique_lock<std::timed_mutex> lock)
            : entry_(std::move(entry)), lock_(std::move(lock)) {}
        
    public:
        // Default constructor for invalid/empty state
        Scoped() = default;
        
        // Move constructor
        Scoped(Scoped&& other) noexcept 
            : entry_(std::move(other.entry_)), lock_(std::move(other.lock_)) {}
        
        // Move assignment
        Scoped& operator=(Scoped&& other) noexcept {
            if (this != &other) {
                // Release current lock first
                reset();
                entry_ = std::move(other.entry_);
                lock_ = std::move(other.lock_);
            }
            return *this;
        }
        
        // Non-copyable
        Scoped(const Scoped&) = delete;
        Scoped& operator=(const Scoped&) = delete;
        
        // Destructor - releases the lock
        ~Scoped() {
            reset();
        }
        
        // Check if this scoped lock is valid/holding a lock
        bool owns_lock() const noexcept {
            return lock_.owns_lock();
        }
        
        // Explicit conversion to bool
        explicit operator bool() const noexcept {
            return owns_lock();
        }
        
        // Release the lock early
        void reset() {
            if (entry_ && lock_.owns_lock()) {
                lock_.unlock();
                entry_->refcount.fetch_sub(1, std::memory_order_acq_rel);
                entry_.reset();
            }
        }
    };
    
    // Timed scoped lock for timeout-based acquisition
    class TimedScoped {
    private:
        std::shared_ptr<LockEntry> entry_;
        std::unique_lock<std::timed_mutex> lock_;  // Changed to timed_mutex
        
        friend class NamedLock<T>;
        
        TimedScoped(std::shared_ptr<LockEntry> entry, std::unique_lock<std::timed_mutex> lock)
            : entry_(std::move(entry)), lock_(std::move(lock)) {}
        
    public:
        TimedScoped() = default;
        
        TimedScoped(TimedScoped&& other) noexcept 
            : entry_(std::move(other.entry_)), lock_(std::move(other.lock_)) {}
        
        TimedScoped& operator=(TimedScoped&& other) noexcept {
            if (this != &other) {
                reset();
                entry_ = std::move(other.entry_);
                lock_ = std::move(other.lock_);
            }
            return *this;
        }
        
        TimedScoped(const TimedScoped&) = delete;
        TimedScoped& operator=(const TimedScoped&) = delete;
        
        ~TimedScoped() {
            reset();
        }
        
        bool owns_lock() const noexcept {
            return lock_.owns_lock();
        }
        
        explicit operator bool() const noexcept {
            return owns_lock();
        }
        
        void reset() {
            if (entry_ && lock_.owns_lock()) {
                lock_.unlock();
                entry_->refcount.fetch_sub(1, std::memory_order_acq_rel);
                entry_.reset();
            }
        }
    };

public:
    NamedLock() = default;
    
    // Non-copyable, but movable
    NamedLock(const NamedLock&) = delete;
    NamedLock& operator=(const NamedLock&) = delete;
    
    NamedLock(NamedLock&&) = default;
    NamedLock& operator=(NamedLock&&) = default;
    
    // Acquire a scoped lock for a key (blocking)
    Scoped acquire(const T& key) {
        std::shared_ptr<LockEntry> entry;
        
        // Critical section: get or create the lock entry
        {
            std::lock_guard<std::mutex> global_lock(global_mutex_);
            auto it = lock_map_.find(key);
            if (it != lock_map_.end()) {
                entry = it->second;
            } else {
                entry = std::make_shared<LockEntry>();
                lock_map_[key] = entry;
            }
            // Increment refcount while holding global mutex to prevent races
            entry->refcount.fetch_add(1, std::memory_order_acq_rel);
        }
        
        // Acquire the per-key lock (outside global mutex to avoid contention)
        std::unique_lock<std::timed_mutex> lock(entry->mtx);
        
        return Scoped(std::move(entry), std::move(lock));
    }
    
    // Try to acquire a scoped lock for a key (non-blocking)
    std::optional<Scoped> try_acquire(const T& key) {
        std::shared_ptr<LockEntry> entry;
        
        // Critical section: get or create the lock entry
        {
            std::lock_guard<std::mutex> global_lock(global_mutex_);
            auto it = lock_map_.find(key);
            if (it != lock_map_.end()) {
                entry = it->second;
            } else {
                entry = std::make_shared<LockEntry>();
                lock_map_[key] = entry;
            }
            entry->refcount.fetch_add(1, std::memory_order_acq_rel);
        }
        
        // Try to acquire the per-key lock
        std::unique_lock<std::timed_mutex> lock(entry->mtx, std::try_to_lock);
        
        if (lock.owns_lock()) {
            return Scoped(std::move(entry), std::move(lock));
        } else {
            // Failed to acquire, decrement refcount
            entry->refcount.fetch_sub(1, std::memory_order_acq_rel);
            return std::nullopt;
        }
    }
    
    // Try to acquire with timeout
    template<typename Rep, typename Period>
    std::optional<TimedScoped> try_acquire_for(const T& key, 
                                               const std::chrono::duration<Rep, Period>& timeout) {
        std::shared_ptr<LockEntry> entry;
        
        {
            std::lock_guard<std::mutex> global_lock(global_mutex_);
            auto it = lock_map_.find(key);
            if (it != lock_map_.end()) {
                entry = it->second;
            } else {
                entry = std::make_shared<LockEntry>();
                lock_map_[key] = entry;
            }
            entry->refcount.fetch_add(1, std::memory_order_acq_rel);
        }
        
        std::unique_lock<std::timed_mutex> lock(entry->mtx, std::defer_lock);
        
        if (lock.try_lock_for(timeout)) {
            return TimedScoped(std::move(entry), std::move(lock));
        } else {
            entry->refcount.fetch_sub(1, std::memory_order_acq_rel);
            return std::nullopt;
        }
    }
    
    // Cleanup unused keys (removes entries with refcount == 0)
    void cleanup_unused() {
        std::lock_guard<std::mutex> global_lock(global_mutex_);
        
        auto it = lock_map_.begin();
        while (it != lock_map_.end()) {
            if (it->second->refcount.load(std::memory_order_acquire) == 0) {
                it = lock_map_.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    // Return number of keys currently being tracked
    size_t key_count() const {
        std::lock_guard<std::mutex> global_lock(global_mutex_);
        return lock_map_.size();
    }
    
    // Return number of currently active locks (sum of all refcounts)
    size_t active_lock_count() const {
        std::lock_guard<std::mutex> global_lock(global_mutex_);
        size_t total = 0;
        for (const auto& pair : lock_map_) {
            total += pair.second->refcount.load(std::memory_order_acquire);
        }
        return total;
    }
    
    // Get lock metrics for debugging/monitoring
    struct LockMetrics {
        size_t total_keys;
        size_t active_locks;
        size_t unused_keys;  // keys with refcount == 0
    };
    
    LockMetrics get_metrics() const {
        std::lock_guard<std::mutex> global_lock(global_mutex_);
        
        LockMetrics metrics{};
        metrics.total_keys = lock_map_.size();
        
        for (const auto& pair : lock_map_) {
            size_t refcount = pair.second->refcount.load(std::memory_order_acquire);
            metrics.active_locks += refcount;
            if (refcount == 0) {
                ++metrics.unused_keys;
            }
        }
        
        return metrics;
    }
    
    // Clear all locks (dangerous - only use when you're sure no locks are held)
    void clear() {
        std::lock_guard<std::mutex> global_lock(global_mutex_);
        lock_map_.clear();
    }
};

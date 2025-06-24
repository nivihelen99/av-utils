#pragma once

#include <atomic>
#include <memory>
#include <optional>
#include <functional>
#include <mutex>

namespace retain_latest {

// Forward declarations
template <typename T>
class RetainLatest;

template <typename T>
struct Versioned {
    T value;
    uint64_t version;
    
    Versioned(T val, uint64_t ver) : value(std::move(val)), version(ver) {}
};

template <typename T>
class VersionedRetainLatest;

/**
 * RetainLatest<T> - Thread-safe utility that retains only the most recent value
 * 
 * Features:
 * - Thread-safe implementation with minimal locking
 * - Always holds exactly one value (no unbounded growth)
 * - Thread-safe for concurrent producer/consumer access
 * - Clear-on-read semantics with consume()
 * - Non-blocking peek() for read-only access
 * 
 * Note: Uses a lightweight mutex for C++17 compatibility. In C++20, this could
 * use std::atomic<std::shared_ptr<T>> which has specialized atomic operations.
 */
template <typename T>
class RetainLatest {
public:
    using ValueType = T;
    using CallbackType = std::function<void(const T&)>;

    RetainLatest() = default;
    
    // Non-copyable but movable
    RetainLatest(const RetainLatest&) = delete;
    RetainLatest& operator=(const RetainLatest&) = delete;
    RetainLatest(RetainLatest&&) = default;
    RetainLatest& operator=(RetainLatest&&) = default;

    /**
     * Update with new value (copy version)
     * Atomically replaces the current value, dropping any previous one
     */
    void update(const T& value) {
        std::shared_ptr<T> new_value = std::make_shared<T>(value);
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            value_ptr_ = new_value;
        }
        
        // Notify callback if set (outside lock to avoid deadlock)
        if (callback_) {
            callback_(value);
        }
    }

    /**
     * Update with new value (move version)
     * Atomically replaces the current value, dropping any previous one
     */
    void update(T&& value) {
        std::shared_ptr<T> new_value = std::make_shared<T>(std::move(value));
        const T& value_ref = *new_value;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            value_ptr_ = new_value;
        }
        
        // Notify callback if set (outside lock to avoid deadlock)
        if (callback_) {
            callback_(value_ref);
        }
    }

    /**
     * Emplace construct value in-place
     */
    template<typename... Args>
    void emplace(Args&&... args) {
        std::shared_ptr<T> new_value = std::make_shared<T>(std::forward<Args>(args)...);
        const T& value_ref = *new_value;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            value_ptr_ = new_value;
        }
        
        // Notify callback if set (outside lock to avoid deadlock)
        if (callback_) {
            callback_(value_ref);
        }
    }

    /**
     * Atomically consume the latest value
     * Returns the value and clears the internal storage
     * Returns nullopt if no value is available
     */
    std::optional<T> consume() {
        std::shared_ptr<T> ptr;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            ptr = std::move(value_ptr_);
            value_ptr_.reset();
        }
        
        if (ptr) {
            return std::make_optional<T>(std::move(*ptr));
        }
        return std::nullopt;
    }

    /**
     * Peek at the current value without consuming it
     * Returns a copy of the current value if available
     */
    std::optional<T> peek() const {
        std::shared_ptr<T> ptr;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            ptr = value_ptr_;
        }
        
        if (ptr) {
            return std::make_optional<T>(*ptr);
        }
        return std::nullopt;
    }

    /**
     * Check if a value is currently available
     */
    bool has_value() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return value_ptr_ != nullptr;
    }

    /**
     * Set callback to be invoked when value is updated
     * Note: Callback is invoked synchronously during update()
     */
    void on_update(CallbackType callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        callback_ = std::move(callback);
    }

    /**
     * Clear any stored value
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        value_ptr_.reset();
    }

private:
    mutable std::mutex mutex_;
    std::shared_ptr<T> value_ptr_;
    CallbackType callback_;
};

/**
 * VersionedRetainLatest<T> - RetainLatest with automatic version tracking
 * 
 * Each update increments a monotonic version counter
 * Useful for deduplication and detecting stale data
 */
template <typename T>
class VersionedRetainLatest {
public:
    using ValueType = Versioned<T>;
    using CallbackType = std::function<void(const Versioned<T>&)>;

    VersionedRetainLatest() = default;
    
    // Non-copyable but movable
    VersionedRetainLatest(const VersionedRetainLatest&) = delete;
    VersionedRetainLatest& operator=(const VersionedRetainLatest&) = delete;
    VersionedRetainLatest(VersionedRetainLatest&&) = default;
    VersionedRetainLatest& operator=(VersionedRetainLatest&&) = default;

    /**
     * Update with new value (copy version)
     * Automatically increments version counter
     */
    void update(const T& value) {
        auto version = next_version_.fetch_add(1, std::memory_order_relaxed);
        auto versioned = std::make_shared<Versioned<T>>(value, version);
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            value_ptr_ = versioned;
        }
        
        if (callback_) {
            callback_(*versioned);
        }
    }

    /**
     * Update with new value (move version)
     * Automatically increments version counter
     */
    void update(T&& value) {
        auto version = next_version_.fetch_add(1, std::memory_order_relaxed);
        auto versioned = std::make_shared<Versioned<T>>(std::move(value), version);
        const Versioned<T>& versioned_ref = *versioned;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            value_ptr_ = versioned;
        }
        
        if (callback_) {
            callback_(versioned_ref);
        }
    }

    /**
     * Emplace construct value in-place with automatic versioning
     */
    template<typename... Args>
    void emplace(Args&&... args) {
        auto version = next_version_.fetch_add(1, std::memory_order_relaxed);
        auto versioned = std::make_shared<Versioned<T>>(T(std::forward<Args>(args)...), version);
        const Versioned<T>& versioned_ref = *versioned;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            value_ptr_ = versioned;
        }
        
        if (callback_) {
            callback_(versioned_ref);
        }
    }

    /**
     * Compare and update - only update if current version matches expected
     * Returns true if update succeeded, false if version mismatch
     */
    bool compare_and_update(const T& value, uint64_t expected_version) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!value_ptr_ || value_ptr_->version != expected_version) {
            return false;
        }
        
        auto version = next_version_.fetch_add(1, std::memory_order_relaxed);
        auto versioned = std::make_shared<Versioned<T>>(value, version);
        value_ptr_ = versioned;
        
        // Call callback outside the lock context would be better, but we need
        // to ensure the callback sees the value that was actually stored
        if (callback_) {
            callback_(*versioned);
        }
        
        return true;
    }

    /**
     * Atomically consume the latest versioned value
     */
    std::optional<Versioned<T>> consume() {
        std::shared_ptr<Versioned<T>> ptr;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            ptr = std::move(value_ptr_);
            value_ptr_.reset();
        }
        
        if (ptr) {
            return std::make_optional<Versioned<T>>(std::move(*ptr));
        }
        return std::nullopt;
    }

    /**
     * Peek at the current versioned value without consuming it
     */
    std::optional<Versioned<T>> peek() const {
        std::shared_ptr<Versioned<T>> ptr;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            ptr = value_ptr_;
        }
        
        if (ptr) {
            return std::make_optional<Versioned<T>>(*ptr);
        }
        return std::nullopt;
    }

    /**
     * Check if a value is currently available
     */
    bool has_value() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return value_ptr_ != nullptr;
    }

    /**
     * Check if consumer is behind (has older version than current)
     */
    bool is_stale(uint64_t consumer_version) const {
        std::shared_ptr<Versioned<T>> ptr;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            ptr = value_ptr_;
        }
        
        return ptr && ptr->version > consumer_version;
    }

    /**
     * Get current version without consuming value
     */
    std::optional<uint64_t> current_version() const {
        std::shared_ptr<Versioned<T>> ptr;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            ptr = value_ptr_;
        }
        
        return ptr ? std::make_optional(ptr->version) : std::nullopt;
    }

    /**
     * Set callback to be invoked when value is updated
     */
    void on_update(CallbackType callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        callback_ = std::move(callback);
    }

    /**
     * Clear any stored value
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        value_ptr_.reset();
    }

private:
    mutable std::mutex mutex_;
    std::shared_ptr<Versioned<T>> value_ptr_;
    std::atomic<uint64_t> next_version_{0};
    CallbackType callback_;
};

} // namespace retain_latest

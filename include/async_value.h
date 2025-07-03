#ifndef ASYNC_VALUE_H
#define ASYNC_VALUE_H

#include <functional>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <optional> // For T storage
#include <stdexcept> // For get() if not ready
#include <atomic>   // For ready flag
#include <utility>  // For std::move

template<typename T>
class AsyncValue {
public:
    AsyncValue() : m_ready(false) {}

    AsyncValue(const AsyncValue&) = delete;
    AsyncValue& operator=(const AsyncValue&) = delete;
    AsyncValue(AsyncValue&&) = delete; // For now, let's keep it simple. Move semantics can be tricky with mutexes.
    AsyncValue& operator=(AsyncValue&&) = delete;

    // Sets the value and notifies waiting callbacks.
    // This can only be called once.
    void set_value(T value) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_ready.load(std::memory_order_acquire)) {
            // Or throw an exception, or log. For now, assert for debug.
            // In a production system, might throw std::logic_error.
            assert(!"AsyncValue::set_value called more than once.");
            return;
        }

        m_value.emplace(std::move(value));
        m_ready.store(true, std::memory_order_release);

        // Notify all registered callbacks
        // Iterate over a copy of callbacks to allow callbacks to modify the list (e.g. re-register)
        // and to avoid holding the lock for too long if callbacks are slow.
        std::vector<std::function<void(const T&)>> callbacks_copy;
        callbacks_copy = m_callbacks; // Copy while holding the lock
        lock.unlock(); // Release lock before invoking callbacks

        for (const auto& callback : callbacks_copy) {
            if (m_value.has_value()) { // Double check, just in case reset was called concurrently (though not expected with current design)
                 callback(m_value.value());
            }
        }

        // Clear callbacks after they have been invoked, as they are one-shot
        std::unique_lock<std::mutex> clear_lock(m_mutex);
        m_callbacks.clear(); // Clear them after invocation.
        clear_lock.unlock();

        m_cv.notify_all(); // Notify any potential waiters on a condition variable (if we add blocking get)
    }

    // Returns true if the value has been set.
    bool ready() const {
        return m_ready.load(std::memory_order_acquire);
    }

    // Returns a pointer to the value if ready, otherwise nullptr.
    const T* get_if() const {
        std::unique_lock<std::mutex> lock(m_mutex_get); // Protects m_value access
        if (ready()) {
            return &m_value.value();
        }
        return nullptr;
    }

    T* get_if() {
        std::unique_lock<std::mutex> lock(m_mutex_get); // Protects m_value access
        if (ready()) {
            return &m_value.value();
        }
        return nullptr;
    }

    // Returns a const reference to the value.
    // Throws std::logic_error if the value is not ready.
    const T& get() const {
        std::unique_lock<std::mutex> lock(m_mutex_get); // Protects m_value access
        if (!ready()) {
            throw std::logic_error("AsyncValue: Value not ready.");
        }
        return m_value.value();
    }

    // Returns a reference to the value.
    // Throws std::logic_error if the value is not ready.
    T& get() {
        std::unique_lock<std::mutex> lock(m_mutex_get); // Protects m_value access
        if (!ready()) {
            throw std::logic_error("AsyncValue: Value not ready.");
        }
        return m_value.value();
    }

    // Registers a callback to be invoked when the value is set.
    // If the value is already set, the callback is invoked immediately.
    void on_ready(std::function<void(const T&)> callback) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_ready.load(std::memory_order_acquire)) {
            // Value is already set, invoke callback immediately
            // Unlock before calling to prevent potential deadlocks if callback tries to access AsyncValue
            T value_copy = m_value.value(); // Copy value to avoid issues if 'this' is destroyed during callback
            lock.unlock();
            callback(value_copy);
        } else {
            // Value not set yet, store callback
            m_callbacks.push_back(std::move(callback));
        }
    }

    // Resets the AsyncValue to its initial state.
    // Clears the value and any registered callbacks.
    void reset() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_value.reset();
        m_ready.store(false, std::memory_order_release);
        m_callbacks.clear();
        // m_cv is fine, it's stateless until waited on
    }

private:
    std::optional<T> m_value;
    std::atomic<bool> m_ready;
    std::vector<std::function<void(const T&)>> m_callbacks;
    mutable std::mutex m_mutex; // Mutex for m_value (on set), m_callbacks, and m_ready flag (during set_value critical section)
    mutable std::mutex m_mutex_get; // Separate mutex for get operations to allow concurrent reads if value is ready
    std::condition_variable m_cv; // For potential future blocking operations
};

// Specialization for void
template<>
class AsyncValue<void> {
public:
    AsyncValue() : m_ready(false) {}

    AsyncValue(const AsyncValue&) = delete;
    AsyncValue& operator=(const AsyncValue&) = delete;
    AsyncValue(AsyncValue&&) = delete;
    AsyncValue& operator=(AsyncValue&&) = delete;

    // Sets the event and notifies waiting callbacks.
    // This can only be called once.
    void set() {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_ready.load(std::memory_order_acquire)) {
            assert(!"AsyncValue<void>::set called more than once.");
            return;
        }

        m_ready.store(true, std::memory_order_release);

        std::vector<std::function<void()>> callbacks_copy;
        callbacks_copy = m_callbacks;
        lock.unlock();

        for (const auto& callback : callbacks_copy) {
            callback();
        }

        std::unique_lock<std::mutex> clear_lock(m_mutex);
        m_callbacks.clear();
        clear_lock.unlock();

        m_cv.notify_all();
    }

    // Returns true if the event has been set.
    bool ready() const {
        return m_ready.load(std::memory_order_acquire);
    }

    // Registers a callback to be invoked when the event is set.
    // If the event is already set, the callback is invoked immediately.
    void on_ready(std::function<void()> callback) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_ready.load(std::memory_order_acquire)) {
            lock.unlock();
            callback();
        } else {
            m_callbacks.push_back(std::move(callback));
        }
    }

    // Resets the AsyncValue<void> to its initial state.
    void reset() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_ready.store(false, std::memory_order_release);
        m_callbacks.clear();
    }

    // get() for void specialization: can be used to check readiness and perhaps assert/throw.
    // It doesn't return a value. Could be used as a synchronization point if it blocked,
    // but per requirements, we are non-blocking by default.
    // For consistency, we can make it assert if not ready.
    void get() const {
        if (!ready()) {
            // In a non-debug build, this might throw or be a no-op if called when not ready.
            // For now, an assert similar to the T version's throw.
            // Or simply rely on ready() and not provide get() for void.
            // The requirement "get() Returns the value if available; throws/asserts otherwise"
            // implies it should exist. Since there's no value, it's more of a "check_ready_or_assert".
            assert(!"AsyncValue<void>::get() called before event was set.");
             // throw std::logic_error("AsyncValue<void>: Event not set."); // Alternative
        }
        // If ready, it's a no-op.
    }


private:
    std::atomic<bool> m_ready;
    std::vector<std::function<void()>> m_callbacks;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
};

#endif // ASYNC_VALUE_H

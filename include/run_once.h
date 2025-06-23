#pragma once

#include <mutex>
#include <atomic>
#include <utility>

/**
 * RunOnce - A thread-safe utility to ensure a callable is executed exactly once
 * 
 * Provides a cleaner, more ergonomic alternative to std::once_flag + std::call_once
 * for ensuring initialization code or expensive setup runs only once, regardless
 * of how many times it's invoked across multiple threads.
 * 
 * Features:
 * - Thread-safe execution with proper synchronization
 * - Lambda-friendly interface accepting any callable
 * - Exception-aware: only marks as "run" if callable completes successfully
 * - Introspection via has_run() method
 * - Header-only C++17 implementation
 * 
 * Example usage:
 * 
 *   static RunOnce init_once;
 *   init_once([] {
 *       initialize_logging();
 *       setup_database_connection();
 *   });
 */
class RunOnce {
private:
    mutable std::once_flag flag_;
    mutable std::atomic<bool> has_run_{false};

public:
    /**
     * Default constructor - creates a fresh RunOnce instance
     */
    RunOnce() = default;
    
    /**
     * Non-copyable and non-movable due to std::once_flag constraints
     * Use pointers or references if you need to store RunOnce in containers
     */
    RunOnce(const RunOnce&) = delete;
    RunOnce& operator=(const RunOnce&) = delete;
    RunOnce(RunOnce&&) = delete;
    RunOnce& operator=(RunOnce&&) = delete;

    /**
     * Execute the provided callable exactly once
     * 
     * If multiple threads call this simultaneously, only one will execute
     * the callable while others wait. If the callable throws an exception,
     * it is not considered "run" and future calls will retry.
     * 
     * @param fn Callable object (lambda, function, functor) to execute once
     */
    template <typename Callable>
    void operator()(Callable&& fn) {
        std::call_once(flag_, [this, &fn]() {
            // Execute the callable - if it throws, has_run_ stays false
            std::forward<Callable>(fn)();
            // Only mark as run if we get here (no exception)
            has_run_.store(true, std::memory_order_release);
        });
    }

    /**
     * Check if the callable has been successfully executed
     * 
     * @return true if the callable has completed successfully, false otherwise
     */
    bool has_run() const noexcept {
        return has_run_.load(std::memory_order_acquire);
    }

    /**
     * Reset the RunOnce state for testing or special control flows
     * 
     * WARNING: This is NOT thread-safe and should only be used when
     * you can guarantee no other threads are accessing this RunOnce instance.
     * Primarily intended for unit testing scenarios.
     * 
     * Since std::once_flag cannot be reset, this uses placement new to
     * reconstruct the object in place.
     */
    void reset() noexcept {
        // Destroy current state and reconstruct in place
        this->~RunOnce();
        new (this) RunOnce();
    }
};

/**
 * RunOnceReturn<T> - Extended version that captures and returns the result
 * 
 * Similar to RunOnce but stores the return value of the callable for
 * retrieval on subsequent calls. Useful for expensive computations that
 * should only be done once but whose result is needed multiple times.
 * 
 * Example:
 * 
 *   RunOnceReturn<std::string> cached_config;
 *   auto config = cached_config([] { return load_expensive_config(); });
 */
template <typename T>
class RunOnceReturn {
private:
    mutable std::once_flag flag_;
    mutable std::atomic<bool> has_run_{false};
    mutable T result_;

public:
    RunOnceReturn() = default;
    
    // Non-copyable, non-movable due to std::once_flag constraints
    RunOnceReturn(const RunOnceReturn&) = delete;
    RunOnceReturn& operator=(const RunOnceReturn&) = delete;
    RunOnceReturn(RunOnceReturn&&) = delete;
    RunOnceReturn& operator=(RunOnceReturn&&) = delete;

    /**
     * Execute the callable once and return its result
     * 
     * On first call, executes the callable and stores the result.
     * On subsequent calls, returns the stored result immediately.
     * 
     * @param fn Callable that returns type T
     * @return The result of calling fn() (cached after first execution)
     */
    template <typename Callable>
    const T& operator()(Callable&& fn) {
        std::call_once(flag_, [this, &fn]() {
            result_ = std::forward<Callable>(fn)();
            has_run_.store(true, std::memory_order_release);
        });
        return result_;
    }

    /**
     * Check if the callable has been executed and result is available
     */
    bool has_run() const noexcept {
        return has_run_.load(std::memory_order_acquire);
    }

    /**
     * Get the cached result (only valid if has_run() returns true)
     * 
     * @return Reference to the cached result
     * @throws Undefined behavior if called before the callable has been executed
     */
    const T& get() const noexcept {
        return result_;
    }

    /**
     * Reset for testing (same caveats as RunOnce::reset())
     */
    void reset() noexcept {
        this->~RunOnceReturn();
        new (this) RunOnceReturn<T>();
    }
};

// Convenience type aliases for common patterns
using OnceFlag = RunOnce;  // Alternative name for familiarity with std::once_flag

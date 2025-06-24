#pragma once

#include <utility>
#include <type_traits>
#include <exception>
#include <functional>
#include <variant>

namespace raii {

// Forward declarations
template<typename Resource, typename Func, typename Cleanup>
class ScopedContext;

template<typename Resource, typename Func, typename Cleanup>
class ReturningContext;

namespace detail {
    // Helper to detect if a type has a callable destructor
    template<typename T>
    struct has_destructor {
        static constexpr bool value = std::is_destructible_v<T>;
    };

    // Default cleanup that does nothing (relies on RAII destructor)
    struct NoOpCleanup {
        template<typename T>
        void operator()(T&) const noexcept {}
    };

    // Helper to determine if we should use custom cleanup or RAII
    template<typename Cleanup>
    constexpr bool is_noop_cleanup_v = std::is_same_v<std::decay_t<Cleanup>, NoOpCleanup>;
}

/**
 * RAII Context Manager - Non-returning version
 * Manages resource lifetime and ensures cleanup on scope exit
 */
template<typename Resource, typename Func, typename Cleanup>
class ScopedContext {
public:
    ScopedContext(Resource&& resource, Func&& func, Cleanup&& cleanup)
        : resource_(std::forward<Resource>(resource))
        , cleanup_(std::forward<Cleanup>(cleanup))
        , cleanup_called_(false) {
        
        // Execute the main function with the resource
        try {
            func(resource_);
        } catch (...) {
            // Ensure cleanup runs even if func throws
            try_cleanup();
            throw;
        }
    }

    ~ScopedContext() noexcept {
        try_cleanup();
    }

    // Non-copyable, non-movable to ensure single cleanup
    ScopedContext(const ScopedContext&) = delete;
    ScopedContext& operator=(const ScopedContext&) = delete;
    ScopedContext(ScopedContext&&) = delete;
    ScopedContext& operator=(ScopedContext&&) = delete;

private:
    void try_cleanup() noexcept {
        if (!cleanup_called_) {
            cleanup_called_ = true;
            try {
                if constexpr (!detail::is_noop_cleanup_v<Cleanup>) {
                    cleanup_(resource_);
                }
                // For NoOpCleanup, rely on RAII destructor of resource_
            } catch (...) {
                // Cleanup should not throw in destructor context
                // In a real implementation, you might want to log this
            }
        }
    }

    Resource resource_;
    Cleanup cleanup_;
    bool cleanup_called_;
};

/**
 * RAII Context Manager - Returning version
 * Like ScopedContext but captures and returns the result of func
 */
template<typename Resource, typename Func, typename Cleanup>
class ReturningContext {
public:
    using ReturnType = std::invoke_result_t<Func, Resource&>;

    ReturningContext(Resource&& resource, Func&& func, Cleanup&& cleanup)
        : resource_(std::forward<Resource>(resource))
        , cleanup_(std::forward<Cleanup>(cleanup))
        , cleanup_called_(false) {
        
        // Execute function and capture result
        try {
            if constexpr (std::is_void_v<ReturnType>) {
                func(resource_);
            } else {
                result_ = func(resource_);
            }
        } catch (...) {
            try_cleanup();
            throw;
        }
    }

    ~ReturningContext() noexcept {
        try_cleanup();
    }

    // Get the result (only for non-void return types)
    template<typename T = ReturnType>
    std::enable_if_t<!std::is_void_v<T>, T> get_result() && {
        return std::move(result_);
    }

    // Non-copyable, non-movable
    ReturningContext(const ReturningContext&) = delete;
    ReturningContext& operator=(const ReturningContext&) = delete;
    ReturningContext(ReturningContext&&) = delete;
    ReturningContext& operator=(ReturningContext&&) = delete;

private:
    void try_cleanup() noexcept {
        if (!cleanup_called_) {
            cleanup_called_ = true;
            try {
                if constexpr (!detail::is_noop_cleanup_v<Cleanup>) {
                    cleanup_(resource_);
                }
            } catch (...) {
                // Swallow cleanup exceptions in destructor
            }
        }
    }

    Resource resource_;
    Cleanup cleanup_;
    bool cleanup_called_;
    
    // Only store result for non-void return types
    [[no_unique_address]] std::conditional_t<
        std::is_void_v<ReturnType>, 
        std::monostate, 
        ReturnType
    > result_;
};

/**
 * Primary interface - with_resource (void version)
 * Uses RAII destructor for cleanup
 */
template<typename Resource, typename Func>
void with_resource(Resource&& resource, Func&& func) {
    ScopedContext<Resource, Func, detail::NoOpCleanup> context(
        std::forward<Resource>(resource),
        std::forward<Func>(func),
        detail::NoOpCleanup{}
    );
}

/**
 * with_resource with custom cleanup
 */
template<typename Resource, typename Func, typename Cleanup>
void with_resource(Resource&& resource, Func&& func, Cleanup&& cleanup) {
    ScopedContext<Resource, Func, Cleanup> context(
        std::forward<Resource>(resource),
        std::forward<Func>(func),
        std::forward<Cleanup>(cleanup)
    );
}

/**
 * with_resource_returning - captures and returns the result of func
 */
template<typename Resource, typename Func>
auto with_resource_returning(Resource&& resource, Func&& func) {
    using ReturnType = std::invoke_result_t<Func, Resource&>;
    
    if constexpr (std::is_void_v<ReturnType>) {
        with_resource(std::forward<Resource>(resource), std::forward<Func>(func));
    } else {
        ReturningContext<Resource, Func, detail::NoOpCleanup> context(
            std::forward<Resource>(resource),
            std::forward<Func>(func),
            detail::NoOpCleanup{}
        );
        return std::move(context).get_result();
    }
}

/**
 * with_resource_returning with custom cleanup
 */
template<typename Resource, typename Func, typename Cleanup>
auto with_resource_returning(Resource&& resource, Func&& func, Cleanup&& cleanup) {
    using ReturnType = std::invoke_result_t<Func, Resource&>;
    
    if constexpr (std::is_void_v<ReturnType>) {
        with_resource(std::forward<Resource>(resource), std::forward<Func>(func), std::forward<Cleanup>(cleanup));
    } else {
        ReturningContext<Resource, Func, Cleanup> context(
            std::forward<Resource>(resource),
            std::forward<Func>(func),
            std::forward<Cleanup>(cleanup)
        );
        return std::move(context).get_result();
    }
}

// Convenience macro for cleaner syntax (optional)
#define WITH_RESOURCE(resource, var) \
    raii::with_resource(resource, [&](auto& var)

#define WITH_RESOURCE_CLEANUP(resource, var, cleanup) \
    raii::with_resource(resource, [&](auto& var), cleanup

} // namespace raii

/*
Example usage:

#include <fstream>
#include <iostream>
#include <mutex>
#include <chrono>

void example_usage() {
    // File handling
    raii::with_resource(std::ofstream("test.txt"), [](auto& file) {
        file << "Hello, World!" << std::endl;
    });

    // Mutex locking
    std::mutex mtx;
    raii::with_resource(std::lock_guard<std::mutex>(mtx), [](auto&) {
        std::cout << "In critical section" << std::endl;
    });

    // Custom cleanup - Timer example
    auto start = std::chrono::steady_clock::now();
    raii::with_resource(42, [](auto& value) {
        std::cout << "Processing value: " << value << std::endl;
        // Simulate work
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }, [start](auto&) {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Operation took: " << duration.count() << "ms" << std::endl;
    });

    // With return value
    int result = raii::with_resource_returning(std::vector<int>{1, 2, 3, 4, 5}, [](auto& vec) {
        return std::accumulate(vec.begin(), vec.end(), 0);
    });
    std::cout << "Sum: " << result << std::endl;

    // Using the macro for cleaner syntax
    WITH_RESOURCE(std::ofstream("macro_test.txt"), file) {
        file << "Using macro syntax" << std::endl;
    });
}
*/

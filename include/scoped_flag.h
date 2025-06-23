#pragma once

#include <atomic>
#include <type_traits>

namespace utils {

/**
 * @brief RAII-based scoped flag modifier for temporary flag changes
 * 
 * Inspired by Python's context managers, this utility allows temporary
 * modification of bool-like flags with automatic restoration on scope exit.
 * Exception-safe and supports bool, std::atomic_bool, and thread_local flags.
 * 
 * @example
 * extern bool g_logging_enabled;
 * {
 *     ScopedFlag guard(g_logging_enabled, false);  // disable temporarily
 *     run_silently();
 * }  // re-enabled here automatically
 */
class ScopedFlag {
private:
    bool* bool_ptr = nullptr;
    std::atomic_bool* atomic_ptr = nullptr;
    bool old_value;

public:
    /**
     * @brief Construct a scoped flag guard for a regular bool
     * @param flag Reference to the flag to modify
     * @param new_value Value to set temporarily
     */
    explicit ScopedFlag(bool& flag, bool new_value) 
        : bool_ptr(&flag), old_value(flag) {
        flag = new_value;
    }

    /**
     * @brief Construct a scoped flag guard for an atomic bool
     * @param flag Reference to the atomic flag to modify
     * @param new_value Value to set temporarily
     */
    explicit ScopedFlag(std::atomic_bool& flag, bool new_value) 
        : atomic_ptr(&flag), old_value(flag.load()) {
        flag.store(new_value);
    }

    /**
     * @brief Destructor - automatically restores the original flag value
     */
    ~ScopedFlag() {
        if (bool_ptr) {
            *bool_ptr = old_value;
        }
        if (atomic_ptr) {
            atomic_ptr->store(old_value);
        }
    }

    /**
     * @brief Get the original value that was saved
     * @return The flag's value before modification
     */
    bool previous() const noexcept {
        return old_value;
    }

    // Non-copyable and non-movable to prevent accidental misuse
    ScopedFlag(const ScopedFlag&) = delete;
    ScopedFlag& operator=(const ScopedFlag&) = delete;
    ScopedFlag(ScopedFlag&&) = delete;
    ScopedFlag& operator=(ScopedFlag&&) = delete;
};

/**
 * @brief Generic RAII flag guard for any assignable type
 * 
 * Template version that works with any type that supports assignment
 * and copy construction. Useful for non-bool flags like integers,
 * enums, or other scalar types.
 * 
 * @tparam T Type of the flag (must be assignable and copy-constructible)
 * 
 * @example
 * int g_verbosity_level = 1;
 * {
 *     FlagGuard<int> guard(g_verbosity_level, 0);  // silence temporarily
 *     run_quietly();
 * }  // restored here
 */
template <typename T>
class FlagGuard {
    static_assert(std::is_assignable_v<T&, T>, "Type T must be assignable");
    static_assert(std::is_copy_constructible_v<T>, "Type T must be copy constructible");

private:
    T* flag_ptr;
    T old_value;

public:
    /**
     * @brief Construct a generic flag guard
     * @param flag Reference to the flag to modify
     * @param new_value Value to set temporarily
     */
    explicit FlagGuard(T& flag, T new_value) 
        : flag_ptr(&flag), old_value(flag) {
        flag = std::move(new_value);
    }

    /**
     * @brief Destructor - automatically restores the original flag value
     */
    ~FlagGuard() {
        *flag_ptr = std::move(old_value);
    }

    /**
     * @brief Get the original value that was saved
     * @return The flag's value before modification
     */
    const T& previous() const noexcept {
        return old_value;
    }

    /**
     * @brief Conditionally set the flag only if it's not already the target value
     * @param flag Reference to the flag
     * @param new_value Value to set if different from current
     * @return FlagGuard that may or may not have changed the flag
     */
    static FlagGuard<T> set_if_not(T& flag, T new_value) {
        return FlagGuard<T>(flag, flag != new_value ? std::move(new_value) : flag);
    }

    // Non-copyable and non-movable
    FlagGuard(const FlagGuard&) = delete;
    FlagGuard& operator=(const FlagGuard&) = delete;
    FlagGuard(FlagGuard&&) = delete;
    FlagGuard& operator=(FlagGuard&&) = delete;
};

// Convenience type aliases
using BoolGuard = FlagGuard<bool>;
using IntGuard = FlagGuard<int>;

/**
 * @brief Convenience function to create a ScopedFlag that temporarily disables a flag
 * @param flag Flag to disable
 * @return ScopedFlag that sets the flag to false
 */
inline ScopedFlag temporarily_disable(bool& flag) {
    return ScopedFlag(flag, false);
}

/**
 * @brief Convenience function to create a ScopedFlag that temporarily disables an atomic flag
 * @param flag Atomic flag to disable
 * @return ScopedFlag that sets the flag to false
 */
inline ScopedFlag temporarily_disable(std::atomic_bool& flag) {
    return ScopedFlag(flag, false);
}

/**
 * @brief Convenience function to create a ScopedFlag that temporarily enables a flag
 * @param flag Flag to enable
 * @return ScopedFlag that sets the flag to true
 */
inline ScopedFlag temporarily_enable(bool& flag) {
    return ScopedFlag(flag, true);
}

/**
 * @brief Convenience function to create a ScopedFlag that temporarily enables an atomic flag
 * @param flag Atomic flag to enable
 * @return ScopedFlag that sets the flag to true
 */
inline ScopedFlag temporarily_enable(std::atomic_bool& flag) {
    return ScopedFlag(flag, true);
}

} // namespace utils

// Example usage and test cases
#ifdef SCOPED_FLAG_INCLUDE_EXAMPLES

#include <iostream>
#include <thread>

namespace utils::examples {

// Global flags for demonstration
bool g_logging_enabled = true;
std::atomic_bool g_debug_enabled{true};
thread_local bool g_in_progress = false;
int g_verbosity_level = 2;

void demonstrate_basic_usage() {
    std::cout << "=== Basic ScopedFlag Usage ===\n";
    
    std::cout << "Before: logging=" << g_logging_enabled 
              << ", debug=" << g_debug_enabled << "\n";
    
    {
        ScopedFlag log_guard(g_logging_enabled, false);
        ScopedFlag debug_guard(g_debug_enabled, false);
        
        std::cout << "Inside: logging=" << g_logging_enabled 
                  << ", debug=" << g_debug_enabled << "\n";
        std::cout << "Previous logging value was: " << log_guard.previous() << "\n";
    }
    
    std::cout << "After: logging=" << g_logging_enabled 
              << ", debug=" << g_debug_enabled << "\n\n";
}

void demonstrate_generic_guard() {
    std::cout << "=== Generic FlagGuard Usage ===\n";
    
    std::cout << "Before: verbosity=" << g_verbosity_level << "\n";
    
    {
        FlagGuard<int> guard(g_verbosity_level, 0);
        std::cout << "Inside: verbosity=" << g_verbosity_level << "\n";
        std::cout << "Previous verbosity was: " << guard.previous() << "\n";
    }
    
    std::cout << "After: verbosity=" << g_verbosity_level << "\n\n";
}

void demonstrate_convenience_functions() {
    std::cout << "=== Convenience Functions ===\n";
    
    std::cout << "Before: logging=" << g_logging_enabled << "\n";
    
    {
        auto guard = temporarily_disable(g_logging_enabled);
        std::cout << "Inside: logging=" << g_logging_enabled << "\n";
    }
    
    std::cout << "After: logging=" << g_logging_enabled << "\n\n";
}

void demonstrate_exception_safety() {
    std::cout << "=== Exception Safety ===\n";
    
    std::cout << "Before: logging=" << g_logging_enabled << "\n";
    
    try {
        ScopedFlag guard(g_logging_enabled, false);
        std::cout << "Inside try: logging=" << g_logging_enabled << "\n";
        throw std::runtime_error("Test exception");
    } catch (const std::exception& e) {
        std::cout << "Caught: " << e.what() << "\n";
    }
    
    std::cout << "After exception: logging=" << g_logging_enabled << "\n\n";
}

void demonstrate_recursion_protection() {
    std::cout << "=== Recursion Protection ===\n";
    
    std::function<void(int)> recursive_func = [&](int depth) {
        if (g_in_progress) {
            std::cout << "Recursion detected at depth " << depth << ", skipping\n";
            return;
        }
        
        ScopedFlag guard(g_in_progress, true);
        std::cout << "Processing at depth " << depth << "\n";
        
        if (depth < 3) {
            recursive_func(depth + 1);  // This will be blocked
        }
    };
    
    recursive_func(0);
    std::cout << "\n";
}

void run_all_examples() {
    demonstrate_basic_usage();
    demonstrate_generic_guard();
    demonstrate_convenience_functions();
    demonstrate_exception_safety();
    demonstrate_recursion_protection();
}

} // namespace utils::examples

#endif // SCOPED_FLAG_INCLUDE_EXAMPLES

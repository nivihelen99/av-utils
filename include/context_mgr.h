#pragma once

#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

namespace context {

/**
 * @brief A generic context manager that executes enter/exit functions on construction/destruction
 * 
 * Similar to Python's contextlib.contextmanager, this provides RAII-based scoped execution
 * of setup and teardown code with minimal syntax.
 * 
 * @tparam FEnter Type of the enter function
 * @tparam FExit Type of the exit function
 */
template<typename FEnter, typename FExit>
class ContextManager {
public:
    /**
     * @brief Construct a context manager with enter and exit functions
     * 
     * @param enter Function to execute on construction
     * @param exit Function to execute on destruction (unless cancelled)
     */
    ContextManager(FEnter enter, FExit exit) 
        : exit_func_(std::move(exit)) {
        // Execute enter function immediately
        std::forward<FEnter>(enter)();
    }

    /**
     * @brief Destructor - executes exit function if not cancelled
     */
    ~ContextManager() noexcept(std::is_nothrow_destructible_v<FExit>) {
        if (exit_func_.has_value()) {
            try {
                (*exit_func_)();
            } catch (...) {
                // Destructors should not throw - swallow exceptions
                // In debug builds, you might want to assert or log here
            }
        }
    }

    // Move-only semantics
    ContextManager(const ContextManager&) = delete;
    ContextManager& operator=(const ContextManager&) = delete;
    
    ContextManager(ContextManager&& other) noexcept 
        : exit_func_(std::move(other.exit_func_)) {
        other.exit_func_.reset();
    }
    
    ContextManager& operator=(ContextManager&& other) noexcept {
        if (this != &other) {
            // Execute current exit function if present
            if (exit_func_.has_value()) {
                try {
                    (*exit_func_)();
                } catch (...) {
                    // Swallow exceptions in assignment
                }
            }
            exit_func_ = std::move(other.exit_func_);
            other.exit_func_.reset();
        }
        return *this;
    }

    /**
     * @brief Cancel the exit function - it will not be executed on destruction
     */
    void cancel() noexcept {
        exit_func_.reset();
    }

    /**
     * @brief Check if the exit function is still active (not cancelled)
     */
    bool is_active() const noexcept {
        return exit_func_.has_value();
    }

private:
    std::optional<FExit> exit_func_;
};

/**
 * @brief A scope exit guard that only executes cleanup on destruction
 * 
 * Similar to Go's defer statement or Rust's Drop trait.
 * 
 * @tparam F Type of the exit function
 */
template<typename F>
class ScopeExit {
public:
    /**
     * @brief Construct a scope exit guard with a cleanup function
     * 
     * @param exit_func Function to execute on destruction (unless dismissed)
     */
    explicit ScopeExit(F exit_func) 
        : exit_func_(std::move(exit_func)) {}

    /**
     * @brief Destructor - executes exit function if not dismissed
     */
    ~ScopeExit() noexcept(std::is_nothrow_destructible_v<F>) {
        if (exit_func_.has_value()) {
            try {
                (*exit_func_)();
            } catch (...) {
                // Destructors should not throw - swallow exceptions
            }
        }
    }

    // Move-only semantics
    ScopeExit(const ScopeExit&) = delete;
    ScopeExit& operator=(const ScopeExit&) = delete;
    
    ScopeExit(ScopeExit&& other) noexcept 
        : exit_func_(std::move(other.exit_func_)) {
        other.exit_func_.reset();
    }
    
    ScopeExit& operator=(ScopeExit&& other) noexcept {
        if (this != &other) {
            // Execute current exit function if present
            if (exit_func_.has_value()) {
                try {
                    (*exit_func_)();
                } catch (...) {
                    // Swallow exceptions in assignment
                }
            }
            exit_func_ = std::move(other.exit_func_);
            other.exit_func_.reset();
        }
        return *this;
    }

    /**
     * @brief Dismiss the exit function - it will not be executed on destruction
     */
    void dismiss() noexcept {
        exit_func_.reset();
    }

    /**
     * @brief Check if the exit function is still active (not dismissed)
     */
    bool is_active() const noexcept {
        return exit_func_.has_value();
    }

private:
    std::optional<F> exit_func_;
};

/**
 * @brief Helper function to create a ContextManager with type deduction
 * 
 * @param enter Function to execute on construction
 * @param exit Function to execute on destruction
 * @return ContextManager with deduced types
 */
template<typename FEnter, typename FExit>
auto make_context(FEnter&& enter, FExit&& exit) {
    return ContextManager<std::decay_t<FEnter>, std::decay_t<FExit>>(
        std::forward<FEnter>(enter), 
        std::forward<FExit>(exit)
    );
}

/**
 * @brief Helper function to create a ScopeExit with type deduction
 * 
 * @param exit_func Function to execute on destruction
 * @return ScopeExit with deduced type
 */
template<typename F>
auto make_scope_exit(F&& exit_func) {
    return ScopeExit<std::decay_t<F>>(std::forward<F>(exit_func));
}

/**
 * @brief Convenience macro for creating scope exit guards
 * 
 * Usage: SCOPE_EXIT { cleanup_code(); };
 */
#define SCOPE_EXIT \
    auto CONTEXT_UNIQUE_NAME(__scope_exit_) = ::context::make_scope_exit([&]()

#define CONTEXT_UNIQUE_NAME(base) CONTEXT_CONCAT(base, __LINE__)
#define CONTEXT_CONCAT(a, b) CONTEXT_CONCAT_IMPL(a, b)
#define CONTEXT_CONCAT_IMPL(a, b) a##b

/**
 * @brief Named scope context manager for logging and debugging
 */
class NamedScope {
public:
    explicit NamedScope(const std::string& name) : name_(name) {
        std::cout << "[ENTER] " << name_ << std::endl;
    }
    
    ~NamedScope() {
        std::cout << "[EXIT]  " << name_ << std::endl;
    }
    
    // Move-only
    NamedScope(const NamedScope&) = delete;
    NamedScope& operator=(const NamedScope&) = delete;
    NamedScope(NamedScope&&) = default;
    NamedScope& operator=(NamedScope&&) = default;

private:
    std::string name_;
};

/**
 * @brief Thread-local context manager for configuration overrides
 */
template<typename T>
class ThreadLocalOverride {
public:
    ThreadLocalOverride(T& variable, T new_value) 
        : variable_(variable), old_value_(variable) {
        variable_ = std::move(new_value);
    }
    
    ~ThreadLocalOverride() {
        variable_ = std::move(old_value_);
    }
    
    // Move-only
    ThreadLocalOverride(const ThreadLocalOverride&) = delete;
    ThreadLocalOverride& operator=(const ThreadLocalOverride&) = delete;
    ThreadLocalOverride(ThreadLocalOverride&&) = default;
    ThreadLocalOverride& operator=(ThreadLocalOverride&&) = default;

private:
    T& variable_;
    T old_value_;
};

/**
 * @brief Helper function to create thread-local overrides
 */
template<typename T>
auto make_override(T& variable, T new_value) {
    return ThreadLocalOverride<T>(variable, std::move(new_value));
}

} // namespace context

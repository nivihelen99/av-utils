#pragma once

#include <type_traits>
#include <stdexcept>
#include <utility>
#include <cassert>

/**
 * @brief Policy enum for DelayedInit behavior
 */
enum class DelayedInitPolicy {
    OnceOnly,    // Can only call init() once (default)
    Mutable,     // init() can overwrite existing value
    Nullable     // Behaves like optional - reset() allowed
};

/**
 * @brief Exception thrown when accessing uninitialized DelayedInit or 
 *        attempting to initialize already-initialized OnceOnly DelayedInit
 */
class DelayedInitError : public std::logic_error {
public:
    explicit DelayedInitError(const std::string& msg) : std::logic_error(msg) {}
};

/**
 * @brief DelayedInit<T> - Type-safe deferred initialization utility
 * 
 * Provides Python-like "declare first, assign later" semantics for C++.
 * Access before initialization throws DelayedInitError.
 * 
 * @tparam T The type to be lazily initialized
 * @tparam Policy Initialization policy (OnceOnly, Mutable, or Nullable)
 */
template<typename T, DelayedInitPolicy Policy = DelayedInitPolicy::OnceOnly>
class DelayedInit {
private:
    static_assert(std::is_destructible_v<T>, "T must be destructible");
    static_assert(std::is_copy_constructible_v<T> || std::is_move_constructible_v<T>, 
                  "T must be copy or move constructible");

    alignas(T) char storage_[sizeof(T)];
    bool initialized_ = false;

    T* ptr() noexcept { return reinterpret_cast<T*>(storage_); }
    const T* ptr() const noexcept { return reinterpret_cast<const T*>(storage_); }

    void destroy() noexcept {
        if (initialized_) {
            ptr()->~T();
            initialized_ = false;
        }
    }

public:
    /**
     * @brief Default constructor - creates uninitialized DelayedInit
     */
    DelayedInit() = default;

    /**
     * @brief Destructor - destroys contained value if initialized
     */
    ~DelayedInit() {
        destroy();
    }

    /**
     * @brief Copy constructor
     */
    DelayedInit(const DelayedInit& other) {
        if (other.initialized_) {
            init(*other.ptr());
        }
    }

    /**
     * @brief Move constructor
     */
    DelayedInit(DelayedInit&& other) noexcept(std::is_nothrow_move_constructible_v<T>) {
        if (other.initialized_) {
            init(std::move(*other.ptr()));
            if constexpr (Policy != DelayedInitPolicy::Nullable) {
                // For OnceOnly and Mutable, clear the source
                other.destroy();
            }
        }
    }

    /**
     * @brief Copy assignment operator
     */
    DelayedInit& operator=(const DelayedInit& other) {
        if (this != &other) {
            if (other.initialized_) {
                init(*other.ptr());
            } else if constexpr (Policy == DelayedInitPolicy::Nullable) {
                reset();
            }
        }
        return *this;
    }

    /**
     * @brief Move assignment operator
     */
    DelayedInit& operator=(DelayedInit&& other) noexcept(std::is_nothrow_move_constructible_v<T>) {
        if (this != &other) {
            if (other.initialized_) {
                init(std::move(*other.ptr()));
                if constexpr (Policy != DelayedInitPolicy::Nullable) {
                    other.destroy();
                }
            } else if constexpr (Policy == DelayedInitPolicy::Nullable) {
                reset();
            }
        }
        return *this;
    }

    /**
     * @brief Initialize with copy of value
     * @param value Value to copy-construct
     * @throws DelayedInitError if already initialized and Policy is OnceOnly
     */
    void init(const T& value) {
        if (initialized_) {
            if constexpr (Policy == DelayedInitPolicy::OnceOnly) {
                throw DelayedInitError("DelayedInit already initialized (OnceOnly policy)");
            } else {
                // Mutable or Nullable - destroy and reconstruct
                destroy();
            }
        }
        new (ptr()) T(value);
        initialized_ = true;
    }

    /**
     * @brief Initialize with moved value
     * @param value Value to move-construct
     * @throws DelayedInitError if already initialized and Policy is OnceOnly
     */
    void init(T&& value) {
        if (initialized_) {
            if constexpr (Policy == DelayedInitPolicy::OnceOnly) {
                throw DelayedInitError("DelayedInit already initialized (OnceOnly policy)");
            } else {
                // Mutable or Nullable - destroy and reconstruct
                destroy();
            }
        }
        new (ptr()) T(std::move(value));
        initialized_ = true;
    }

    /**
     * @brief Emplace-construct the value in place
     * @param args Arguments to forward to T's constructor
     * @throws DelayedInitError if already initialized and Policy is OnceOnly
     */
    template<typename... Args>
    void emplace(Args&&... args) {
        if (initialized_) {
            if constexpr (Policy == DelayedInitPolicy::OnceOnly) {
                throw DelayedInitError("DelayedInit already initialized (OnceOnly policy)");
            } else {
                destroy();
            }
        }
        new (ptr()) T(std::forward<Args>(args)...);
        initialized_ = true;
    }

    /**
     * @brief Get reference to contained value
     * @return Reference to contained value
     * @throws DelayedInitError if not initialized
     */
    T& get() {
        if (!initialized_) {
            throw DelayedInitError("DelayedInit not initialized - cannot access value");
        }
        return *ptr();
    }

    /**
     * @brief Get const reference to contained value
     * @return Const reference to contained value
     * @throws DelayedInitError if not initialized
     */
    const T& get() const {
        if (!initialized_) {
            throw DelayedInitError("DelayedInit not initialized - cannot access value");
        }
        return *ptr();
    }

    /**
     * @brief Dereference operator - get reference to contained value
     * @return Reference to contained value
     * @throws DelayedInitError if not initialized
     */
    T& operator*() {
        return get();
    }

    /**
     * @brief Const dereference operator
     * @return Const reference to contained value
     * @throws DelayedInitError if not initialized
     */
    const T& operator*() const {
        return get();
    }

    /**
     * @brief Arrow operator - get pointer to contained value
     * @return Pointer to contained value
     * @throws DelayedInitError if not initialized
     */
    T* operator->() {
        return &get();
    }

    /**
     * @brief Const arrow operator
     * @return Const pointer to contained value
     * @throws DelayedInitError if not initialized
     */
    const T* operator->() const {
        return &get();
    }

    /**
     * @brief Check if value has been initialized
     * @return true if initialized, false otherwise
     */
    bool is_initialized() const noexcept {
        return initialized_;
    }

    /**
     * @brief Explicit bool conversion - same as is_initialized()
     * @return true if initialized, false otherwise
     */
    explicit operator bool() const noexcept {
        return initialized_;
    }

    /**
     * @brief Reset/clear the contained value
     * Only available for Mutable and Nullable policies
     */
    void reset() noexcept {
        static_assert(Policy != DelayedInitPolicy::OnceOnly, 
                     "reset() not available for OnceOnly policy");
        destroy();
    }

    /**
     * @brief Get value or return default if not initialized
     * Only available for Nullable policy
     * @param default_value Default value to return if not initialized
     * @return Contained value or default_value
     */
    template<typename U = T>
    T value_or(U&& default_value) const {
        static_assert(Policy == DelayedInitPolicy::Nullable,
                     "value_or() only available for Nullable policy");
        return initialized_ ? *ptr() : static_cast<T>(std::forward<U>(default_value));
    }
};

// Convenience type aliases
template<typename T>
using DelayedInitOnce = DelayedInit<T, DelayedInitPolicy::OnceOnly>;

template<typename T>
using DelayedInitMutable = DelayedInit<T, DelayedInitPolicy::Mutable>;

template<typename T>
using DelayedInitNullable = DelayedInit<T, DelayedInitPolicy::Nullable>;

/**
 * @brief Example usage and demonstration
 */
namespace delayed_init_examples {
    
    struct Config {
        std::string name;
        int value;
        Config(const std::string& n, int v) : name(n), value(v) {}
    };

    void basic_usage_example() {
        // Basic usage - declare first, initialize later
        DelayedInit<std::string> name;
        assert(!name.is_initialized());
        
        name.init("Hello, World!");
        assert(name.is_initialized());
        
        std::cout << *name << std::endl;  // prints: Hello, World!
        
        // This would throw DelayedInitError:
        // name.init("Again");  // OnceOnly policy
    }

    void mutable_policy_example() {
        DelayedInitMutable<int> counter;
        
        counter.init(1);
        assert(*counter == 1);
        
        counter.init(2);  // OK with Mutable policy
        assert(*counter == 2);
    }

    void nullable_policy_example() {
        DelayedInitNullable<std::string> optional_name;
        
        optional_name.init("test");
        assert(optional_name.is_initialized());
        
        optional_name.reset();  // OK with Nullable policy
        assert(!optional_name.is_initialized());
        
        std::string result = optional_name.value_or("default");
        assert(result == "default");
    }

    void two_phase_init_example() {
        DelayedInit<Config> cfg;
        
        // Later in code...
        cfg.emplace("MyApp", 42);
        
        // Use the config
        std::cout << "Config: " << cfg->name << " = " << cfg->value << std::endl;
    }

    class Session {
        DelayedInit<Config> context_;
    public:
        void set_context(Config ctx) {
            context_.init(std::move(ctx));
        }
        
        void handle_request() {
            if (!context_.is_initialized()) {
                throw std::runtime_error("Session context not set");
            }
            
            // Safe to use context
            std::cout << "Handling request for: " << context_->name << std::endl;
        }
    };
}

// Example main function demonstrating usage
#ifdef DELAYED_INIT_EXAMPLE_MAIN
#include <iostream>

int main() {
    using namespace delayed_init_examples;
    
    try {
        basic_usage_example();
        mutable_policy_example();
        nullable_policy_example();
        two_phase_init_example();
        
        Session session;
        session.set_context(Config("TestSession", 100));
        session.handle_request();
        
        std::cout << "All examples completed successfully!" << std::endl;
        
    } catch (const DelayedInitError& e) {
        std::cerr << "DelayedInit error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

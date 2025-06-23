#pragma once

#include <functional>
#include <iostream>
#include <unordered_map>
#include <tuple>
#include <exception>
#include <type_traits>
#include <sstream>
#include <mutex>
#include <utility>
#include <any>
#include <variant>
#include <typeinfo>

namespace function_decoration {

// Helper traits for detecting void return types
template<typename T>
struct is_void_return : std::is_void<T> {};

// Hash function for tuple (needed for cache key)
template<typename... Args>
struct tuple_hash {
    std::size_t operator()(const std::tuple<Args...>& t) const {
        return hash_impl(t, std::index_sequence_for<Args...>{});
    }
    
private:
    template<std::size_t... I>
    std::size_t hash_impl(const std::tuple<Args...>& t, std::index_sequence<I...>) const {
        std::size_t seed = 0;
        (..., hash_combine(seed, std::get<I>(t)));
        return seed;
    }
    
    template<typename T>
    void hash_combine(std::size_t& seed, const T& v) const {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
};

// Helper to format arguments for logging
template<typename... Args>
std::string format_args(const Args&... args) {
    std::ostringstream oss;
    oss << "(";
    bool first = true;
    (..., (oss << (first ? (first = false, "") : ", ") << args));
    oss << ")";
    return oss.str();
}

// Helper to get function name (simplified - in real code you might use __PRETTY_FUNCTION__)
template<typename F>
std::string get_function_name() {
    return "function";
}

template<typename F>
class CallableWrapper {
private:
    F func_;
    bool enable_logging_ = false;
    bool enable_caching_ = false;
    size_t retry_count_ = 0;
    
    // Type-erased cache storage
    mutable std::unordered_map<std::string, std::any> type_erased_caches_;
    mutable std::mutex cache_mutex_;
    
    // Helper type aliases
    template<typename... Args>
    using cache_key_t = std::tuple<std::decay_t<Args>...>;
    
    // Use std::monostate as a placeholder for void return types
    template<typename RetType>
    using cache_value_t = std::conditional_t<std::is_void_v<RetType>, std::monostate, RetType>;
    
    template<typename RetType, typename... Args>
    using cache_map_t = std::unordered_map<
        cache_key_t<Args...>, 
        cache_value_t<RetType>,
        tuple_hash<std::decay_t<Args>...>
    >;

public:
    explicit CallableWrapper(F func) : func_(std::move(func)) {}
    
    // Make the class movable but not copyable due to mutex
    CallableWrapper(const CallableWrapper&) = delete;
    CallableWrapper& operator=(const CallableWrapper&) = delete;
    
    CallableWrapper(CallableWrapper&& other) noexcept 
        : func_(std::move(other.func_))
        , enable_logging_(other.enable_logging_)
        , enable_caching_(other.enable_caching_)
        , retry_count_(other.retry_count_)
        , type_erased_caches_(std::move(other.type_erased_caches_))
    {
        // Note: mutex is not moved, it's reconstructed
    }
    
    CallableWrapper& operator=(CallableWrapper&& other) noexcept {
        if (this != &other) {
            func_ = std::move(other.func_);
            enable_logging_ = other.enable_logging_;
            enable_caching_ = other.enable_caching_;
            retry_count_ = other.retry_count_;
            type_erased_caches_ = std::move(other.type_erased_caches_);
            // Note: mutex is not moved
        }
        return *this;
    }
    
    // Decorator methods - return *this for chaining (moved object)
    CallableWrapper&& log() && {
        enable_logging_ = true;
        return std::move(*this);
    }
    
    CallableWrapper&& retry(size_t times) && {
        retry_count_ = times;
        return std::move(*this);
    }
    
    CallableWrapper&& cache() && {
        enable_caching_ = true;
        return std::move(*this);
    }
    
    // Lvalue overloads for when wrapper is stored in a variable
    CallableWrapper& log() & {
        enable_logging_ = true;
        return *this;
    }
    
    CallableWrapper& retry(size_t times) & {
        retry_count_ = times;
        return *this;
    }
    
    CallableWrapper& cache() & {
        enable_caching_ = true;
        return *this;
    }
    
    // Main call operator
    template<typename... Args>
    decltype(auto) operator()(Args&&... args) {
        using ReturnType = std::invoke_result_t<F, Args...>;
        
        // Generate cache key type signature for type-erased storage
        std::string cache_key_type = std::string(typeid(cache_key_t<Args...>).name()) + 
                                   std::string(typeid(cache_value_t<ReturnType>).name());
        
        // Check cache first if enabled
        if (enable_caching_) {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            
            // Get or create the typed cache
            if (type_erased_caches_.find(cache_key_type) == type_erased_caches_.end()) {
                type_erased_caches_[cache_key_type] = cache_map_t<ReturnType, Args...>{};
            }
            
            auto& typed_cache = std::any_cast<cache_map_t<ReturnType, Args...>&>(
                type_erased_caches_[cache_key_type]);
            auto cache_key = std::make_tuple(std::decay_t<Args>(args)...);
            
            auto it = typed_cache.find(cache_key);
            if (it != typed_cache.end()) {
                if (enable_logging_) {
                    std::cout << "Cache hit for " << get_function_name<F>() 
                              << format_args(args...) << std::endl;
                }
                if constexpr (std::is_void_v<ReturnType>) {
                    return; // For void functions, just return without value
                } else {
                    return it->second;
                }
            }
        }
        
        // Execute with retry logic
        size_t attempts = std::max(retry_count_, size_t(1));
        std::exception_ptr last_exception;
        
        for (size_t attempt = 0; attempt < attempts; ++attempt) {
            try {
                if (enable_logging_ && attempt > 0) {
                    std::cout << "Retrying " << get_function_name<F>() 
                              << format_args(args...) << " (attempt " << (attempt + 1) << ")" << std::endl;
                }
                
                if (enable_logging_ && attempt == 0) {
                    std::cout << "Calling " << get_function_name<F>() 
                              << format_args(args...) << std::endl;
                }
                
                // Execute the function
                if constexpr (std::is_void_v<ReturnType>) {
                    func_(std::forward<Args>(args)...);
                    
                    if (enable_logging_) {
                        std::cout << get_function_name<F>() << " completed" << std::endl;
                    }
                    
                    // For void functions, store std::monostate to indicate success
                    if (enable_caching_) {
                        std::lock_guard<std::mutex> lock(cache_mutex_);
                        auto& typed_cache = std::any_cast<cache_map_t<ReturnType, Args...>&>(
                            type_erased_caches_[cache_key_type]);
                        auto cache_key = std::make_tuple(std::decay_t<Args>(args)...);
                        typed_cache[cache_key] = std::monostate{};
                    }
                    return;
                } else {
                    auto result = func_(std::forward<Args>(args)...);
                    
                    if (enable_logging_) {
                        std::cout << get_function_name<F>() << " returned " << result << std::endl;
                    }
                    
                    // Store in cache if enabled
                    if (enable_caching_) {
                        std::lock_guard<std::mutex> lock(cache_mutex_);
                        auto& typed_cache = std::any_cast<cache_map_t<ReturnType, Args...>&>(
                            type_erased_caches_[cache_key_type]);
                        auto cache_key = std::make_tuple(std::decay_t<Args>(args)...);
                        typed_cache[cache_key] = result;
                    }
                    
                    return result;
                }
            } catch (const std::exception& e) {
                last_exception = std::current_exception();
                if (enable_logging_) {
                    std::cout << "Exception in " << get_function_name<F>() 
                              << ": " << e.what() << std::endl;
                }
                
                if (attempt == attempts - 1) {
                    // This was the last attempt, re-throw
                    break;
                }
            }
        }
        
        // If we get here, all retries failed
        if (last_exception) {
            std::rethrow_exception(last_exception);
        }
        
        // This should never be reached, but needed for compilation
        if constexpr (!std::is_void_v<ReturnType>) {
            return ReturnType{};
        }
    }
};

// Factory function
template<typename F>
auto wrap(F&& func) -> CallableWrapper<std::decay_t<F>> {
    return CallableWrapper<std::decay_t<F>>(std::forward<F>(func));
}

} // namespace function_decoration

// Example usage and tests
#ifdef CALLABLE_WRAPPER_EXAMPLE

#include <iostream>
#include <chrono>
#include <thread>

using namespace function_decoration;

// Example functions to wrap
int add(int a, int b) {
    std::cout << "  [Computing " << a << " + " << b << "]" << std::endl;
    return a + b;
}

int flaky_function(int x) {
    static int call_count = 0;
    call_count++;
    
    if (call_count < 3) {
        throw std::runtime_error("Simulated failure");
    }
    
    return x * 2;
}

void void_function(const std::string& msg) {
    std::cout << "  [Processing: " << msg << "]" << std::endl;
}

int main() {
    std::cout << "=== CallableWrapper Demo ===" << std::endl;
    
    // Basic logging
    std::cout << "\n1. Basic logging:" << std::endl;
    auto logged_add = wrap(add).log();
    int result1 = logged_add(2, 3);
    
    // Chained decorators: log + cache
    std::cout << "\n2. Logging + Caching:" << std::endl;
    auto cached_add = wrap(add).log().cache();
    int result2 = cached_add(5, 7);  // First call - computes
    int result3 = cached_add(5, 7);  // Second call - from cache
    
    // Retry with logging
    std::cout << "\n3. Retry + Logging:" << std::endl;
    auto flaky = wrap(flaky_function).retry(3).log();
    try {
        int result4 = flaky(10);
        std::cout << "Final result: " << result4 << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Still failed after retries: " << e.what() << std::endl;
    }
    
    // Void function with logging
    std::cout << "\n4. Void function with logging:" << std::endl;
    auto logged_void = wrap(void_function).log();
    logged_void("Hello World");
    
    // Lambda with full decoration
    std::cout << "\n5. Lambda with all decorators:" << std::endl;
    auto fibonacci = wrap([](int n) -> int {
        if (n <= 1) return n;
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate work
        return n; // Simplified for demo
    }).log().cache().retry(2);
    
    int fib_result = fibonacci(5);
    int fib_result2 = fibonacci(5); // Should hit cache
    
    // Test stored wrapper (lvalue)
    std::cout << "\n6. Stored wrapper:" << std::endl;
    auto stored_wrapper = wrap(add);
    stored_wrapper.log().cache();
    int stored_result = stored_wrapper(10, 20);
    int stored_result2 = stored_wrapper(10, 20); // Cache hit
    
    std::cout << "\nDemo completed!" << std::endl;
    return 0;
}

#endif // CALLABLE_WRAPPER_EXAMPLE

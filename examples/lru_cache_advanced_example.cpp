#include "lru_cache.h"
#include <iostream>
#include <chrono>
#include <thread>

// Example 1: Simple decorator-like usage
auto expensive_function = make_cached<int, int>([](int x) -> int {
    // Simulate expensive computation
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "Computing " << x << " * " << x << '\n';
    return x * x;
}, 128);

// Example 2: Using the macro for cleaner syntax
CACHED_FUNCTION(square_function, int, int, 64, {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::cout << "Computing square of " << arg << '\n';
    return arg * arg;
});

// Example 3: Recursive function with manual caching
class FibonacciCalculator {
private:
    LRUCache<int, long long> cache_{1000};
    
public:
    long long calculate(int n) {
        if (n <= 1) return n;
        
        if (auto cached = cache_.get(n)) {
            return *cached;
        }
        
        long long result = calculate(n - 1) + calculate(n - 2);
        cache_.put(n, result);
        return result;
    }
    
    void clear_cache() { cache_.clear(); }
    size_t cache_size() const { return cache_.size(); }
    auto get_stats() const { return cache_.get_stats(); }
};

// Example 4: Class method caching
class DatabaseService {
private:
    LRUCache<std::string, std::string> query_cache_{500};
    
public:
    std::string get_user_data(const std::string& user_id) {
        // Check cache first
        if (auto cached = query_cache_.get(user_id)) {
            std::cout << "Cache hit for user: " << user_id << '\n';
            return *cached;
        }
        
        // Simulate database query
        std::cout << "Database query for user: " << user_id << '\n';
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::string user_data = "UserData:" + user_id;
        
        // Store in cache
        query_cache_.put(user_id, user_data);
        return user_data;
    }
    
    void invalidate_user(const std::string& user_id) {
        query_cache_.erase(user_id);
    }
    
    void print_cache_stats() {
        auto stats = query_cache_.get_stats();
        std::cout << "Cache stats - Hits: " << stats.hits 
                  << ", Misses: " << stats.misses 
                  << ", Hit rate: " << (stats.hit_rate() * 100) << "%" << '\n';
    }
};

// Example 5: Generic caching wrapper
template<typename Key, typename Value>
class CacheWrapper {
private:
    LRUCache<Key, Value> cache_;
    std::function<Value(const Key&)> compute_func_;
    
public:
    CacheWrapper(std::function<Value(const Key&)> func, size_t max_size = 128)
        : cache_(max_size), compute_func_(std::move(func)) {}
    
    Value get(const Key& key) {
        if (auto cached = cache_.get(key)) {
            return *cached;
        }
        
        Value result = compute_func_(key);
        cache_.put(key, result);
        return result;
    }
    
    void invalidate(const Key& key) { cache_.erase(key); }
    void clear() { cache_.clear(); }
    size_t size() const { return cache_.size(); }
    auto get_stats() const { return cache_.get_stats(); }
};

int main() {
    std::cout << "=== LRU Cache Examples ===" << '\n';
    
    // Example 1: Using decorator-like cached function
    std::cout << "\n1. Decorator-like usage:" << '\n';
    auto start = std::chrono::high_resolution_clock::now();
    std::cout << "First call result: " << expensive_function(5) << '\n';
    auto mid = std::chrono::high_resolution_clock::now();
    std::cout << "Second call result: " << expensive_function(5) << '\n';
    auto end = std::chrono::high_resolution_clock::now();
    
    auto first_duration = std::chrono::duration_cast<std::chrono::milliseconds>(mid - start);
    auto second_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - mid);
    
    std::cout << "First call took: " << first_duration.count() << "ms" << '\n';
    std::cout << "Second call took: " << second_duration.count() << "μs" << '\n';
    
    auto stats = expensive_function.cache_stats();
    std::cout << "Cache hit rate: " << (stats.hit_rate() * 100) << "%" << '\n';
    
    // Example 1b: Using macro syntax
    std::cout << "\n1b. Macro syntax:" << '\n';
    std::cout << "square_function(4) = " << square_function(4) << '\n';
    std::cout << "square_function(4) = " << square_function(4) << " (cached)" << '\n';
    
    // Example 2: Recursive fibonacci
    std::cout << "\n2. Recursive Fibonacci with caching:" << '\n';
    FibonacciCalculator fib_calc;
    
    start = std::chrono::high_resolution_clock::now();
    std::cout << "fib(40) = " << fib_calc.calculate(40) << '\n';
    end = std::chrono::high_resolution_clock::now();
    auto fib_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Calculation took: " << fib_duration.count() << "ms" << '\n';
    std::cout << "Cache size: " << fib_calc.cache_size() << '\n';
    
    auto fib_stats = fib_calc.get_stats();
    std::cout << "Fibonacci cache hit rate: " << (fib_stats.hit_rate() * 100) << "%" << '\n';
    
    // Example 3: Database service
    std::cout << "\n3. Database service with caching:" << '\n';
    DatabaseService db;
    db.get_user_data("user123");  // Cache miss
    db.get_user_data("user123");  // Cache hit
    db.get_user_data("user456");  // Cache miss
    db.get_user_data("user123");  // Cache hit
    db.print_cache_stats();
    
    // Example 4: Generic cache wrapper
    std::cout << "\n4. Generic cache wrapper:" << '\n';
    auto slow_computation = [](const int& x) -> int {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return x * x * x;
    };
    
    CacheWrapper<int, int> cached_computation(slow_computation, 50);
    
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 5; ++i) {
        std::cout << "compute(" << i << ") = " << cached_computation.get(i) << '\n';
    }
    mid = std::chrono::high_resolution_clock::now();
    
    // Call again - should be from cache
    std::cout << "Calling again (should be cached):" << '\n';
    for (int i = 0; i < 5; ++i) {
        cached_computation.get(i);
    }
    end = std::chrono::high_resolution_clock::now();
    
    auto first_round = std::chrono::duration_cast<std::chrono::milliseconds>(mid - start);
    auto second_round = std::chrono::duration_cast<std::chrono::microseconds>(end - mid);
    
    std::cout << "First round (computing): " << first_round.count() << "ms" << '\n';
    std::cout << "Second round (cached): " << second_round.count() << "μs" << '\n';
    
    auto wrapper_stats = cached_computation.get_stats();
    std::cout << "Wrapper cache hit rate: " << (wrapper_stats.hit_rate() * 100) << "%" << '\n';
    
    // Example 5: Show memory efficiency
    std::cout << "\n5. Memory management with eviction:" << '\n';
    auto memory_test = make_cached<int, std::string>([](int x) -> std::string {
        return "Result_" + std::to_string(x * x);
    }, 3); // Very small cache to demonstrate eviction
    
    for (int i = 0; i < 6; ++i) {
        std::cout << "memory_test(" << i << ") = " << memory_test(i) << '\n';
    }
    
    std::cout << "Cache size after 6 insertions (max=3): " << memory_test.cache_size() << '\n';
    
    // Test that early items were evicted
    std::cout << "Re-accessing early items (should recompute):" << '\n';
    std::cout << "memory_test(0) = " << memory_test(0) << '\n';
    
    auto final_stats = memory_test.cache_stats();
    std::cout << "Final stats - Hits: " << final_stats.hits 
              << ", Misses: " << final_stats.misses 
              << ", Evictions: " << final_stats.evictions << '\n';
    
    return 0;
}

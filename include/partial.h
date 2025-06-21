#pragma once

#include <tuple>
#include <utility>
#include <functional>
#include <type_traits>

namespace functools {

// Forward declaration
template<typename F, typename... BoundArgs>
class partial_impl;

namespace detail {
    // Helper to check if a type is a partial object
    template<typename T>
    struct is_partial : std::false_type {};
    
    template<typename F, typename... Args>
    struct is_partial<partial_impl<F, Args...>> : std::true_type {};
    
    // Helper for perfect forwarding in tuple
    template<typename T>
    using decay_if_not_ref = std::conditional_t<
        std::is_reference_v<T>, T, std::decay_t<T>
    >;
    
    // Index sequence helpers for C++14 compatibility
    template<std::size_t... Is>
    using index_sequence = std::index_sequence<Is...>;
    
    template<std::size_t N>
    using make_index_sequence = std::make_index_sequence<N>;
}

// Main partial implementation class
template<typename F, typename... BoundArgs>
class partial_impl {
private:
    F func_;
    std::tuple<detail::decay_if_not_ref<BoundArgs>...> bound_args_;
    
    // Helper to invoke with bound + remaining args
    template<std::size_t... Is, typename... RemainingArgs>
    constexpr decltype(auto) invoke_impl(
        detail::index_sequence<Is...>,
        RemainingArgs&&... remaining_args
    ) const {
        if constexpr (std::is_member_function_pointer_v<std::decay_t<F>>) {
            // Handle member function pointers
            return std::invoke(func_, std::get<Is>(bound_args_)..., 
                             std::forward<RemainingArgs>(remaining_args)...);
        } else {
            // Handle regular callables (including other partial objects)
            return std::invoke(func_, std::get<Is>(bound_args_)..., 
                             std::forward<RemainingArgs>(remaining_args)...);
        }
    }

public:
    // Constructor that matches the template parameters exactly
    constexpr explicit partial_impl(F func, BoundArgs... args)
        : func_(std::move(func))
        , bound_args_(std::move(args)...) {}
    
    // Constructor template for forwarding
    template<typename Func, typename... Args>
    constexpr explicit partial_impl(Func&& f, Args&&... args)
        : func_(std::forward<Func>(f))
        , bound_args_(std::forward<Args>(args)...) {}
    
    // Copy constructor
    partial_impl(const partial_impl&) = default;
    
    // Move constructor  
    partial_impl(partial_impl&&) = default;
    
    // Call operator
    template<typename... RemainingArgs>
    constexpr decltype(auto) operator()(RemainingArgs&&... remaining_args) const {
        return invoke_impl(
            detail::make_index_sequence<sizeof...(BoundArgs)>{},
            std::forward<RemainingArgs>(remaining_args)...
        );
    }
    
    // Allow conversion to std::function when types are compatible
    template<typename R, typename... Args>
    operator std::function<R(Args...)>() const {
        return [*this](Args... args) -> R {
            return (*this)(std::forward<Args>(args)...);
        };
    }
};

// Deduction guide for the partial_impl class
template<typename F, typename... Args>
partial_impl(F&&, Args&&...) -> partial_impl<std::decay_t<F>, std::decay_t<Args>...>;

// Main partial function
template<typename F, typename... BoundArgs>
constexpr auto partial(F&& func, BoundArgs&&... bound_args) {
    return partial_impl<std::decay_t<F>, std::decay_t<BoundArgs>...>(
        std::forward<F>(func), 
        std::forward<BoundArgs>(bound_args)...
    );
}

// Convenience alias for partial objects
template<typename F, typename... BoundArgs>
using Partial = partial_impl<F, BoundArgs...>;

} // namespace functools

// Example usage and test code
#ifdef FUNCTOOLS_PARTIAL_EXAMPLES

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

namespace examples {

// Test function for basic usage
void print_message(const std::string& prefix, int code, const std::string& msg) {
    std::cout << prefix << " [" << code << "]: " << msg << "\n";
}

// Test class for member function binding
class Logger {
public:
    void log(const std::string& level, const std::string& message) const {
        std::cout << "[" << level << "] " << message << "\n";
    }
    
    int add_with_base(int base, int value) const {
        return base + value;
    }
};

// Generic lambda for testing
auto multiply = [](int a, int b, int c) { return a * b * c; };

void run_examples() {
    std::cout << "=== functools::partial Examples ===\n\n";
    
    // Example 1: Basic function binding
    std::cout << "1. Basic function binding:\n";
    auto info_logger = functools::partial(print_message, "INFO");
    auto error_logger = functools::partial(print_message, "ERROR", 500);
    
    info_logger(200, "System started");
    error_logger("Database connection failed");
    std::cout << "\n";
    
    // Example 2: Lambda binding
    std::cout << "2. Lambda binding:\n";
    auto add = [](int x, int y) { return x + y; };
    auto add_ten = functools::partial(add, 10);
    
    std::cout << "10 + 5 = " << add_ten(5) << "\n";
    std::cout << "10 + 15 = " << add_ten(15) << "\n\n";
    
    // Example 3: Member function binding
    std::cout << "3. Member function binding:\n";
    Logger logger;
    auto log_info = functools::partial(&Logger::log, &logger, "INFO");
    auto log_error = functools::partial(&Logger::log, &logger, "ERROR");
    
    log_info("Application initialized");
    log_error("Configuration file not found");
    
    // Member function with return value
    auto add_base_100 = functools::partial(&Logger::add_with_base, &logger, 100);
    std::cout << "100 + 42 = " << add_base_100(42) << "\n\n";
    
    // Example 4: Nested partials
    std::cout << "4. Nested partials:\n";
    auto multiply_by_2 = functools::partial(multiply, 2);
    auto multiply_by_2_and_3 = functools::partial(multiply_by_2, 3);
    
    std::cout << "2 * 3 * 4 = " << multiply_by_2_and_3(4) << "\n\n";
    
    // Example 5: Using with STL algorithms
    std::cout << "5. Using with STL algorithms:\n";
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    std::vector<int> results;
    
    auto multiply_by_10 = functools::partial([](int factor, int x) { return factor * x; }, 10);
    
    std::transform(numbers.begin(), numbers.end(), std::back_inserter(results), multiply_by_10);
    
    std::cout << "Original: ";
    for (int n : numbers) std::cout << n << " ";
    std::cout << "\nMultiplied by 10: ";
    for (int n : results) std::cout << n << " ";
    std::cout << "\n\n";
    
    // Example 6: std::function conversion
    std::cout << "6. std::function conversion:\n";
    std::function<void(const std::string&)> callback = functools::partial(
        [](const std::string& prefix, const std::string& msg) {
            std::cout << prefix << ": " << msg << "\n";
        }, 
        "CALLBACK"
    );
    
    callback("This works with std::function!");
    std::cout << "\n";
    
    // Example 7: Factory pattern with partial
    std::cout << "7. Factory pattern:\n";
    auto make_multiplier = [](int factor) {
        return functools::partial([](int f, int x) { return f * x; }, factor);
    };
    
    auto double_it = make_multiplier(2);
    auto triple_it = make_multiplier(3);
    auto quadruple_it = make_multiplier(4);
    
    int value = 7;
    std::cout << value << " * 2 = " << double_it(value) << "\n";
    std::cout << value << " * 3 = " << triple_it(value) << "\n";
    std::cout << value << " * 4 = " << quadruple_it(value) << "\n";
}

} // namespace examples

// Uncomment to run examples
// int main() {
//     examples::run_examples();
//     return 0;
// }

#endif // FUNCTOOLS_PARTIAL_EXAMPLES

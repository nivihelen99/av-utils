#pragma once

#include <optional>
#include <functional>
#include <type_traits>
#include <string>
#include <stdexcept>
#include <regex>
#include <concepts>

namespace pipeline {
namespace optional {

// ============================================================================
// CONCEPTS FOR BETTER TYPE SAFETY
// ============================================================================

template<typename T>
concept OptionalLike = requires(T t) {
    { t.has_value() } -> std::convertible_to<bool>;
    { t.value() };
    typename T::value_type;
};

template<typename F, typename T>
concept UnaryPredicate = std::predicate<F, T>;

template<typename F, typename T>
concept UnaryFunction = std::invocable<F, T>;

// ============================================================================
// PERFORMANCE-OPTIMIZED OPTIONAL TRANSFORMATION UTILITIES
// ============================================================================

/**
 * Maps a function over an optional value (functorial map).
 * Optimized with perfect forwarding and SFINAE constraints.
 */
template<UnaryFunction<typename std::decay_t<decltype(std::declval<std::optional<int>>())>::value_type> F>
constexpr auto map(F&& func) noexcept {
    return [func = std::forward<F>(func)](auto&& opt) noexcept(noexcept(func(std::forward<decltype(opt)>(opt).value()))) {
        using OptType = std::decay_t<decltype(opt)>;
        using ValueType = typename OptType::value_type;
        using ResultType = std::invoke_result_t<F, ValueType>;
        
        if constexpr (std::is_nothrow_invocable_v<F, ValueType>) {
            if (opt.has_value()) {
                if constexpr (std::is_void_v<ResultType>) {
                    func(std::forward<decltype(opt)>(opt).value());
                    return std::optional<std::monostate>{std::monostate{}};
                } else {
                    return std::optional<ResultType>{func(std::forward<decltype(opt)>(opt).value())};
                }
            }
        } else {
            if (opt.has_value()) {
                try {
                    if constexpr (std::is_void_v<ResultType>) {
                        func(std::forward<decltype(opt)>(opt).value());
                        return std::optional<std::monostate>{std::monostate{}};
                    } else {
                        return std::optional<ResultType>{func(std::forward<decltype(opt)>(opt).value())};
                    }
                } catch (...) {
                    // Function threw, return empty optional
                }
            }
        }
        
        if constexpr (std::is_void_v<ResultType>) {
            return std::optional<std::monostate>{};
        } else {
            return std::optional<ResultType>{};
        }
    };
}

/**
 * Monadic bind operation for optionals with move semantics optimization.
 */
template<typename F>
constexpr auto and_then(F&& func) noexcept {
    return [func = std::forward<F>(func)](auto&& opt) noexcept(noexcept(func(std::forward<decltype(opt)>(opt).value()))) {
        using ValueType = typename std::decay_t<decltype(opt)>::value_type;
        using ResultType = std::invoke_result_t<F, ValueType>;
        
        if (opt.has_value()) {
            if constexpr (std::is_nothrow_invocable_v<F, ValueType>) {
                return func(std::forward<decltype(opt)>(opt).value());
            } else {
                try {
                    return func(std::forward<decltype(opt)>(opt).value());
                } catch (...) {
                    return ResultType{};
                }
            }
        }
        return ResultType{};
    };
}

/**
 * Lazy evaluation version of and_then for expensive computations.
 */
template<typename F>
constexpr auto and_then_lazy(F&& func) noexcept {
    return [func = std::forward<F>(func)](auto&& opt) noexcept {
        return [func, opt = std::forward<decltype(opt)>(opt)]() mutable {
            if (opt.has_value()) {
                return func(std::move(opt).value());
            }
            using ResultType = std::invoke_result_t<F, typename std::decay_t<decltype(opt)>::value_type>;
            return ResultType{};
        };
    };
}

/**
 * Optimized filter with short-circuit evaluation.
 */
template<UnaryPredicate<typename std::decay_t<decltype(std::declval<std::optional<int>>())>::value_type> F>
constexpr auto filter(F&& predicate) noexcept {
    return [predicate = std::forward<F>(predicate)](auto&& opt) noexcept(noexcept(predicate(opt.value()))) {
        using OptType = std::decay_t<decltype(opt)>;
        
        if constexpr (std::is_nothrow_invocable_v<F, typename OptType::value_type>) {
            if (opt.has_value() && predicate(opt.value())) {
                return std::forward<decltype(opt)>(opt);
            }
        } else {
            if (opt.has_value()) {
                try {
                    if (predicate(opt.value())) {
                        return std::forward<decltype(opt)>(opt);
                    }
                } catch (...) {
                    // Predicate threw, treat as false
                }
            }
        }
        return OptType{};
    };
}

/**
 * Optimized value_or with perfect forwarding.
 */
template<typename T>
constexpr auto value_or(T&& default_value) noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T>) {
    return [default_value = std::forward<T>(default_value)](const auto& opt) noexcept {
        return opt.value_or(default_value);
    };
}

/**
 * Exception-safe expect with custom exception types.
 */
template<typename Exception = std::runtime_error>
constexpr auto expect(std::string_view message = "Optional was empty") {
    return [message = std::string(message)](const auto& opt) -> decltype(auto) {
        if (opt.has_value()) {
            return opt.value();
        }
        throw Exception(message);
    };
}

// ============================================================================
// ENHANCED UTILITY FUNCTIONS
// ============================================================================

/**
 * Compile-time safe division with overflow checks.
 */
template<std::floating_point T = double>
constexpr auto safe_divide(T denominator) noexcept {
    return [denominator](T numerator) noexcept -> std::optional<T> {
        if (denominator == T{0} || !std::isfinite(denominator) || !std::isfinite(numerator)) {
            return std::nullopt;
        }
        T result = numerator / denominator;
        if (!std::isfinite(result)) {
            return std::nullopt;
        }
        return result;
    };
}

/**
 * Enhanced string parsing with better error handling.
 */
template<typename T = int>
constexpr auto safe_parse() noexcept {
    return [](std::string_view s) noexcept -> std::optional<T> {
        if (s.empty()) return std::nullopt;
        
        try {
            std::string str(s);  // string_view to string conversion
            std::size_t pos = 0;
            T result;
            
            if constexpr (std::is_same_v<T, int>) {
                result = std::stoi(str, &pos);
            } else if constexpr (std::is_same_v<T, long>) {
                result = std::stol(str, &pos);
            } else if constexpr (std::is_same_v<T, long long>) {
                result = std::stoll(str, &pos);
            } else if constexpr (std::is_same_v<T, float>) {
                result = std::stof(str, &pos);
            } else if constexpr (std::is_same_v<T, double>) {
                result = std::stod(str, &pos);
            } else if constexpr (std::is_same_v<T, long double>) {
                result = std::stold(str, &pos);
            } else {
                static_assert(std::is_arithmetic_v<T>, "Unsupported type for safe_parse");
            }
            
            // Check if entire string was consumed (no trailing characters)
            if (pos == str.length()) {
                return result;
            }
        } catch (...) {
            // Parsing failed
        }
        return std::nullopt;
    };
}

/**
 * Optimized optional creation with perfect forwarding.
 */
template<typename T>
constexpr auto some(T&& value) noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T>) {
    return std::optional<std::decay_t<T>>{std::forward<T>(value)};
}

/**
 * Type-safe none creation.
 */
template<typename T>
constexpr auto none() noexcept {
    return std::optional<T>{};
}

/**
 * Variadic lift with better performance for multiple optionals.
 */
template<typename F>
constexpr auto lift(F&& func) noexcept {
    return [func = std::forward<F>(func)](const auto&... opts) noexcept(noexcept(func(opts.value()...))) {
        // Early return optimization - check all values first
        const bool all_have_value = (opts.has_value() && ...);
        if (!all_have_value) {
            using ResultType = std::invoke_result_t<F, typename std::decay_t<decltype(opts)>::value_type...>;
            return std::optional<ResultType>{};
        }
        
        if constexpr (std::is_nothrow_invocable_v<F, typename std::decay_t<decltype(opts)>::value_type...>) {
            return some(func(opts.value()...));
        } else {
            try {
                return some(func(opts.value()...));
            } catch (...) {
                using ResultType = std::invoke_result_t<F, typename std::decay_t<decltype(opts)>::value_type...>;
                return std::optional<ResultType>{};
            }
        }
    };
}

/**
 * Optimized flatten with move semantics.
 */
template<typename T>
constexpr auto flatten(std::optional<std::optional<T>>&& nested_opt) noexcept -> std::optional<T> {
    if (nested_opt.has_value()) {
        return std::move(nested_opt.value());
    }
    return std::nullopt;
}

template<typename T>
constexpr auto flatten(const std::optional<std::optional<T>>& nested_opt) noexcept -> std::optional<T> {
    if (nested_opt.has_value()) {
        return nested_opt.value();
    }
    return std::nullopt;
}

/**
 * Side-effect function with guaranteed optional passthrough.
 */
template<typename F>
constexpr auto tap(F&& func) noexcept {
    return [func = std::forward<F>(func)](auto&& opt) noexcept(noexcept(func(opt.value()))) {
        if (opt.has_value()) {
            if constexpr (std::is_nothrow_invocable_v<F, typename std::decay_t<decltype(opt)>::value_type>) {
                func(opt.value());
            } else {
                try {
                    func(opt.value());
                } catch (...) {
                    // Ignore exceptions in tap
                }
            }
        }
        return std::forward<decltype(opt)>(opt);
    };
}

/**
 * Curried zip_with for better composability.
 */
template<typename F>
constexpr auto zip_with(F&& func) noexcept {
    return [func = std::forward<F>(func)](const auto& opt1) noexcept {
        return [func, &opt1](const auto& opt2) noexcept(noexcept(func(opt1.value(), opt2.value()))) {
            if (opt1.has_value() && opt2.has_value()) {
                if constexpr (std::is_nothrow_invocable_v<F, 
                    typename std::decay_t<decltype(opt1)>::value_type,
                    typename std::decay_t<decltype(opt2)>::value_type>) {
                    return some(func(opt1.value(), opt2.value()));
                } else {
                    try {
                        return some(func(opt1.value(), opt2.value()));
                    } catch (...) {
                        using ResultType = std::invoke_result_t<F, 
                            typename std::decay_t<decltype(opt1)>::value_type,
                            typename std::decay_t<decltype(opt2)>::value_type>;
                        return std::optional<ResultType>{};
                    }
                }
            }
            using ResultType = std::invoke_result_t<F, 
                typename std::decay_t<decltype(opt1)>::value_type,
                typename std::decay_t<decltype(opt2)>::value_type>;
            return std::optional<ResultType>{};
        };
    };
}

// ============================================================================
// ENHANCED VALIDATION UTILITIES
// ============================================================================

/**
 * Generic validation with custom error handling.
 */
template<typename F>
constexpr auto validate(F&& predicate, std::string_view error_msg = "Validation failed") {
    return [predicate = std::forward<F>(predicate), error_msg = std::string(error_msg)]
           (auto&& value) noexcept(noexcept(predicate(value))) -> std::optional<std::decay_t<decltype(value)>> {
        
        if constexpr (std::is_nothrow_invocable_v<F, decltype(value)>) {
            if (predicate(value)) {
                return std::forward<decltype(value)>(value);
            }
        } else {
            try {
                if (predicate(value)) {
                    return std::forward<decltype(value)>(value);
                }
            } catch (...) {
                // Predicate threw, treat as validation failure
            }
        }
        return std::nullopt;
    };
}

/**
 * Optimized range validation with compile-time checks.
 */
template<typename T>
constexpr auto validate_range(T min_val, T max_val) noexcept {
    static_assert(std::is_arithmetic_v<T>, "Range validation requires arithmetic type");
    return validate([min_val, max_val](const T& val) noexcept {
        return val >= min_val && val <= max_val;
    }, "Value out of range");
}

/**
 * Enhanced string validation utilities.
 */
constexpr auto validate_non_empty() noexcept {
    return validate([](std::string_view s) noexcept {
        return !s.empty();
    }, "String is empty");
}

/**
 * Robust email validation using regex (cached for performance).
 */
inline auto validate_email() {
    static const std::regex email_pattern(
        R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)"
    );
    
    return validate([](const std::string& email) {
        return std::regex_match(email, email_pattern);
    }, "Invalid email format");
}

/**
 * URL validation.
 */
inline auto validate_url() {
    static const std::regex url_pattern(
        R"(^https?://(?:[-\w.])+(?:\:[0-9]+)?(?:/(?:[\w/_.])*(?:\?(?:[\w&=%.])*)?(?:\#(?:[\w.])*)?)?$)"
    );
    
    return validate([](const std::string& url) {
        return std::regex_match(url, url_pattern);
    }, "Invalid URL format");
}

// ============================================================================
// ADVANCED UTILITY FUNCTIONS
// ============================================================================

/**
 * Exception-safe wrapper with specific exception handling.
 */
template<typename F, typename... ExceptionTypes>
constexpr auto try_optional(F&& func) noexcept {
    return [func = std::forward<F>(func)](auto&&... args) noexcept 
           -> std::optional<std::invoke_result_t<F, decltype(args)...>> {
        try {
            if constexpr (std::is_void_v<std::invoke_result_t<F, decltype(args)...>>) {
                func(std::forward<decltype(args)>(args)...);
                return std::optional<std::monostate>{std::monostate{}};
            } else {
                return func(std::forward<decltype(args)>(args)...);
            }
        } catch (const ExceptionTypes&...) {
            // Catch only specified exception types
            return std::nullopt;
        } catch (...) {
            if constexpr (sizeof...(ExceptionTypes) == 0) {
                // Catch all if no specific types specified
                return std::nullopt;
            } else {
                // Re-throw unspecified exceptions
                throw;
            }
        }
    };
}

/**
 * Pattern matching with automatic type deduction.
 */
template<typename OnSome, typename OnNone>
constexpr auto match(OnSome&& on_some, OnNone&& on_none) noexcept {
    return [on_some = std::forward<OnSome>(on_some), 
            on_none = std::forward<OnNone>(on_none)](const auto& opt) 
           noexcept(noexcept(on_some(opt.value())) && noexcept(on_none())) {
        using SomeResult = std::invoke_result_t<OnSome, typename std::decay_t<decltype(opt)>::value_type>;
        using NoneResult = std::invoke_result_t<OnNone>;
        static_assert(std::is_same_v<SomeResult, NoneResult>, 
                     "Both branches of match must return the same type");
        
        if (opt.has_value()) {
            return on_some(opt.value());
        } else {
            return on_none();
        }
    };
}

/**
 * Conditional transformation based on predicate.
 */
template<typename Predicate, typename Transform>
constexpr auto transform_if(Predicate&& pred, Transform&& transform) noexcept {
    return [pred = std::forward<Predicate>(pred), 
            transform = std::forward<Transform>(transform)](auto&& opt) 
           noexcept(noexcept(pred(opt.value())) && noexcept(transform(opt.value()))) {
        if (opt.has_value() && pred(opt.value())) {
            return map(transform)(std::forward<decltype(opt)>(opt));
        }
        return std::forward<decltype(opt)>(opt);
    };
}

/**
 * Accumulate multiple optionals into a container.
 */
template<template<typename...> class Container = std::vector>
constexpr auto collect() noexcept {
    return [](const auto&... opts) noexcept {
        using ValueType = std::common_type_t<typename std::decay_t<decltype(opts)>::value_type...>;
        Container<ValueType> result;
        
        if constexpr (requires { result.reserve(sizeof...(opts)); }) {
            result.reserve(sizeof...(opts));
        }
        
        ((opts.has_value() ? result.emplace_back(opts.value()) : void()), ...);
        return result;
    };
}

// ============================================================================
// PIPELINE COMPOSITION HELPER
// ============================================================================

/**
 * Pipeline composition operator for chaining operations.
 */
template<typename T>
class pipeline_wrapper {
private:
    T value_;

public:
    constexpr explicit pipeline_wrapper(T&& value) noexcept : value_(std::forward<T>(value)) {}
    
    template<typename F>
    constexpr auto then(F&& func) && noexcept(noexcept(func(std::move(value_)))) {
        return pipeline_wrapper{func(std::move(value_))};
    }
    
    template<typename F>
    constexpr auto then(F&& func) const& noexcept(noexcept(func(value_))) {
        return pipeline_wrapper{func(value_)};
    }
    
    constexpr auto get() && noexcept { return std::move(value_); }
    constexpr const auto& get() const& noexcept { return value_; }
    
    // Implicit conversion for final result
    constexpr operator T() && { return std::move(value_); }
    constexpr operator const T&() const& { return value_; }
};

/**
 * Entry point for pipeline composition.
 */
template<typename T>
constexpr auto pipe(T&& value) noexcept {
    return pipeline_wrapper{std::forward<T>(value)};
}

} // namespace optional
} // namespace pipeline

#pragma once

#include <optional>
#include <functional>
#include <type_traits>

namespace pipeline {
namespace optional {

// ============================================================================
// OPTIONAL TRANSFORMATION UTILITIES
// ============================================================================

/**
 * Maps a function over an optional value (functorial map).
 * If optional is empty, returns empty optional of result type.
 * 
 * Usage: pipe(opt).then(map([](int x) { return x * 2; }))
 */
template<typename F>
auto map(F&& func) {
    return [func = std::forward<F>(func)](const auto& opt) {
        using OptType = std::decay_t<decltype(opt)>;
        using ValueType = typename OptType::value_type;
        using ResultType = std::invoke_result_t<F, ValueType>;
        
        if (opt.has_value()) {
            return std::optional<ResultType>{func(opt.value())};
        }
        return std::optional<ResultType>{};
    };
}

/**
 * Monadic bind operation for optionals.
 * Function should return std::optional<U>.
 * 
 * Usage: pipe(opt).then(and_then([](int x) -> std::optional<string> { ... }))
 */
template<typename F>
auto and_then(F&& func) {
    return [func = std::forward<F>(func)](const auto& opt) {
        using OptType = std::decay_t<decltype(opt)>;
        using ValueType = typename OptType::value_type;
        
        if (opt.has_value()) {
            return func(opt.value());
        }
        return std::invoke_result_t<F, ValueType>{};
    };
}

/**
 * Filters optional based on predicate.
 * If predicate returns false, optional becomes empty.
 * 
 * Usage: pipe(opt).then(filter([](int x) { return x > 0; }))
 */
template<typename F>
auto filter(F&& predicate) {
    return [predicate = std::forward<F>(predicate)](const auto& opt) {
        using OptType = std::decay_t<decltype(opt)>;
        
        if (opt.has_value() && predicate(opt.value())) {
            return opt;
        }
        return OptType{};
    };
}

/**
 * Provides default value if optional is empty.
 * 
 * Usage: pipe(opt).then(value_or(42))
 */
template<typename T>
auto value_or(T&& default_value) {
    return [default_value = std::forward<T>(default_value)](const auto& opt) {
        using ValueType = typename std::decay_t<decltype(opt)>::value_type;
        return opt.value_or(static_cast<ValueType>(default_value));
    };
}

/**
 * Transforms empty optional to exception.
 * 
 * Usage: pipe(opt).then(expect("Value expected"))
 */
template<typename Exception = std::runtime_error>
auto expect(const std::string& message = "Optional was empty") {
    return [message](const auto& opt) {
        if (opt.has_value()) {
            return opt.value();
        }
        throw Exception(message);
    };
}

/**
 * Safe division that returns optional.
 */
auto safe_divide(double denominator) {
    return [denominator](double numerator) -> std::optional<double> {
        if (denominator == 0.0) {
            return std::nullopt;
        }
        return numerator / denominator;
    };
}

/**
 * Safe string to number conversion.
 */
template<typename T = int>
auto safe_parse() {
    return [](const std::string& s) -> std::optional<T> {
        try {
            if constexpr (std::is_same_v<T, int>) {
                return std::stoi(s);
            } else if constexpr (std::is_same_v<T, long>) {
                return std::stol(s);
            } else if constexpr (std::is_same_v<T, float>) {
                return std::stof(s);
            } else if constexpr (std::is_same_v<T, double>) {
                return std::stod(s);
            } else {
                static_assert(std::is_same_v<T, int>, "Unsupported type for safe_parse");
            }
        } catch (...) {
            return std::nullopt;
        }
    };
}

/**
 * Converts regular value to optional.
 */
template<typename T>
auto some(T&& value) {
    return std::optional<std::decay_t<T>>{std::forward<T>(value)};
}

/**
 * Creates empty optional of specified type.
 */
template<typename T>
auto none() {
    return std::optional<T>{};
}

/**
 * Lifts a regular function to work with optionals.
 * 
 * Usage: auto opt_add = lift([](int a, int b) { return a + b; });
 */
template<typename F>
auto lift(F&& func) {
    return [func = std::forward<F>(func)](const auto&... opts) {
        // Check if all optionals have values
        if ((opts.has_value() && ...)) {
            return some(func(opts.value()...));
        }
        using ResultType = std::invoke_result_t<F, typename std::decay_t<decltype(opts)>::value_type...>;
        return std::optional<ResultType>{};
    };
}

/**
 * Flattens nested optional.
 * std::optional<std::optional<T>> -> std::optional<T>
 */
template<typename T>
auto flatten(const std::optional<std::optional<T>>& nested_opt) -> std::optional<T> {
    if (nested_opt.has_value()) {
        return nested_opt.value();
    }
    return std::nullopt;
}

/**
 * Applies function if optional has value, otherwise returns empty optional.
 * Similar to map but for void functions.
 */
template<typename F>
auto tap(F&& func) {
    return [func = std::forward<F>(func)](const auto& opt) {
        if (opt.has_value()) {
            func(opt.value());
        }
        return opt;
    };
}

/**
 * Combines two optionals using a binary function.
 */
template<typename F>
auto zip_with(F&& func) {
    return [func = std::forward<F>(func)](const auto& opt1) {
        return [func, opt1](const auto& opt2) {
            if (opt1.has_value() && opt2.has_value()) {
                return some(func(opt1.value(), opt2.value()));
            }
            using ResultType = std::invoke_result_t<F, 
                typename std::decay_t<decltype(opt1)>::value_type,
                typename std::decay_t<decltype(opt2)>::value_type>;
            return std::optional<ResultType>{};
        };
    };
}

// ============================================================================
// VALIDATION UTILITIES
// ============================================================================

/**
 * Validates value with predicate, returns optional.
 */
template<typename F>
auto validate(F&& predicate, const std::string& error_msg = "Validation failed") {
    return [predicate = std::forward<F>(predicate), error_msg](const auto& value) -> std::optional<std::decay_t<decltype(value)>> {
        if (predicate(value)) {
            return value;
        }
        return std::nullopt;
    };
}

/**
 * Validates that value is in range [min, max].
 */
template<typename T>
auto validate_range(T min_val, T max_val) {
    return validate([min_val, max_val](const T& val) {
        return val >= min_val && val <= max_val;
    }, "Value out of range");
}

/**
 * Validates that string is not empty.
 */
auto validate_non_empty() {
    return validate([](const std::string& s) {
        return !s.empty();
    }, "String is empty");
}

/**
 * Validates that string matches basic email pattern.
 */
auto validate_email() {
    return validate([](const std::string& email) {
        return email.find('@') != std::string::npos && 
               email.find('.') != std::string::npos &&
               email.length() > 3;
    }, "Invalid email format");
}

// ============================================================================
// UTILITY FUNCTIONS FOR COMMON PATTERNS
// ============================================================================

/**
 * Try-catch wrapper that converts exceptions to empty optional.
 */
template<typename F>
auto try_optional(F&& func) {
    return [func = std::forward<F>(func)](const auto&... args) -> std::optional<std::invoke_result_t<F, decltype(args)...>> {
        try {
            return func(args...);
        } catch (...) {
            return std::nullopt;
        }
    };
}

/**
 * Conditional execution based on optional state.
 */
template<typename OnSome, typename OnNone>
auto match(OnSome&& on_some, OnNone&& on_none) {
    return [on_some = std::forward<OnSome>(on_some), 
            on_none = std::forward<OnNone>(on_none)](const auto& opt) {
        if (opt.has_value()) {
            return on_some(opt.value());
        } else {
            return on_none();
        }
    };
}

} // namespace optional
} // namespace pipeline

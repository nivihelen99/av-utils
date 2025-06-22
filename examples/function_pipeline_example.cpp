
#define PIPELINE_EXAMPLES
#include "function_pipeline.h"

#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <functional>

void test_basic_pipeline() {
    using namespace pipeline;
    
    std::cout << "Testing basic pipeline...\n";
    
    // Simple arithmetic pipeline
    auto p1 = pipe([](int x) { return x + 1; })
                 .then([](int x) { return x * 2; });
    
    assert(p1(5) == 12);  // (5 + 1) * 2 = 12
    std::cout << "✓ Basic arithmetic pipeline works\n";
    
    // Three-stage pipeline
    auto p2 = pipe([](int x) { return x * 2; })
                 .then([](int x) { return x + 3; })
                 .then([](int x) { return x * x; });
    
    assert(p2(5) == 169);  // ((5 * 2) + 3)^2 = 13^2 = 169
    std::cout << "✓ Three-stage pipeline works\n";
}

void test_type_transformations() {
    using namespace pipeline;
    
    std::cout << "\nTesting type transformations...\n";
    
    // int -> string -> string
    auto p1 = pipe([](int x) { return std::to_string(x); })
                 .then([](const std::string& s) { return "Value: " + s; });
    
    assert(p1(42) == "Value: 42");
    std::cout << "✓ int -> string transformation works\n";
    
    // string -> int -> double
    auto p2 = pipe([](const std::string& s) { return static_cast<int>(s.length()); })
                 .then([](int len) { return static_cast<double>(len) / 2.0; });
    
    assert(p2("hello") == 2.5);  // "hello".length() = 5, 5/2.0 = 2.5
    std::cout << "✓ string -> int -> double transformation works\n";
}

void test_variadic_pipe() {
    using namespace pipeline;
    
    std::cout << "\nTesting variadic pipe...\n";
    
    // Test with 3 functions
    auto p1 = pipe(
        [](int x) { return x + 1; },
        [](int x) { return x * 2; },
        [](int x) { return x - 5; }
    );
    
    assert(p1(10) == 17);  // ((10 + 1) * 2) - 5 = 22 - 5 = 17
    std::cout << "✓ Three-function variadic pipe works\n";
    
    // Test with 4 functions
    auto p2 = pipe(
        [](int x) { return x * 2; },
        [](int x) { return x + 1; },
        [](int x) { return x * x; },
        [](int x) { return x - 10; }
    );
    
    assert(p2(3) == 39);  // (((3 * 2) + 1)^2) - 10 = (7^2) - 10 = 49 - 10 = 39
    std::cout << "✓ Four-function variadic pipe works\n";
}

void test_composition() {
    using namespace pipeline;
    
    std::cout << "\nTesting right-to-left composition...\n";
    
    // compose(f, g, h) should create f(g(h(x)))
    auto c1 = compose(
        [](int x) { return x * x; },      // f - applied last
        [](int x) { return x + 1; },      // g - applied second  
        [](int x) { return x * 2; }       // h - applied first
    );
    
    // For input 3: h(3) = 6, g(6) = 7, f(7) = 49
    assert(c1(3) == 49);
    std::cout << "✓ Three-function composition works\n";
    
    // Compare with left-to-right pipe
    auto p1 = pipe([](int x) { return x * 2; })
                 .then([](int x) { return x + 1; })
                 .then([](int x) { return x * x; });
    
    assert(p1(3) == 49);  // Same result since it's the same order
    std::cout << "✓ Pipe and compose give same result for same function order\n";
}

void test_std_function_compatibility() {
    using namespace pipeline;
    
    std::cout << "\nTesting std::function compatibility...\n";
    
    std::function<int(int)> f1 = [](int x) { return x + 10; };
    std::function<int(int)> f2 = [](int x) { return x * 3; };
    
    auto p1 = pipe(f1).then(f2);
    assert(p1(5) == 45);  // (5 + 10) * 3 = 45
    std::cout << "✓ std::function compatibility works\n";
}

void test_perfect_forwarding() {
    using namespace pipeline;
    
    std::cout << "\nTesting perfect forwarding...\n";
    
    // Test with move-only type simulation
    struct MoveOnly {
        int value;
        MoveOnly(int v) : value(v) {}
        MoveOnly(const MoveOnly&) = delete;
        MoveOnly& operator=(const MoveOnly&) = delete;
        MoveOnly(MoveOnly&&) = default;
        MoveOnly& operator=(MoveOnly&&) = default;
    };
    
    auto p1 = pipe([](MoveOnly&& m) { return m.value * 2; })
                 .then([](int x) { return x + 1; });
    
    assert(p1(MoveOnly{5}) == 11);  // 5 * 2 + 1 = 11
    std::cout << "✓ Perfect forwarding works with move-only types\n";
}

void test_complex_scenarios() {
    using namespace pipeline;
    
    std::cout << "\nTesting complex scenarios...\n";
    
    // Mixed types and operations
    auto complex = pipe([](const std::vector<int>& v) { 
                           int sum = 0;
                           for (int x : v) sum += x;
                           return sum;
                       })
                      .then([](int sum) { return static_cast<double>(sum) / 2.0; })
                      .then([](double avg) { return std::to_string(avg); })
                      .then([](const std::string& s) { return "Average: " + s; });
    
    std::vector<int> data = {1, 2, 3, 4, 5};
    std::string result = complex(data);
    assert(result == "Average: 7.500000");
    std::cout << "✓ Complex multi-type pipeline works\n";
    
    // Nested pipelines
    auto inner = pipe([](int x) { return x * 2; })
                    .then([](int x) { return x + 1; });
    
    auto outer = pipe([inner](int x) { return inner(x); })  // Use inner pipeline as function
                    .then([](int x) { return x * x; });
    
    assert(outer(3) == 49);  // inner(3) = 7, 7^2 = 49
    std::cout << "✓ Nested pipelines work\n";
}

int main() {
    std::cout << "=== FunctionPipeline Test Suite ===\n\n";
    
    try {
        test_basic_pipeline();
        test_type_transformations();
        test_variadic_pipe();
        test_composition();
        test_std_function_compatibility();
        test_perfect_forwarding();
        test_complex_scenarios();
        
        std::cout << "\n=== All Tests Passed! ===\n\n";
        
        // Run examples
        examples::run_examples();
        
    } catch (const std::exception& e) {
        std::cout << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}


#if 0
#include "function_pipeline.hpp"
#include <optional>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cctype>

namespace optional_examples {

using namespace pipeline;

// Helper functions for optional transformations
template<typename T, typename F>
auto optional_map(F&& func) {
    return [func = std::forward<F>(func)](const std::optional<T>& opt) -> std::optional<decltype(func(std::declval<T>()))> {
        if (opt.has_value()) {
            return func(opt.value());
        }
        return std::nullopt;
    };
}

template<typename T, typename F>
auto optional_and_then(F&& func) {
    return [func = std::forward<F>(func)](const std::optional<T>& opt) -> decltype(func(std::declval<T>())) {
        if (opt.has_value()) {
            return func(opt.value());
        }
        return std::nullopt;
    };
}

template<typename T>
auto optional_filter(std::function<bool(const T&)> predicate) {
    return [predicate](const std::optional<T>& opt) -> std::optional<T> {
        if (opt.has_value() && predicate(opt.value())) {
            return opt;
        }
        return std::nullopt;
    };
}

// Example 1: Basic optional pipeline
void example_basic_optional() {
    std::cout << "=== Example 1: Basic Optional Pipeline ===\n";
    
    // Pipeline that works with optionals
    auto safe_math = pipe(optional_map<int>([](int x) { return x * 2; }))
                        .then(optional_map<int>([](int x) { return x + 10; }))
                        .then(optional_map<int>([](int x) { return x * x; }));
    
    // Test with valid optional
    std::optional<int> input1 = 5;
    auto result1 = safe_math(input1);
    std::cout << "safe_math(5) = " << (result1 ? std::to_string(*result1) : "nullopt") << std::endl;
    // Expected: ((5 * 2) + 10)^2 = 20^2 = 400
    
    // Test with empty optional
    std::optional<int> input2 = std::nullopt;
    auto result2 = safe_math(input2);
    std::cout << "safe_math(nullopt) = " << (result2 ? std::to_string(*result2) : "nullopt") << std::endl;
    std::cout << std::endl;
}

// Example 2: Safe string parsing pipeline
void example_safe_parsing() {
    std::cout << "=== Example 2: Safe String Parsing ===\n";
    
    // Safe string to int conversion
    auto safe_stoi = [](const std::string& s) -> std::optional<int> {
        try {
            return std::stoi(s);
        } catch (...) {
            return std::nullopt;
        }
    };
    
    // Safe division
    auto safe_divide = [](std::pair<int, int> p) -> std::optional<double> {
        if (p.second == 0) return std::nullopt;
        return static_cast<double>(p.first) / p.second;
    };
    
    // Pipeline for safe string -> int -> calculation
    auto safe_calculator = pipe(safe_stoi)
                              .then(optional_and_then<int>([](int x) -> std::optional<std::pair<int, int>> {
                                  if (x > 0) return std::make_pair(100, x);
                                  return std::nullopt;
                              }))
                              .then(optional_and_then<std::pair<int, int>>(safe_divide))
                              .then(optional_map<double>([](double d) { return std::to_string(d); }));
    
    // Test cases
    std::vector<std::string> test_inputs = {"5", "0", "-3", "abc", "20"};
    
    for (const auto& input : test_inputs) {
        auto result = safe_calculator(input);
        std::cout << "safe_calculator(\"" << input << "\") = " 
                  << (result ? *result : "nullopt") << std::endl;
    }
    std::cout << std::endl;
}

// Example 3: Optional chaining with validation
void example_validation_chain() {
    std::cout << "=== Example 3: Validation Chain ===\n";
    
    struct Person {
        std::string name;
        int age;
        std::string email;
    };
    
    // Validation functions
    auto validate_name = [](const Person& p) -> std::optional<Person> {
        if (!p.name.empty() && p.name.length() >= 2) {
            return p;
        }
        return std::nullopt;
    };
    
    auto validate_age = [](const Person& p) -> std::optional<Person> {
        if (p.age >= 18 && p.age <= 120) {
            return p;
        }
        return std::nullopt;
    };
    
    auto validate_email = [](const Person& p) -> std::optional<Person> {
        if (p.email.find('@') != std::string::npos && p.email.find('.') != std::string::npos) {
            return p;
        }
        return std::nullopt;
    };
    
    auto format_person = [](const Person& p) -> std::string {
        return p.name + " (" + std::to_string(p.age) + ") <" + p.email + ">";
    };
    
    // Validation pipeline
    auto person_validator = pipe([](const Person& p) { return std::optional<Person>{p}; })
                               .then(optional_and_then<Person>(validate_name))
                               .then(optional_and_then<Person>(validate_age))
                               .then(optional_and_then<Person>(validate_email))
                               .then(optional_map<Person>(format_person));
    
    // Test cases
    std::vector<Person> test_people = {
        {"John Doe", 25, "john@example.com"},      // Valid
        {"", 30, "test@example.com"},              // Invalid name
        {"Jane Smith", 15, "jane@example.com"},    // Invalid age
        {"Bob Wilson", 35, "bobexample.com"},      // Invalid email
        {"Alice Brown", 28, "alice@test.co.uk"}    // Valid
    };
    
    for (const auto& person : test_people) {
        auto result = person_validator(person);
        std::cout << "Validating " << person.name << ": " 
                  << (result ? *result : "INVALID") << std::endl;
    }
    std::cout << std::endl;
}

// Example 4: Optional with error accumulation
void example_error_accumulation() {
    std::cout << "=== Example 4: Error Accumulation ===\n";
    
    // Result type that can hold either value or error
    template<typename T>
    struct Result {
        std::optional<T> value;
        std::vector<std::string> errors;
        
        Result(T val) : value(val) {}
        Result(std::string error) : errors{error} {}
        Result(std::optional<T> val, std::vector<std::string> errs) 
            : value(val), errors(errs) {}
        
        bool is_ok() const { return value.has_value(); }
        T unwrap() const { return value.value(); }
    };
    
    // Helper for result mapping
    template<typename T, typename F>
    auto result_map(F&& func, const std::string& error_msg) {
        return [func = std::forward<F>(func), error_msg](const Result<T>& result) -> Result<decltype(func(std::declval<T>()))> {
            if (result.is_ok()) {
                try {
                    return Result<decltype(func(std::declval<T>()))>(func(result.unwrap()));
                } catch (...) {
                    auto errors = result.errors;
                    errors.push_back(error_msg);
                    return Result<decltype(func(std::declval<T>()))>(std::nullopt, errors);
                }
            } else {
                return Result<decltype(func(std::declval<T>()))>(std::nullopt, result.errors);
            }
        };
    }
    
    // Pipeline with error accumulation
    auto error_prone_pipeline = pipe(result_map<std::string>([](const std::string& s) {
                                        return std::stoi(s);
                                    }, "Failed to parse integer"))
                                   .then(result_map<int>([](int x) {
                                        if (x == 0) throw std::runtime_error("Zero division");
                                        return 100 / x;
                                    }, "Division by zero"))
                                   .then(result_map<int>([](int x) {
                                        return std::to_string(x * x);
                                    }, "Failed to square and convert"));
    
    // Test cases
    std::vector<std::string> inputs = {"5", "0", "abc", "10", "-2"};
    
    for (const auto& input : inputs) {
        Result<std::string> initial_result(input);
        auto result = error_prone_pipeline(initial_result);
        
        std::cout << "Processing \"" << input << "\": ";
        if (result.is_ok()) {
            std::cout << "Success = " << result.unwrap() << std::endl;
        } else {
            std::cout << "Errors: ";
            for (size_t i = 0; i < result.errors.size(); ++i) {
                std::cout << result.errors[i];
                if (i < result.errors.size() - 1) std::cout << ", ";
            }
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;
}

// Example 5: Optional data processing pipeline
void example_data_processing() {
    std::cout << "=== Example 5: Optional Data Processing ===\n";
    
    // Simulate reading data that might fail
    auto read_data = [](const std::string& filename) -> std::optional<std::vector<std::string>> {
        if (filename.empty() || filename == "invalid.txt") {
            return std::nullopt;
        }
        // Simulate file content
        return std::vector<std::string>{"1", "2", "3", "invalid", "5", "6"};
    };
    
    // Parse numbers, filtering out invalid ones
    auto parse_numbers = [](const std::vector<std::string>& lines) -> std::vector<int> {
        std::vector<int> numbers;
        for (const auto& line : lines) {
            try {
                numbers.push_back(std::stoi(line));
            } catch (...) {
                // Skip invalid numbers
            }
        }
        return numbers;
    };
    
    // Calculate statistics
    auto calculate_stats = [](const std::vector<int>& numbers) -> std::optional<std::string> {
        if (numbers.empty()) return std::nullopt;
        
        int sum = 0;
        int min = numbers[0];
        int max = numbers[0];
        
        for (int n : numbers) {
            sum += n;
            if (n < min) min = n;
            if (n > max) max = n;
        }
        
        double avg = static_cast<double>(sum) / numbers.size();
        
        std::ostringstream oss;
        oss << "Count: " << numbers.size() 
            << ", Sum: " << sum 
            << ", Avg: " << std::fixed << avg
            << ", Min: " << min 
            << ", Max: " << max;
        
        return oss.str();
    };
    
    // Complete data processing pipeline
    auto data_processor = pipe(read_data)
                             .then(optional_map<std::vector<std::string>>(parse_numbers))
                             .then(optional_and_then<std::vector<int>>(calculate_stats));
    
    // Test with different filenames
    std::vector<std::string> filenames = {"data.txt", "invalid.txt", "", "numbers.csv"};
    
    for (const auto& filename : filenames) {
        auto result = data_processor(filename);
        std::cout << "Processing \"" << filename << "\": " 
                  << (result ? *result : "Failed to process") << std::endl;
    }
    std::cout << std::endl;
}

// Example 6: Combining optionals with regular values
void example_mixed_pipeline() {
    std::cout << "=== Example 6: Mixed Optional/Regular Pipeline ===\n";
    
    // Pipeline that starts with regular value, goes through optional, back to regular
    auto mixed_pipeline = pipe([](int x) { 
                                 // Regular -> Optional
                                 if (x > 0) return std::optional<int>{x * 2};
                                 return std::optional<int>{};
                             })
                             .then([](const std::optional<int>& opt) {
                                 // Optional -> Optional
                                 if (opt) return std::optional<std::string>{std::to_string(*opt)};
                                 return std::optional<std::string>{};
                             })
                             .then([](const std::optional<std::string>& opt) {
                                 // Optional -> Regular (with default)
                                 return opt.value_or("DEFAULT");
                             })
                             .then([](const std::string& s) {
                                 // Regular -> Regular
                                 return "Result: " + s;
                             });
    
    // Test cases
    std::vector<int> test_values = {5, -3, 0, 10};
    
    for (int value : test_values) {
        auto result = mixed_pipeline(value);
        std::cout << "mixed_pipeline(" << value << ") = \"" << result << "\"" << std::endl;
    }
}

} // namespace optional_examples

int main() {
    std::cout << "=== FunctionPipeline with std::optional Examples ===\n\n";
    
    optional_examples::example_basic_optional();
    optional_examples::example_safe_parsing();

}

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


#endif

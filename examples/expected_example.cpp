#include "expected.h" // Include the optimized Expected header
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cmath>

using namespace aos_utils;

// Example 1: Basic usage with different construction methods
void basic_usage_example() {
    std::cout << "=== Basic Usage Example ===\n";
    
    // Success case
    Expected<int> success_result = 42;
    std::cout << "Success result has value: " << success_result.has_value() << "\n";
    std::cout << "Success value: " << success_result.value() << "\n";
    std::cout << "Success value (unchecked): " << *success_result << "\n";
    
    // Error case
    Expected<int> error_result = make_unexpected("Something went wrong");
    std::cout << "Error result has value: " << error_result.has_value() << "\n";
    try {
        std::cout << "Error value: " << error_result.value() << "\n";
    } catch (const std::exception& e) {
        std::cout << "Exception caught: " << e.what() << "\n";
        std::cout << "Error message: " << error_result.error() << "\n";
    }
    
    // Using value_or for safe access
    std::cout << "Error result with default: " << error_result.value_or(-1) << "\n";
    std::cout << "\n";
}

// Example 2: File operations with error handling
Expected<std::string> read_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return make_unexpected("Could not open file: " + filename);
    }
    
    std::string content;
    std::string line;
    while (std::getline(file, line)) {
        content += line + "\n";
    }
    
    if (content.empty()) {
        return make_unexpected("File is empty: " + filename);
    }
    
    return content;
}

void file_operations_example() {
    std::cout << "=== File Operations Example ===\n";
    
    // Try to read an existing file (this will likely fail in demo)
    auto result = read_file("test.txt");
    
    if (result) {
        std::cout << "File content:\n" << *result << "\n";
    } else {
        std::cout << "Error reading file: " << result.error() << "\n";
    }
    std::cout << "\n";
}

// Example 3: Mathematical operations with custom error types
enum class MathError {
    DivisionByZero,
    NegativeSquareRoot,
    InvalidInput
};

std::ostream& operator<<(std::ostream& os, MathError error) {
    switch (error) {
        case MathError::DivisionByZero: return os << "Division by zero";
        case MathError::NegativeSquareRoot: return os << "Negative square root";
        case MathError::InvalidInput: return os << "Invalid input";
    }
    return os;
}

Expected<double, MathError> safe_divide(double a, double b) {
    if (b == 0.0) {
        return Unexpected{MathError::DivisionByZero};
    }
    return a / b;
}

Expected<double, MathError> safe_sqrt(double x) {
    if (x < 0.0) {
        return Unexpected{MathError::NegativeSquareRoot};
    }
    return std::sqrt(x);
}

void math_operations_example() {
    std::cout << "=== Math Operations Example ===\n";
    
    // Successful operations
    auto div_result = safe_divide(10.0, 2.0);
    std::cout << "10 / 2 = " << div_result.value_or(0.0) << "\n";
    
    auto sqrt_result = safe_sqrt(16.0);
    std::cout << "sqrt(16) = " << sqrt_result.value_or(0.0) << "\n";
    
    // Error cases
    auto div_error = safe_divide(10.0, 0.0);
    if (!div_error) {
        std::cout << "Division error: " << div_error.error() << "\n";
    }
    
    auto sqrt_error = safe_sqrt(-4.0);
    if (!sqrt_error) {
        std::cout << "Square root error: " << sqrt_error.error() << "\n";
    }
    std::cout << "\n";
}

// Example 4: Monadic operations (map, and_then, or_else)
Expected<int> parse_int(const std::string& str) {
    try {
        return std::stoi(str);
    } catch (const std::exception&) {
        return make_unexpected("Invalid integer: " + str);
    }
}

Expected<int> multiply_by_2(int x) {
    return x * 2;
}

Expected<int> add_10(int x) {
    return x + 10;
}

Expected<std::string> fallback_handler(const std::string& error) {
    std::cout << "Handling error: " << error << "\n";
    return "0"; // Return a default valid input
}

// New fallback handler for compatible or_else usage in the problematic chain
Expected<int, std::string> new_fallback_handler(const std::string& error_msg) {
    std::cout << "Handling error with new_fallback_handler: " << error_msg << "\n";
    // This handler provides a default integer value.
    return 0;
}

void monadic_operations_example() {
    std::cout << "=== Monadic Operations Example ===\n";
    
    // Successful chain
    auto result1 = parse_int("5")
        .map([](int x) { return x * 2; })          // Transform: 5 -> 10
        .and_then([](int x) { return add_10(x); }) // Chain: 10 -> 20
        .map([](int x) { return std::to_string(x); }); // Transform: 20 -> "20"
    
    std::cout << "Chain result (success): " << result1.value_or("error") << "\n";
    
    // Chain with error
    auto result2 = parse_int("abc")
        .map([](int x) { return x * 2; })
        .and_then([](int x) { return add_10(x); })
        .map([](int x) { return std::to_string(x); });
    
    std::cout << "Chain result (error): " << result2.value_or("default") << "\n";
    if (!result2) {
        std::cout << "Error in chain: " << result2.error() << "\n";
    }
    
    // Using or_else for error recovery
    // Original chain that is problematic with the new or_else:
    // auto result3_original = parse_int("invalid")
    //     .or_else(fallback_handler) // fallback_handler returns Expected<std::string, std::string>
    //                                  // This causes static_assert failure if parse_int succeeded, as
    //                                  // Expected<std::string, std::string> is not constructible from int.
    //     .and_then([](const std::string& s) { return parse_int(s); })
    //     .map([](int x) { return x + 100; });
    // std::cout << "Chain with recovery (original, problematic): " << result3_original.value_or(-1) << "\n";

    // Revised chain for result3 to be compatible with the new or_else definition
    auto result3 = parse_int("invalid") // Returns Expected<int, std::string>
        .or_else(new_fallback_handler)  // new_fallback_handler returns Expected<int, std::string>
                                        // The static_assert (is_constructible<Expected<int,std::string>, const int&>) passes.
                                        // Output of or_else is Expected<int, std::string>
        .map([](int x) { return x + 100; }); // map operates on the int.

    std::cout << "Chain with recovery (revised): " << result3.value_or(-1) << "\n";
    std::cout << "\n";
}

// Example 5: Working with containers and algorithms
std::vector<Expected<int>> process_strings(const std::vector<std::string>& strings) {
    std::vector<Expected<int>> results;
    results.reserve(strings.size());
    
    for (const auto& str : strings) {
        results.push_back(parse_int(str)
            .map([](int x) { return x * x; })); // Square the number
    }
    
    return results;
}

void container_example() {
    std::cout << "=== Container Example ===\n";
    
    std::vector<std::string> inputs = {"1", "2", "abc", "4", "xyz", "5"};
    auto results = process_strings(inputs);
    
    std::cout << "Processing results:\n";
    for (size_t i = 0; i < results.size(); ++i) {
        std::cout << "Input: " << inputs[i] << " -> ";
        if (results[i]) {
            std::cout << "Result: " << *results[i] << "\n";
        } else {
            std::cout << "Error: " << results[i].error() << "\n";
        }
    }
    
    // Count successful results
    int success_count = 0;
    int total_sum = 0;
    for (const auto& result : results) {
        if (result) {
            success_count++;
            total_sum += *result;
        }
    }
    
    std::cout << "Successful operations: " << success_count << "/" << results.size() << "\n";
    std::cout << "Sum of successful results: " << total_sum << "\n";
    std::cout << "\n";
}

// Example 6: Error transformation with map_error
Expected<int, std::string> validate_positive(int x) {
    if (x <= 0) {
        return make_unexpected("Value must be positive");
    }
    return x;
}

void error_transformation_example() {
    std::cout << "=== Error Transformation Example ===\n";
    
    auto result = validate_positive(-5)
        .map_error([](const std::string& error) {
            return "Validation failed: " + error + " (code: -1)";
        });
    
    if (!result) {
        std::cout << "Transformed error: " << result.error() << "\n";
    }
    std::cout << "\n";
}

// Example 7: Exception handling with make_expected
int risky_operation(int x) {
    if (x < 0) {
        throw std::invalid_argument("Negative input not allowed");
    }
    if (x > 1000) {
        throw std::runtime_error("Input too large");
    }
    return x * x;
}

void exception_handling_example() {
    std::cout << "=== Exception Handling Example ===\n";
    
    // Safe operation
    auto safe_result = make_expected([](){ return risky_operation(10); });
    std::cout << "Safe operation result: " << safe_result.value_or(-1) << "\n";
    
    // Exception cases
    auto error_result1 = make_expected([](){ return risky_operation(-5); });
    if (!error_result1) {
        std::cout << "Caught exception: " << error_result1.error() << "\n";
    }
    
    auto error_result2 = make_expected([](){ return risky_operation(2000); });
    if (!error_result2) {
        std::cout << "Caught exception: " << error_result2.error() << "\n";
    }
    std::cout << "\n";
}

// Example 8: Comparison operations
void comparison_example() {
    std::cout << "=== Comparison Example ===\n";
    
    Expected<int> a = 42;
    Expected<int> b = 42;
    Expected<int> c = 24;
    Expected<int> error = make_unexpected("error");
    
    std::cout << "a == b: " << (a == b) << "\n";
    std::cout << "a == c: " << (a == c) << "\n";
    std::cout << "a == error: " << (a == error) << "\n";
    std::cout << "a == 42: " << (a == 42) << "\n";
    std::cout << "error == make_unexpected(\"error\"): " << (error == make_unexpected("error")) << "\n";
    std::cout << "\n";
}

// Example 9: Real-world scenario - Configuration parser
struct Config {
    std::string host;
    int port;
    bool ssl_enabled;
    
    friend std::ostream& operator<<(std::ostream& os, const Config& config) {
        return os << "Config{host: " << config.host 
                  << ", port: " << config.port 
                  << ", ssl: " << config.ssl_enabled << "}";
    }
};

Expected<Config> parse_config(const std::vector<std::string>& lines) {
    Config config;
    
    for (const auto& line : lines) {
        auto eq_pos = line.find('=');
        if (eq_pos == std::string::npos) {
            continue; // Skip invalid lines
        }
        
        std::string key = line.substr(0, eq_pos);
        std::string value = line.substr(eq_pos + 1);
        
        if (key == "host") {
            config.host = value;
        } else if (key == "port") {
            auto port_result = parse_int(value);
            if (!port_result) {
                return make_unexpected("Invalid port number: " + value);
            }
            if (*port_result <= 0 || *port_result > 65535) {
                return make_unexpected("Port out of range: " + std::to_string(*port_result));
            }
            config.port = *port_result;
        } else if (key == "ssl") {
            config.ssl_enabled = (value == "true" || value == "1");
        }
    }
    
    if (config.host.empty()) {
        return make_unexpected("Host is required");
    }
    if (config.port == 0) {
        return make_unexpected("Port is required");
    }
    
    return config;
}

void real_world_example() {
    std::cout << "=== Real-world Configuration Parser Example ===\n";
    
    // Valid configuration
    std::vector<std::string> valid_config = {
        "host=localhost",
        "port=8080", 
        "ssl=true"
    };
    
    auto config_result = parse_config(valid_config);
    if (config_result) {
        std::cout << "Parsed config: " << *config_result << "\n";
    } else {
        std::cout << "Config error: " << config_result.error() << "\n";
    }
    
    // Invalid configuration
    std::vector<std::string> invalid_config = {
        "host=localhost",
        "port=abc",  // Invalid port
        "ssl=true"
    };
    
    auto invalid_result = parse_config(invalid_config);
    if (!invalid_result) {
        std::cout << "Invalid config error: " << invalid_result.error() << "\n";
    }
    std::cout << "\n";
}

int main() {
    std::cout << "Expected Class Demonstration\n";
    std::cout << "============================\n\n";
    
    basic_usage_example();
    file_operations_example();
    math_operations_example();
    monadic_operations_example();
    container_example();
    error_transformation_example();
    exception_handling_example();
    comparison_example();
    real_world_example();
    
    std::cout << "All examples completed successfully!\n";
    return 0;
}

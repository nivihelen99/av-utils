#include "optional_pipeline.h" // Your enhanced header
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>

using namespace pipeline::optional;

// ============================================================================
// BASIC USAGE EXAMPLES
// ============================================================================

void basic_examples() {
    std::cout << "=== Basic Usage Examples ===\n";
    
    // Simple transformation chain
    auto result1 = pipe(some(42))
        .then(map([](int x) { return x * 2; }))
        .then(filter([](int x) { return x > 50; }))
        .then(map([](int x) { return std::to_string(x); }))
        .then(value_or(std::string("default")))
        .get();
    
    std::cout << "Chain result: " << result1 << "\n"; // "84"
    
    // Working with empty optionals
    auto result2 = pipe(none<int>())
        .then(map([](int x) { return x * 2; }))
        .then(value_or(100))
        .get();
    
    std::cout << "Empty optional result: " << result2 << "\n"; // 100
    
    // Monadic chaining with and_then
    auto safe_sqrt = [](double x) -> std::optional<double> {
        return x >= 0 ? some(std::sqrt(x)) : none<double>();
    };
    
    auto result3 = pipe(some(16.0))
        .then(and_then(safe_sqrt))
        .then(and_then(safe_sqrt))
        .then(value_or(0.0))
        .get();
    
    std::cout << "Safe sqrt chain: " << result3 << "\n"; // 2.0
}

// ============================================================================
// STRING PROCESSING EXAMPLES
// ============================================================================

struct User {
    std::string name;
    std::string email;
    int age;
};

void string_processing_examples() {
    std::cout << "\n=== String Processing Examples ===\n";
    
    // Email validation and processing
    auto process_email = [](const std::string& input) {
        return pipe(some(input))
            .then(validate_non_empty())
            .then(validate_email())
            .then(map([](const std::string& email) {
                std::string result = email;
                std::transform(result.begin(), result.end(), result.begin(), ::tolower);
                return result;
            }));
    };
    
    auto valid_email = process_email("John.Doe@Example.COM").get();
    auto invalid_email = process_email("not-an-email").get();
    
    std::cout << "Valid email: " << (valid_email ? *valid_email : "None") << "\n";
    std::cout << "Invalid email: " << (invalid_email ? *invalid_email : "None") << "\n";
    
    // Safe string to number conversion with validation
    auto parse_age = [](const std::string& input) {
        return pipe(some(input))
            .then(validate_non_empty())
            .then(and_then(safe_parse<int>()))
            .then(validate_range(0, 120));
    };
    
    auto age1 = parse_age("25").get();
    auto age2 = parse_age("150").get(); // Out of range
    auto age3 = parse_age("abc").get(); // Invalid format
    
    std::cout << "Valid age: " << (age1 ? std::to_string(*age1) : "None") << "\n";
    std::cout << "Invalid age (>120): " << (age2 ? std::to_string(*age2) : "None") << "\n";
    std::cout << "Invalid age (text): " << (age3 ? std::to_string(*age3) : "None") << "\n";
}

// ============================================================================
// COMPLEX DATA PROCESSING EXAMPLES
// ============================================================================

void data_processing_examples() {
    std::cout << "\n=== Data Processing Examples ===\n";
    
    // User creation from form data
    auto create_user = [](const std::string& name, const std::string& email, const std::string& age_str) -> std::optional<User> {
        auto validated_name = pipe(some(name))
            .then(validate_non_empty())
            .then(validate([](const std::string& s) { return s.length() <= 50; }, "Name too long"))
            .get();
        
        auto validated_email = pipe(some(email))
            .then(validate_non_empty())
            .then(validate_email())
            .get();
        
        auto validated_age = pipe(some(age_str))
            .then(and_then(safe_parse<int>()))
            .then(validate_range(13, 120))
            .get();
        
        // Use lift to combine all validations
        return lift([](const std::string& n, const std::string& e, int a) {
            return User{n, e, a};
        })(validated_name, validated_email, validated_age);
    };
    
    auto user1 = create_user("John Doe", "john@example.com", "30");
    auto user2 = create_user("", "invalid-email", "25"); // Invalid name and email
    
    if (user1) {
        std::cout << "Created user: " << user1->name << " (" << user1->email << ", " << user1->age << ")\n";
    }
    if (!user2) {
        std::cout << "Failed to create user (validation errors)\n";
    }
    
    // Collection processing with filtering and transformation
    std::vector<std::string> numbers = {"1", "2", "abc", "4", "5", "def", "7"};
    std::vector<int> valid_numbers;
    
    for (const auto& num_str : numbers) {
        pipe(some(num_str))
            .then(and_then(safe_parse<int>()))
            .then(filter([](int x) { return x % 2 == 0; })) // Only even numbers
            .then(tap([&valid_numbers](int x) { valid_numbers.push_back(x); }));
    }
    
    std::cout << "Valid even numbers: ";
    for (int n : valid_numbers) {
        std::cout << n << " ";
    }
    std::cout << "\n";
}

// ============================================================================
// MATHEMATICAL OPERATIONS EXAMPLES
// ============================================================================

void mathematical_examples() {
    std::cout << "\n=== Mathematical Operations Examples ===\n";
    
    // Safe mathematical operations
    auto safe_math_chain = [](double a, double b, double c) {
        return pipe(some(a))
            .then(and_then(safe_divide(b)))
            .then(map([](double x) { return x * x; }))
            .then(and_then([c](double x) -> std::optional<double> {
                return x >= 0 ? some(std::sqrt(x) + c) : none<double>();
            }));
    };
    
    auto result1 = safe_math_chain(100.0, 5.0, 10.0).get(); // ((100/5)^2)^0.5 + 10 = 30
    auto result2 = safe_math_chain(100.0, 0.0, 10.0).get(); // Division by zero
    
    std::cout << "Safe math result 1: " << (result1 ? std::to_string(*result1) : "None") << "\n";
    std::cout << "Safe math result 2: " << (result2 ? std::to_string(*result2) : "None") << "\n";
    
    // Working with multiple optionals
    auto opt1 = some(10);
    auto opt2 = some(20);
    auto opt3 = none<int>();
    
    auto sum_result = lift([](int a, int b) { return a + b; })(opt1, opt2);
    auto sum_with_none = lift([](int a, int b) { return a + b; })(opt1, opt3);
    
    std::cout << "Sum of 10 and 20: " << (sum_result ? std::to_string(*sum_result) : "None") << "\n";
    std::cout << "Sum with none: " << (sum_with_none ? std::to_string(*sum_with_none) : "None") << "\n";
    
    // Zip operations
    auto multiply_optionals = zip_with([](int a, int b) { return a * b; });
    auto product = multiply_optionals(some(6))(some(7));
    
    std::cout << "Product of 6 and 7: " << (product ? std::to_string(*product) : "None") << "\n";
}

// ============================================================================
// ERROR HANDLING AND RECOVERY EXAMPLES
// ============================================================================

void error_handling_examples() {
    std::cout << "\n=== Error Handling Examples ===\n";
    
    // Exception-safe operations
    auto risky_operation = try_optional([](const std::string& input) {
        if (input == "throw") {
            throw std::runtime_error("Intentional error");
        }
        return std::stoi(input) * 2;
    });
    
    auto safe_result1 = risky_operation("42");
    auto safe_result2 = risky_operation("throw");
    auto safe_result3 = risky_operation("abc");
    
    std::cout << "Safe operation '42': " << (safe_result1 ? std::to_string(*safe_result1) : "None") << "\n";
    std::cout << "Safe operation 'throw': " << (safe_result2 ? std::to_string(*safe_result2) : "None") << "\n";
    std::cout << "Safe operation 'abc': " << (safe_result3 ? std::to_string(*safe_result3) : "None") << "\n";
    
    // Pattern matching for different outcomes
    auto handle_result = [](const std::optional<int>& opt) {
        return match(
            [](int value) { return "Success: " + std::to_string(value); },
            []() { return std::string("Failed to process"); }
        )(opt);
    };
    
    std::cout << "Pattern match result: " << handle_result(some(42)) << "\n";
    std::cout << "Pattern match empty: " << handle_result(none<int>()) << "\n";
    
    // Fallback chains
    auto get_config_value = [](const std::string& key) -> std::optional<std::string> {
        // Simulate config lookup that might fail
        if (key == "database_url") return some(std::string("localhost:5432"));
        if (key == "api_key") return none<std::string>();
        return none<std::string>();
    };
    
    auto get_env_value = [](const std::string& key) -> std::optional<std::string> {
        // Simulate environment variable lookup
        if (key == "API_KEY") return some(std::string("env_api_key_123"));
        return none<std::string>();
    };
    
    // Try config first, then environment, then default
    auto api_key = pipe(get_config_value("api_key"))
        .then([&](const auto& opt) {
            return opt.has_value() ? opt : get_env_value("API_KEY");
        })
        .then(value_or(std::string("default_key")))
        .get();
    
    std::cout << "API key resolved: " << api_key << "\n";
}

// ============================================================================
// PERFORMANCE OPTIMIZATION EXAMPLES
// ============================================================================

void performance_examples() {
    std::cout << "\n=== Performance Examples ===\n";
    
    // Lazy evaluation for expensive operations
    auto expensive_computation = [](int x) -> std::optional<int> {
        std::cout << "Performing expensive computation for " << x << "\n";
        // Simulate expensive operation
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return x * x;
    };
    
    // Regular eager evaluation
    auto start = std::chrono::high_resolution_clock::now();
    auto eager_result = pipe(some(5))
        .then(and_then(expensive_computation))
        .then(filter([](int x) { return false; })) // This will make the computation useless
        .get();
    auto eager_time = std::chrono::high_resolution_clock::now() - start;
    
    // Lazy evaluation (computation is still performed because and_then is eager)
    // For true laziness, you'd need to implement lazy combinators
    start = std::chrono::high_resolution_clock::now();
    auto lazy_pipeline = and_then_lazy(expensive_computation);
    auto lazy_computation = lazy_pipeline(some(5));
    // Computation hasn't happened yet
    auto lazy_time_setup = std::chrono::high_resolution_clock::now() - start;
    
    start = std::chrono::high_resolution_clock::now();
    auto lazy_result = lazy_computation(); // Now computation happens
    auto lazy_time_exec = std::chrono::high_resolution_clock::now() - start;
    
    std::cout << "Eager computation took: " 
              << std::chrono::duration_cast<std::chrono::microseconds>(eager_time).count() 
              << " microseconds\n";
    std::cout << "Lazy setup took: " 
              << std::chrono::duration_cast<std::chrono::microseconds>(lazy_time_setup).count() 
              << " microseconds\n";
    std::cout << "Lazy execution took: " 
              << std::chrono::duration_cast<std::chrono::microseconds>(lazy_time_exec).count() 
              << " microseconds\n";
    
    // Move semantics optimization
    std::vector<std::string> large_strings = {
        "This is a very long string that we want to avoid copying",
        "Another long string for testing move semantics",
        "Yet another string to demonstrate efficiency"
    };
    
    auto process_strings = [](std::vector<std::string>&& strings) {
        std::vector<std::optional<size_t>> results;
        results.reserve(strings.size());
        
        for (auto&& str : strings) {
            auto result = pipe(some(std::move(str)))
                .then(validate_non_empty())
                .then(map([](const std::string& s) { return s.length(); }))
                .get();
            results.push_back(result);
        }
        return results;
    };
    
    auto string_lengths = process_strings(std::move(large_strings));
    std::cout << "Processed string lengths: ";
    for (const auto& len : string_lengths) {
        std::cout << (len ? std::to_string(*len) : "None") << " ";
    }
    std::cout << "\n";
}

// ============================================================================
// ADVANCED COMPOSITION EXAMPLES
// ============================================================================

void advanced_composition_examples() {
    std::cout << "\n=== Advanced Composition Examples ===\n";
    
    // Building reusable validation pipelines
    auto validate_username = [](const std::string& username) {
        return pipe(some(username))
            .then(validate_non_empty())
            .then(validate([](const std::string& s) { 
                return s.length() >= 3 && s.length() <= 20; 
            }, "Username must be 3-20 characters"))
            .then(validate([](const std::string& s) {
                return std::all_of(s.begin(), s.end(), [](char c) {
                    return std::isalnum(c) || c == '_' || c == '-';
                });
            }, "Username contains invalid characters"));
    };
    
    auto validate_password = [](const std::string& password) {
        return pipe(some(password))
            .then(validate([](const std::string& s) { return s.length() >= 8; }, "Password too short"))
            .then(validate([](const std::string& s) {
                return std::any_of(s.begin(), s.end(), [](char c) { return std::isupper(c); });
            }, "Password must contain uppercase letter"))
            .then(validate([](const std::string& s) {
                return std::any_of(s.begin(), s.end(), [](char c) { return std::isdigit(c); });
            }, "Password must contain digit"));
    };
    
    // Combining multiple validations
    auto validate_registration = [&](const std::string& username, const std::string& email, 
                                    const std::string& password) {
        auto valid_username = validate_username(username);
        auto valid_email = pipe(some(email)).then(validate_email()).get();
        auto valid_password = validate_password(password);
        
        return lift([](const std::string& u, const std::string& e, const std::string& p) {
            return std::make_tuple(u, e, p);
        })(valid_username, valid_email, valid_password);
    };
    
    auto registration1 = validate_registration("john_doe", "john@example.com", "SecurePass123");
    auto registration2 = validate_registration("x", "invalid-email", "weak");
    
    std::cout << "Registration 1 valid: " << (registration1.has_value() ? "Yes" : "No") << "\n";
    std::cout << "Registration 2 valid: " << (registration2.has_value() ? "Yes" : "No") << "\n";
    
    // Conditional transformations
    auto process_number = [](int value) {
        return pipe(some(value))
            .then(transform_if(
                [](int x) { return x < 0; },                    // If negative
                [](int x) { return std::abs(x); }               // Make positive
            ))
            .then(transform_if(
                [](int x) { return x > 100; },                  // If too large
                [](int x) { return 100; }                       // Cap at 100
            ))
            .then(map([](int x) { return x * 2; }));            // Always double
    };
    
    auto result1 = process_number(-50);   // |-50| * 2 = 100
    auto result2 = process_number(150);   // min(150, 100) * 2 = 200
    auto result3 = process_number(25);    // 25 * 2 = 50
    
    std::cout << "Process -50: " << (result1 ? std::to_string(*result1) : "None") << "\n";
    std::cout << "Process 150: " << (result2 ? std::to_string(*result2) : "None") << "\n";
    std::cout << "Process 25: " << (result3 ? std::to_string(*result3) : "None") << "\n";
    
    // Collecting results from multiple operations
    std::vector<std::optional<int>> optionals = {
        some(1), none<int>(), some(3), some(4), none<int>(), some(6)
    };
    
    auto collected = collect<std::vector>()(optionals[0], optionals[1], optionals[2], 
                                          optionals[3], optionals[4], optionals[5]);
    
    std::cout << "Collected values: ";
    for (int val : collected) {
        std::cout << val << " ";
    }
    std::cout << "\n";
}

// ============================================================================
// REAL-WORLD EXAMPLES
// ============================================================================

struct DatabaseConnection {
    bool connected = false;
    std::string connection_string;
};

struct ApiResponse {
    int status_code;
    std::string body;
};

void real_world_examples() {
    std::cout << "\n=== Real-World Examples ===\n";
    
    // Database connection with fallback
    auto connect_to_database = [](const std::string& connection_string) -> std::optional<DatabaseConnection> {
        if (connection_string.find("localhost") != std::string::npos) {
            return DatabaseConnection{true, connection_string};
        }
        return std::nullopt;
    };
    
    auto get_database_connection = []() {
        return pipe(some(std::string("primary_db_host:5432")))
            .then(and_then(connect_to_database))
            .then([&](const auto& opt) -> std::optional<DatabaseConnection> {
                if (opt.has_value()) return opt;
                // Fallback to localhost
                return connect_to_database("localhost:5432");
            })
            .then([](const auto& opt) -> std::optional<DatabaseConnection> {
                if (opt.has_value()) return opt;
                // Final fallback to memory database
                return DatabaseConnection{true, ":memory:"};
            });
    };
    
    auto db_conn = get_database_connection().get();
    std::cout << "Database connected: " << (db_conn->connected ? "Yes" : "No") 
              << " (" << db_conn->connection_string << ")\n";
    
    // API response processing
    auto process_api_response = [](const ApiResponse& response) {
        return pipe(some(response))
            .then(validate([](const ApiResponse& r) { return r.status_code == 200; }, "API error"))
            .then(map([](const ApiResponse& r) { return r.body; }))
            .then(validate_non_empty())
            .then(and_then(safe_parse<int>()));
    };
    
    ApiResponse success_response{200, "42"};
    ApiResponse error_response{404, "Not Found"};
    ApiResponse invalid_response{200, "invalid_number"};
    
    auto result1 = process_api_response(success_response);
    auto result2 = process_api_response(error_response);
    auto result3 = process_api_response(invalid_response);
    
    std::cout << "API Success: " << (result1 ? std::to_string(*result1) : "None") << "\n";
    std::cout << "API Error: " << (result2 ? std::to_string(*result2) : "None") << "\n";
    std::cout << "API Invalid: " << (result3 ? std::to_string(*result3) : "None") << "\n";
    
    // Configuration loading with multiple sources
    auto load_config = [](const std::string& key) {
        // Try multiple sources in order
        auto from_file = [](const std::string& k) -> std::optional<std::string> {
            if (k == "port") return some(std::string("8080"));
            return std::nullopt;
        };
        
        auto from_env = [](const std::string& k) -> std::optional<std::string> {
            if (k == "host") return some(std::string("0.0.0.0"));
            return std::nullopt;
        };
        
        auto defaults = [](const std::string& k) -> std::string {
            if (k == "port") return "3000";
            if (k == "host") return "localhost";
            return "unknown";
        };
        
        return pipe(from_file(key))
            .then([&](const auto& opt) { return opt.has_value() ? opt : from_env(key); })
            .then(value_or(defaults(key)))
            .get();
    };
    
    std::cout << "Config port: " << load_config("port") << "\n";
    std::cout << "Config host: " << load_config("host") << "\n";
    std::cout << "Config timeout: " << load_config("timeout") << "\n";
}

// ============================================================================
// MAIN FUNCTION
// ============================================================================

int main() {
    try {
        basic_examples();
        string_processing_examples();
        data_processing_examples();
        mathematical_examples();
        error_handling_examples();
        performance_examples();
        advanced_composition_examples();
        real_world_examples();
        
        std::cout << "\n=== All examples completed successfully! ===\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}

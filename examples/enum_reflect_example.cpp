#include "enum_reflect.h"
#include <iostream>
#include <cassert>
#include <sstream>

// Test enums
enum class Status {
    PENDING = 0,
    RUNNING = 1,
    COMPLETE = 2,
    ERROR = 10
};

enum class LogLevel : int {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

enum class Color : std::uint8_t {
    RED = 1,
    GREEN = 2,
    BLUE = 4,
    YELLOW = RED | GREEN,  // Note: This might not work as expected
    PURPLE = RED | BLUE
};

// Non-contiguous enum
enum class HttpStatus {
    OK = 200,
    NOT_FOUND = 404,
    INTERNAL_ERROR = 500
};

void test_basic_functionality() {
    std::cout << "=== Basic Functionality Tests ===" << std::endl;
    
    using namespace enum_reflect;
    
    // Test enum_name
    std::cout << "Status::PENDING = " << enum_name(Status::PENDING) << std::endl;
    std::cout << "Status::RUNNING = " << enum_name(Status::RUNNING) << std::endl;
    std::cout << "Status::COMPLETE = " << enum_name(Status::COMPLETE) << std::endl;
    std::cout << "Status::ERROR = " << enum_name(Status::ERROR) << std::endl;
    
    // Test from_name
    auto status = enum_from_name<Status>("RUNNING");
    if (status) {
        std::cout << "Found RUNNING: " << static_cast<int>(*status) << std::endl;
        assert(*status == Status::RUNNING);
    }
    
    auto invalid_status = enum_from_name<Status>("INVALID");
    assert(!invalid_status.has_value());
    std::cout << "Invalid enum name correctly rejected" << std::endl;
    
    // Test validation
    assert(is_valid_enum(Status::PENDING));
    assert(is_valid_enum(Status::ERROR));
    std::cout << "Validation tests passed" << std::endl;
}

void test_iteration() {
    std::cout << "\n=== Iteration Tests ===" << std::endl;
    
    using namespace enum_reflect;
    
    // Range-based for loop
    std::cout << "All Status values (range-based for):" << std::endl;
    for (auto status : enum_range<Status>()) {
        std::cout << "  " << enum_name(status) << " = " << static_cast<int>(status) << std::endl;
    }
    
    // Direct access to values array
    std::cout << "\nAll LogLevel values (direct array access):" << std::endl;
    constexpr auto log_levels = enum_values<LogLevel>();
    for (std::size_t i = 0; i < log_levels.size(); ++i) {
        std::cout << "  [" << i << "] " << enum_name(log_levels[i]) 
                  << " = " << static_cast<int>(log_levels[i]) << std::endl;
    }
    
    // Get all names
    std::cout << "\nAll LogLevel names:" << std::endl;
    constexpr auto log_names = enum_names<LogLevel>();
    for (const auto& name : log_names) {
        std::cout << "  " << name << std::endl;
    }
}

void test_compile_time_features() {
    std::cout << "\n=== Compile-Time Features ===" << std::endl;
    
    using namespace enum_reflect;
    
    // Compile-time size
    constexpr auto status_count = enum_size<Status>();
    constexpr auto log_count = enum_size<LogLevel>();
    
    std::cout << "Status enum count: " << status_count << std::endl;
    std::cout << "LogLevel enum count: " << log_count << std::endl;
    
    // Compile-time name lookup
    constexpr auto pending_name = enum_info<Status>::name<Status::PENDING>();
    std::cout << "Compile-time Status::PENDING name: " << pending_name << std::endl;
    
    // Compile-time arrays
    constexpr auto all_statuses = enum_values<Status>();
    std::cout << "First status: " << enum_name(all_statuses[0]) << std::endl;
}

void test_edge_cases() {
    std::cout << "\n=== Edge Cases ===" << std::endl;
    
    using namespace enum_reflect;
    
    // Non-contiguous enum
    std::cout << "HttpStatus values:" << std::endl;
    for (auto status : enum_range<HttpStatus>()) {
        std::cout << "  " << enum_name(status) << " = " << static_cast<int>(status) << std::endl;
    }
    
    // Different underlying types
    std::cout << "\nColor values (uint8_t underlying):" << std::endl;
    for (auto color : enum_range<Color>()) {
        std::cout << "  " << enum_name(color) << " = " << static_cast<int>(color) << std::endl;
    }
}

void test_stream_output() {
    std::cout << "\n=== Stream Output Tests ===" << std::endl;
    
    // Direct stream output
    std::cout << "Direct output: " << Status::COMPLETE << std::endl;
    std::cout << "Direct output: " << LogLevel::WARN << std::endl;
    
    // Stringstream test
    std::stringstream ss;
    ss << Status::RUNNING << " and " << LogLevel::ERROR;
    std::cout << "Stringstream result: " << ss.str() << std::endl;
}

void test_enum_info_class() {
    std::cout << "\n=== enum_info Class Tests ===" << std::endl;
    
    using StatusInfo = enum_reflect::enum_info<Status>;
    
    // Static methods
    std::cout << "StatusInfo::size(): " << StatusInfo::size() << std::endl;
    std::cout << "StatusInfo::name(Status::PENDING): " << StatusInfo::name(Status::PENDING) << std::endl;
    std::cout << "StatusInfo::to_string(Status::ERROR): " << StatusInfo::to_string(Status::ERROR) << std::endl;
    
    // from_string
    auto result = StatusInfo::from_string("COMPLETE");
    if (result) {
        std::cout << "from_string('COMPLETE'): " << StatusInfo::name(*result) << std::endl;
    }
    
    // is_valid
    std::cout << "is_valid(Status::RUNNING): " << StatusInfo::is_valid(Status::RUNNING) << std::endl;
}

void performance_test() {
    std::cout << "\n=== Performance Test ===" << std::endl;
    
    using namespace enum_reflect;
    
    const int iterations = 1000000;
    
    // Test name lookup performance
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        volatile auto name = enum_name(Status::RUNNING);
        (void)name; // Prevent optimization
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Name lookup: " << iterations << " iterations in " 
              << duration.count() << " μs" << std::endl;
    std::cout << "Average: " << (double)duration.count() / iterations << " μs per lookup" << std::endl;
}

void demonstrate_use_cases() {
    std::cout << "\n=== Use Case Demonstrations ===" << std::endl;
    
    using namespace enum_reflect;
    
    // 1. Command-line argument parsing
    auto parse_log_level = [](const std::string& arg) -> std::optional<LogLevel> {
        return enum_from_name<LogLevel>(arg);
    };
    
    std::cout << "Command-line parsing:" << std::endl;
    for (const auto& arg : {"DEBUG", "INFO", "INVALID"}) {
        auto level = parse_log_level(arg);
        if (level) {
            std::cout << "  '" << arg << "' -> " << enum_name(*level) << std::endl;
        } else {
            std::cout << "  '" << arg << "' -> INVALID" << std::endl;
        }
    }
    
    // 2. Configuration validation
    auto validate_config = [](Status status) {
        std::cout << "Validating status: " << enum_name(status);
        if (is_valid_enum(status)) {
            std::cout << " [VALID]" << std::endl;
        } else {
            std::cout << " [INVALID]" << std::endl;
        }
    };
    
    std::cout << "\nConfiguration validation:" << std::endl;
    validate_config(Status::PENDING);
    validate_config(static_cast<Status>(999)); // Invalid value
    
    // 3. Serialization helper
    auto serialize_enum = [](Status status) -> std::string {
        return std::string(enum_name(status));
    };
    
    auto deserialize_enum = [](const std::string& str) -> std::optional<Status> {
        return enum_from_name<Status>(str);
    };
    
    std::cout << "\nSerialization:" << std::endl;
    auto serialized = serialize_enum(Status::COMPLETE);
    std::cout << "  Serialized: " << serialized << std::endl;
    
    auto deserialized = deserialize_enum(serialized);
    if (deserialized) {
        std::cout << "  Deserialized: " << enum_name(*deserialized) << std::endl;
    }
}

int main() {
    std::cout << "Enum Reflection Library Test Suite" << std::endl;
    std::cout << "C++ Standard: " << __cplusplus << std::endl;
    
#ifdef ENUM_REFLECT_CPP20
    std::cout << "Using C++20 features" << std::endl;
#elif defined(ENUM_REFLECT_CPP17)
    std::cout << "Using C++17 features" << std::endl;
#endif
    
    try {
        test_basic_functionality();
        test_iteration();
        test_compile_time_features();
        test_edge_cases();
        test_stream_output();
        test_enum_info_class();
        demonstrate_use_cases();
        
        std::cout << "\n=== All Tests Passed! ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

// Compilation commands:
// C++17: g++ -std=c++17 -O2 -Wall -Wextra example.cpp -o example_cpp17
// C++20: g++ -std=c++20 -O2 -Wall -Wextra example.cpp -o example_cpp20
// Clang: clang++ -std=c++17 -O2 -Wall -Wextra example.cpp -o example_clang

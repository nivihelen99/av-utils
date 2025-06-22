#include <iostream>
#include <string>
#include <functional>
#include <cassert>

// Include the ConstDict header (would be #include "const_dict.hpp")
// For this example, assume the header content is already available

enum class LogLevel { DEBUG, INFO, WARNING, ERROR };
enum class CommandType { HELP, EXIT, SAVE, LOAD };

using CommandHandler = std::function<void()>;

void demonstrate_basic_usage() {
    std::cout << "=== Basic Usage Demo ===" << std::endl;
    
    // Create from initializer list
    ConstDict<std::string, int> weekdays = {
        {"Monday", 1},
        {"Tuesday", 2},
        {"Wednesday", 3},
        {"Thursday", 4},
        {"Friday", 5}
    };

    // Safe read-only access
    std::cout << "Wednesday is day: " << weekdays.at("Wednesday") << std::endl;
    std::cout << "Friday is day: " << weekdays["Friday"] << std::endl;
    
    // Check if key exists
    if (weekdays.contains("Saturday")) {
        std::cout << "Saturday found!" << std::endl;
    } else {
        std::cout << "Saturday not found (weekend!)" << std::endl;
    }
    
    // Size and empty checks
    std::cout << "Dictionary size: " << weekdays.size() << std::endl;
    std::cout << "Dictionary is empty: " << std::boolalpha << weekdays.empty() << std::endl;
    
    std::cout << std::endl;
}

void demonstrate_enum_usage() {
    std::cout << "=== Enum Key Usage Demo ===" << std::endl;
    
    // Log level names mapping
    ConstDict<LogLevel, std::string> levelNames = {
        {LogLevel::DEBUG, "Debug"},
        {LogLevel::INFO, "Info"},
        {LogLevel::WARNING, "Warning"},
        {LogLevel::ERROR, "Error"}
    };

    // Access using enum keys
    std::cout << "Log levels:" << std::endl;
    for (const auto& [level, name] : levelNames) {
        std::cout << "  " << static_cast<int>(level) << ": " << name << std::endl;
    }
    
    // Safe lookup
    try {
        std::cout << "ERROR level name: " << levelNames.at(LogLevel::ERROR) << std::endl;
    } catch (const std::out_of_range& e) {
        std::cout << "Level not found: " << e.what() << std::endl;
    }
    
    std::cout << std::endl;
}

void demonstrate_command_table() {
    std::cout << "=== Command Table Demo ===" << std::endl;
    
    // Command handlers
    auto help_handler = []() { std::cout << "Help: Available commands are HELP, EXIT, SAVE, LOAD" << std::endl; };
    auto exit_handler = []() { std::cout << "Exit: Goodbye!" << std::endl; };
    auto save_handler = []() { std::cout << "Save: Data saved successfully" << std::endl; };
    auto load_handler = []() { std::cout << "Load: Data loaded successfully" << std::endl; };
    
    // Create immutable command table
    ConstDict<CommandType, CommandHandler> commandTable = {
        {CommandType::HELP, help_handler},
        {CommandType::EXIT, exit_handler},
        {CommandType::SAVE, save_handler},
        {CommandType::LOAD, load_handler}
    };
    
    // Execute commands safely
    std::cout << "Executing commands:" << std::endl;
    if (commandTable.contains(CommandType::HELP)) {
        commandTable.at(CommandType::HELP)();
    }
    if (commandTable.contains(CommandType::SAVE)) {
        commandTable.at(CommandType::SAVE)();
    }
    
    std::cout << std::endl;
}

void demonstrate_construction_methods() {
    std::cout << "=== Construction Methods Demo ===" << std::endl;
    
    // Method 1: From initializer list
    auto dict1 = ConstDict<std::string, double>{
        {"pi", 3.14159},
        {"e", 2.71828},
        {"phi", 1.61803}
    };
    
    // Method 2: From existing map
    std::unordered_map<std::string, double> temp_map = {
        {"sqrt2", 1.41421},
        {"sqrt3", 1.73205}
    };
    auto dict2 = ConstDict(temp_map);  // Using deduction guide
    
    // Method 3: Using factory function
    auto dict3 = make_const_dict<std::string, double>({
        {"log2", 0.69314},
        {"log10", 2.30258}
    });
    
    std::cout << "Dict1 - Mathematical constants:" << std::endl;
    for (const auto& [name, value] : dict1) {
        std::cout << "  " << name << " = " << value << std::endl;
    }
    
    std::cout << "Dict2 - Square roots:" << std::endl;
    for (const auto& [name, value] : dict2) {
        std::cout << "  " << name << " = " << value << std::endl;
    }
    
    std::cout << "Dict3 - Logarithms:" << std::endl;
    for (const auto& [name, value] : dict3) {
        std::cout << "  " << name << " = " << value << std::endl;
    }
    
    std::cout << std::endl;
}

void demonstrate_config_usage() {
    std::cout << "=== Configuration Usage Demo ===" << std::endl;
    
    // Application configuration
    const ConstDict<std::string, std::string> config = {
        {"host", "localhost"},
        {"port", "8080"},
        {"database", "myapp.db"},
        {"log_level", "INFO"},
        {"max_connections", "100"}
    };
    
    // Safe configuration access
    std::cout << "Application Configuration:" << std::endl;
    std::cout << "  Server: " << config.at("host") << ":" << config.at("port") << std::endl;
    std::cout << "  Database: " << config.at("database") << std::endl;
    std::cout << "  Log Level: " << config.at("log_level") << std::endl;
    
    // Safe lookup with fallback
    auto get_config_value = [&](const std::string& key, const std::string& default_value) {
        auto it = config.find(key);
        return (it != config.end()) ? it->second : default_value;
    };
    
    std::cout << "  Timeout: " << get_config_value("timeout", "30") << " seconds (default)" << std::endl;
    
    std::cout << std::endl;
}

void demonstrate_comparison_and_copying() {
    std::cout << "=== Comparison and Copying Demo ===" << std::endl;
    
    auto dict1 = make_const_dict<std::string, int>({
        {"apple", 5},
        {"banana", 3},
        {"cherry", 8}
    });
    
    auto dict2 = dict1;  // Copy constructor
    auto dict3 = make_const_dict<std::string, int>({
        {"apple", 5},
        {"banana", 3},
        {"cherry", 8}
    });
    
    std::cout << "dict1 == dict2: " << std::boolalpha << (dict1 == dict2) << std::endl;
    std::cout << "dict1 == dict3: " << std::boolalpha << (dict1 == dict3) << std::endl;
    
    auto dict4 = make_const_dict<std::string, int>({
        {"apple", 10},
        {"banana", 3}
    });
    
    std::cout << "dict1 == dict4: " << std::boolalpha << (dict1 == dict4) << std::endl;
    std::cout << "dict1 != dict4: " << std::boolalpha << (dict1 != dict4) << std::endl;
    
    std::cout << std::endl;
}

void demonstrate_different_map_types() {
    std::cout << "=== Different Map Types Demo ===" << std::endl;
    
    // Using std::unordered_map (default)
    ConstUnorderedDict<std::string, int> unordered_dict = {
        {"zebra", 1},
        {"apple", 2},
        {"banana", 3}
    };
    
    // Using std::map (ordered)
    ConstOrderedDict<std::string, int> ordered_dict = {
        {"zebra", 1},
        {"apple", 2},
        {"banana", 3}
    };
    
    std::cout << "Unordered dict iteration:" << std::endl;
    for (const auto& [key, value] : unordered_dict) {
        std::cout << "  " << key << ": " << value << std::endl;
    }
    
    std::cout << "Ordered dict iteration:" << std::endl;
    for (const auto& [key, value] : ordered_dict) {
        std::cout << "  " << key << ": " << value << std::endl;
    }
    
    std::cout << std::endl;
}

void demonstrate_error_handling() {
    std::cout << "=== Error Handling Demo ===" << std::endl;
    
    ConstDict<std::string, int> small_dict = {
        {"one", 1},
        {"two", 2}
    };
    
    // Test bounds checking
    try {
        std::cout << "Accessing existing key 'one': " << small_dict.at("one") << std::endl;
        std::cout << "Accessing non-existing key 'three': " << small_dict.at("three") << std::endl;
    } catch (const std::out_of_range& e) {
        std::cout << "Caught expected exception: " << e.what() << std::endl;
    }
    
    // Test using operator[]
    try {
        std::cout << "Using operator[] with existing key 'two': " << small_dict["two"] << std::endl;
        std::cout << "Using operator[] with non-existing key 'four': " << small_dict["four"] << std::endl;
    } catch (const std::out_of_range& e) {
        std::cout << "Caught expected exception from operator[]: " << e.what() << std::endl;
    }
    
    std::cout << std::endl;
}

int main() {
    std::cout << "ConstDict - Immutable Dictionary Examples" << std::endl;
    std::cout << "=========================================" << std::endl << std::endl;
    
    demonstrate_basic_usage();
    demonstrate_enum_usage();
    demonstrate_command_table();
    demonstrate_construction_methods();
    demonstrate_config_usage();
    demonstrate_comparison_and_copying();
    demonstrate_different_map_types();
    demonstrate_error_handling();
    
    std::cout << "All demonstrations completed successfully!" << std::endl;
    
    return 0;
}

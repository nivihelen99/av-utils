#include "duration_parser.h"
#include <iostream>
#include <chrono>
#include <iomanip>

using namespace duration_parser;

void basic_examples() {
    std::cout << "=== Basic Examples ===\n";
    
    // Simple duration parsing
    auto dur1 = parse_duration<std::chrono::seconds>("30s");
    std::cout << "30s = " << dur1.count() << " seconds\n";
    
    auto dur2 = parse_duration<std::chrono::milliseconds>("1h");
    std::cout << "1h = " << dur2.count() << " milliseconds\n";
    
    auto dur3 = parse_duration<std::chrono::minutes>("90s");
    std::cout << "90s = " << dur3.count() << " minutes\n";
    
    // Compound durations
    auto dur4 = parse_duration<std::chrono::seconds>("1h30m45s");
    std::cout << "1h30m45s = " << dur4.count() << " seconds\n";
    
    auto dur5 = parse_duration<std::chrono::milliseconds>("2h15m");
    std::cout << "2h15m = " << dur5.count() << " milliseconds\n";
    
    std::cout << "\n";
}

void fractional_examples() {
    std::cout << "=== Fractional Examples ===\n";
    
    // Fractional durations
    auto dur1 = parse_duration<std::chrono::seconds>("1.5m");
    std::cout << "1.5m = " << dur1.count() << " seconds\n";
    
    auto dur2 = parse_duration<std::chrono::milliseconds>("2.75s");
    std::cout << "2.75s = " << dur2.count() << " milliseconds\n";
    
    auto dur3 = parse_duration<std::chrono::microseconds>("0.5ms");
    std::cout << "0.5ms = " << dur3.count() << " microseconds\n";
    
    std::cout << "\n";
}

void high_precision_examples() {
    std::cout << "=== High Precision Examples ===\n";
    
    // Microsecond and nanosecond precision
    auto dur1 = parse_duration<std::chrono::microseconds>("500us");
    std::cout << "500us = " << dur1.count() << " microseconds\n";
    
    auto dur2 = parse_duration<std::chrono::nanoseconds>("1500ns");
    std::cout << "1500ns = " << dur2.count() << " nanoseconds\n";
    
    auto dur3 = parse_duration<std::chrono::nanoseconds>("1ms500us250ns");
    std::cout << "1ms500us250ns = " << dur3.count() << " nanoseconds\n";
    
    std::cout << "\n";
}

void real_world_examples() {
    std::cout << "=== Real-World Use Cases ===\n";
    
    // Configuration timeouts
    std::cout << "Configuration Timeouts:\n";
    auto connection_timeout = parse_duration<std::chrono::seconds>("30s");
    auto read_timeout = parse_duration<std::chrono::milliseconds>("5s");
    auto retry_interval = parse_duration<std::chrono::seconds>("2m");
    
    std::cout << "  Connection timeout: " << connection_timeout.count() << "s\n";
    std::cout << "  Read timeout: " << read_timeout.count() << "ms\n";
    std::cout << "  Retry interval: " << retry_interval.count() << "s\n";
    
    // Performance monitoring
    std::cout << "\nPerformance Monitoring:\n";
    auto sample_rate = parse_duration<std::chrono::milliseconds>("100ms");
    auto alert_threshold = parse_duration<std::chrono::seconds>("5m");
    auto cleanup_interval = parse_duration<std::chrono::hours>("24h");
    
    std::cout << "  Sample rate: " << sample_rate.count() << "ms\n";
    std::cout << "  Alert threshold: " << alert_threshold.count() << "s\n";
    std::cout << "  Cleanup interval: " << cleanup_interval.count() << "h\n";
    
    // Scheduling
    std::cout << "\nScheduling:\n";
    auto job_interval = parse_duration<std::chrono::minutes>("15m");
    auto backup_frequency = parse_duration<std::chrono::hours>("6h");
    auto log_rotation = parse_duration<std::chrono::hours>("1h");
    
    std::cout << "  Job interval: " << job_interval.count() << " minutes\n";
    std::cout << "  Backup frequency: " << backup_frequency.count() << " hours\n";
    std::cout << "  Log rotation: " << log_rotation.count() << " hours\n";
    
    std::cout << "\n";
}

void error_handling_examples() {
    std::cout << "=== Error Handling Examples ===\n";
    
    // Invalid format handling
    std::vector<std::string> invalid_inputs = {
        "invalid",
        "20parsecs", 
        "1x2y",
        "",
        "5.5.5s",
        "abc123"
    };
    
    for (const auto& input : invalid_inputs) {
        try {
            auto dur = parse_duration<std::chrono::seconds>(input);
            std::cout << "Unexpectedly parsed: " << input << " = " << dur.count() << "s\n";
        } catch (const parse_error& e) {
            std::cout << "Failed to parse '" << input << "': " << e.what() << "\n";
        }
    }
    
    // Using parse_duration_or_default
    std::cout << "\nUsing parse_duration_or_default:\n";
    auto default_timeout = std::chrono::seconds(30);
    
    auto valid_timeout = parse_duration_or_default<std::chrono::seconds>("2m", default_timeout);
    std::cout << "Valid input '2m': " << valid_timeout.count() << "s\n";
    
    auto invalid_timeout = parse_duration_or_default<std::chrono::seconds>("invalid", default_timeout);
    std::cout << "Invalid input 'invalid': " << invalid_timeout.count() << "s (default)\n";
    
    std::cout << "\n";
}

void cli_simulation() {
    std::cout << "=== CLI Arguments Simulation ===\n";
    
    // Simulate command-line arguments
    std::vector<std::pair<std::string, std::string>> cli_args = {
        {"--sleep", "10s"},
        {"--poll-interval", "500ms"},
        {"--ttl", "2h"},
        {"--timeout", "30s"},
        {"--retry-delay", "5m"}
    };
    
    for (const auto& [flag, value] : cli_args) {
        try {
            if (flag == "--sleep") {
                auto dur = parse_duration<std::chrono::milliseconds>(value);
                std::cout << flag << "=" << value << " -> " << dur.count() << "ms\n";
            } else if (flag == "--poll-interval") {
                auto dur = parse_duration<std::chrono::microseconds>(value);
                std::cout << flag << "=" << value << " -> " << dur.count() << "μs\n";
            } else if (flag == "--ttl") {
                auto dur = parse_duration<std::chrono::seconds>(value);
                std::cout << flag << "=" << value << " -> " << dur.count() << "s\n";
            } else if (flag == "--timeout") {
                auto dur = parse_duration<std::chrono::seconds>(value);
                std::cout << flag << "=" << value << " -> " << dur.count() << "s\n";
            } else if (flag == "--retry-delay") {
                auto dur = parse_duration<std::chrono::minutes>(value);
                std::cout << flag << "=" << value << " -> " << dur.count() << " minutes\n";
            }
        } catch (const parse_error& e) {
            std::cout << "Error parsing " << flag << "=" << value << ": " << e.what() << "\n";
        }
    }
    
    std::cout << "\n";
}

void conversion_examples() {
    std::cout << "=== Unit Conversion Examples ===\n";
    
    // Same duration in different units
    std::string duration_str = "1h30m";
    
    auto in_hours = parse_duration<std::chrono::hours>(duration_str);
    auto in_minutes = parse_duration<std::chrono::minutes>(duration_str);
    auto in_seconds = parse_duration<std::chrono::seconds>(duration_str);
    auto in_milliseconds = parse_duration<std::chrono::milliseconds>(duration_str);
    
    std::cout << "'" << duration_str << "' converted to different units:\n";
    std::cout << "  Hours: " << in_hours.count() << "h\n";
    std::cout << "  Minutes: " << in_minutes.count() << "m\n";
    std::cout << "  Seconds: " << in_seconds.count() << "s\n";
    std::cout << "  Milliseconds: " << in_milliseconds.count() << "ms\n";
    
    std::cout << "\n";
}

void performance_showcase() {
    std::cout << "=== Performance Showcase ===\n";
    
    // Show that we can parse complex duration strings efficiently
    std::vector<std::string> complex_durations = {
        "2h30m45s500ms",
        "1h15m30s250ms100us",
        "72h59m59s999ms",
        "0.5h2.25m3.75s",
        "100ms500us750ns"
    };
    
    for (const auto& duration_str : complex_durations) {
        auto start = std::chrono::high_resolution_clock::now();
        auto parsed = parse_duration<std::chrono::nanoseconds>(duration_str);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto parse_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "'" << duration_str << "' -> " 
                  << parsed.count() << "ns (parsed in " 
                  << parse_time.count() << "μs)\n";
    }
    
    std::cout << "\n";
}

int main() {
    std::cout << std::fixed << std::setprecision(2);
    
    basic_examples();
    fractional_examples();
    high_precision_examples();
    real_world_examples();
    error_handling_examples();
    cli_simulation();
    conversion_examples();
    performance_showcase();
    
    std::cout << "All examples completed successfully!\n";
    
    return 0;
}

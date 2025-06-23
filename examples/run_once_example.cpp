#include "run_once.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <string>
#include <cassert>

// Example 1: Global initialization
static RunOnce global_init;

void initialize_system() {
    global_init([] {
        std::cout << "Initializing system resources...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "System initialized!\n";
    });
}

// Example 2: Singleton-like pattern with deferred initialization
class DatabaseConnection {
private:
    static RunOnce init_once_;
    static bool initialized_;

public:
    static void ensure_initialized() {
        init_once_([] {
            std::cout << "Connecting to database...\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            initialized_ = true;
            std::cout << "Database connected!\n";
        });
    }
    
    static bool is_ready() {
        return initialized_;
    }
};

RunOnce DatabaseConnection::init_once_;
bool DatabaseConnection::initialized_ = false;

// Example 3: Per-instance initialization
class Logger {
private:
    RunOnce setup_once_;
    std::string name_;
    bool configured_ = false;

public:
    explicit Logger(std::string name) : name_(std::move(name)) {}
    
    void log(const std::string& message) {
        setup_once_([this] {
            std::cout << "Setting up logger: " << name_ << "\n";
            configured_ = true;
        });
        
        if (configured_) {
            std::cout << "[" << name_ << "] " << message << "\n";
        }
    }
    
    bool is_configured() const {
        return setup_once_.has_run();
    }
};

// Example 4: Using RunOnceReturn for expensive computations
RunOnceReturn<std::string> expensive_config;

const std::string& get_config() {
    return expensive_config([] {
        std::cout << "Loading expensive configuration...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return std::string("config_value_12345");
    });
}

// Example 5: Exception handling demonstration
void demonstrate_exception_handling() {
    std::cout << "\nTesting exception handling:\n";
    
    RunOnce error_prone;
    int attempt = 0;
    
    for (int i = 0; i < 3; ++i) {
        try {
            error_prone([&attempt] {
                ++attempt;
                std::cout << "Attempt #" << attempt << "\n";
                if (attempt < 3) {
                    throw std::runtime_error("Simulated failure");
                }
                std::cout << "Finally succeeded!\n";
            });
        } catch (const std::exception& e) {
            std::cout << "Caught: " << e.what() << "\n";
        }
        
        std::cout << "Has run: " << error_prone.has_run() << "\n";
    }
}

// Example 6: Thread safety demonstration
void demonstrate_thread_safety() {
    std::cout << "\nTesting thread safety:\n";
    
    RunOnce thread_safe_init;
    std::atomic<int> counter{0};
    
    auto worker = [&](int id) {
        thread_safe_init([&counter, id] {
            std::cout << "Thread " << id << " is doing the work\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            counter++;
            std::cout << "Thread " << id << " finished the work\n";
        });
        std::cout << "Thread " << id << " completed (counter: " << counter.load() << ")\n";
    };
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(worker, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "Final counter value: " << counter.load() << " (should be 1)\n";
    assert(counter.load() == 1);
}

// Example 7: Using with different callable types
void demonstrate_callable_types() {
    std::cout << "\nTesting different callable types:\n";
    
    // Lambda
    RunOnce lambda_test;
    lambda_test([] { std::cout << "Lambda executed\n"; });
    
    // Function pointer
    RunOnce function_test;
    function_test([]() { std::cout << "Function pointer executed\n"; });
    
    // Functor
    struct Functor {
        void operator()() const { std::cout << "Functor executed\n"; }
    };
    RunOnce functor_test;
    functor_test(Functor{});
    
    // Capturing lambda
    int value = 42;
    RunOnce capture_test;
    capture_test([value] { std::cout << "Captured value: " << value << "\n"; });
}

int main() {
    std::cout << "=== RunOnce Utility Examples ===\n\n";
    
    // Basic usage
    std::cout << "1. Global initialization:\n";
    initialize_system();
    initialize_system();  // Should not print again
    initialize_system();  // Should not print again
    
    std::cout << "\n2. Database connection:\n";
    DatabaseConnection::ensure_initialized();
    DatabaseConnection::ensure_initialized();  // Should not print again
    std::cout << "Database ready: " << DatabaseConnection::is_ready() << "\n";
    
    std::cout << "\n3. Per-instance logger:\n";
    Logger logger1("APP");
    Logger logger2("DB");
    
    logger1.log("First message");
    logger1.log("Second message");  // Setup should not run again
    logger2.log("Database message");  // Different instance, setup runs
    
    std::cout << "\n4. Expensive configuration:\n";
    std::cout << "Config 1: " << get_config() << "\n";
    std::cout << "Config 2: " << get_config() << "\n";  // Should be cached
    
    // Advanced examples
    demonstrate_exception_handling();
    demonstrate_thread_safety();
    demonstrate_callable_types();
    
    std::cout << "\nAll examples completed successfully!\n";
    return 0;
}

#include "context_mgr.h"
#include <iostream>
#include <chrono>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <thread>

// Global variables for demonstration
bool g_verbose = false;
int g_indent_level = 0;

void print_separator(const std::string& title) {
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << " " << title << "\n";
    std::cout << std::string(50, '=') << "\n";
}

void example_scoped_timer() {
    print_separator("Scoped Timer Example");
    
    auto start = std::chrono::high_resolution_clock::now();
    auto timer = context::make_scope_exit([=] {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "⏱️  Elapsed time: " << duration.count() << " microseconds\n";
    });
    
    std::cout << "Performing some work...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "Work completed!\n";
    
    // Timer will automatically print elapsed time when scope exits
}

void example_scoped_logging() {
    print_separator("Scoped Logging Example");
    
    auto log_scope = context::make_context(
        [] { std::cout << "🔍 Entering critical section\n"; },
        [] { std::cout << "✅ Exiting critical section\n"; }
    );
    
    std::cout << "Doing important work inside critical section...\n";
    std::cout << "More important work...\n";
    
    // Enter and exit messages will be printed automatically
}

void example_variable_override() {
    print_separator("Variable Override Example");
    
    std::cout << "g_verbose before: " << std::boolalpha << g_verbose << "\n";
    
    {
        auto verbose_override = context::make_override(g_verbose, true);
        std::cout << "g_verbose inside scope: " << std::boolalpha << g_verbose << "\n";
        
        if (g_verbose) {
            std::cout << "This message prints because verbose is true!\n";
        }
    }
    
    std::cout << "g_verbose after: " << std::boolalpha << g_verbose << "\n";
}

void example_file_handling() {
    print_separator("File Handling Example");
    
    const char* filename = "test_output.txt";
    
    // Traditional C-style file handling with automatic cleanup
    FILE* file = fopen(filename, "w");
    if (!file) {
        std::cerr << "Failed to open file\n";
        return;
    }
    
    auto file_guard = context::make_scope_exit([=] {
        fclose(file);
        std::cout << "📁 File closed automatically\n";
    });
    
    fprintf(file, "Hello from context manager!\n");
    fprintf(file, "This file will be closed automatically.\n");
    
    std::cout << "File operations completed\n";
    
    // File will be closed automatically when scope exits
}

void example_indent_manager() {
    print_separator("Indent Manager Example");
    
    auto print_with_indent = [](const std::string& msg) {
        for (int i = 0; i < g_indent_level; ++i) {
            std::cout << "  ";
        }
        std::cout << msg << "\n";
    };
    
    print_with_indent("Root level");
    
    {
        auto indent_guard = context::make_context(
            [] { 
                ++g_indent_level; 
                std::cout << "📝 Increased indent level to " << g_indent_level << "\n";
            },
            [] { 
                --g_indent_level; 
                std::cout << "📝 Decreased indent level to " << g_indent_level << "\n";
            }
        );
        
        print_with_indent("Level 1");
        
        {
            auto inner_indent = context::make_context(
                [] { ++g_indent_level; },
                [] { --g_indent_level; }
            );
            
            print_with_indent("Level 2");
            print_with_indent("Still level 2");
        }
        
        print_with_indent("Back to level 1");
    }
    
    print_with_indent("Back to root level");
}

void example_cancellation() {
    print_separator("Cancellation Example");
    
    auto cleanup = context::make_scope_exit([] {
        std::cout << "❌ This should NOT print - cleanup was cancelled\n";
    });
    
    std::cout << "Cleanup is active: " << std::boolalpha << cleanup.is_active() << "\n";
    
    // Cancel the cleanup
    cleanup.dismiss();
    
    std::cout << "Cleanup is active after dismiss: " << std::boolalpha << cleanup.is_active() << "\n";
    std::cout << "✅ Cleanup was successfully cancelled\n";
}

void example_exception_safety() {
    print_separator("Exception Safety Example");
    
    try {
        auto cleanup = context::make_scope_exit([] {
            std::cout << "🛡️  Exception-safe cleanup executed\n";
        });
        
        std::cout << "About to throw an exception...\n";
        throw std::runtime_error("Test exception");
        
    } catch (const std::exception& e) {
        std::cout << "Caught exception: " << e.what() << "\n";
        std::cout << "Notice that cleanup still ran!\n";
    }
}

void example_resource_acquisition() {
    print_separator("Resource Acquisition Example");
    
    // Simulate acquiring multiple resources
    std::vector<std::unique_ptr<int>> resources;
    
    auto resource_manager = context::make_context(
        [&resources] {
            std::cout << "🔄 Acquiring resources...\n";
            resources.push_back(std::make_unique<int>(1));
            resources.push_back(std::make_unique<int>(2));
            resources.push_back(std::make_unique<int>(3));
            std::cout << "✅ Acquired " << resources.size() << " resources\n";
        },
        [&resources] {
            std::cout << "🧹 Releasing " << resources.size() << " resources...\n";
            resources.clear();
            std::cout << "✅ All resources released\n";
        }
    );
    
    std::cout << "Using resources...\n";
    for (const auto& resource : resources) {
        std::cout << "  Resource value: " << *resource << "\n";
    }
}

void example_macro_usage() {
    print_separator("Macro Usage Example");
    
    std::cout << "Before scope exit\n";
    
    SCOPE_EXIT({
        std::cout << "🎯 Macro-based cleanup executed!\n";
    });
    
    std::cout << "Inside scope\n";
    std::cout << "About to exit scope...\n";
    
    // Cleanup will run automatically
}

void example_named_scope() {
    print_separator("Named Scope Example");
    
    {
        context::NamedScope scope("Database Transaction");
        std::cout << "Performing database operations...\n";
        std::cout << "Committing transaction...\n";
    }
    
    std::cout << "\n";
    
    {
        context::NamedScope outer_scope("Outer Process");
        std::cout << "Starting outer process...\n";
        
        {
            context::NamedScope inner_scope("Inner Process");
            std::cout << "Performing inner process work...\n";
        }
        
        std::cout << "Continuing outer process...\n";
    }
}

void example_complex_scenario() {
    print_separator("Complex Scenario Example");
    
    // Simulate a complex operation with multiple scoped resources
    bool transaction_active = false;
    bool lock_held = false;
    
    auto transaction_scope = context::make_context(
        [&transaction_active] {
            transaction_active = true;
            std::cout << "🔄 Transaction started\n";
        },
        [&transaction_active] {
            if (transaction_active) {
                std::cout << "💾 Transaction committed\n";
                transaction_active = false;
            }
        }
    );
    
    auto lock_scope = context::make_context(
        [&lock_held] {
            lock_held = true;
            std::cout << "🔒 Lock acquired\n";
        },
        [&lock_held] {
            if (lock_held) {
                std::cout << "🔓 Lock released\n";
                lock_held = false;
            }
        }
    );
    
    auto timer = context::make_scope_exit([start = std::chrono::high_resolution_clock::now()] {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "⏱️  Total operation time: " << duration.count() << " ms\n";
    });
    
    std::cout << "Performing complex database operation...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    std::cout << "Operation completed successfully!\n";
    
    // All resources will be cleaned up automatically in reverse order
}

int main() {
    std::cout << "🚀 Context Manager Examples\n";
    std::cout << "C++17 Header-only Implementation\n";
    
    example_scoped_timer();
    example_scoped_logging();
    example_variable_override();
    example_file_handling();
    example_indent_manager();
    example_cancellation();
    example_exception_safety();
    example_resource_acquisition();
    example_macro_usage();
    example_named_scope();
    example_complex_scenario();
    
    std::cout << "\n🎉 All examples completed successfully!\n";
    
    return 0;
}

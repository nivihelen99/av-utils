#include "scoped_timer.h" // Assuming scoped_timer.h will be in an include path
#include <iostream>
#include <sstream> // For std::ostringstream
#include <thread>  // For std::this_thread::sleep_for
#include <vector>  // For potential complex examples later

// Original ScopedTimerExamples content, adapted

// Example integration with existing logging system (as it was in the header)
class Logger {
public:
    static void log_performance(const std::string& operation, std::chrono::microseconds duration) {
        std::cout << "[PERF] " << operation << ": " << duration.count() << " µs" << std::endl;
    }
};

void demonstrate_basic_usage() {
    std::cout << "=== ScopedTimer Basic Usage Demo ===\n";

    // Basic usage with label
    {
        ScopedTimer timer("basic operation");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Anonymous timer
    {
        ScopedTimer timer; // Will use default label or be optimized if no label is used in output
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    // Custom callback
    {
        ScopedTimer timer("custom callback", [](const std::string& label, auto duration) {
            std::cout << "CUSTOM: " << label << " took " << duration.count() << " microseconds\n";
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }

    // Using with custom stream
    std::ostringstream oss;
    {
        ScopedTimer timer("stream output", oss);
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }
    std::cout << "Stream captured: " << oss.str(); // Note: oss.str() might not have a newline, ScopedTimer adds it.

    // Using convenience macros
    {
        SCOPED_TIMER("macro usage");
        std::this_thread::sleep_for(std::chrono::milliseconds(12));
    }
     {
        SCOPED_TIMER_AUTO();
        std::this_thread::sleep_for(std::chrono::milliseconds(7));
    }

    // Reset functionality
    {
        ScopedTimer timer("reset demo");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        // Note: The original example printed intermediate elapsed time.
        // We can add that back or test it separately. For this example, let's keep it simple.
        std::cout << "[Demo] Intermediate elapsed for 'reset demo': " << timer.elapsed().count() << " µs\n";
        timer.reset();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    } // Timer goes out of scope and prints for the time after reset
}

void demonstrate_integration() {
    std::cout << "\n=== ScopedTimer Integration Demo ===\n";
    ScopedTimer timer("database query", [](const std::string& label, auto dur) {
        Logger::log_performance(label, dur);
    });

    // Simulate database work
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

int main() {
    demonstrate_basic_usage();
    demonstrate_integration();
    return 0;
}

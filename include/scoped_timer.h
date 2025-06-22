#pragma once

#include <chrono>
#include <functional>
#include <iostream>
#include <string>

/**
 * @brief RAII-based automatic timer that measures elapsed time in a scope
 * 
 * ScopedTimer starts timing on construction and reports the elapsed time
 * on destruction. It's designed to be lightweight, header-only, and
 * exception-safe for profiling code blocks.
 * 
 * Example usage:
 * @code
 * {
 *     ScopedTimer timer("my operation");
 *     expensive_computation();
 * } // Automatically prints: "[ScopedTimer] my operation: 1234 µs"
 * @endcode
 */
class ScopedTimer {
public:
    /// Callback function type for custom output handling
    using Callback = std::function<void(const std::string& label, std::chrono::microseconds duration)>;

    /**
     * @brief Construct anonymous timer with default output to std::cout
     */
    ScopedTimer() 
        : ScopedTimer("") {}

    /**
     * @brief Construct timer with label, output to std::cout
     * @param label Descriptive name for this timer
     */
    explicit ScopedTimer(std::string label) 
        : ScopedTimer(std::move(label), std::cout) {}

    /**
     * @brief Construct timer with label and custom output stream
     * @param label Descriptive name for this timer
     * @param out Output stream for timer results
     */
    ScopedTimer(std::string label, std::ostream& out)
        : start_(std::chrono::steady_clock::now())
        , label_(std::move(label))
        , callback_([&out](const std::string& lbl, std::chrono::microseconds dur) {
            out << "[ScopedTimer] " << lbl << ": " << dur.count() << " µs\n";
        }) {}

    /**
     * @brief Construct timer with label and custom callback
     * @param label Descriptive name for this timer
     * @param callback Custom function to handle timing results
     */
    ScopedTimer(std::string label, Callback callback)
        : start_(std::chrono::steady_clock::now())
        , label_(std::move(label))
        , callback_(std::move(callback)) {}

    /**
     * @brief Destructor - computes elapsed time and reports it
     */
    ~ScopedTimer() noexcept {
        try {
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
            
            if (callback_) {
                callback_(label_, duration);
            }
        } catch (...) {
            // Swallow exceptions in destructor to maintain exception safety
            // In a production environment, you might want to log this
        }
    }

    /**
     * @brief Reset the timer - restart timing from now
     * @note This is an optional feature for mid-block timing restart
     */
    void reset() noexcept {
        start_ = std::chrono::steady_clock::now();
    }

    /**
     * @brief Get current elapsed time without stopping the timer
     * @return Elapsed microseconds since construction or last reset
     */
    std::chrono::microseconds elapsed() const noexcept {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(now - start_);
    }

    // Non-copyable and non-movable to prevent timing confusion
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;
    ScopedTimer(ScopedTimer&&) = delete;
    ScopedTimer& operator=(ScopedTimer&&) = delete;

private:
    std::chrono::steady_clock::time_point start_;
    std::string label_;
    Callback callback_;
};

/**
 * @brief Convenience macro for creating a scoped timer with automatic variable naming
 * 
 * Usage: SCOPED_TIMER("operation name");
 */
#define SCOPED_TIMER(label) ScopedTimer _scoped_timer_##__LINE__(label)

/**
 * @brief Convenience macro for anonymous scoped timer
 * 
 * Usage: SCOPED_TIMER_AUTO();
 */
#define SCOPED_TIMER_AUTO() ScopedTimer _scoped_timer_##__LINE__()

// Example usage and demonstration
namespace ScopedTimerExamples {

inline void demonstrate_basic_usage() {
    std::cout << "=== ScopedTimer Demo ===\n";
    
    // Basic usage with label
    {
        ScopedTimer timer("basic operation");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Anonymous timer
    {
        ScopedTimer timer;
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
    std::cout << "Stream captured: " << oss.str();
    
    // Using convenience macros
    {
        SCOPED_TIMER("macro usage");
        std::this_thread::sleep_for(std::chrono::milliseconds(12));
    }
    
    // Reset functionality
    {
        ScopedTimer timer("reset demo");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        std::cout << "Intermediate elapsed: " << timer.elapsed().count() << " µs\n";
        timer.reset();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

// Example integration with existing logging system
class Logger {
public:
    static void log_performance(const std::string& operation, std::chrono::microseconds duration) {
        std::cout << "[PERF] " << operation << ": " << duration.count() << " µs\n";
    }
};

inline void demonstrate_integration() {
    ScopedTimer timer("database query", [](const std::string& label, auto dur) {
        Logger::log_performance(label, dur);
    });
    
    // Simulate database work
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

} // namespace ScopedTimerExamples

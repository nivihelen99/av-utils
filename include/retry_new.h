#pragma once

#include <functional>
#include <chrono>
#include <thread>
#include <exception>
#include <type_traits>
#include <stdexcept>

namespace retry_util {

namespace detail {
    // Helper to detect if type is void
    template<typename T>
    struct is_void_type : std::false_type {};
    
    template<>
    struct is_void_type<void> : std::true_type {};
}

// Forward declaration
template<typename Func, typename = void>
class Retriable;

// Specialization for non-void return types
template<typename Func>
class Retriable<Func, std::enable_if_t<!std::is_void_v<std::invoke_result_t<Func>>>> {
public:
    using ReturnType = std::invoke_result_t<Func>;
    
private:
    Func fn_;
    std::size_t max_retries_ = 3;
    std::chrono::milliseconds delay_ = std::chrono::milliseconds(0);
    std::function<bool(const std::exception&)> exception_handler_ = nullptr;
    std::function<bool(const ReturnType&)> value_predicate_ = nullptr;
    std::function<void(std::size_t, const std::exception*)> retry_callback_ = nullptr;
    double backoff_factor_ = 1.0;
    std::chrono::milliseconds max_timeout_ = std::chrono::milliseconds(0);
    
public:
    explicit Retriable(Func fn) : fn_(std::move(fn)) {}
    
    Retriable& times(std::size_t n) {
        max_retries_ = n;
        return *this;
    }
    
    Retriable& with_delay(std::chrono::milliseconds delay) {
        delay_ = delay;
        return *this;
    }
    
    Retriable& with_backoff(double factor) {
        backoff_factor_ = factor;
        return *this;
    }
    
    Retriable& timeout(std::chrono::milliseconds max_timeout) {
        max_timeout_ = max_timeout;
        return *this;
    }
    
    template<typename Pred>
    Retriable& until(Pred pred) {
        value_predicate_ = pred;
        return *this;
    }
    
    template<typename ExceptionPred>
    Retriable& on_exception(ExceptionPred handler) {
        exception_handler_ = handler;
        return *this;
    }
    
    template<typename ExceptionType>
    Retriable& on_exception() {
        exception_handler_ = [](const std::exception& e) {
            return dynamic_cast<const ExceptionType*>(&e) != nullptr;
        };
        return *this;
    }
    
    template<typename Callback>
    Retriable& on_retry(Callback callback) {
        retry_callback_ = callback;
        return *this;
    }
    
    ReturnType run() {
        auto start_time = std::chrono::steady_clock::now();
        std::exception_ptr last_exception = nullptr;
        
        for (std::size_t attempt = 0; attempt < max_retries_; ++attempt) {
            // Check timeout
            if (max_timeout_.count() > 0) {
                auto elapsed = std::chrono::steady_clock::now() - start_time;
                if (elapsed >= max_timeout_) {
                    throw std::runtime_error("Retry timeout exceeded");
                }
            }
            
            try {
                auto result = fn_();
                
                // Check if result satisfies the success condition
                if (!value_predicate_ || value_predicate_(result)) {
                    return result; // Success
                }
                
                // Result doesn't satisfy condition, will retry if more attempts available
                if (attempt < max_retries_ - 1) {
                    if (retry_callback_) {
                        retry_callback_(attempt + 1, nullptr);
                    }
                    sleep_with_backoff(attempt);
                }
            } catch (const std::exception& e) {
                last_exception = std::current_exception();
                
                // Check if we should retry on this exception
                if (exception_handler_ && exception_handler_(e)) {
                    if (attempt < max_retries_ - 1) {
                        if (retry_callback_) {
                            retry_callback_(attempt + 1, &e);
                        }
                        sleep_with_backoff(attempt);
                        continue;
                    }
                }
                
                // Don't retry on this exception, or max attempts reached
                std::rethrow_exception(last_exception);
            } catch (...) {
                // Unknown exception, don't retry unless we have a generic handler
                if (exception_handler_) {
                    std::runtime_error generic_error("Unknown exception");
                    if (exception_handler_(generic_error)) {
                        if (attempt < max_retries_ - 1) {
                            if (retry_callback_) {
                                retry_callback_(attempt + 1, &generic_error);
                            }
                            sleep_with_backoff(attempt);
                            continue;
                        }
                    }
                }
                throw;
            }
        }
        
        // All attempts exhausted
        if (last_exception) {
            std::rethrow_exception(last_exception);
        } else {
            throw std::runtime_error("Retry failed: condition not met after all attempts");
        }
    }
    
    ReturnType operator()() {
        return run();
    }
    
private:
    void sleep_with_backoff(std::size_t attempt) {
        if (delay_.count() > 0) {
            auto current_delay = delay_;
            if (backoff_factor_ > 1.0) {
                double multiplier = 1.0;
                for (std::size_t i = 0; i < attempt; ++i) {
                    multiplier *= backoff_factor_;
                }
                current_delay = std::chrono::milliseconds(
                    static_cast<long long>(delay_.count() * multiplier)
                );
            }
            std::this_thread::sleep_for(current_delay);
        }
    }
};

// Specialization for void return types
template<typename Func>
class Retriable<Func, std::enable_if_t<std::is_void_v<std::invoke_result_t<Func>>>> {
public:
    using ReturnType = std::invoke_result_t<Func>;
    
private:
    Func fn_;
    std::size_t max_retries_ = 3;
    std::chrono::milliseconds delay_ = std::chrono::milliseconds(0);
    std::function<bool(const std::exception&)> exception_handler_ = nullptr;
    std::function<void(std::size_t, const std::exception*)> retry_callback_ = nullptr;
    double backoff_factor_ = 1.0;
    std::chrono::milliseconds max_timeout_ = std::chrono::milliseconds(0);
    
public:
    explicit Retriable(Func fn) : fn_(std::move(fn)) {}
    
    Retriable& times(std::size_t n) {
        max_retries_ = n;
        return *this;
    }
    
    Retriable& with_delay(std::chrono::milliseconds delay) {
        delay_ = delay;
        return *this;
    }
    
    Retriable& with_backoff(double factor) {
        backoff_factor_ = factor;
        return *this;
    }
    
    Retriable& timeout(std::chrono::milliseconds max_timeout) {
        max_timeout_ = max_timeout;
        return *this;
    }
    
    // Note: No until() method for void functions
    
    template<typename ExceptionPred>
    Retriable& on_exception(ExceptionPred handler) {
        exception_handler_ = handler;
        return *this;
    }
    
    template<typename ExceptionType>
    Retriable& on_exception() {
        exception_handler_ = [](const std::exception& e) {
            return dynamic_cast<const ExceptionType*>(&e) != nullptr;
        };
        return *this;
    }
    
    template<typename Callback>
    Retriable& on_retry(Callback callback) {
        retry_callback_ = callback;
        return *this;
    }
    
    void run() {
        auto start_time = std::chrono::steady_clock::now();
        std::exception_ptr last_exception = nullptr;
        
        for (std::size_t attempt = 0; attempt < max_retries_; ++attempt) {
            // Check timeout
            if (max_timeout_.count() > 0) {
                auto elapsed = std::chrono::steady_clock::now() - start_time;
                if (elapsed >= max_timeout_) {
                    throw std::runtime_error("Retry timeout exceeded");
                }
            }
            
            try {
                fn_();
                return; // Success for void functions
            } catch (const std::exception& e) {
                last_exception = std::current_exception();
                
                // Check if we should retry on this exception
                if (exception_handler_ && exception_handler_(e)) {
                    if (attempt < max_retries_ - 1) {
                        if (retry_callback_) {
                            retry_callback_(attempt + 1, &e);
                        }
                        sleep_with_backoff(attempt);
                        continue;
                    }
                }
                
                // Don't retry on this exception, or max attempts reached
                std::rethrow_exception(last_exception);
            } catch (...) {
                // Unknown exception, don't retry unless we have a generic handler
                if (exception_handler_) {
                    std::runtime_error generic_error("Unknown exception");
                    if (exception_handler_(generic_error)) {
                        if (attempt < max_retries_ - 1) {
                            if (retry_callback_) {
                                retry_callback_(attempt + 1, &generic_error);
                            }
                            sleep_with_backoff(attempt);
                            continue;
                        }
                    }
                }
                throw;
            }
        }
        
        // All attempts exhausted
        if (last_exception) {
            std::rethrow_exception(last_exception);
        } else {
            throw std::runtime_error("Retry failed: all attempts exhausted");
        }
    }
    
    void operator()() {
        run();
    }
    
private:
    void sleep_with_backoff(std::size_t attempt) {
        if (delay_.count() > 0) {
            auto current_delay = delay_;
            if (backoff_factor_ > 1.0) {
                double multiplier = 1.0;
                for (std::size_t i = 0; i < attempt; ++i) {
                    multiplier *= backoff_factor_;
                }
                current_delay = std::chrono::milliseconds(
                    static_cast<long long>(delay_.count() * multiplier)
                );
            }
            std::this_thread::sleep_for(current_delay);
        }
    }
};

// Factory function for creating Retriable instances
template<typename Func>
auto retry(Func&& fn) -> Retriable<std::decay_t<Func>> {
    return Retriable<std::decay_t<Func>>(std::forward<Func>(fn));
}

// Convenience class for common retry scenarios
class RetryBuilder {
public:
    template<typename Func>
    static auto simple(Func&& fn, std::size_t times = 3, 
                      std::chrono::milliseconds delay = std::chrono::milliseconds(100)) {
        return retry(std::forward<Func>(fn)).times(times).with_delay(delay);
    }
    
    template<typename Func>
    static auto with_backoff(Func&& fn, std::size_t times = 3, 
                           std::chrono::milliseconds initial_delay = std::chrono::milliseconds(100),
                           double factor = 2.0) {
        return retry(std::forward<Func>(fn))
               .times(times)
               .with_delay(initial_delay)
               .with_backoff(factor);
    }
    
    template<typename Func, typename ExceptionType>
    static auto on_exception(Func&& fn, std::size_t times = 3,
                           std::chrono::milliseconds delay = std::chrono::milliseconds(100)) {
        return retry(std::forward<Func>(fn))
               .times(times)
               .with_delay(delay)
               .template on_exception<ExceptionType>();
    }
};

} // namespace retry_util

// Example usage and test functions
#ifdef RETRY_UTIL_EXAMPLES

#include <iostream>
#include <random>

namespace retry_util::examples {

// Example: Flaky function that fails a few times before succeeding
int flaky_function() {
    static int call_count = 0;
    ++call_count;
    
    if (call_count < 3) {
        throw std::runtime_error("Temporary failure #" + std::to_string(call_count));
    }
    
    return 42;
}

// Example: Function that returns success/failure
bool unreliable_ping() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1, 10);
    
    return dis(gen) > 7; // 30% chance of success
}

// Example: Network connection simulation
void connect_to_server() {
    static int attempts = 0;
    ++attempts;
    
    if (attempts < 2) {
        throw std::runtime_error("Connection failed");
    }
    
    std::cout << "Connected successfully!\n";
}

void run_examples() {
    std::cout << "=== Retry Utility Examples ===\n\n";
    
    // Example 1: Basic retry with delay
    std::cout << "1. Basic retry with delay:\n";
    try {
        auto result = retry(flaky_function)
                        .times(5)
                        .with_delay(std::chrono::milliseconds(50))
                        .run();
        std::cout << "Result: " << result << "\n\n";
    } catch (const std::exception& e) {
        std::cout << "Failed: " << e.what() << "\n\n";
    }
    
    // Example 2: Retry until condition is met
    std::cout << "2. Retry until condition is met:\n";
    try {
        auto success = retry(unreliable_ping)
                         .times(10)
                         .with_delay(std::chrono::milliseconds(100))
                         .until([](bool result) { return result; })
                         .on_retry([](std::size_t attempt, const std::exception*) {
                             std::cout << "  Attempt " << attempt << " failed, retrying...\n";
                         })
                         .run();
        std::cout << "Ping successful: " << std::boolalpha << success << "\n\n";
    } catch (const std::exception& e) {
        std::cout << "Failed: " << e.what() << "\n\n";
    }
    
    // Example 3: Exponential backoff with void function
    std::cout << "3. Exponential backoff:\n";
    try {
        retry(connect_to_server)
            .times(4)
            .with_delay(std::chrono::milliseconds(50))
            .with_backoff(2.0)
            .on_retry([](std::size_t attempt, const std::exception* e) {
                if (e) {
                    std::cout << "  Attempt " << attempt << " failed: " << e->what() << "\n";
                }
            })
            .run();
    } catch (const std::exception& e) {
        std::cout << "Failed: " << e.what() << "\n\n";
    }
    
    // Example 4: Using RetryBuilder for common patterns
    std::cout << "4. Using RetryBuilder:\n";
    try {
        auto result = RetryBuilder::simple([]() { return 123; }, 3, std::chrono::milliseconds(10))
                        .run();
        std::cout << "Simple retry result: " << result << "\n";
    } catch (const std::exception& e) {
        std::cout << "Failed: " << e.what() << "\n";
    }
}

} // namespace retry_util::examples

#endif // RETRY_UTIL_EXAMPLES

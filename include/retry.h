#pragma once

#include <functional>
#include <chrono>
#include <thread>
#include <exception>
#include <type_traits>
#include <stdexcept>
#include <random> // For jitter

namespace retry_util {

template<typename Func>
class Retriable {
public:
    using ReturnType = std::invoke_result_t<Func>;
    
private:
    Func fn_;
    std::size_t max_retries_ = 3;
    std::chrono::milliseconds delay_ = std::chrono::milliseconds(0);
    std::function<bool(const std::exception&)> exception_handler_ = nullptr;

    // Conditional value_predicate_
    template<typename T, typename = void>
    struct ValuePredicateMember {
        // No member if T is void
    };

    template<typename T>
    struct ValuePredicateMember<T, std::enable_if_t<!std::is_void_v<T>>> {
        std::function<bool(const T&)> member = nullptr;
    };
    ValuePredicateMember<ReturnType> value_predicate_helper_;

    std::function<void(std::size_t, const std::exception*)> retry_callback_ = nullptr;
    double backoff_factor_ = 1.0;
    std::chrono::milliseconds max_timeout_ = std::chrono::milliseconds(0);
    bool jitter_ = false;
    double jitter_factor_ = 0.1; // Default jitter factor of 10%
    std::chrono::milliseconds max_delay_ = std::chrono::milliseconds::max(); // Default to very large max_delay
    
public:
    explicit Retriable(Func fn) : fn_(std::move(fn)) {}
    
    // Set maximum number of retry attempts
    Retriable& times(std::size_t n) {
        max_retries_ = n;
        return *this;
    }
    
    // Set fixed delay between retries
    Retriable& with_delay(std::chrono::milliseconds delay) {
        delay_ = delay;
        return *this;
    }
    
    // Set exponential backoff factor
    Retriable& with_backoff(double factor) {
        backoff_factor_ = factor;
        return *this;
    }
    
    // Set maximum total timeout
    Retriable& timeout(std::chrono::milliseconds max_timeout) {
        max_timeout_ = max_timeout;
        return *this;
    }
    
    // Retry until predicate returns true for the result
    template<typename Pred>
    Retriable& until(Pred pred) {
        static_assert(!std::is_void_v<ReturnType>, 
                     "Cannot use 'until' with void-returning functions");
        if constexpr (!std::is_void_v<ReturnType>) {
            value_predicate_helper_.member = pred;
        }
        return *this;
    }
    
    // Retry on specific exceptions (predicate returns true to retry)
    template<typename ExceptionPred>
    Retriable& on_exception(ExceptionPred handler) {
        exception_handler_ = handler;
        return *this;
    }
    
    // Retry on specific exception types
    template<typename ExceptionType>
    Retriable& on_exception() {
        exception_handler_ = [](const std::exception& e) {
            return dynamic_cast<const ExceptionType*>(&e) != nullptr;
        };
        return *this;
    }
    
    // Set callback for retry attempts
    template<typename Callback>
    Retriable& on_retry(Callback callback) {
        retry_callback_ = callback;
        return *this;
    }

    // Enable jitter in delay calculations
    Retriable& with_jitter(bool jitter = true, double factor = 0.1) {
        jitter_ = jitter;
        if (factor < 0.0 || factor > 1.0) {
            throw std::out_of_range("Jitter factor must be between 0.0 and 1.0");
        }
        jitter_factor_ = factor;
        return *this;
    }

    // Set maximum delay cap for backoff
    Retriable& with_max_delay(std::chrono::milliseconds max_delay) {
        if (max_delay.count() < 0) {
            throw std::out_of_range("Max delay cannot be negative");
        }
        max_delay_ = max_delay;
        return *this;
    }
    
    // Execute the function with retry logic
    decltype(auto) run() {
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
                if constexpr (std::is_void_v<ReturnType>) {
                    fn_();
                    return; // Success for void functions
                } else {
                    auto result = fn_();
                    
                    // Check if result satisfies the success condition
                    if constexpr (!std::is_void_v<ReturnType>) {
                        if (!value_predicate_helper_.member || value_predicate_helper_.member(result)) {
                            return result; // Success
                        }
                        // Result doesn't satisfy condition, will retry if more attempts available
                        if (attempt < max_retries_ - 1) {
                            if (retry_callback_) {
                                retry_callback_(attempt + 1, nullptr); // nullptr because it's not an exception, but a value predicate failure
                            }
                            sleep_with_backoff(attempt);
                        }
                        // If it's the last attempt and condition not met, loop will terminate and throw outside.
                    }
                    // For void functions, if fn_() didn't throw, it's considered success already by the 'if constexpr (std::is_void_v<ReturnType>)' block.
                    // This part of the 'else' for non-void will only be reached if value_predicate exists and fails.
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
            if constexpr (std::is_void_v<ReturnType>) {
                throw std::runtime_error("Retry failed: all attempts exhausted");
            } else {
                throw std::runtime_error("Retry failed: condition not met after all attempts");
            }
        }
    }
    
    // Convenience operator for immediate execution
    decltype(auto) operator()() {
        return run();
    }
    
private:
    void sleep_with_backoff(std::size_t attempt) {
        if (delay_.count() <= 0) {
            return;
        }

        auto current_delay_ms = static_cast<double>(delay_.count());

        if (backoff_factor_ > 1.0 && attempt > 0) { // Apply backoff only after the first attempt (attempt index starts at 0 for first sleep)
            double multiplier = 1.0;
            // The 'attempt' parameter here is the number of *completed* attempts.
            // So for the sleep *after* the first failure (attempt == 0 completed), 
            // we want to apply backoff once if attempt_idx_for_backoff = 0.
            // Let's adjust to use 'attempt' directly as it signifies number of *previous* failures.
            for (std::size_t i = 0; i < attempt; ++i) { 
                multiplier *= backoff_factor_;
            }
            current_delay_ms *= multiplier;
        }
        
        std::chrono::milliseconds actual_delay(static_cast<long long>(current_delay_ms));

        // Apply jitter if enabled
        if (jitter_) {
            std::random_device rd;
            std::mt19937 gen(rd());
            // Calculate jitter range: delay ± delay * jitter_factor_
            double min_jitter = actual_delay.count() * (1.0 - jitter_factor_);
            double max_jitter = actual_delay.count() * (1.0 + jitter_factor_);
            if (min_jitter < 0) min_jitter = 0;
            
            std::uniform_real_distribution<> dis(min_jitter, max_jitter);
            actual_delay = std::chrono::milliseconds(static_cast<long long>(dis(gen)));
        }

        // Cap delay by max_delay_
        if (actual_delay > max_delay_) {
            actual_delay = max_delay_;
        }
        
        if (actual_delay.count() > 0) {
            std::this_thread::sleep_for(actual_delay);
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
    
    // Changed order of template parameters: ExceptionType first, then Func (deduced)
    template<typename ExceptionType, typename Func>
    static auto on_exception(Func&& fn, std::size_t times = 3,
                           std::chrono::milliseconds delay = std::chrono::milliseconds(100)) {
        return retry(std::forward<Func>(fn))
               .times(times)
               .with_delay(delay)
               .template on_exception<ExceptionType>(); // Retriable::on_exception<E>() is fine
    }
};

} // namespace retry_util

#pragma once

#include <functional>
#include <chrono>
#include <thread>
#include <exception>
#include <type_traits>
#include <stdexcept>
#include <random> // For jitter
#include <memory> // For std::unique_ptr

namespace retry_util {

// Forward declaration of the primary template
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
    std::function<bool(const ReturnType&)> value_predicate_ = nullptr; // Direct member
    std::function<void(std::size_t, const std::exception*)> retry_callback_ = nullptr;
    double backoff_factor_ = 1.0;
    std::chrono::milliseconds max_timeout_ = std::chrono::milliseconds(0);
    bool jitter_ = false;
    double jitter_factor_ = 0.1;
    std::chrono::milliseconds max_delay_ = std::chrono::milliseconds::max();
    
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
        if (factor < 1.0) {
             // Or throw, or log. For now, let's ensure it's at least 1.0 to avoid shrinking delays.
            backoff_factor_ = 1.0;
        } else {
            backoff_factor_ = factor;
        }
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

    Retriable& with_jitter(bool jitter_enabled = true, double factor = 0.1) {
        jitter_ = jitter_enabled;
        if (factor < 0.0 || factor > 1.0) {
            throw std::out_of_range("Jitter factor must be between 0.0 and 1.0");
        }
        jitter_factor_ = factor;
        return *this;
    }

    Retriable& with_max_delay(std::chrono::milliseconds max_val_delay) {
        if (max_val_delay.count() < 0) {
            throw std::out_of_range("Max delay cannot be negative");
        }
        max_delay_ = max_val_delay;
        return *this;
    }
    
    ReturnType run() {
        auto start_time = std::chrono::steady_clock::now();
        std::exception_ptr last_exception = nullptr;
        
        for (std::size_t attempt = 0; attempt < max_retries_; ++attempt) {
            if (max_timeout_.count() > 0) {
                auto elapsed = std::chrono::steady_clock::now() - start_time;
                if (elapsed >= max_timeout_) {
                    if (last_exception) std::rethrow_exception(last_exception);
                    throw std::runtime_error("Retry timeout exceeded before new attempt");
                }
            }
            
            try {
                ReturnType result = fn_();

                if (max_timeout_.count() > 0 && (std::chrono::steady_clock::now() - start_time >= max_timeout_)) {
                    throw std::runtime_error("Retry timeout exceeded after function execution");
                }

                if (!value_predicate_ || value_predicate_(result)) {
                    return result; // Success
                }

                last_exception = nullptr; // Clear previous exception if current failure is due to value predicate
                if (attempt < max_retries_ - 1) {
                    if (retry_callback_) {
                        retry_callback_(attempt + 1, nullptr); // Value predicate failure
                    }
                    if (max_timeout_.count() > 0 && (std::chrono::steady_clock::now() - start_time >= max_timeout_)) {
                         throw std::runtime_error("Retry timeout exceeded before sleeping for value predicate retry");
                    }
                    sleep_with_backoff(attempt);
                } else {
                    // Last attempt and value predicate failed
                    throw std::runtime_error("Retry failed: condition not met after all attempts");
                }
            } catch (const std::exception& e) {
                if (max_timeout_.count() > 0 && (std::chrono::steady_clock::now() - start_time >= max_timeout_)) {
                     // If timeout occurred, prioritize timeout exception
                    throw std::runtime_error("Retry timeout exceeded during exception handling");
                }
                last_exception = std::current_exception();
                
                bool should_retry = true; // Default to retry
                if (exception_handler_) {
                    should_retry = exception_handler_(e);
                }

                if (should_retry && (attempt < max_retries_ - 1)) {
                    if (retry_callback_) {
                        retry_callback_(attempt + 1, &e);
                    }
                    sleep_with_backoff(attempt); // Timeout check within sleep_with_backoff is not present, but overall loop timeout will catch it.
                                               // Consider if a pre-sleep timeout check is critical here too.
                                               // The main loop check and post-fn check should be sufficient.
                } else {
                    std::rethrow_exception(last_exception);
                }
            } catch (...) { // Catches non-std::exception types
                 if (max_timeout_.count() > 0 && (std::chrono::steady_clock::now() - start_time >= max_timeout_)) {
                    throw std::runtime_error("Retry timeout exceeded during unknown exception handling");
                }
                last_exception = std::current_exception();
                bool should_retry = true; // Default to retry for unknown exceptions if no handler
                std::unique_ptr<std::runtime_error> handler_exception_obj;

                if (exception_handler_) {
                    handler_exception_obj = std::make_unique<std::runtime_error>("Unknown exception occurred during retry");
                    should_retry = exception_handler_(*handler_exception_obj);
                }

                if (should_retry && (attempt < max_retries_ - 1)) {
                    if (retry_callback_) {
                        retry_callback_(attempt + 1, handler_exception_obj.get());
                    }
                     sleep_with_backoff(attempt);
                } else {
                     std::rethrow_exception(last_exception);
                }
            }
        }
        
        if (last_exception) {
            std::rethrow_exception(last_exception);
        }
        // If loop finishes without returning/throwing, it implies max_retries_ was 0 or predicate never met on last try.
        // The non-void version's predicate failure on the last attempt is handled inside the loop.
        // This path should ideally not be reached if max_retries_ > 0.
        // If max_retries_ is 0, it will fall through.
        if (max_retries_ == 0 && value_predicate_) { // If 0 retries and a predicate exists, it's effectively a single try that must satisfy predicate.
             throw std::runtime_error("Retry failed: condition not met on initial attempt (0 retries specified)");
        } else if (max_retries_ == 0) { // 0 retries, no predicate, means it should have returned if fn didn't throw. This is odd.
            // This case implies fn_() was called, didn't throw, but we are here.
            // This indicates an issue with control flow if fn_ itself should have returned.
            // However, for 0 retries, the loop `attempt < max_retries_` (0 < 0) is false, so fn_() isn't called.
            // The behavior for 0 retries means the function is never called.
            // If the user wants at least one attempt, they should use times(1).
            // To make `times(0)` behave as "run once, no retry", we'd need to change the loop to `attempt <= max_retries_` and adjust `max_retries_` default.
            // Current `max_retries_ = 3` means 3 attempts *in total*. `times(1)` means 1 attempt. `times(0)` means 0 attempts.
            // Let's assume `times(0)` means "do not attempt".
             throw std::runtime_error("Retry policy specified 0 attempts.");
        }
        // This specific path for non-void should be covered by the throw inside the loop for the last attempt value predicate failure.
        // If it reaches here, it's an unexpected state, likely due to max_retries_ == 0.
        throw std::runtime_error("Retry failed: unexpected state (non-void specialization).");
    }
    
    ReturnType operator()() {
        return run();
    }
    
private:
    void sleep_with_backoff(std::size_t current_attempt_idx) { // current_attempt_idx is 0-based index of the attempt that just failed
        if (delay_.count() <= 0 && backoff_factor_ == 1.0) { // No delay and no backoff means no sleep
            return;
        }

        double current_delay_ms_double = static_cast<double>(delay_.count());

        if (backoff_factor_ > 1.0) {
            // Apply backoff: for delay after attempt 0, factor^0. After attempt 1, factor^1 etc.
            // So, for sleep after 'current_attempt_idx' failed, we use 'current_attempt_idx' as exponent power.
            double multiplier = 1.0;
            for (std::size_t i = 0; i < current_attempt_idx; ++i) { // Multiplier grows after the first actual base delay
                multiplier *= backoff_factor_;
            }
             if (current_attempt_idx > 0) { // Only apply multiplier if not the first sleep period based on initial delay
                current_delay_ms_double *= multiplier;
             } else if (delay_.count() == 0 && current_attempt_idx == 0) {
                // If base delay is 0, and it's the first sleep, there's no delay.
                // This case is tricky. If initial delay is 0, backoff on 0 is still 0.
                // Let's assume initial delay_ is the one for the *first* sleep.
                // The loop above for multiplier should be:
                // for (std::size_t i = 0; i < current_attempt_idx; ++i)
                // This implies that for the sleep *after* attempt 0 (current_attempt_idx = 0), multiplier is 1 (loop doesn't run).
                // delay * factor^0
                // For sleep *after* attempt 1 (current_attempt_idx = 1), multiplier is backoff_factor.
                // delay * factor^1
                // This seems correct. Let's recalculate the multiplier properly.
             }
        }
        
        // Recalculate delay with backoff
        current_delay_ms_double = static_cast<double>(delay_.count()); // Start with base delay
        if (backoff_factor_ > 1.0) {
            for (std::size_t i = 0; i < current_attempt_idx; ++i) { // Number of previous sleeps where backoff was applied
                 current_delay_ms_double *= backoff_factor_;
            }
        }


        std::chrono::milliseconds actual_delay(static_cast<long long>(current_delay_ms_double));

        if (jitter_) {
            std::random_device rd;
            std::mt19937 gen(rd());
            double min_jitter_val = actual_delay.count() * (1.0 - jitter_factor_);
            double max_jitter_val = actual_delay.count() * (1.0 + jitter_factor_);
            if (min_jitter_val < 0) min_jitter_val = 0;

            std::uniform_real_distribution<> dis(min_jitter_val, max_jitter_val);
            actual_delay = std::chrono::milliseconds(static_cast<long long>(dis(gen)));
        }

        if (actual_delay > max_delay_) {
            actual_delay = max_delay_;
        }

        if (actual_delay.count() > 0) {
            std::this_thread::sleep_for(actual_delay);
        }
    }
};


// Specialization for void return types
template<typename Func>
class Retriable<Func, std::enable_if_t<std::is_void_v<std::invoke_result_t<Func>>>> {
public:
    using ReturnType = void; // Correctly void

private:
    Func fn_;
    std::size_t max_retries_ = 3;
    std::chrono::milliseconds delay_ = std::chrono::milliseconds(0);
    std::function<bool(const std::exception&)> exception_handler_ = nullptr;
    std::function<void(std::size_t, const std::exception*)> retry_callback_ = nullptr;
    double backoff_factor_ = 1.0;
    std::chrono::milliseconds max_timeout_ = std::chrono::milliseconds(0);
    bool jitter_ = false;
    double jitter_factor_ = 0.1;
    std::chrono::milliseconds max_delay_ = std::chrono::milliseconds::max();

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
         if (factor < 1.0) {
            backoff_factor_ = 1.0;
        } else {
            backoff_factor_ = factor;
        }
        return *this;
    }

    Retriable& timeout(std::chrono::milliseconds max_timeout_val) {
        max_timeout_ = max_timeout_val;
        return *this;
    }

    // No 'until' method for void functions

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

    Retriable& with_jitter(bool jitter_enabled = true, double factor = 0.1) {
        jitter_ = jitter_enabled;
        if (factor < 0.0 || factor > 1.0) {
            throw std::out_of_range("Jitter factor must be between 0.0 and 1.0");
        }
        jitter_factor_ = factor;
        return *this;
    }

    Retriable& with_max_delay(std::chrono::milliseconds max_val_delay) {
         if (max_val_delay.count() < 0) {
            throw std::out_of_range("Max delay cannot be negative");
        }
        max_delay_ = max_val_delay;
        return *this;
    }

    void run() {
        auto start_time = std::chrono::steady_clock::now();
        std::exception_ptr last_exception = nullptr;

        for (std::size_t attempt = 0; attempt < max_retries_; ++attempt) {
            if (max_timeout_.count() > 0) {
                auto elapsed = std::chrono::steady_clock::now() - start_time;
                if (elapsed >= max_timeout_) {
                    if (last_exception) std::rethrow_exception(last_exception);
                    throw std::runtime_error("Retry timeout exceeded before new attempt");
                }
            }

            try {
                fn_();
                if (max_timeout_.count() > 0 && (std::chrono::steady_clock::now() - start_time >= max_timeout_)) {
                    throw std::runtime_error("Retry timeout exceeded after function execution (void)");
                }
                return; // Success for void functions
            } catch (const std::exception& e) {
                 if (max_timeout_.count() > 0 && (std::chrono::steady_clock::now() - start_time >= max_timeout_)) {
                    throw std::runtime_error("Retry timeout exceeded during exception handling (void)");
                }
                last_exception = std::current_exception();
                bool should_retry = true;
                if (exception_handler_) {
                    should_retry = exception_handler_(e);
                }

                if (should_retry && (attempt < max_retries_ - 1)) {
                    if (retry_callback_) {
                        retry_callback_(attempt + 1, &e);
                    }
                    sleep_with_backoff(attempt);
                } else {
                    std::rethrow_exception(last_exception);
                }
            } catch (...) {
                if (max_timeout_.count() > 0 && (std::chrono::steady_clock::now() - start_time >= max_timeout_)) {
                    throw std::runtime_error("Retry timeout exceeded during unknown exception handling (void)");
                }
                last_exception = std::current_exception();
                bool should_retry = true;
                std::unique_ptr<std::runtime_error> handler_exception_obj;

                if (exception_handler_) {
                    handler_exception_obj = std::make_unique<std::runtime_error>("Unknown exception occurred during retry (void)");
                    should_retry = exception_handler_(*handler_exception_obj);
                }

                if (should_retry && (attempt < max_retries_ - 1)) {
                    if (retry_callback_) {
                        retry_callback_(attempt + 1, handler_exception_obj.get());
                    }
                    sleep_with_backoff(attempt);
                } else {
                    std::rethrow_exception(last_exception);
                }
            }
        }

        if (last_exception) {
            std::rethrow_exception(last_exception);
        } else {
             if (max_retries_ == 0) {
                 throw std::runtime_error("Retry policy specified 0 attempts (void).");
             }
            // If loop finishes, it means all retries were exhausted without success (should be an exception)
            // or max_retries_ was 0.
            throw std::runtime_error("Retry failed: all attempts exhausted (void)");
        }
    }

    void operator()() {
        run();
    }

private:
    // Copied sleep_with_backoff, ensure it's identical or refactored if common parts can be shared
    // For now, direct copy for simplicity during refactor.
    void sleep_with_backoff(std::size_t current_attempt_idx) {
        if (delay_.count() <= 0 && backoff_factor_ == 1.0) {
            return;
        }

        double current_delay_ms_double = static_cast<double>(delay_.count());

        if (backoff_factor_ > 1.0) {
            for (std::size_t i = 0; i < current_attempt_idx; ++i) {
                 current_delay_ms_double *= backoff_factor_;
            }
        }

        std::chrono::milliseconds actual_delay(static_cast<long long>(current_delay_ms_double));

        if (jitter_) {
            std::random_device rd;
            std::mt19937 gen(rd());
            double min_jitter_val = actual_delay.count() * (1.0 - jitter_factor_);
            double max_jitter_val = actual_delay.count() * (1.0 + jitter_factor_);
            if (min_jitter_val < 0) min_jitter_val = 0;
            
            std::uniform_real_distribution<> dis(min_jitter_val, max_jitter_val);
            actual_delay = std::chrono::milliseconds(static_cast<long long>(dis(gen)));
        }

        if (actual_delay > max_delay_) {
            actual_delay = max_delay_;
        }
        
        if (actual_delay.count() > 0) {
            std::this_thread::sleep_for(actual_delay);
        }
    }
};

// Factory function for creating Retriable instances
// This remains the same, as it deduces Func and relies on the primary template
// which will then pick the correct specialization.
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
    
    // Reverted order: ExceptionType first, then Func (deduced) for conventional calling
    template<typename ExceptionType, typename Func>
    static auto on_exception(Func&& fn, std::size_t times = 3,
                           std::chrono::milliseconds delay = std::chrono::milliseconds(100)) {
        return retry(std::forward<Func>(fn))
               .times(times)
               .with_delay(delay)
               .template on_exception<ExceptionType>();
    }
};

} // namespace retry_util

#pragma once

#include <functional>
#include <thread>
#include <chrono>
#include <atomic>
#include <future>
#include <memory>
#include <mutex>
#include <condition_variable>

namespace util {

/**
 * DelayedCall - Timer-based deferred execution utility for C++17
 * 
 * Inspired by Python's asyncio.call_later() and threading.Timer(),
 * this class allows scheduling a callable to execute after a specified delay.
 * 
 * Features:
 * - Schedule tasks with millisecond precision
 * - Cancel pending tasks before execution
 * - Reschedule with new delays
 * - Thread-safe operations
 * - Optional std::future support for result tracking
 * - Header-only implementation with STL dependencies only
 */
class DelayedCall {
public:
    using Clock = std::chrono::steady_clock;
    using Duration = std::chrono::milliseconds;
    using TimePoint = Clock::time_point;
    
    /**
     * Construct and schedule a delayed call
     * @param task Callable to execute (must be invocable with no arguments)
     * @param delay Duration to wait before execution
     */
    template<typename Callable>
    DelayedCall(Callable&& task, Duration delay)
        : task_fn_(std::forward<Callable>(task))
        , delay_(delay)
        , is_cancelled_(false)
        , has_fired_(false)
        , is_rescheduling_(false)
    {
        schedule_internal();
    }
    
    /**
     * Construct and schedule with chrono duration literals
     */
    template<typename Callable, typename Rep, typename Period>
    DelayedCall(Callable&& task, std::chrono::duration<Rep, Period> delay)
        : DelayedCall(std::forward<Callable>(task), 
                     std::chrono::duration_cast<Duration>(delay))
    {
    }
    
    /**
     * Destructor ensures clean shutdown
     */
    ~DelayedCall() {
        cancel();
        if (timer_thread_.joinable()) {
            timer_thread_.join();
        }
    }
    
    // Non-copyable but movable
    DelayedCall(const DelayedCall&) = delete;
    DelayedCall& operator=(const DelayedCall&) = delete;
    
    DelayedCall(DelayedCall&& other) noexcept
        : task_fn_(std::move(other.task_fn_))
        , delay_(other.delay_)
        , scheduled_time_(other.scheduled_time_)
        , is_cancelled_(other.is_cancelled_.load())
        , has_fired_(other.has_fired_.load())
        , is_rescheduling_(other.is_rescheduling_.load())
        , timer_thread_(std::move(other.timer_thread_))
    {
        other.is_cancelled_ = true;
    }
    
    DelayedCall& operator=(DelayedCall&& other) noexcept {
        if (this != &other) {
            cancel();
            if (timer_thread_.joinable()) {
                timer_thread_.join();
            }
            
            task_fn_ = std::move(other.task_fn_);
            delay_ = other.delay_;
            scheduled_time_ = other.scheduled_time_;
            is_cancelled_ = other.is_cancelled_.load();
            has_fired_ = other.has_fired_.load();
            is_rescheduling_ = other.is_rescheduling_.load();
            timer_thread_ = std::move(other.timer_thread_);
            
            other.is_cancelled_ = true;
        }
        return *this;
    }
    
    /**
     * Cancel the pending call
     * No-op if already executed or cancelled
     */
    void cancel() {
        is_cancelled_ = true;
        cv_.notify_all();
    }
    
    /**
     * Reschedule with a new delay
     * Cancels current timer and starts new one if not already fired
     * @param new_delay New duration to wait
     */
    void reschedule(Duration new_delay) {
        std::lock_guard<std::mutex> lock(reschedule_mutex_);
        
        if (has_fired_) {
            return; // Already executed, can't reschedule
        }
        
        is_rescheduling_ = true;
        is_cancelled_ = true;
        cv_.notify_all();
        
        if (timer_thread_.joinable()) {
            timer_thread_.join();
        }
        
        delay_ = new_delay;
        is_cancelled_ = false;
        is_rescheduling_ = false;
        
        schedule_internal();
    }
    
    /**
     * Reschedule with chrono duration literals
     */
    template<typename Rep, typename Period>
    void reschedule(std::chrono::duration<Rep, Period> new_delay) {
        reschedule(std::chrono::duration_cast<Duration>(new_delay));
    }
    
    /**
     * Check if the call has expired (executed or cancelled)
     */
    bool expired() const {
        return has_fired_ || is_cancelled_;
    }
    
    /**
     * Check if the call is still valid (scheduled to run)
     */
    bool valid() const {
        return !expired() && !is_rescheduling_;
    }
    
    /**
     * Get the remaining time until execution
     * Returns zero if expired or invalid
     */
    Duration remaining_time() const {
        if (expired()) {
            return Duration::zero();
        }
        
        auto now = Clock::now();
        if (now >= scheduled_time_) {
            return Duration::zero();
        }
        
        return std::chrono::duration_cast<Duration>(scheduled_time_ - now);
    }
    
    /**
     * Get the original delay duration
     */
    Duration delay() const {
        return delay_;
    }
    
private:
    void schedule_internal() {
        scheduled_time_ = Clock::now() + delay_;
        
        timer_thread_ = std::thread([this]() {
            std::unique_lock<std::mutex> lock(wait_mutex_);
            
            // Wait for the delay period or until cancelled
            if (cv_.wait_until(lock, scheduled_time_, [this]() { 
                return is_cancelled_.load(); 
            })) {
                // Was cancelled during wait
                return;
            }
            
            // Check again after wait to handle race conditions
            if (is_cancelled_) {
                return;
            }
            
            // Execute the task
            has_fired_ = true;
            try {
                task_fn_();
            } catch (...) {
                // Swallow exceptions to prevent thread termination
                // In a production system, you might want to log this
            }
        });
    }
    
    std::function<void()> task_fn_;
    Duration delay_;
    TimePoint scheduled_time_;
    
    std::atomic<bool> is_cancelled_;
    std::atomic<bool> has_fired_;
    std::atomic<bool> is_rescheduling_;
    
    std::thread timer_thread_;
    std::mutex wait_mutex_;
    std::mutex reschedule_mutex_;
    std::condition_variable cv_;
};

/**
 * DelayedCallWithFuture - Version that returns std::future for result tracking
 */
template<typename T = void>
class DelayedCallWithFuture {
public:
    using Clock = std::chrono::steady_clock;
    using Duration = std::chrono::milliseconds;
    
    template<typename Callable>
    DelayedCallWithFuture(Callable&& task, Duration delay) {
        auto promise = std::make_shared<std::promise<T>>();
        future_ = promise->get_future();
        
        auto wrapped_task = [task = std::forward<Callable>(task), promise]() {
            try {
                if constexpr (std::is_void_v<T>) {
                    task();
                    promise->set_value();
                } else {
                    auto result = task();
                    promise->set_value(result);
                }
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        };
        
        delayed_call_ = std::make_unique<DelayedCall>(wrapped_task, delay);
    }
    
    template<typename Callable, typename Rep, typename Period>
    DelayedCallWithFuture(Callable&& task, std::chrono::duration<Rep, Period> delay)
        : DelayedCallWithFuture(std::forward<Callable>(task), 
                               std::chrono::duration_cast<Duration>(delay))
    {
    }
    
    void cancel() {
        if (delayed_call_) {
            delayed_call_->cancel();
        }
    }
    
    void reschedule(Duration new_delay) {
        if (delayed_call_) {
            delayed_call_->reschedule(new_delay);
        }
    }
    
    template<typename Rep, typename Period>
    void reschedule(std::chrono::duration<Rep, Period> new_delay) {
        reschedule(std::chrono::duration_cast<Duration>(new_delay));
    }
    
    bool expired() const {
        return delayed_call_ ? delayed_call_->expired() : true;
    }
    
    bool valid() const {
        return delayed_call_ ? delayed_call_->valid() : false;
    }
    
    std::future<T>& get_future() {
        return future_;
    }
    
private:
    std::unique_ptr<DelayedCall> delayed_call_;
    std::future<T> future_;
};

// Convenience factory functions
template<typename Callable>
auto make_delayed_call(Callable&& task, std::chrono::milliseconds delay) {
    return DelayedCall(std::forward<Callable>(task), delay);
}

template<typename Callable, typename Rep, typename Period>
auto make_delayed_call(Callable&& task, std::chrono::duration<Rep, Period> delay) {
    return DelayedCall(std::forward<Callable>(task), delay);
}

template<typename T = void, typename Callable>
auto make_delayed_call_with_future(Callable&& task, std::chrono::milliseconds delay) {
    return DelayedCallWithFuture<T>(std::forward<Callable>(task), delay);
}

template<typename T = void, typename Callable, typename Rep, typename Period>
auto make_delayed_call_with_future(Callable&& task, std::chrono::duration<Rep, Period> delay) {
    return DelayedCallWithFuture<T>(std::forward<Callable>(task), delay);
}

} // namespace util

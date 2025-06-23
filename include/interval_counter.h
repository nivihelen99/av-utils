#pragma once

#include <chrono>
#include <deque>
#include <mutex>
#include <atomic>
#include <map>
#include <numeric>
#include <stdexcept>

namespace util {

/**
 * @brief Efficient time-windowed event counter with sliding window support
 * 
 * Tracks event counts over a rolling time window with configurable resolution.
 * Designed for high-performance rate tracking and monitoring use cases.
 * 
 * Features:
 * - O(1) record() and count() operations
 * - Sliding window with automatic cleanup
 * - Configurable time resolution
 * - Thread-safe variant available
 * - Header-only, C++17 compliant
 */
class IntervalCounter {
private:
    struct Bucket {
        std::chrono::steady_clock::time_point timestamp;
        std::atomic<int> count{0};
        
        Bucket(std::chrono::steady_clock::time_point ts) : timestamp(ts) {}
    };

    std::chrono::seconds window_duration_;
    std::chrono::milliseconds resolution_;
    std::deque<Bucket> buckets_;
    std::atomic<size_t> total_count_{0};
    mutable std::mutex mutex_;

    void cleanup_expired_buckets() {
        auto now = std::chrono::steady_clock::now();
        auto cutoff = now - window_duration_;
        
        while (!buckets_.empty() && buckets_.front().timestamp < cutoff) {
            total_count_ -= buckets_.front().count.load();
            buckets_.pop_front();
        }
    }

    Bucket& get_or_create_current_bucket() {
        auto now = std::chrono::steady_clock::now();
        
        // Round down to resolution boundary
        auto epoch = now.time_since_epoch();
        auto resolution_count = std::chrono::duration_cast<std::chrono::milliseconds>(epoch) / resolution_;
        auto bucket_time = std::chrono::steady_clock::time_point{
            std::chrono::milliseconds{resolution_count * resolution_.count()}
        };
        
        if (buckets_.empty() || buckets_.back().timestamp != bucket_time) {
            buckets_.emplace_back(bucket_time);
        }
        
        return buckets_.back();
    }

public:
    /**
     * @brief Construct IntervalCounter with specified window and resolution
     * @param window Total duration to track events (e.g., 60s)
     * @param resolution Bucket size for time quantization (e.g., 1s, 100ms)
     */
    explicit IntervalCounter(
        std::chrono::seconds window, 
        std::chrono::milliseconds resolution = std::chrono::milliseconds{1000}
    ) : window_duration_(window), resolution_(resolution) {
        if (window.count() <= 0) {
            throw std::invalid_argument("Window duration must be positive");
        }
        if (resolution.count() <= 0) {
            throw std::invalid_argument("Resolution must be positive");
        }
    }

    /**
     * @brief Record a single event at current time
     */
    void record() {
        record(1);
    }

    /**
     * @brief Record multiple events at current time
     * @param count Number of events to record
     */
    void record(int count) {
        if (count <= 0) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        cleanup_expired_buckets();
        
        auto& bucket = get_or_create_current_bucket();
        bucket.count += count;
        total_count_ += count;
    }

    /**
     * @brief Get total count of events in current window
     * @return Number of events in the sliding window
     */
    size_t count() {
        std::lock_guard<std::mutex> lock(mutex_);
        cleanup_expired_buckets();
        return total_count_.load();
    }

    /**
     * @brief Calculate average events per second in current window
     * @return Rate as events per second
     */
    double rate_per_second() {
        auto current_count = count();
        return static_cast<double>(current_count) / window_duration_.count();
    }

    /**
     * @brief Clear all recorded events
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        buckets_.clear();
        total_count_ = 0;
    }

    /**
     * @brief Get bucket breakdown for debugging/statistics
     * @return Map of timestamp to count for each bucket
     */
    std::map<std::chrono::steady_clock::time_point, int> bucket_counts() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::map<std::chrono::steady_clock::time_point, int> result;
        
        for (const auto& bucket : buckets_) {
            result[bucket.timestamp] = bucket.count.load();
        }
        
        return result;
    }

    /**
     * @brief Get window configuration
     */
    std::chrono::seconds window_duration() const { return window_duration_; }
    std::chrono::milliseconds resolution() const { return resolution_; }
};

/**
 * @brief Lock-free single-threaded variant for maximum performance
 * 
 * Same interface as IntervalCounter but without thread synchronization.
 * Use only when accessed from a single thread.
 */
class IntervalCounterST {
private:
    struct Bucket {
        std::chrono::steady_clock::time_point timestamp;
        int count = 0;
        
        Bucket(std::chrono::steady_clock::time_point ts) : timestamp(ts) {}
    };

    std::chrono::seconds window_duration_;
    std::chrono::milliseconds resolution_;
    std::deque<Bucket> buckets_;
    size_t total_count_ = 0;

    void cleanup_expired_buckets() {
        auto now = std::chrono::steady_clock::now();
        auto cutoff = now - window_duration_;
        
        while (!buckets_.empty() && buckets_.front().timestamp < cutoff) {
            total_count_ -= buckets_.front().count;
            buckets_.pop_front();
        }
    }

    Bucket& get_or_create_current_bucket() {
        auto now = std::chrono::steady_clock::now();
        
        auto epoch = now.time_since_epoch();
        auto resolution_count = std::chrono::duration_cast<std::chrono::milliseconds>(epoch) / resolution_;
        auto bucket_time = std::chrono::steady_clock::time_point{
            std::chrono::milliseconds{resolution_count * resolution_.count()}
        };
        
        if (buckets_.empty() || buckets_.back().timestamp != bucket_time) {
            buckets_.emplace_back(bucket_time);
        }
        
        return buckets_.back();
    }

public:
    explicit IntervalCounterST(
        std::chrono::seconds window, 
        std::chrono::milliseconds resolution = std::chrono::milliseconds{1000}
    ) : window_duration_(window), resolution_(resolution) {
        if (window.count() <= 0) {
            throw std::invalid_argument("Window duration must be positive");
        }
        if (resolution.count() <= 0) {
            throw std::invalid_argument("Resolution must be positive");
        }
    }

    void record() { record(1); }

    void record(int count) {
        if (count <= 0) return;
        
        cleanup_expired_buckets();
        auto& bucket = get_or_create_current_bucket();
        bucket.count += count;
        total_count_ += count;
    }

    size_t count() {
        cleanup_expired_buckets();
        return total_count_;
    }

    double rate_per_second() {
        auto current_count = count();
        return static_cast<double>(current_count) / window_duration_.count();
    }

    void clear() {
        buckets_.clear();
        total_count_ = 0;
    }

    std::map<std::chrono::steady_clock::time_point, int> bucket_counts() const {
        std::map<std::chrono::steady_clock::time_point, int> result;
        for (const auto& bucket : buckets_) {
            result[bucket.timestamp] = bucket.count;
        }
        return result;
    }

    std::chrono::seconds window_duration() const { return window_duration_; }
    std::chrono::milliseconds resolution() const { return resolution_; }
};

// Convenience alias for thread-safe version
using RateTracker = IntervalCounter;
using RateTrackerST = IntervalCounterST;

} // namespace util

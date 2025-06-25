#pragma once

#include <chrono>
#include <mutex>
#include <algorithm> // For std::min
#include <stdexcept> // For std::invalid_argument

namespace cpp_utils {

/**
 * @brief Implements a Token Bucket rate limiting algorithm.
 *
 * This class allows limiting the rate of operations by requiring tokens to be
 * acquired before proceeding. Tokens are refilled at a constant rate up to a
 * specified capacity.
 *
 * The implementation is thread-safe.
 */
class TokenBucketRateLimiter {
public:
    /**
     * @brief Constructs a TokenBucketRateLimiter.
     *
     * @param capacity The maximum number of tokens the bucket can hold. Must be > 0.
     * @param tokens_per_second The rate at which tokens are added to the bucket. Must be > 0.
     * @throw std::invalid_argument if capacity or tokens_per_second is non-positive.
     */
    TokenBucketRateLimiter(size_t capacity, double tokens_per_second)
        : capacity_(capacity),
          tokens_per_second_(tokens_per_second),
          current_tokens_(capacity),
          last_refill_timestamp_(std::chrono::steady_clock::now()) {
        if (capacity == 0) {
            throw std::invalid_argument("Capacity must be greater than 0.");
        }
        if (tokens_per_second <= 0.0) {
            throw std::invalid_argument("Tokens per second must be greater than 0.");
        }
    }

    /**
     * @brief Attempts to acquire a specified number of tokens from the bucket.
     *
     * If the bucket contains enough tokens, they are consumed, and the method
     * returns true. Otherwise, no tokens are consumed, and the method returns
     * false.
     *
     * This method is thread-safe.
     *
     * @param tokens_to_acquire The number of tokens to attempt to acquire.
     *                          Defaults to 1.
     * @return True if tokens were successfully acquired, false otherwise.
     */
    bool try_acquire(size_t tokens_to_acquire = 1) {
        if (tokens_to_acquire == 0) {
            return true; // Successfully "acquired" zero tokens
        }

        std::lock_guard<std::mutex> lock(mutex_);

        refill_tokens();

        if (current_tokens_ >= tokens_to_acquire) {
            current_tokens_ -= tokens_to_acquire;
            return true;
        }

        return false;
    }

    /**
     * @brief Gets the current capacity of the token bucket.
     * @return The capacity.
     */
    size_t get_capacity() const {
        return capacity_;
    }

    /**
     * @brief Gets the configured token refill rate.
     * @return Tokens per second.
     */
    double get_tokens_per_second() const {
        return tokens_per_second_;
    }

    /**
     * @brief Gets the current number of available tokens (primarily for testing/monitoring).
     * This method also triggers a token refill.
     * @return Current number of tokens.
     */
    size_t get_current_tokens() {
        std::lock_guard<std::mutex> lock(mutex_);
        refill_tokens();
        return static_cast<size_t>(current_tokens_);
    }

private:
    /**
     * @brief Refills tokens based on the time elapsed since the last refill.
     * This method MUST be called with the mutex_ held.
     */
    void refill_tokens() {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> time_elapsed = now - last_refill_timestamp_;

        double tokens_to_add = time_elapsed.count() * tokens_per_second_;

        if (tokens_to_add > 0) { // Only update timestamp if there's a potential change
            current_tokens_ = std::min(static_cast<double>(capacity_), current_tokens_ + tokens_to_add);
            last_refill_timestamp_ = now;
        }
    }

    const size_t capacity_;
    const double tokens_per_second_;

    double current_tokens_; // Using double for precision in refills
    std::chrono::steady_clock::time_point last_refill_timestamp_;

    std::mutex mutex_;
};

} // namespace cpp_utils

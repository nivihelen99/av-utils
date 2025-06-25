# Rate Limiter (`rate_limiter.h`)

## Overview

The `rate_limiter.h` header provides a `TokenBucketRateLimiter` class, which implements the token bucket algorithm for rate limiting. Rate limiting is a crucial mechanism for controlling the frequency of operations, such as API requests, logging, or resource access, to prevent overload and ensure fair usage.

## Token Bucket Algorithm

The Token Bucket algorithm is a popular method for rate limiting. It works as follows:

1.  **Bucket Capacity**: A token bucket has a fixed capacity, representing the maximum number of tokens it can hold.
2.  **Token Refill**: Tokens are added to the bucket at a constant rate (e.g., N tokens per second) until the bucket is full.
3.  **Operation Request**: When an operation needs to be performed, it must acquire one or more tokens from the bucket.
    *   If the bucket contains enough tokens, the required number of tokens are consumed, and the operation is permitted.
    *   If the bucket does not have enough tokens, the operation is denied (rate-limited). Tokens are not consumed in this case.

This algorithm allows for bursts of operations (up to the bucket's capacity) while enforcing an average rate over time.

## `TokenBucketRateLimiter` Class

The `TokenBucketRateLimiter` class provides a thread-safe implementation of this algorithm.

### Public Interface

```cpp
namespace cpp_utils {

class TokenBucketRateLimiter {
public:
    /**
     * @brief Constructs a TokenBucketRateLimiter.
     *
     * @param capacity The maximum number of tokens the bucket can hold. Must be > 0.
     * @param tokens_per_second The rate at which tokens are added to the bucket. Must be > 0.
     * @throw std::invalid_argument if capacity or tokens_per_second is non-positive.
     */
    TokenBucketRateLimiter(size_t capacity, double tokens_per_second);

    /**
     * @brief Attempts to acquire a specified number of tokens from the bucket.
     *
     * If the bucket contains enough tokens, they are consumed, and the method
     * returns true. Otherwise, no tokens are consumed, and the method returns
     * false. This method is thread-safe.
     *
     * @param tokens_to_acquire The number of tokens to attempt to acquire. Defaults to 1.
     * @return True if tokens were successfully acquired, false otherwise.
     */
    bool try_acquire(size_t tokens_to_acquire = 1);

    /**
     * @brief Gets the current capacity of the token bucket.
     * @return The capacity.
     */
    size_t get_capacity() const;

    /**
     * @brief Gets the configured token refill rate.
     * @return Tokens per second.
     */
    double get_tokens_per_second() const;

    /**
     * @brief Gets the current number of available tokens.
     * This method also triggers a token refill before returning the count.
     * Useful for monitoring or testing.
     * @return Current number of tokens (after potential refill).
     */
    size_t get_current_tokens();
};

} // namespace cpp_utils
```

### Thread Safety

All methods of `TokenBucketRateLimiter` that modify or access shared state (like `try_acquire` and internal token refill logic) are thread-safe, typically using `std::mutex`.

## Usage Example

Here's a basic example of how to use `TokenBucketRateLimiter`:

```cpp
#include "rate_limiter.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    // Initialize a rate limiter: capacity of 5 tokens, refills 2 tokens per second.
    cpp_utils::TokenBucketRateLimiter limiter(5, 2.0);

    for (int i = 0; i < 10; ++i) {
        std::cout << "Attempting operation " << (i + 1) << "... ";
        if (limiter.try_acquire(1)) {
            std::cout << "Operation permitted. Current tokens: " << limiter.get_current_tokens() << std::endl;
            // Perform the actual operation here
        } else {
            std::cout << "Operation rate-limited. Current tokens: " << limiter.get_current_tokens() << std::endl;
        }
        // Wait for some time before the next attempt
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    return 0;
}
```

In this example:
-   A limiter is set up to allow bursts of up to 5 operations.
-   It refills at a rate of 2 tokens per second.
-   Operations are attempted every 300 milliseconds.
-   If an operation is attempted when no tokens are available, it will be denied. Over time, as tokens refill, subsequent operations will be permitted.

This component is useful for managing access to shared resources, preventing system abuse, or ensuring smooth service operation under varying loads.

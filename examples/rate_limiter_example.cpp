#include "rate_limiter.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip> // For std::fixed and std::setprecision

void print_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);
    std::cout << std::put_time(&tm, "%H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << ms.count() << " - ";
}

int main() {
    // Create a rate limiter:
    // Capacity of 5 tokens.
    // Refills 2 tokens per second.
    cpp_utils::TokenBucketRateLimiter limiter(5, 2.0);

    std::cout << "Rate Limiter Example: Capacity=" << limiter.get_capacity()
              << ", Rate=" << limiter.get_tokens_per_second() << " tokens/sec." << std::endl;
    std::cout << "Attempting to perform 20 tasks. Some should be rate-limited." << std::endl;
    std::cout << "-----------------------------------------------------------" << std::endl;

    int tasks_done = 0;
    int tasks_attempted = 20;

    for (int i = 0; i < tasks_attempted; ++i) {
        print_timestamp();
        std::cout << "Attempting task " << (i + 1) << ". ";
        if (limiter.try_acquire(1)) {
            tasks_done++;
            std::cout << "Task permitted. (Tokens remaining: ~" << limiter.get_current_tokens() << ")" << std::endl;
            // Simulate doing some work
            // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else {
            std::cout << "Task rate-limited. (Tokens remaining: ~" << limiter.get_current_tokens() << ")" << std::endl;
        }

        // Sleep for a short duration to simulate time between task attempts
        // This also allows tokens to refill over time.
        // If this sleep is shorter than the token refill interval, we'll see more rate limiting.
        std::this_thread::sleep_for(std::chrono::milliseconds(300)); // 0.3 seconds between attempts
    }

    std::cout << "-----------------------------------------------------------" << std::endl;
    std::cout << "Total tasks attempted: " << tasks_attempted << std::endl;
    std::cout << "Total tasks permitted: " << tasks_done << std::endl;
    std::cout << "Expected tasks (approx): Initial (5) + (20 attempts * 0.3s/attempt * 2 tokens/s) = 5 + 12 = 17" << std::endl;
    std::cout << "Actual result can vary slightly due to timing of operations and refills." << std::endl;

    std::cout << "\nDemonstrating acquiring multiple tokens:" << std::endl;
    cpp_utils::TokenBucketRateLimiter multi_limiter(10, 5.0); // 10 capacity, 5 tokens/sec
     print_timestamp();
    std::cout << "Initial tokens: " << multi_limiter.get_current_tokens() << std::endl;

    print_timestamp();
    if(multi_limiter.try_acquire(7)) {
        std::cout << "Acquired 7 tokens. Tokens remaining: " << multi_limiter.get_current_tokens() << std::endl;
    } else {
        std::cout << "Failed to acquire 7 tokens. Tokens remaining: " << multi_limiter.get_current_tokens() << std::endl;
    }

    print_timestamp();
    if(multi_limiter.try_acquire(4)) { // This should fail as only 3 are left (10-7)
        std::cout << "Acquired 4 tokens. Tokens remaining: " << multi_limiter.get_current_tokens() << std::endl;
    } else {
        std::cout << "Failed to acquire 4 tokens. Tokens remaining: " << multi_limiter.get_current_tokens() << std::endl;
    }

    std::cout << "Waiting for 1 second to refill..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Should refill 5 tokens. Current: 3 + 5 = 8

    print_timestamp();
    std::cout << "Tokens after 1s refill: " << multi_limiter.get_current_tokens() << std::endl;
    if(multi_limiter.try_acquire(8)) {
        std::cout << "Acquired 8 tokens. Tokens remaining: " << multi_limiter.get_current_tokens() << std::endl;
    } else {
        std::cout << "Failed to acquire 8 tokens. Tokens remaining: " << multi_limiter.get_current_tokens() << std::endl;
    }

    return 0;
}

#include "retry.h" // Assuming retry.h will be in an include path
#include <iostream>
#include <random> // Already in retry.h but good for explicitness if examples are standalone
#include <vector>   // For potential future examples, not strictly needed by current ones
#include <stdexcept> // For std::runtime_error

// Forward declare retry_util namespace if not brought in by retry.h fully for examples
// Or ensure retry.h has everything needed.
// using namespace retry_util; // This might be too broad for a header, but okay for example .cpp

namespace retry_util_examples { // Changed from retry_util::examples to avoid potential collision if retry_util is opened

// Example: Flaky function that fails a few times before succeeding
int flaky_function() {
    static int call_count = 0;
    ++call_count;
    std::cout << "flaky_function called, count: " << call_count << std::endl;
    
    if (call_count < 3) {
        throw std::runtime_error("Temporary failure #" + std::to_string(call_count));
    }
    std::cout << "flaky_function succeeded." << std::endl;
    return 42;
}

// Example: Function that returns success/failure based on value
bool unreliable_ping() {
    // Re-seed for more varied results if called multiple times across runs / different examples
    static thread_local std::mt19937 gen(std::random_device{}()); 
    std::uniform_int_distribution<> dis(1, 10);
    bool result = dis(gen) > 7; // 30% chance of success (true)
    std::cout << "unreliable_ping called, result: " << std::boolalpha << result << std::endl;
    return result;
}

// Example: Network connection simulation (void function)
void connect_to_server() {
    static int attempts = 0;
    ++attempts;
    std::cout << "connect_to_server attempt: " << attempts << std::endl;
    
    if (attempts < 2) {
        throw std::runtime_error("Connection failed on attempt " + std::to_string(attempts));
    }
    
    std::cout << "Connected successfully after " << attempts << " attempts!\n";
}

// Example: Function that should timeout
int function_that_takes_too_long() {
    std::cout << "function_that_takes_too_long called, sleeping for 5 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "function_that_takes_too_long finished sleeping." << std::endl;
    return 100;
}

// Example: Function for jitter and max_delay test
int flaky_for_jitter_test() {
    static int call_count_jitter = 0;
    ++call_count_jitter;
    std::cout << "flaky_for_jitter_test called, count: " << call_count_jitter << std::endl;
    if (call_count_jitter < 4) { // Fail 3 times
        throw std::runtime_error("Jitter test temporary failure #" + std::to_string(call_count_jitter));
    }
    std::cout << "flaky_for_jitter_test succeeded." << std::endl;
    return 200;
}


void run_examples() {
    std::cout << "=== Retry Utility Examples ===\n\n";
    
    // Reset counters for examples
    try{
        flaky_function(); // Call once to reset its static counter if tests run multiple times in one go
    } 
    catch(...)
    {
    }
            
    // This is a bit of a hack for static counters in example functions. 
    // A better way would be to pass state or use classes for test functions.
    // For now, let's assume examples are run sequentially and counters are reset manually if needed.
    // To properly reset, flaky_function's counter would need to be accessible or reset.
    // Let's make example functions classes or pass counters by reference for better testability if needed.
    // For now, we'll rely on the initial state of static counters for a single run of run_examples().

    std::cout << "--- Example 1: Basic retry with delay ---\n";
    // Reset static counter for flaky_function for this specific example
    // This is tricky with static variables. For true isolation, functions shouldn't rely on static state like this for examples.
    // One way: make flaky_function a class with member `call_count`.
    // Or, a simpler approach for example: a lambda that captures a counter.
    int flaky_call_count_1 = 0;
    auto flaky_fn_ex1 = [&]() {
        ++flaky_call_count_1;
        std::cout << "flaky_fn_ex1 called, count: " << flaky_call_count_1 << std::endl;
        if (flaky_call_count_1 < 3) {
            throw std::runtime_error("Ex1 Temp failure #" + std::to_string(flaky_call_count_1));
        }
        std::cout << "flaky_fn_ex1 succeeded." << std::endl;
        return 42;
    };

    try {
        auto result = retry_util::retry(flaky_fn_ex1)
                        .times(5)
                        .with_delay(std::chrono::milliseconds(50))
                        .on_retry([](std::size_t attempt, const std::exception* e){
                            std::cout << "  Retrying (attempt " << attempt << ")...";
                            if(e) std::cout << " due to: " << e->what();
                            std::cout << std::endl;
                        })
                        .run();
        std::cout << "Result: " << result << "\n\n";
    } catch (const std::exception& e) {
        std::cout << "Failed: " << e.what() << "\n\n";
    }
    
    std::cout << "--- Example 2: Retry until condition is met ---\n";
    try {
        // unreliable_ping uses its own static counter, be mindful if re-running.
        // For this example, its internal static is fine.
        auto success = retry_util::retry(unreliable_ping)
                         .times(10)
                         .with_delay(std::chrono::milliseconds(100))
                         .until([](bool result) { return result; }) // Retry until true
                         .on_retry([](std::size_t attempt, const std::exception*) { // Exception ptr will be null here
                             std::cout << "  Attempt " << attempt << " value was not true, retrying...\n";
                         })
                         .run();
        std::cout << "Ping successful: " << std::boolalpha << success << "\n\n";
    } catch (const std::exception& e) {
        std::cout << "Failed: " << e.what() << "\n\n"; // Should happen if 10 attempts don't yield true
    }
    
    std::cout << "--- Example 3: Exponential backoff ---\n";
    int connect_attempts_ex3 = 0;
    auto connect_fn_ex3 = [&]() {
        ++connect_attempts_ex3;
        std::cout << "connect_fn_ex3 attempt: " << connect_attempts_ex3 << std::endl;
        if (connect_attempts_ex3 < 2) { // Fail on first attempt
            throw std::runtime_error("Ex3 Connection failed on attempt " + std::to_string(connect_attempts_ex3));
        }
        std::cout << "Ex3 Connected successfully after " << connect_attempts_ex3 << " attempts!\n";
    };
    try {
        retry_util::retry(connect_fn_ex3)
            .times(4)
            .with_delay(std::chrono::milliseconds(50))
            .with_backoff(2.0)
            .on_retry([](std::size_t attempt, const std::exception* e) {
                if (e) {
                    std::cout << "  Attempt " << attempt << " failed: " << e->what() << ". Retrying with backoff.\n";
                }
            })
            .run();
    } catch (const std::exception& e) {
        std::cout << "Failed: " << e.what() << "\n\n";
    }
    
    std::cout << "--- Example 4: Using RetryBuilder ---\n";
    try {
        auto result = retry_util::RetryBuilder::simple([]() { 
                        std::cout << "RetryBuilder simple function called." << std::endl;
                        return 123; }, 
                        3, std::chrono::milliseconds(10))
                        .run();
        std::cout << "Simple retry result: " << result << "\n";
    } catch (const std::exception& e) {
        std::cout << "Failed: " << e.what() << "\n";
    }
    std::cout << "\n";

    std::cout << "--- Example 5: Timeout ---\n";
    try {
        std::cout << "Expecting timeout after 1 second..." << std::endl;
        auto result = retry_util::retry(function_that_takes_too_long)
                        .times(3)
                        .with_delay(std::chrono::milliseconds(100)) // Delay between retries
                        .timeout(std::chrono::seconds(1))       // Overall timeout
                        .run();
        std::cout << "Timeout example result: " << result << "\n\n";
    } catch (const std::exception& e) {
        std::cout << "Caught expected exception: " << e.what() << "\n\n";
    }

    std::cout << "--- Example 6: Jitter and Max Delay ---\n";
    int jitter_call_count = 0;
    auto flaky_fn_jitter = [&]() {
        ++jitter_call_count;
        std::cout << "flaky_fn_jitter called, count: " << jitter_call_count << std::endl;
        if (jitter_call_count < 4) { // Fail 3 times
            throw std::runtime_error("Jitter example Temp failure #" + std::to_string(jitter_call_count));
        }
        std::cout << "flaky_fn_jitter succeeded." << std::endl;
        return 200;
    };
    try {
        std::cout << "Retrying with initial delay 100ms, backoff 2.0, jitter 0.2, max_delay 500ms" << std::endl;
        auto result = retry_util::retry(flaky_fn_jitter)
                        .times(5)
                        .with_delay(std::chrono::milliseconds(100))
                        .with_backoff(2.0)
                        .with_jitter(true, 0.2) // enable jitter with 20% factor
                        .with_max_delay(std::chrono::milliseconds(500))
                        .on_retry([](std::size_t attempt, const std::exception* e){
                            std::cout << "  Retrying jitter example (attempt " << attempt << ")...";
                            if(e) std::cout << " due to: " << e->what();
                            std::cout << std::endl;
                        })
                        .run();
        std::cout << "Jitter example result: " << result << "\n\n";
    } catch (const std::exception& e) {
        std::cout << "Jitter example failed: " << e.what() << "\n\n";
    }

    std::cout << "--- Example 7: Retrying a void function that fails initially ---\n";
    int void_connect_attempts = 0;
    auto void_connect_fn = [&]() {
        ++void_connect_attempts;
        std::cout << "void_connect_fn attempt: " << void_connect_attempts << std::endl;
        if (void_connect_attempts < 3) { // Fail twice
            throw std::runtime_error("Void connect failed on attempt " + std::to_string(void_connect_attempts));
        }
        std::cout << "Void connect successful after " << void_connect_attempts << " attempts!\n";
    };
    try {
        retry_util::retry(void_connect_fn)
            .times(5)
            .with_delay(std::chrono::milliseconds(50))
            .on_retry([](std::size_t attempt, const std::exception* e){
                std::cout << "  Retrying void function (attempt " << attempt << ")...";
                if(e) std::cout << " due to: " << e->what();
                std::cout << std::endl;
            })
            .run();
        std::cout << "Void function retry completed.\n\n";
    } catch (const std::exception& e) {
        std::cout << "Void function retry failed: " << e.what() << "\n\n";
    }

}

} // namespace retry_util_examples

int main() {
    retry_util_examples::run_examples();
    return 0;
}

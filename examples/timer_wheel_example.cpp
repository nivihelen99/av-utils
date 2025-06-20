#include "timer_wheel.h" // Adjust path if necessary e.g., "../include/timer_wheel.hpp"
#include <iostream>
#include <thread>   // For std::this_thread::sleep_for
#include <chrono>   // For std::chrono::milliseconds
#include <atomic>   // For std::atomic_int
#include <string>   // For std::string cookie
#include <any>      // For std::any and std::any_cast
#include <cassert>  // For assert()

int main() {
    std::cout << "TimerWheel Example with Cookies" << std::endl;

    // Create a TimerWheel: 100ms resolution, 50 slots (wheel cycle of 5 seconds)
    cpp_utils::TimerWheel tw(100, 50);
    std::cout << "TimerWheel created with 100ms resolution and 50 slots." << std::endl;

    std::atomic<int> one_shot_fired_count(0);
    std::atomic<int> periodic_fired_count(0);
    std::atomic<int> default_cookie_timer_fired_count(0);

    // Data to be used as cookies
    std::string one_shot_cookie_data = "OneShotContext_UserData123";
    int periodic_cookie_data = 777;

    std::cout << "Adding a one-shot timer for 500ms with a string cookie." << std::endl;
    int one_shot_id = tw.addTimer(
        500, // delay_ms
        [&](std::any cookie) { // Lambda now accepts std::any cookie
            one_shot_fired_count++;
            std::cout << "[Callback] One-shot timer (500ms) fired! Count: " << one_shot_fired_count.load() << std::endl;
            if (cookie.has_value()) {
                try {
                    const std::string& data = std::any_cast<const std::string&>(cookie);
                    std::cout << "  Received one-shot cookie: \"" << data << "\"" << std::endl;
                    assert(data == one_shot_cookie_data);
                } catch (const std::bad_any_cast& e) {
                    std::cerr << "  Error casting one-shot cookie: " << e.what() << std::endl;
                    assert(false); // Should not fail casting
                }
            } else {
                std::cerr << "  Error: One-shot cookie has no value!" << std::endl;
                assert(false); // Should have a value
            }
        },
        one_shot_cookie_data, // Pass the string as a cookie
        cpp_utils::TimerType::OneShot
    );
    if (one_shot_id < 0) {
        std::cerr << "Failed to add one-shot timer." << std::endl;
        return 1;
    }

    std::cout << "Adding a periodic timer for 1000ms interval with an int cookie." << std::endl;
    int periodic_id = tw.addTimer(
        1000, // delay_ms (and interval for periodic)
        [&](std::any cookie) { // Lambda now accepts std::any cookie
            periodic_fired_count++;
            std::cout << "[Callback] Periodic timer (1000ms) fired! Count: " << periodic_fired_count.load() << std::endl;
            if (cookie.has_value()) {
                try {
                    int data = std::any_cast<int>(cookie);
                    std::cout << "  Received periodic cookie: " << data << std::endl;
                    assert(data == periodic_cookie_data);
                } catch (const std::bad_any_cast& e) {
                    std::cerr << "  Error casting periodic cookie: " << e.what() << std::endl;
                    assert(false); // Should not fail casting
                }
            } else {
                std::cerr << "  Error: Periodic cookie has no value!" << std::endl;
                assert(false); // Should have a value
            }
        },
        periodic_cookie_data, // Pass the int as a cookie
        cpp_utils::TimerType::Periodic
    );
    if (periodic_id < 0) {
        std::cerr << "Failed to add periodic timer." << std::endl;
        return 1;
    }

    std::cout << "Adding a one-shot timer (300ms) using default cookie (implicit std::any())." << std::endl;
    int default_cookie_timer_id = tw.addTimer(300,
        [&](std::any c) {
            default_cookie_timer_fired_count++;
            std::cout << "[Callback] Default cookie timer (300ms) fired." << std::endl;
            assert(!c.has_value()); // Default std::any() should not have a value
        }
        // Using overload that defaults cookie to std::any()
    );
     if (default_cookie_timer_id < 0) {
        std::cerr << "Failed to add default cookie timer." << std::endl;
        return 1;
    }


    std::cout << "\nStarting simulation loop (30 ticks, 3 seconds total)..." << std::endl;
    for (int i = 0; i < 30; ++i) {
        std::cout << "Tick " << i + 1 << "/30" << std::endl;
        tw.tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate 100ms passing

        if (i == 14) { // 0-indexed, so i=14 is the 15th tick (after 1.5 seconds)
            std::cout << "\n--- Cancelling periodic timer (ID: " << periodic_id << ") after 1.5 seconds. ---" << std::endl;
            if (tw.cancelTimer(periodic_id)) {
                std::cout << "Periodic timer successfully cancelled." << std::endl;
            } else {
                std::cout << "Failed to cancel periodic timer (already fired and removed, or invalid ID)." << std::endl;
            }
            std::cout << "--- Resuming simulation. ---\n" << std::endl;
        }
    }

    std::cout << "\nSimulation finished." << std::endl;
    std::cout << "One-shot timer (with string cookie) fired count: " << one_shot_fired_count.load() << " (Expected 1)" << std::endl;
    std::cout << "Periodic timer (with int cookie) fired count: " << periodic_fired_count.load() << " (Expected 1, at 1000ms before cancellation at 1500ms)" << std::endl;
    std::cout << "Default cookie timer fired count: " << default_cookie_timer_fired_count.load() << " (Expected 1)" << std::endl;

    if (one_shot_fired_count.load() == 1 && periodic_fired_count.load() == 1 && default_cookie_timer_fired_count.load() == 1) {
        std::cout << "\nExample finished successfully!" << std::endl;
        return 0;
    } else {
        std::cout << "\nExample finished with unexpected results." << std::endl;
        std::cout << "  One-shot (string cookie) expected 1, got " << one_shot_fired_count.load() << std::endl;
        std::cout << "  Periodic (int cookie) expected 1, got " << periodic_fired_count.load() << std::endl;
        std::cout << "  Default cookie timer expected 1, got " << default_cookie_timer_fired_count.load() << std::endl;
        return 1;
    }
}

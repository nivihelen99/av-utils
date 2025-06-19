#include "../include/timer_wheel.hpp" // Adjust path if necessary e.g., "../include/timer_wheel.hpp"
#include <iostream>
#include <thread>   // For std::this_thread::sleep_for
#include <chrono>   // For std::chrono::milliseconds
#include <atomic>   // For std::atomic_int

int main() {
    std::cout << "TimerWheel Example" << std::endl;

    // Create a TimerWheel: 100ms resolution, 50 slots (wheel cycle of 5 seconds)
    cpp_utils::TimerWheel tw(100, 50);
    std::cout << "TimerWheel created with 100ms resolution and 50 slots." << std::endl;

    std::atomic<int> one_shot_fired_count(0);
    std::atomic<int> periodic_fired_count(0);

    // Add a one-shot timer: fires once after 500ms
    std::cout << "Adding a one-shot timer for 500ms." << std::endl;
    int one_shot_id = tw.addTimer(
        500, // delay_ms
        [&]() {
            one_shot_fired_count++;
            std::cout << "[Callback] One-shot timer (500ms) fired! Count: " << one_shot_fired_count.load() << std::endl;
        },
        cpp_utils::TimerType::OneShot
    );
    if (one_shot_id < 0) {
        std::cerr << "Failed to add one-shot timer." << std::endl;
        return 1;
    }

    // Add a periodic timer: fires every 1000ms (1 second)
    std::cout << "Adding a periodic timer for 1000ms interval." << std::endl;
    int periodic_id = tw.addTimer(
        1000, // delay_ms (and interval for periodic)
        [&]() {
            periodic_fired_count++;
            std::cout << "[Callback] Periodic timer (1000ms) fired! Count: " << periodic_fired_count.load() << std::endl;
        },
        cpp_utils::TimerType::Periodic
    );
    if (periodic_id < 0) {
        std::cerr << "Failed to add periodic timer." << std::endl;
        return 1;
    }

    std::cout << "\nStarting simulation loop (30 ticks, 3 seconds total)..." << std::endl;
    // Simulate time advancing by calling tick()
    // Each tick is 100ms. Loop for 30 ticks = 3000ms = 3 seconds.
    for (int i = 0; i < 30; ++i) {
        std::cout << "Tick " << i + 1 << "/30" << std::endl;
        tw.tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate 100ms passing

        // After 1.5 seconds (15 ticks), cancel the periodic timer
        if (i == 14) { // 0-indexed, so i=14 is the 15th tick
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
    std::cout << "One-shot timer fired count: " << one_shot_fired_count.load() << " (Expected 1)" << std::endl;
    std::cout << "Periodic timer fired count: " << periodic_fired_count.load() << " (Expected 1, at 1000ms before cancellation at 1500ms)" << std::endl;

    // Assertions for expected behavior
    if (one_shot_fired_count.load() == 1 && periodic_fired_count.load() == 1) {
        std::cout << "\nExample finished successfully!" << std::endl;
        return 0;
    } else {
        std::cout << "\nExample finished with unexpected results." << std::endl;
        std::cout << "  One-shot expected 1, got " << one_shot_fired_count.load() << std::endl;
        std::cout << "  Periodic expected 1, got " << periodic_fired_count.load() << std::endl;
        return 1;
    }
}

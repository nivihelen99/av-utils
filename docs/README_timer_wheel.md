# `cpp_utils::TimerWheel`

## Overview

The `cpp_utils::TimerWheel` class (`timer_wheel.h`) implements a timer wheel data structure. A timer wheel is an efficient mechanism for managing a large number of timers (scheduled events) with O(1) complexity for starting and cancelling timers, and O(1) amortized complexity for advancing time (processing a "tick").

It works by maintaining a circular array of "slots," where each slot represents a unit of time (the resolution). Timers are placed into a slot corresponding to their expiration time modulo the total wheel size. The wheel also handles timers whose expiration times are beyond the current cycle by using a "rounds" counter.

## Key Concepts

-   **Resolution (`resolution_ms_`):** The smallest unit of time the timer wheel can distinguish, typically in milliseconds. Each slot in the wheel represents one such unit.
-   **Wheel Size (`wheel_size_`):** The number of slots in the wheel. The total time for one full rotation of the wheel is `resolution_ms_ * wheel_size_`.
-   **Ticks (`current_tick_absolute_`):** The timer wheel advances in discrete "ticks." Each call to the `tick()` method effectively advances time by `resolution_ms_`.
-   **Slots (`wheel_`):** A `std::vector` where each element is a `std::list` of timer IDs. Timers are placed into the slot corresponding to their scheduled expiry time.
-   **Rounds (`remaining_rounds`):** For timers whose delay exceeds the wheel's cycle time, a round counter tracks how many full wheel rotations must complete before the timer is eligible to fire.
-   **Timer Object (`Timer` struct):** Internally stores details for each timer, including its ID, callback, type (one-shot or periodic), interval, remaining rounds, and a `std::any` cookie for user data.
-   **Callbacks (`TimerCallback`):** Functions of type `std::function<void(std::any cookie)>` that are executed when a timer expires.

## Features

-   **Efficient Timer Management:** O(1) for adding and cancelling timers. `tick()` operation is O(1) plus O(k) where k is the number of timers expiring at the current tick.
-   **One-Shot and Periodic Timers:** Supports both types of timers.
-   **Customizable Resolution and Size:** The granularity and cycle time of the wheel can be configured.
-   **User Data (Cookies):** Allows associating arbitrary data (via `std::any`) with each timer, which is passed to its callback.
-   **ID-Based Cancellation:** Timers can be cancelled using their unique integer ID.

## Public Interface

### Constructor
-   **`TimerWheel(size_t resolution_ms, size_t wheel_size)`**:
    -   `resolution_ms`: The time duration each slot represents, in milliseconds.
    -   `wheel_size`: The number of slots in the wheel.
    -   Throws `std::invalid_argument` if `resolution_ms` or `wheel_size` is 0.

### Timer Management
-   **`int addTimer(uint64_t delay_ms, TimerCallback cb, std::any cookie, TimerType type = TimerType::OneShot)`**:
    -   Schedules a timer.
    -   `delay_ms`: Delay until the first execution (for one-shot) or the interval (for periodic).
    -   `cb`: The callback function to execute.
    -   `cookie`: User data passed to the callback.
    -   `type`: `TimerType::OneShot` or `TimerType::Periodic`.
    -   Returns a unique timer ID (integer).
-   **`int addTimer(uint64_t delay_ms, TimerCallback cb, TimerType type = TimerType::OneShot)`**:
    -   Overload that uses a default (empty) `std::any` cookie.
-   **`bool cancelTimer(int timer_id)`**:
    -   Cancels an active timer. Returns `true` if successful.

### Advancing Time
-   **`void tick()`**:
    -   Advances the timer wheel by one resolution unit.
    -   Processes the current slot, executing callbacks for timers that expire at this tick (and have 0 remaining rounds).
    -   Reschedules periodic timers.
    -   This method should be called regularly by the application (e.g., from a dedicated timer thread or event loop) at intervals matching `resolution_ms_`.

## Usage Examples

(Based on `examples/timer_wheel_example.cpp`)

### Basic Setup and Adding Timers

```cpp
#include "timer_wheel.h" // Adjust path as needed
#include <iostream>
#include <string>
#include <thread>   // For std::this_thread::sleep_for
#include <chrono>   // For std::chrono literals
#include <any>      // For std::any, std::any_cast
#include <atomic>   // For std::atomic in examples

// For std::chrono_literals like `ms`
using namespace std::chrono_literals;

std::atomic<int> counter_a(0);
std::atomic<int> counter_b(0);

void callback_a(std::any cookie) {
    counter_a++;
    std::string data = std::any_cast<std::string>(cookie);
    std::cout << "[Timer A fired] Count: " << counter_a.load()
              << ", Cookie: " << data << std::endl;
}

void callback_b(std::any cookie) {
    counter_b++;
    int data = std::any_cast<int>(cookie);
    std::cout << "[Timer B fired] Count: " << counter_b.load()
              << ", Cookie: " << data << std::endl;
}

int main() {
    // Timer wheel with 50ms resolution and 20 slots (1-second cycle)
    cpp_utils::TimerWheel timer_wheel(50, 20);
    std::cout << "TimerWheel created (50ms resolution, 20 slots)." << std::endl;

    // Add a one-shot timer to fire after 200ms
    std::string cookie_a_data = "TaskA_Context";
    int timer_a_id = timer_wheel.addTimer(200, callback_a, cookie_a_data, cpp_utils::TimerType::OneShot);
    std::cout << "Added OneShot Timer A (200ms), ID: " << timer_a_id << std::endl;

    // Add a periodic timer to fire every 300ms, starting after an initial 300ms delay
    int cookie_b_data = 12345;
    int timer_b_id = timer_wheel.addTimer(300, callback_b, cookie_b_data, cpp_utils::TimerType::Periodic);
    std::cout << "Added Periodic Timer B (300ms interval), ID: " << timer_b_id << std::endl;

    // Simulate time passing by calling tick()
    std::cout << "\nSimulating time (10 ticks, 500ms total):" << std::endl;
    for (int i = 0; i < 10; ++i) { // 10 ticks * 50ms/tick = 500ms
        std::cout << "Tick #" << (i + 1) << std::endl;
        timer_wheel.tick();
        std::this_thread::sleep_for(50ms); // Simulate time passing equivalent to resolution
    }
    // Expected: Timer A fires around tick 4-5. Timer B fires around tick 6-7.

    std::cout << "\nCancelling periodic timer B." << std::endl;
    timer_wheel.cancelTimer(timer_b_id);

    std::cout << "\nSimulating more time (10 more ticks, 500ms total):" << std::endl;
    for (int i = 0; i < 10; ++i) {
        std::cout << "Tick #" << (i + 11) << std::endl;
        timer_wheel.tick();
        std::this_thread::sleep_for(50ms);
    }
    // Expected: Timer A should not fire again. Timer B should not fire again.

    std::cout << "\nFinal counts:" << std::endl;
    std::cout << "Timer A (OneShot) fired: " << counter_a.load() << " times." << std::endl;
    std::cout << "Timer B (Periodic) fired: " << counter_b.load() << " times." << std::endl;
    // Expected for Timer B depends on exact timing of cancellation relative to its period.
    // If it fired once before cancellation, count will be 1.
}
```

## Dependencies
- `<vector>`, `<list>`, `<functional>`, `<cstdint>`, `<unordered_map>`, `<memory>`, `<stdexcept>`, `<utility>`, `<algorithm>`, `<cmath>`, `<any>`, `<chrono>`, `<thread>` (thread for sleep in examples).

The `TimerWheel` is a classic data structure for managing a large number of timers efficiently, particularly in systems like network stacks, game engines, or event-driven applications where many timed events need to be scheduled and processed with low overhead.

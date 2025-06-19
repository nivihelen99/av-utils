#include "../include/timer_wheel.hpp" // Adjust path if necessary
#include <cassert>
#include <iostream>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>

// Forward declare the debug accessor if we can't modify timer_wheel.hpp directly in this subtask run
// namespace cpp_utils { class TimerWheel { public: size_t DEBUG_getCurrentTickAbsolute() const; }; }


void printTestHeader(const std::string& test_name) {
    std::cout << "\n--- " << test_name << " ---" << std::endl;
}

// Test 1: One-shot timer fires once and is removed
void test_one_shot_timer_fires_once() {
    printTestHeader("TestOneShotTimerFiresOnce");
    cpp_utils::TimerWheel tw(10, 100);
    std::atomic<int> fire_count(0);
    // Corrected Logic A: delay_ms=50 -> num_total_ticks_to_wait=5. Effective fire at CTA_proc = CTA_add + (5-1) = 4.
    int timer_id = tw.addTimer(50, [&]() { fire_count++; std::cout << "One-shot timer fired." << std::endl; });
    for (int i = 0; i < 10; ++i) { tw.tick(); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    assert(fire_count == 1);
    assert(!tw.cancelTimer(timer_id));
    std::cout << "TestOneShotTimerFiresOnce PASSED" << std::endl;
}

// Test 2: Periodic timer fires repeatedly until cancelled
void test_periodic_timer_fires_repeatedly() {
    printTestHeader("TestPeriodicTimerFiresRepeatedly");
    cpp_utils::TimerWheel tw(10, 50);
    std::atomic<int> fire_count(0);
    // Corrected Logic A: delay_ms=30 -> num_total_ticks_to_wait=3. Effective interval (3-1)*10 = 20ms.
    // Fires at CTA_proc=2 (20ms), 4 (40ms), 6 (60ms), 8 (80ms). Total 4 firings in 10 ticks (100ms).
    int timer_id = tw.addTimer(30, [&]() { fire_count++; std::cout << "Periodic timer fired (" << fire_count << ")" << std::endl; }, cpp_utils::TimerType::Periodic);
    for (int i = 0; i < 10; ++i) { tw.tick(); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    assert(fire_count == 4);
    assert(tw.cancelTimer(timer_id));
    for (int i = 0; i < 10; ++i) { tw.tick(); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    assert(fire_count == 4);
    std::cout << "TestPeriodicTimerFiresRepeatedly PASSED (expected 4 firings)" << std::endl;
}

// Test 3: Multiple one-shot timers with staggered delays
void test_multiple_one_shot_staggered() {
    printTestHeader("TestMultipleOneShotStaggered");
    cpp_utils::TimerWheel tw(10, 100);
    std::atomic<int> fire_count1(0), fire_count2(0), fire_count3(0);
    // Corrected Logic A: Effective fire at CTA_proc = CTA_add + (num_ticks-1)
    // Timer 1 (20ms): num_ticks=2. Fires CTA_proc = 0 + (2-1) = 1. (Loop i=1)
    // Timer 3 (30ms): num_ticks=3. Fires CTA_proc = 0 + (3-1) = 2. (Loop i=2)
    // Timer 2 (50ms): num_ticks=5. Fires CTA_proc = 0 + (5-1) = 4. (Loop i=4)
    tw.addTimer(20, [&](){ fire_count1++; std::cout << "Timer 1 (20ms) fired." << std::endl; });
    tw.addTimer(50, [&](){ fire_count2++; std::cout << "Timer 2 (50ms) fired." << std::endl; });
    tw.addTimer(30, [&](){ fire_count3++; std::cout << "Timer 3 (30ms) fired." << std::endl; });
    for(int i = 0; i < 7; ++i) {
        tw.tick(); // CTA_proc for this iteration is 'i'. CTA becomes i+1 afterwards.
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
         // Iteration i=1: CTA_proc=1. Timer 1 (20ms) fires.
         if (i == 1) { assert(fire_count1 == 1 && fire_count2 == 0 && fire_count3 == 0); }
         // Iteration i=2: CTA_proc=2. Timer 3 (30ms) fires.
         if (i == 2) { assert(fire_count1 == 1 && fire_count2 == 0 && fire_count3 == 1); }
         // Iteration i=4: CTA_proc=4. Timer 2 (50ms) fires.
         if (i == 4) { assert(fire_count1 == 1 && fire_count2 == 1 && fire_count3 == 1); }
    }
    assert(fire_count1 == 1 && fire_count2 == 1 && fire_count3 == 1);
    std::cout << "TestMultipleOneShotStaggered PASSED" << std::endl;
}

// Test 4: Multiple periodic timers with different intervals
void test_multiple_periodic_different_intervals() {
    printTestHeader("TestMultiplePeriodicDifferentIntervals");
    cpp_utils::TimerWheel tw(10, 100);
    std::atomic<int> fire_A(0), fire_B(0);
    // Corrected Logic A: Effective interval = (num_ticks-1)*R
    // Timer A (20ms): num_ticks=2. Effective interval (2-1)*10=10ms. Fires CTA=1,2,3,4,5,6,7,8,9,10,11. (11 times in 12 ticks)
    // Timer B (30ms): num_ticks=3. Effective interval (3-1)*10=20ms. Fires CTA=2,4,6,8,10. (5 times in 12 ticks)
    // The test loop is for 12 ticks (i=0 to 11). CTA_proc goes from 0 to 11.
    int idA = tw.addTimer(20, [&](){ fire_A++; std::cout << "Timer A fires, count " << fire_A.load() << std::endl; }, cpp_utils::TimerType::Periodic);
    int idB = tw.addTimer(30, [&](){ fire_B++; std::cout << "Timer B fires, count " << fire_B.load() << std::endl; }, cpp_utils::TimerType::Periodic);
    for(int i = 0; i < 12; ++i) { tw.tick(); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }

    // Recalculate expected counts for Corrected Logic A:
    // Timer A (eff. 10ms interval): Fires when CTA_proc = 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11. (11 times)
    // Timer B (eff. 20ms interval): Fires when CTA_proc = 2, 4, 6, 8, 10. (5 times)
    assert(fire_A == 11);
    assert(fire_B == 5);
    tw.cancelTimer(idA); tw.cancelTimer(idB);
    std::cout << "TestMultiplePeriodicDifferentIntervals PASSED" << std::endl;
}

// Test 5: Cancel one-shot before it fires
void test_cancel_one_shot_before_fire() {
    printTestHeader("TestCancelOneShotBeforeFire");
    cpp_utils::TimerWheel tw(10, 100);
    std::atomic<int> fire_count(0);
    int timer_id = tw.addTimer(50, [&](){ fire_count++; });
    assert(tw.cancelTimer(timer_id));
    for(int i = 0; i < 7; ++i) { tw.tick(); std::this_thread::sleep_for(std::chrono::milliseconds(1));}
    assert(fire_count == 0);
    std::cout << "TestCancelOneShotBeforeFire PASSED" << std::endl;
}

// Test 7: Timer added from within a callback
void test_timer_added_from_callback() {
    printTestHeader("TestTimerAddedFromCallback");
    cpp_utils::TimerWheel tw(10, 100);
    std::atomic<int> fire_count_outer(0);
    std::atomic<int> fire_count_inner(0);

    // Outer (20ms): num_ticks=2. Fires CTA_proc = 0 + (2-1) = 1. (During loop i=1)
    // Inner (30ms): num_ticks=3. Added when CTA_for_calc = 1 (outer's firing tick).
    // Inner fires CTA_proc = CTA_for_calc + (num_ticks_inner-1) = 1 + (3-1) = 1 + 2 = 3. (During loop i=3)
    tw.addTimer(20, [&]() {
        fire_count_outer++;
        std::cout << "Outer timer (20ms) fired." << std::endl;
        tw.addTimer(30, [&]() {
            fire_count_inner++;
            std::cout << "Inner timer (30ms from outer) fired." << std::endl;
        });
    });

    // Loop i from 0 to 6. CTA_proc takes values 0,1,2,3,4,5,6.
    // Outer fires at CTA_proc=1. Inner fires at CTA_proc=3.
    for(int i = 0; i < 7; ++i) { tw.tick(); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    assert(fire_count_outer == 1);
    assert(fire_count_inner == 1);
    std::cout << "TestTimerAddedFromCallback: Outer fired=" << fire_count_outer << ", Inner fired=" << fire_count_inner << "." << std::endl;
}

// Test 8: Timer with delay > full wheel cycle
void test_timer_delay_greater_than_wheel_cycle() {
    printTestHeader("TestTimerDelayGreaterThanWheelCycle");
    cpp_utils::TimerWheel tw(10, 20); // Wheel cycle = 200ms
    std::atomic<int> fire_count(0);
    // delay_ms=250 -> num_total_ticks_to_wait=25.
    // Effective fire at CTA_proc = CTA_add + (25-1) = 24. (During loop i=24)
    tw.addTimer(250, [&](){ fire_count++; std::cout << "Long delay timer (250ms) fired." << std::endl; });
    for(int i = 0; i < 30; ++i) { tw.tick(); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    assert(fire_count == 1);
    std::cout << "TestTimerDelayGreaterThanWheelCycle PASSED" << std::endl;
}

// Test 9: Tick under load (thousands of timers)
void test_tick_under_load() {
    printTestHeader("TestTickUnderLoad");
    cpp_utils::TimerWheel tw(1, 1000);
    std::atomic<int> fire_count(0);
    const int num_timers = 2000;
    for(int i = 0; i < num_timers; ++i) {
        // Delays from 10ms to 509ms.
        // Corrected Logic A: effective delay (num_ticks-1)*R.
        // Smallest delay 10ms -> num_ticks=10. Fires at (10-1)*1=9ms.
        // Largest delay 509ms -> num_ticks=509. Fires at (509-1)*1=508ms.
        // All should fire within 600ms.
        tw.addTimer(10 + (i % 500), [&](){ fire_count++; });
    }
    std::cout << "Added " << num_timers << " timers. Advancing time..." << std::endl;
    for(int i = 0; i < 600; ++i) { // Loop 600 times, CTA_proc 0 to 599.
        tw.tick();
        if (i % 100 == 0) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::cout << "Load test: Fired " << fire_count.load() << " timers out of " << num_timers << std::endl;
    assert(fire_count == num_timers);
    std::cout << "TestTickUnderLoad PASSED" << std::endl;
}

int main() {
    std::cout << "TimerWheel Tests (using Corrected Logic A expectations)" << std::endl;
    test_one_shot_timer_fires_once();
    test_periodic_timer_fires_repeatedly(); // Expects 4 firings with Corrected Logic A
    test_multiple_one_shot_staggered();
    test_multiple_periodic_different_intervals(); // Expected counts updated for Corrected Logic A
    test_cancel_one_shot_before_fire();
    test_timer_added_from_callback();
    test_timer_delay_greater_than_wheel_cycle();
    test_tick_under_load();

    std::cout << "\nAll TimerWheel tests completed." << std::endl;
    return 0;
}

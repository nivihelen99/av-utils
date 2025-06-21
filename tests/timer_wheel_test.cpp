#include "timer_wheel.h" // Adjust path if necessary
#include <cassert>
#include <iostream>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>
#include <string> // For std::string in cookie example
#include <any>    // For std::any and std::any_cast

void printTestHeader(const std::string& test_name) {
    std::cout << "\n--- " << test_name << " ---" << std::endl;
}

// Test 1: One-shot timer fires once and is removed, checking cookie
void test_one_shot_timer_fires_once() {
    printTestHeader("TestOneShotTimerFiresOnce");
    cpp_utils::TimerWheel tw(10, 100);
    std::atomic<int> fire_count(0);
    std::string cookie_data = "one_shot_cookie_data";

    int timer_id = tw.addTimer(50,
        [&](std::any c) {
            fire_count++;
            std::cout << "One-shot timer fired.";
            if (c.has_value()) {
                try {
                    const std::string& received_cookie = std::any_cast<const std::string&>(c);
                    assert(received_cookie == cookie_data);
                    std::cout << " Cookie: " << received_cookie << std::endl;
                } catch (const std::bad_any_cast& e) {
                    std::cerr << " Bad any_cast: " << e.what() << std::endl;
                    assert(false);
                }
            } else {
                std::cout << " Cookie empty." << std::endl;
                assert(false); // Should have a cookie
            }
        },
        cookie_data // Pass string cookie
    );

    for (int i = 0; i < 10; ++i) { tw.tick(); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    assert(fire_count == 1);
    assert(!tw.cancelTimer(timer_id));
    std::cout << "TestOneShotTimerFiresOnce PASSED" << std::endl;
}

// Test 2: Periodic timer fires repeatedly until cancelled, checking cookie
void test_periodic_timer_fires_repeatedly() {
    printTestHeader("TestPeriodicTimerFiresRepeatedly");
    cpp_utils::TimerWheel tw(10, 50);
    std::atomic<int> fire_count(0);
    int periodic_cookie_val = 42; // Test with an int cookie

    int timer_id = tw.addTimer(30,
        [&](std::any c) {
            fire_count++;
            std::cout << "Periodic timer fired (" << fire_count << ")";
            if (c.has_value()) {
                try {
                    int val = std::any_cast<int>(c);
                    assert(val == periodic_cookie_val);
                    std::cout << " with cookie: " << val << std::endl;
                } catch (const std::bad_any_cast& e) {
                    std::cerr << " Bad any_cast: " << e.what() << std::endl;
                    assert(false);
                }
            } else {
                 std::cout << " Cookie empty." << std::endl;
                 assert(false); // Should have a cookie
            }
        },
        periodic_cookie_val, // Pass int cookie
        cpp_utils::TimerType::Periodic
    );

    for (int i = 0; i < 10; ++i) { tw.tick(); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    assert(fire_count == 4);
    assert(tw.cancelTimer(timer_id));
    for (int i = 0; i < 10; ++i) { tw.tick(); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    assert(fire_count == 4);
    std::cout << "TestPeriodicTimerFiresRepeatedly PASSED (expected 4 firings)" << std::endl;
}

// Test 3: Multiple one-shot timers with staggered delays (using default/empty cookies)
void test_multiple_one_shot_staggered() {
    printTestHeader("TestMultipleOneShotStaggered");
    cpp_utils::TimerWheel tw(10, 100);
    std::atomic<int> fire_count1(0), fire_count2(0), fire_count3(0);

    tw.addTimer(20, [&](std::any /*c*/){ fire_count1++; std::cout << "Timer 1 (20ms) fired." << std::endl; });
    tw.addTimer(50, [&](std::any /*c*/){ fire_count2++; std::cout << "Timer 2 (50ms) fired." << std::endl; });
    tw.addTimer(30, [&](std::any /*c*/){ fire_count3++; std::cout << "Timer 3 (30ms) fired." << std::endl; });

    for(int i = 0; i < 7; ++i) {
        tw.tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
         if (i == 1) { assert(fire_count1 == 1 && fire_count2 == 0 && fire_count3 == 0); }
         if (i == 2) { assert(fire_count1 == 1 && fire_count2 == 0 && fire_count3 == 1); }
         if (i == 4) { assert(fire_count1 == 1 && fire_count2 == 1 && fire_count3 == 1); }
    }
    assert(fire_count1 == 1 && fire_count2 == 1 && fire_count3 == 1);
    std::cout << "TestMultipleOneShotStaggered PASSED" << std::endl;
}

// Test 4: Multiple periodic timers with different intervals (using default/empty cookies)
void test_multiple_periodic_different_intervals() {
    printTestHeader("TestMultiplePeriodicDifferentIntervals");
    cpp_utils::TimerWheel tw(10, 100);
    std::atomic<int> fire_A(0), fire_B(0);

    int idA = tw.addTimer(20, [&](std::any /*c*/){ fire_A++; std::cout << "Timer A fires, count " << fire_A.load() << std::endl; }, cpp_utils::TimerType::Periodic);
    int idB = tw.addTimer(30, [&](std::any /*c*/){ fire_B++; std::cout << "Timer B fires, count " << fire_B.load() << std::endl; }, cpp_utils::TimerType::Periodic);

    for(int i = 0; i < 12; ++i) { tw.tick(); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }

    assert(fire_A == 11);
    assert(fire_B == 5);
    tw.cancelTimer(idA); tw.cancelTimer(idB);
    std::cout << "TestMultiplePeriodicDifferentIntervals PASSED" << std::endl;
}

// Test 5: Cancel one-shot before it fires (using default/empty cookie)
void test_cancel_one_shot_before_fire() {
    printTestHeader("TestCancelOneShotBeforeFire");
    cpp_utils::TimerWheel tw(10, 100);
    std::atomic<int> fire_count(0);
    int timer_id = tw.addTimer(50, [&](std::any /*c*/){ fire_count++; });
    assert(tw.cancelTimer(timer_id));
    for(int i = 0; i < 7; ++i) { tw.tick(); std::this_thread::sleep_for(std::chrono::milliseconds(1));}
    assert(fire_count == 0);
    std::cout << "TestCancelOneShotBeforeFire PASSED" << std::endl;
}

// Test 7: Timer added from within a callback, checking cookies
void test_timer_added_from_callback() {
    printTestHeader("TestTimerAddedFromCallback");
    cpp_utils::TimerWheel tw(10, 100);
    std::atomic<int> fire_count_outer(0);
    std::atomic<int> fire_count_inner(0);
    std::string outer_cookie_data = "outer_payload";
    int inner_cookie_data = 777;

    tw.addTimer(20,
        [&, outer_cookie_data, inner_cookie_data](std::any c_outer) {
            fire_count_outer++;
            std::cout << "Outer timer (20ms) fired.";
            if(c_outer.has_value()) assert(std::any_cast<const std::string&>(c_outer) == outer_cookie_data);
            else assert(false);
            std::cout << " Outer cookie verified." << std::endl;

            tw.addTimer(30,
                [&, inner_cookie_data](std::any c_inner) {
                    fire_count_inner++;
                    std::cout << "Inner timer (30ms from outer) fired.";
                    if(c_inner.has_value()) assert(std::any_cast<int>(c_inner) == inner_cookie_data);
                    else assert(false);
                    std::cout << " Inner cookie verified." << std::endl;
                },
                inner_cookie_data // Pass int cookie for inner
            );
        },
        outer_cookie_data // Pass string cookie for outer
    );

    for(int i = 0; i < 7; ++i) { tw.tick(); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    assert(fire_count_outer == 1);
    assert(fire_count_inner == 1);
    std::cout << "TestTimerAddedFromCallback: Outer fired=" << fire_count_outer << ", Inner fired=" << fire_count_inner << "." << std::endl;
}

// Test 8: Timer with delay > full wheel cycle (using default/empty cookie)
void test_timer_delay_greater_than_wheel_cycle() {
    printTestHeader("TestTimerDelayGreaterThanWheelCycle");
    cpp_utils::TimerWheel tw(10, 20);
    std::atomic<int> fire_count(0);
    tw.addTimer(250, [&](std::any /*c*/){ fire_count++; std::cout << "Long delay timer (250ms) fired." << std::endl; });
    for(int i = 0; i < 30; ++i) { tw.tick(); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    assert(fire_count == 1);
    std::cout << "TestTimerDelayGreaterThanWheelCycle PASSED" << std::endl;
}

// Test 9: Tick under load (thousands of timers) (using default/empty cookies)
void test_tick_under_load() {
    printTestHeader("TestTickUnderLoad");
    cpp_utils::TimerWheel tw(1, 1000);
    std::atomic<int> fire_count(0);
    const int num_timers = 2000;
    for(int i = 0; i < num_timers; ++i) {
        tw.addTimer(10 + (i % 500), [&](std::any /*c*/){ fire_count++; });
    }
    std::cout << "Added " << num_timers << " timers. Advancing time..." << std::endl;
    for(int i = 0; i < 600; ++i) {
        tw.tick();
        if (i % 100 == 0) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::cout << "Load test: Fired " << fire_count.load() << " timers out of " << num_timers << std::endl;
    assert(fire_count == num_timers);
    std::cout << "TestTickUnderLoad PASSED" << std::endl;
}

int main() {
    std::cout << "TimerWheel Tests (using Corrected Logic A expectations, with std::any cookie)" << std::endl;
    test_one_shot_timer_fires_once();
    test_periodic_timer_fires_repeatedly();
    test_multiple_one_shot_staggered();
    test_multiple_periodic_different_intervals();
    test_cancel_one_shot_before_fire();
    test_timer_added_from_callback();
    test_timer_delay_greater_than_wheel_cycle();
    test_tick_under_load();

    std::cout << "\nAll TimerWheel tests completed." << std::endl;
    return 0;
}

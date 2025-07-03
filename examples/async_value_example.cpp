#include "async_value.h"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <memory> // For std::unique_ptr
#include <chrono> // For std::this_thread::sleep_for

// Function to simulate some work and then set a value
void producer_int(AsyncValue<int>& av, int val_to_set, int delay_ms) {
    std::cout << "[Producer Int] Working for " << delay_ms << "ms..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    std::cout << "[Producer Int] Setting value: " << val_to_set << std::endl;
    av.set_value(val_to_set);
    std::cout << "[Producer Int] Value set." << std::endl;
}

void producer_string(AsyncValue<std::string>& av, std::string val_to_set, int delay_ms) {
    std::cout << "[Producer Str] Working for " << delay_ms << "ms..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    std::cout << "[Producer Str] Setting value: \"" << val_to_set << "\"" << std::endl;
    av.set_value(val_to_set);
    std::cout << "[Producer Str] Value set." << std::endl;
}

void producer_void(AsyncValue<void>& av, int delay_ms) {
    std::cout << "[Producer Void] Working for " << delay_ms << "ms..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    std::cout << "[Producer Void] Signaling event." << std::endl;
    av.set();
    std::cout << "[Producer Void] Event signaled." << std::endl;
}

void producer_unique_ptr(AsyncValue<std::unique_ptr<std::string>>& av, std::string val_to_set, int delay_ms) {
    std::cout << "[Producer UPtr] Working for " << delay_ms << "ms..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    auto ptr = std::make_unique<std::string>(val_to_set);
    std::cout << "[Producer UPtr] Setting unique_ptr with value: \"" << *ptr << "\"" << std::endl;
    av.set_value(std::move(ptr));
    std::cout << "[Producer UPtr] Unique_ptr set." << std::endl;
}


int main() {
    std::cout << "--- AsyncValue<int> Example ---" << std::endl;
    AsyncValue<int> async_int;

    // Register callback before value is set
    async_int.on_ready([](int v) {
        std::cout << "[Consumer Int] Received async value via on_ready: " << v << std::endl;
    });

    std::thread t1(producer_int, std::ref(async_int), 42, 100);

    // Try to get value (will likely not be ready immediately)
    std::cout << "[Main Thread] Checking int value (non-blocking)..." << std::endl;
    if (async_int.ready()) {
        std::cout << "[Main Thread] Int value was ready: " << async_int.get() << std::endl;
    } else {
        std::cout << "[Main Thread] Int value not ready yet." << std::endl;
    }

    t1.join(); // Wait for producer to finish

    std::cout << "[Main Thread] After producer joined, int value is: " << async_int.get() << std::endl;

    // Example of on_ready after value is set
    async_int.on_ready([](int v){
        std::cout << "[Consumer Int] Second on_ready (after set) also received: " << v << std::endl;
    });

    std::cout << "\n--- AsyncValue<std::string> Example ---" << std::endl;
    AsyncValue<std::string> async_str;
    std::thread t2(producer_string, std::ref(async_str), "Hello from another thread!", 150);

    async_str.on_ready([](const std::string& s) {
        std::cout << "[Consumer Str] Received async string: \"" << s << "\"" << std::endl;
    });

    t2.join();
    if(async_str.ready()){
        std::cout << "[Main Thread] String value: \"" << async_str.get() << "\"" << std::endl;
    }

    std::cout << "\n--- AsyncValue<void> (Event Signaling) Example ---" << std::endl;
    AsyncValue<void> async_event;
    bool event_fired_flag = false;

    async_event.on_ready([&]() {
        std::cout << "[Consumer Void] Async event has fired!" << std::endl;
        event_fired_flag = true;
    });

    std::thread t3(producer_void, std::ref(async_event), 50);
    t3.join();

    if (async_event.ready()) {
        std::cout << "[Main Thread] Event is ready. Flag: " << (event_fired_flag ? "true" : "false") << std::endl;
        async_event.get(); // Should not throw
    } else {
        std::cout << "[Main Thread] Event not ready. This shouldn't happen if t3 joined." << std::endl;
    }


    std::cout << "\n--- AsyncValue<std::unique_ptr<std::string>> Example ---" << std::endl;
    AsyncValue<std::unique_ptr<std::string>> async_uptr;

    async_uptr.on_ready([](const std::unique_ptr<std::string>& p_val) {
        if (p_val) {
            std::cout << "[Consumer UPtr] Received unique_ptr with value: \"" << *p_val << "\"" << std::endl;
        } else {
            std::cout << "[Consumer UPtr] Received null unique_ptr (error)." << std::endl;
        }
    });

    std::thread t4(producer_unique_ptr, std::ref(async_uptr), "Move me!", 70);
    t4.join();

    if (async_uptr.ready()) {
        const std::unique_ptr<std::string>* p_val_ptr = async_uptr.get_if();
        if (p_val_ptr && *p_val_ptr) {
            std::cout << "[Main Thread] Unique_ptr value: \"" << **p_val_ptr << "\"" << std::endl;
        } else {
            std::cout << "[Main Thread] Unique_ptr not available or null." << std::endl;
        }
    }

    std::cout << "\n--- Reset Example ---" << std::endl;
    AsyncValue<int> resettable_av;
    resettable_av.set_value(100);
    std::cout << "[Main Thread] Resettable AV value: " << resettable_av.get() << std::endl;
    resettable_av.reset();
    std::cout << "[Main Thread] Resettable AV ready after reset: " << resettable_av.ready() << std::endl;

    bool reset_cb_fired = false;
    resettable_av.on_ready([&](int val){
        std::cout << "[Consumer Reset] Resettable AV got value after reset: " << val << std::endl;
        reset_cb_fired = true;
    });
    producer_int(std::ref(resettable_av), 200, 30); // Re-use with a new producer call
    // No join here, but the producer_int itself calls set_value which should trigger callback.
    // To ensure output order for demo, let's wait a bit.
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    if (!reset_cb_fired) {
         std::cout << "[Main Thread] Warning: Reset callback might not have fired in time for this print." << std::endl;
    }


    std::cout << "\nExample finished." << std::endl;
    return 0;
}

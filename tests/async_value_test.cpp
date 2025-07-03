#include "gtest/gtest.h"
#include "async_value.h"
#include <thread>
#include <vector>
#include <string>
#include <memory> // For std::unique_ptr tests
#include <chrono> // For sleeps in tests

// Basic test fixture
class AsyncValueTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup
    }

    void TearDown() override {
        // Common teardown
    }
};

// Test case for AsyncValue<T>
TEST_F(AsyncValueTest, Int_SetValueBeforeOnReady) {
    AsyncValue<int> av;
    ASSERT_FALSE(av.ready());
    av.set_value(42);
    ASSERT_TRUE(av.ready());
    ASSERT_NE(av.get_if(), nullptr);
    ASSERT_EQ(*av.get_if(), 42);
    ASSERT_EQ(av.get(), 42);

    bool callback_fired = false;
    av.on_ready([&](int val) {
        ASSERT_EQ(val, 42);
        callback_fired = true;
    });
    ASSERT_TRUE(callback_fired);
}

TEST_F(AsyncValueTest, Int_OnReadyBeforeSetValue) {
    AsyncValue<int> av;
    bool callback_fired = false;
    int received_value = 0;

    av.on_ready([&](int val) {
        received_value = val;
        callback_fired = true;
    });

    ASSERT_FALSE(av.ready());
    ASSERT_FALSE(callback_fired);

    av.set_value(123);

    ASSERT_TRUE(av.ready());
    ASSERT_TRUE(callback_fired);
    ASSERT_EQ(received_value, 123);
    ASSERT_NE(av.get_if(), nullptr);
    ASSERT_EQ(*av.get_if(), 123);
    ASSERT_EQ(av.get(), 123);
}

TEST_F(AsyncValueTest, Int_MultipleCallbacks) {
    AsyncValue<int> av;
    int callback1_val = 0;
    int callback2_val = 0;
    bool cb1_fired = false;
    bool cb2_fired = false;

    av.on_ready([&](int val) {
        callback1_val = val;
        cb1_fired = true;
    });
    av.on_ready([&](int val) {
        callback2_val = val;
        cb2_fired = true;
    });

    av.set_value(77);

    ASSERT_TRUE(cb1_fired);
    ASSERT_TRUE(cb2_fired);
    ASSERT_EQ(callback1_val, 77);
    ASSERT_EQ(callback2_val, 77);
}

TEST_F(AsyncValueTest, Int_GetIfNullWhenNotReady) {
    AsyncValue<int> av;
    ASSERT_FALSE(av.ready());
    ASSERT_EQ(av.get_if(), nullptr);
    const AsyncValue<int>& cav = av;
    ASSERT_EQ(cav.get_if(), nullptr);
}

TEST_F(AsyncValueTest, Int_GetThrowsWhenNotReady) {
    AsyncValue<int> av;
    ASSERT_FALSE(av.ready());
    EXPECT_THROW(av.get(), std::logic_error);
    const AsyncValue<int>& cav = av;
    EXPECT_THROW(cav.get(), std::logic_error);
}

TEST_F(AsyncValueTest, Int_Reset) {
    AsyncValue<int> av;
    av.set_value(10);
    ASSERT_TRUE(av.ready());
    ASSERT_EQ(av.get(), 10);

    bool callback_should_not_fire = true;
    av.on_ready([&](int /*val*/) { // This callback is registered after set, so it fires immediately
        callback_should_not_fire = false;
    });
    ASSERT_FALSE(callback_should_not_fire); // It did fire.

    av.reset();
    ASSERT_FALSE(av.ready());
    ASSERT_EQ(av.get_if(), nullptr);
    EXPECT_THROW(av.get(), std::logic_error);

    bool callback_after_reset_fired = false;
    av.on_ready([&](int val) {
        ASSERT_EQ(val, 20);
        callback_after_reset_fired = true;
    });

    ASSERT_FALSE(callback_after_reset_fired);
    av.set_value(20); // Set value again after reset
    ASSERT_TRUE(av.ready());
    ASSERT_EQ(av.get(), 20);
    ASSERT_TRUE(callback_after_reset_fired);
}

TEST_F(AsyncValueTest, String_SetValue) {
    AsyncValue<std::string> av_str;
    std::string test_str = "hello async";
    av_str.set_value(test_str);
    ASSERT_TRUE(av_str.ready());
    ASSERT_EQ(av_str.get(), test_str);

    bool fired = false;
    av_str.on_ready([&](const std::string& s) {
        ASSERT_EQ(s, test_str);
        fired = true;
    });
    ASSERT_TRUE(fired);
}

// Test case for AsyncValue<void>
TEST_F(AsyncValueTest, Void_SetBeforeOnReady) {
    AsyncValue<void> av;
    ASSERT_FALSE(av.ready());
    av.set();
    ASSERT_TRUE(av.ready());
    av.get(); // Should not throw

    bool callback_fired = false;
    av.on_ready([&]() {
        callback_fired = true;
    });
    ASSERT_TRUE(callback_fired);
}

TEST_F(AsyncValueTest, Void_OnReadyBeforeSet) {
    AsyncValue<void> av;
    bool callback_fired = false;

    av.on_ready([&]() {
        callback_fired = true;
    });

    ASSERT_FALSE(av.ready());
    ASSERT_FALSE(callback_fired);
    EXPECT_DEATH(av.get(), ".*AsyncValue<void>::get\\(\\) called before event was set.*"); // Using death test for assert

    av.set();

    ASSERT_TRUE(av.ready());
    ASSERT_TRUE(callback_fired);
    av.get(); // Should not throw
}

TEST_F(AsyncValueTest, Void_MultipleCallbacks) {
    AsyncValue<void> av;
    bool cb1_fired = false;
    bool cb2_fired = false;

    av.on_ready([&]() { cb1_fired = true; });
    av.on_ready([&]() { cb2_fired = true; });

    av.set();

    ASSERT_TRUE(cb1_fired);
    ASSERT_TRUE(cb2_fired);
}

TEST_F(AsyncValueTest, Void_Reset) {
    AsyncValue<void> av;
    av.set();
    ASSERT_TRUE(av.ready());

    bool callback_should_not_fire = true;
    av.on_ready([&]() { // This callback is registered after set, so it fires immediately
         callback_should_not_fire = false;
    });
    ASSERT_FALSE(callback_should_not_fire);

    av.reset();
    ASSERT_FALSE(av.ready());
    EXPECT_DEATH(av.get(), ".*AsyncValue<void>::get\\(\\) called before event was set.*");


    bool callback_after_reset_fired = false;
    av.on_ready([&]() {
        callback_after_reset_fired = true;
    });

    ASSERT_FALSE(callback_after_reset_fired);
    av.set(); // Set again after reset
    ASSERT_TRUE(av.ready());
    ASSERT_TRUE(callback_after_reset_fired);
    av.get(); // Should not throw
}

// Thread safety tests
TEST_F(AsyncValueTest, Int_ThreadSafety_SetAndOnReady) {
    AsyncValue<int> av;
    std::atomic<int> callback_count(0);
    const int num_threads = 50;

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            av.on_ready([&](int val) {
                ASSERT_EQ(val, 99);
                callback_count++;
            });
        });
    }

    // Give on_ready threads a chance to register
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::thread setter([&]() {
        av.set_value(99);
    });

    setter.join();
    for (auto& t : threads) {
        t.join();
    }

    ASSERT_TRUE(av.ready());
    ASSERT_EQ(av.get(), 99);
    // Due to the implementation detail of clearing callbacks after invocation,
    // if on_ready is called *after* set_value has completed and cleared the list,
    // those callbacks will fire immediately but not be part of the original list.
    // The critical part is that all callbacks registered *before or during* set_value get triggered.
    // And any callback registered *after* set_value also gets triggered.
    // The current test structure might have a race where some on_ready calls happen after set_value clears.
    // This is okay, as they will fire immediately. The callback_count should be at least num_threads.
    // To be more precise, we'd need external synchronization or check if list was empty before on_ready.
    // For simplicity, we check if it's equal to num_threads, assuming most will register before set_value completes.
    // This test is more about "do they all fire eventually" rather than "were they all in the list at once".
    ASSERT_EQ(callback_count.load(), num_threads);
}


TEST_F(AsyncValueTest, Int_ThreadSafety_ConcurrentSetAndGet) {
    AsyncValue<int> av;
    std::atomic<bool> keep_reading(true);
    std::atomic<int> success_reads(0);
    std::atomic<int> fail_reads(0);

    std::thread setter([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20)); // Let readers start
        av.set_value(101);
        keep_reading.store(false); // Signal readers to stop
    });

    std::vector<std::thread> getters;
    for (int i = 0; i < 5; ++i) {
        getters.emplace_back([&]() {
            while (keep_reading.load()) {
                if (av.ready()) {
                    ASSERT_EQ(av.get(), 101);
                    success_reads++;
                } else {
                    fail_reads++; // Expected before value is set
                }
                std::this_thread::sleep_for(std::chrono::microseconds(100)); // Small delay
            }
            // After loop, value should be ready
            if(av.ready()){
                ASSERT_EQ(av.get(), 101);
                success_reads++;
            } else {
                // This should not happen if setter finishes
                fail_reads++;
            }
        });
    }

    setter.join();
    for (auto& t : getters) {
        t.join();
    }

    ASSERT_TRUE(av.ready());
    ASSERT_EQ(av.get(), 101);
    ASSERT_GT(success_reads.load(), 0); // At least some successful reads
    // fail_reads can be > 0, which is fine.
}


TEST_F(AsyncValueTest, Void_ThreadSafety_SetAndOnReady) {
    AsyncValue<void> av;
    std::atomic<int> callback_count(0);
    const int num_threads = 50;

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            av.on_ready([&]() {
                callback_count++;
            });
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::thread setter([&]() {
        av.set();
    });

    setter.join();
    for (auto& t : threads) {
        t.join();
    }

    ASSERT_TRUE(av.ready());
    ASSERT_EQ(callback_count.load(), num_threads);
}

// Test for move-only types
TEST_F(AsyncValueTest, UniquePtr_SetValue) {
    AsyncValue<std::unique_ptr<int>> av;
    auto ptr = std::make_unique<int>(123);

    ASSERT_FALSE(av.ready());
    av.set_value(std::move(ptr));
    ASSERT_TRUE(av.ready());

    ASSERT_NE(av.get_if(), nullptr);
    ASSERT_NE(*av.get_if(), nullptr);
    ASSERT_EQ(**av.get_if(), 123);

    ASSERT_NE(av.get(), nullptr);
    ASSERT_EQ(*av.get(), 123);

    bool callback_fired = false;
    int val_in_callback = 0;

    // Re-fetch for callback, as original av.get() might have moved if it returned by value (it returns ref)
    // The callback receives const std::unique_ptr<int>&
    av.on_ready([&](const std::unique_ptr<int>& p_val) {
        ASSERT_NE(p_val, nullptr);
        val_in_callback = *p_val;
        callback_fired = true;
    });

    ASSERT_TRUE(callback_fired);
    ASSERT_EQ(val_in_callback, 123);

    // Test reset
    av.reset();
    ASSERT_FALSE(av.ready());
    ASSERT_EQ(av.get_if(), nullptr);

    auto ptr2 = std::make_unique<int>(456);
    av.set_value(std::move(ptr2));
    ASSERT_TRUE(av.ready());
    ASSERT_EQ(**av.get_if(), 456);
}

TEST_F(AsyncValueTest, Int_SetOnceThrows) {
    AsyncValue<int> av;
    av.set_value(1);
    // The current implementation uses assert. For a release build, this might be a throw or specific error code.
    // For testing the assert, we'd need a death test if it's `assert()`.
    // If it were std::logic_error: EXPECT_THROW(av.set_value(2), std::logic_error);
    // For now, let's assume the assert is the primary mechanism as per current code.
    // To test assert, we use EXPECT_DEATH.
#ifndef NDEBUG // Asserts are typically disabled in NDEBUG builds
    EXPECT_DEATH(av.set_value(2), ".*AsyncValue::set_value called more than once.*");
#else
    // If NDEBUG is defined, the assert might be compiled out.
    // Depending on behavior (e.g., it might just return or silently ignore),
    // this part of the test might need adjustment or be conditional.
    // For now, we'll assume it does nothing or the state remains unchanged.
    av.set_value(2); // Call it again
    ASSERT_EQ(av.get(), 1); // Value should remain the first one set.
#endif
}

TEST_F(AsyncValueTest, Void_SetOnceThrows) {
    AsyncValue<void> av;
    av.set();
#ifndef NDEBUG
    EXPECT_DEATH(av.set(), ".*AsyncValue<void>::set called more than once.*");
#else
    av.set(); // Call again
    ASSERT_TRUE(av.ready()); // Should still be ready
#endif
}

// Test that callbacks are cleared after firing for AsyncValue<T>
TEST_F(AsyncValueTest, Int_CallbacksClearedAfterFiring) {
    AsyncValue<int> av;
    int fire_count = 0;
    av.on_ready([&](int val) {
        ASSERT_EQ(val, 10);
        fire_count++;
    });

    av.set_value(10);
    ASSERT_EQ(fire_count, 1);

    // If we were to re-set (after a reset), the old callback should not fire again.
    av.reset();
    // fire_count should still be 1.
    // Register a new callback
    av.on_ready([&](int val){
        ASSERT_EQ(val, 20);
        fire_count++;
    });
    av.set_value(20);
    ASSERT_EQ(fire_count, 2); // First cb fired once, second cb fired once.
}

// Test that callbacks are cleared after firing for AsyncValue<void>
TEST_F(AsyncValueTest, Void_CallbacksClearedAfterFiring) {
    AsyncValue<void> av;
    int fire_count = 0;
    av.on_ready([&]() {
        fire_count++;
    });

    av.set();
    ASSERT_EQ(fire_count, 1);

    av.reset();
    av.on_ready([&](){
        fire_count++;
    });
    av.set();
    ASSERT_EQ(fire_count, 2);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

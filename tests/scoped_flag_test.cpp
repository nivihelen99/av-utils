#include "scoped_flag.h"
#include <gtest/gtest.h>
#include <atomic>
#include <thread> // For thread_local tests if needed, though direct test is tricky

// Test fixture for ScopedFlag tests
class ScopedFlagTest : public ::testing::Test {
protected:
    bool test_flag_bool;
    std::atomic_bool test_flag_atomic_bool;
    int test_flag_int;

    ScopedFlagTest() : test_flag_bool(false), test_flag_atomic_bool(false), test_flag_int(0) {}

    void SetUp() override {
        // Reset flags before each test
        test_flag_bool = false;
        test_flag_atomic_bool.store(false);
        test_flag_int = 0;
    }
};

// Test ScopedFlag with regular bool
TEST_F(ScopedFlagTest, RegularBool) {
    ASSERT_FALSE(test_flag_bool);
    {
        utils::ScopedFlag guard(test_flag_bool, true);
        ASSERT_TRUE(test_flag_bool);
        ASSERT_FALSE(guard.previous()); // Previous value was false
    }
    ASSERT_FALSE(test_flag_bool); // Should be restored
}

// Test ScopedFlag with std::atomic_bool
TEST_F(ScopedFlagTest, AtomicBool) {
    ASSERT_FALSE(test_flag_atomic_bool.load());
    {
        utils::ScopedFlag guard(test_flag_atomic_bool, true);
        ASSERT_TRUE(test_flag_atomic_bool.load());
        ASSERT_FALSE(guard.previous()); // Previous value was false
    }
    ASSERT_FALSE(test_flag_atomic_bool.load()); // Should be restored
}

// Test ScopedFlag restoration upon exception
TEST_F(ScopedFlagTest, ExceptionSafetyBool) {
    ASSERT_FALSE(test_flag_bool);
    try {
        utils::ScopedFlag guard(test_flag_bool, true);
        ASSERT_TRUE(test_flag_bool);
        throw std::runtime_error("Test exception");
    } catch (const std::runtime_error&) {
        // Expected
    }
    ASSERT_FALSE(test_flag_bool); // Should be restored even after exception
}

TEST_F(ScopedFlagTest, ExceptionSafetyAtomicBool) {
    ASSERT_FALSE(test_flag_atomic_bool.load());
    try {
        utils::ScopedFlag guard(test_flag_atomic_bool, true);
        ASSERT_TRUE(test_flag_atomic_bool.load());
        throw std::runtime_error("Test exception");
    } catch (const std::runtime_error&) {
        // Expected
    }
    ASSERT_FALSE(test_flag_atomic_bool.load()); // Should be restored
}


// Test FlagGuard with int
TEST_F(ScopedFlagTest, FlagGuardInt) {
    ASSERT_EQ(test_flag_int, 0);
    {
        utils::FlagGuard<int> guard(test_flag_int, 10);
        ASSERT_EQ(test_flag_int, 10);
        ASSERT_EQ(guard.previous(), 0); // Previous value was 0
    }
    ASSERT_EQ(test_flag_int, 0); // Should be restored
}

// Test FlagGuard with bool (using BoolGuard alias)
TEST_F(ScopedFlagTest, FlagGuardBool) {
    ASSERT_FALSE(test_flag_bool);
    {
        utils::BoolGuard guard(test_flag_bool, true);
        ASSERT_TRUE(test_flag_bool);
        ASSERT_FALSE(guard.previous());
    }
    ASSERT_FALSE(test_flag_bool);
}

// Test FlagGuard restoration upon exception
TEST_F(ScopedFlagTest, ExceptionSafetyFlagGuardInt) {
    ASSERT_EQ(test_flag_int, 0);
    try {
        utils::FlagGuard<int> guard(test_flag_int, 5);
        ASSERT_EQ(test_flag_int, 5);
        throw std::runtime_error("Test exception");
    } catch (const std::runtime_error&) {
        // Expected
    }
    ASSERT_EQ(test_flag_int, 0); // Should be restored
}

// Test convenience function temporarily_disable for bool
TEST_F(ScopedFlagTest, TemporarilyDisableBool) {
    test_flag_bool = true; // Start with true
    ASSERT_TRUE(test_flag_bool);
    {
        auto guard = utils::temporarily_disable(test_flag_bool);
        ASSERT_FALSE(test_flag_bool);
        ASSERT_TRUE(guard.previous());
    }
    ASSERT_TRUE(test_flag_bool); // Should be restored
}

// Test convenience function temporarily_disable for atomic_bool
TEST_F(ScopedFlagTest, TemporarilyDisableAtomicBool) {
    test_flag_atomic_bool.store(true); // Start with true
    ASSERT_TRUE(test_flag_atomic_bool.load());
    {
        auto guard = utils::temporarily_disable(test_flag_atomic_bool);
        ASSERT_FALSE(test_flag_atomic_bool.load());
        ASSERT_TRUE(guard.previous());
    }
    ASSERT_TRUE(test_flag_atomic_bool.load()); // Should be restored
}

// Test convenience function temporarily_enable for bool
TEST_F(ScopedFlagTest, TemporarilyEnableBool) {
    test_flag_bool = false; // Start with false
    ASSERT_FALSE(test_flag_bool);
    {
        auto guard = utils::temporarily_enable(test_flag_bool);
        ASSERT_TRUE(test_flag_bool);
        ASSERT_FALSE(guard.previous());
    }
    ASSERT_FALSE(test_flag_bool); // Should be restored
}

// Test convenience function temporarily_enable for atomic_bool
TEST_F(ScopedFlagTest, TemporarilyEnableAtomicBool) {
    test_flag_atomic_bool.store(false); // Start with false
    ASSERT_FALSE(test_flag_atomic_bool.load());
    {
        auto guard = utils::temporarily_enable(test_flag_atomic_bool);
        ASSERT_TRUE(test_flag_atomic_bool.load());
        ASSERT_FALSE(guard.previous());
    }
    ASSERT_FALSE(test_flag_atomic_bool.load()); // Should be restored
}

// Test FlagGuard::set_if_not when value is different
TEST_F(ScopedFlagTest, FlagGuardSetIfNotChanges) {
    test_flag_int = 0;
    ASSERT_EQ(test_flag_int, 0);
    {
        auto guard = utils::FlagGuard<int>::set_if_not(test_flag_int, 5);
        ASSERT_EQ(test_flag_int, 5); // Value should change
        ASSERT_EQ(guard.previous(), 0);
    }
    ASSERT_EQ(test_flag_int, 0); // Should be restored
}

// Test FlagGuard::set_if_not when value is the same
TEST_F(ScopedFlagTest, FlagGuardSetIfNotNoChange) {
    test_flag_int = 5;
    ASSERT_EQ(test_flag_int, 5);
    {
        auto guard = utils::FlagGuard<int>::set_if_not(test_flag_int, 5);
        ASSERT_EQ(test_flag_int, 5); // Value should not change
        ASSERT_EQ(guard.previous(), 5); // Previous value is the same as current
    }
    ASSERT_EQ(test_flag_int, 5); // Should remain 5 as it was not changed by guard then restored.
}

// Test for thread_local. This is harder to test directly in GTest without
// involving actual threads. We can test the ScopedFlag mechanism with a
// regular bool that mimics the behavior, assuming ScopedFlag itself is
// thread-agnostic (which it is, as it operates on a pointer/reference).
// The actual thread_local nature depends on the variable declaration.
TEST_F(ScopedFlagTest, ThreadLocalLikeBehavior) {
    static thread_local bool tl_flag = false; // Actual thread_local for concept
    // For the test, we use a member bool to simulate one instance of a thread_local
    // as GTest runs tests in the same thread by default for fixtures.

    ASSERT_FALSE(test_flag_bool); // test_flag_bool is our stand-in
    tl_flag = false; // Ensure actual tl_flag is also false for clarity

    {
        utils::ScopedFlag guard(test_flag_bool, true);
        // If tl_flag was passed, it would be true within this scope *for this thread*
        ASSERT_TRUE(test_flag_bool);
        ASSERT_FALSE(guard.previous());
    }
    ASSERT_FALSE(test_flag_bool); // Restored
}

// Example of testing ScopedFlag in a multi-threaded context (conceptual)
// This test would require more setup to run threads and check values.
// For now, we assume ScopedFlag itself is thread-safe if the underlying
// flag operations are (e.g., std::atomic_bool).
TEST_F(ScopedFlagTest, DISABLED_MultiThreadedAtomic) {
    // std::atomic_bool shared_flag{false};
    // auto task = [&]() {
    //     utils::ScopedFlag guard(shared_flag, true);
    //     ASSERT_TRUE(shared_flag.load());
    //     std::this_thread::sleep_for(std::chrono::milliseconds(10));
    //     // other thread might see it true
    // };
    // std::thread t1(task);
    // std::thread t2(task);
    // t1.join();
    // t2.join();
    // ASSERT_FALSE(shared_flag.load()); // Should be restored by both
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

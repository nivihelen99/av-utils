#include "gtest/gtest.h"
#include "named_lock.h"
#include <thread>
#include <vector>
#include <string>
#include <chrono>
#include <atomic>
#include <set>

// Test fixture for NamedLock tests
class NamedLockTest : public ::testing::Test {
protected:
    NamedLock<std::string> string_locks_;
    NamedLock<int> int_locks_;
};

TEST_F(NamedLockTest, TestBasicLockUnlock) {
    ASSERT_EQ(string_locks_.key_count(), 0);
    {
        auto lock = string_locks_.acquire("key1");
        ASSERT_TRUE(lock.owns_lock());
        ASSERT_TRUE(lock); // Test explicit operator bool
        ASSERT_EQ(string_locks_.key_count(), 1);
        auto metrics = string_locks_.get_metrics();
        ASSERT_EQ(metrics.total_keys, 1);
        ASSERT_EQ(metrics.active_locks, 1);
        ASSERT_EQ(metrics.unused_keys, 0);
    } // Lock released here

    auto metrics_after_release = string_locks_.get_metrics();
    ASSERT_EQ(metrics_after_release.total_keys, 1);
    ASSERT_EQ(metrics_after_release.active_locks, 0);
    ASSERT_EQ(metrics_after_release.unused_keys, 1);

    string_locks_.cleanup_unused();
    ASSERT_EQ(string_locks_.key_count(), 0);
    auto metrics_after_cleanup = string_locks_.get_metrics();
    ASSERT_EQ(metrics_after_cleanup.total_keys, 0);
    ASSERT_EQ(metrics_after_cleanup.active_locks, 0);
    ASSERT_EQ(metrics_after_cleanup.unused_keys, 0);
}

TEST_F(NamedLockTest, TestTryLockSuccess) {
    auto lock = string_locks_.try_acquire("key1");
    ASSERT_TRUE(lock.has_value());
    ASSERT_TRUE(lock->owns_lock());
    ASSERT_TRUE(*lock);
    ASSERT_EQ(string_locks_.key_count(), 1);
}

TEST_F(NamedLockTest, TestTryLockFailure) {
    auto lock1 = string_locks_.acquire("key1"); // Acquire the lock
    ASSERT_TRUE(lock1.owns_lock());

    std::thread t([this]() {
        auto lock2 = string_locks_.try_acquire("key1");
        ASSERT_FALSE(lock2.has_value()); // Should fail as it's locked by lock1
    });
    t.join();
    ASSERT_EQ(string_locks_.key_count(), 1); // Still 1 key
}

TEST_F(NamedLockTest, TestTimedLockSuccess) {
    auto lock = string_locks_.try_acquire_for("key1", std::chrono::milliseconds(100));
    ASSERT_TRUE(lock.has_value());
    ASSERT_TRUE(lock->owns_lock());
    ASSERT_TRUE(*lock);
    ASSERT_EQ(string_locks_.key_count(), 1);
}

TEST_F(NamedLockTest, TestTimedLockTimeout) {
    auto lock1 = string_locks_.acquire("key1"); // Acquire the lock
    ASSERT_TRUE(lock1.owns_lock());

    std::atomic<bool> timed_out = false;
    std::thread t([this, &timed_out]() {
        auto lock2 = string_locks_.try_acquire_for("key1", std::chrono::milliseconds(50));
        ASSERT_FALSE(lock2.has_value()); // Should time out
        if (!lock2.has_value()) {
            timed_out = true;
        }
    });
    t.join();
    ASSERT_TRUE(timed_out);
    ASSERT_EQ(string_locks_.key_count(), 1);
}

TEST_F(NamedLockTest, TestMultipleKeys) {
    std::atomic<int> acquired_count = 0;
    std::thread t1([this, &acquired_count]() {
        auto lock = string_locks_.acquire("key1");
        if (lock.owns_lock()) acquired_count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });
    std::thread t2([this, &acquired_count]() {
        auto lock = string_locks_.acquire("key2");
        if (lock.owns_lock()) acquired_count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });

    t1.join();
    t2.join();
    ASSERT_EQ(acquired_count, 2);
    ASSERT_EQ(string_locks_.key_count(), 2);
    string_locks_.cleanup_unused();
    ASSERT_EQ(string_locks_.key_count(), 0);
}

TEST_F(NamedLockTest, TestSameKeyContention) {
    std::atomic<int> counter = 0;
    std::atomic<int> max_concurrent_access = 0;
    std::atomic<int> current_access = 0;
    const int num_threads = 5;
    const int operations_per_thread = 10;

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, &counter, &max_concurrent_access, &current_access, operations_per_thread]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                auto lock = string_locks_.acquire("shared_key");
                ASSERT_TRUE(lock.owns_lock());
                
                current_access++;
                // Update max_concurrent_access if current_access is higher
                int current_max = max_concurrent_access.load();
                while (current_access.load() > current_max) {
                    if (max_concurrent_access.compare_exchange_weak(current_max, current_access.load())) {
                        break; 
                    }
                }

                counter++; // Simulate work
                std::this_thread::sleep_for(std::chrono::microseconds(100)); // Short sleep
                
                current_access--;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_EQ(counter, num_threads * operations_per_thread);
    ASSERT_EQ(max_concurrent_access, 1); // Only one thread should access critical section at a time
    ASSERT_EQ(string_locks_.key_count(), 1);
    string_locks_.cleanup_unused();
    ASSERT_EQ(string_locks_.key_count(), 0);
}

TEST_F(NamedLockTest, TestRefCountAndCleanup) {
    // This test verifies the reference counting and cleanup mechanism.
    // It ensures that locks are counted, keys are tracked, and cleanup removes unused keys.
    // Note: This test does not involve re-entrant locking on the same key by the same thread,
    // as std::timed_mutex is not recursive.

    // Initial state: no locks, no keys
    ASSERT_EQ(int_locks_.get_metrics().active_locks, 0);
    ASSERT_EQ(int_locks_.get_metrics().total_keys, 0);
    ASSERT_EQ(int_locks_.get_metrics().unused_keys, 0);
    int_locks_.clear(); // Ensure a clean slate for this specific test if run after others.
    ASSERT_EQ(int_locks_.get_metrics().active_locks, 0);


    const int test_key = 200;

    {
        auto l1 = int_locks_.acquire(test_key);
        ASSERT_TRUE(l1.owns_lock());
        ASSERT_EQ(int_locks_.get_metrics().active_locks, 1); // l1 holds lock for test_key
        ASSERT_EQ(int_locks_.get_metrics().total_keys, 1);   // One key (test_key) exists
        ASSERT_EQ(int_locks_.get_metrics().unused_keys, 0);  // No unused keys
    } // l1 is released here. Refcount for test_key becomes 0.

    // After l1 is released:
    // The lock is no longer active.
    ASSERT_EQ(int_locks_.get_metrics().active_locks, 0);
    // The key entry itself still exists in the map, but its refcount is 0.
    ASSERT_EQ(int_locks_.get_metrics().total_keys, 1);
    // The key is now considered unused.
    ASSERT_EQ(int_locks_.get_metrics().unused_keys, 1);

    // Perform cleanup
    int_locks_.cleanup_unused();

    // After cleanup:
    // No active locks.
    ASSERT_EQ(int_locks_.get_metrics().active_locks, 0);
    // The unused key (test_key) should have been removed.
    ASSERT_EQ(int_locks_.get_metrics().total_keys, 0);
    // No unused keys remaining.
    ASSERT_EQ(int_locks_.get_metrics().unused_keys, 0);
}


TEST_F(NamedLockTest, TestScopedLockMove) {
    // Part 1: Test move assignment for Scoped
    { // Scope for lock1_outer and initial_lock_assign
        NamedLock<std::string>::Scoped lock1_outer;
        ASSERT_FALSE(lock1_outer.owns_lock());
        ASSERT_FALSE(lock1_outer);

        { // Inner scope for initial_lock_assign
            auto initial_lock_assign = string_locks_.acquire("move_key_assign");
            ASSERT_TRUE(initial_lock_assign.owns_lock());
            ASSERT_EQ(string_locks_.get_metrics().active_locks, 1); // "move_key_assign" active
            
            lock1_outer = std::move(initial_lock_assign); // Move assignment
            ASSERT_FALSE(initial_lock_assign.owns_lock()); 
            ASSERT_FALSE(initial_lock_assign);
            ASSERT_TRUE(lock1_outer.owns_lock());
            ASSERT_TRUE(lock1_outer);
            ASSERT_EQ(string_locks_.get_metrics().active_locks, 1); // "move_key_assign" still active, via lock1_outer
        } // initial_lock_assign destructed (empty)
        
        // lock1_outer still holds the lock for "move_key_assign"
        ASSERT_TRUE(lock1_outer.owns_lock());
        ASSERT_EQ(string_locks_.get_metrics().active_locks, 1);
    } // lock1_outer destructed. Lock for "move_key_assign" released.

    // Check after lock1_outer is destructed
    auto metrics_assign_released = string_locks_.get_metrics();
    ASSERT_EQ(metrics_assign_released.active_locks, 0);
    ASSERT_EQ(metrics_assign_released.unused_keys, 1);  // "move_key_assign" is now unused
    ASSERT_EQ(metrics_assign_released.total_keys, 1);


    // Part 2: Test move constructor for Scoped
    { // Scope for lock2_dest
        auto lock2_source = string_locks_.acquire("move_key_ctor"); // "move_key_ctor" active
                                                                    // "move_key_assign" is unused.
        ASSERT_TRUE(lock2_source.owns_lock());
        // Metrics check:
        auto metrics_ctor_acq = string_locks_.get_metrics();
        ASSERT_EQ(metrics_ctor_acq.active_locks, 1); // "move_key_ctor"
        ASSERT_EQ(metrics_ctor_acq.unused_keys, 1);  // "move_key_assign"
        ASSERT_EQ(metrics_ctor_acq.total_keys, 2);

        NamedLock<std::string>::Scoped lock2_dest(std::move(lock2_source)); // Move constructor
        ASSERT_FALSE(lock2_source.owns_lock());
        ASSERT_TRUE(lock2_dest.owns_lock());
        // Metrics should be the same, lock just moved owner
        auto metrics_ctor_moved = string_locks_.get_metrics();
        ASSERT_EQ(metrics_ctor_moved.active_locks, 1);
        ASSERT_EQ(metrics_ctor_moved.unused_keys, 1);
        ASSERT_EQ(metrics_ctor_moved.total_keys, 2);
    } // lock2_dest destructed. Lock for "move_key_ctor" released.

    // Check after lock2_dest is destructed
    // "move_key_assign" unused, "move_key_ctor" unused
    auto metrics_ctor_released = string_locks_.get_metrics();
    ASSERT_EQ(metrics_ctor_released.active_locks, 0);
    ASSERT_EQ(metrics_ctor_released.unused_keys, 2);
    ASSERT_EQ(metrics_ctor_released.total_keys, 2);
    
    // Part 3: Test move assignment for TimedScoped
    { // Scope for timed_lock1_outer
        NamedLock<std::string>::TimedScoped timed_lock1_outer;
        ASSERT_FALSE(timed_lock1_outer.owns_lock());

        std::optional<NamedLock<std::string>::TimedScoped> initial_timed_lock_opt = 
            string_locks_.try_acquire_for("timed_move_key", std::chrono::milliseconds(10));
        ASSERT_TRUE(initial_timed_lock_opt.has_value());
        
        NamedLock<std::string>::TimedScoped initial_timed_lock = std::move(initial_timed_lock_opt.value());
        // initial_timed_lock_opt.reset(); // Not strictly necessary as .value() on rvalue optional moves from contained value

        ASSERT_TRUE(initial_timed_lock.owns_lock());
        // Metrics: "timed_move_key" active. "move_key_assign", "move_key_ctor" unused.
        auto metrics_timed_acq = string_locks_.get_metrics();
        ASSERT_EQ(metrics_timed_acq.active_locks, 1); // "timed_move_key"
        ASSERT_EQ(metrics_timed_acq.unused_keys, 2);  // "move_key_assign", "move_key_ctor"
        ASSERT_EQ(metrics_timed_acq.total_keys, 3);

        timed_lock1_outer = std::move(initial_timed_lock); // Move assignment for TimedScoped
        ASSERT_FALSE(initial_timed_lock.owns_lock());
        ASSERT_TRUE(timed_lock1_outer.owns_lock());
        // Metrics should be the same
        auto metrics_timed_moved = string_locks_.get_metrics();
        ASSERT_EQ(metrics_timed_moved.active_locks, 1);
        ASSERT_EQ(metrics_timed_moved.unused_keys, 2);
        ASSERT_EQ(metrics_timed_moved.total_keys, 3);
    } // timed_lock1_outer destructed. Lock for "timed_move_key" released.

    // Check after timed_lock1_outer is destructed
    // "move_key_assign", "move_key_ctor", "timed_move_key" all unused.
    auto metrics_timed_released = string_locks_.get_metrics();
    ASSERT_EQ(metrics_timed_released.active_locks, 0);
    ASSERT_EQ(metrics_timed_released.unused_keys, 3);
    ASSERT_EQ(metrics_timed_released.total_keys, 3);

    // Cleanup for this test specifically, to ensure it doesn't affect other tests
    // if they share the same string_locks_ instance (though gtest usually creates fresh fixture per test)
    string_locks_.cleanup_unused();
    ASSERT_EQ(string_locks_.key_count(), 0);
    auto metrics_final_cleanup = string_locks_.get_metrics();
    ASSERT_EQ(metrics_final_cleanup.active_locks, 0);
    ASSERT_EQ(metrics_final_cleanup.unused_keys, 0);
    ASSERT_EQ(metrics_final_cleanup.total_keys, 0);
}


TEST_F(NamedLockTest, TestScopedLockReset) {
    auto lock = string_locks_.acquire("reset_key");
    ASSERT_TRUE(lock.owns_lock());
    ASSERT_EQ(string_locks_.get_metrics().active_locks, 1);

    lock.reset();
    ASSERT_FALSE(lock.owns_lock());
    ASSERT_FALSE(lock);
    ASSERT_EQ(string_locks_.get_metrics().active_locks, 0);
    ASSERT_EQ(string_locks_.get_metrics().unused_keys, 1); // Key is now unused

    // Ensure it doesn't double-decrement or crash on destruction
    // (lock is already reset, destructor should be a no-op for refcount)
    // No explicit check needed, absence of crash is the test.
}

TEST_F(NamedLockTest, TestClear) {
    string_locks_.acquire("key1");
    string_locks_.acquire("key2");
    ASSERT_EQ(string_locks_.key_count(), 2);
    // Note: Locks are still held by the returned Scoped objects, but they are not stored in this test.
    // This is okay for testing clear(), as clear() doesn't care about held locks, only map entries.
    // In a real scenario, this would be problematic if other threads were using these locks.

    string_locks_.clear();
    ASSERT_EQ(string_locks_.key_count(), 0);
    auto metrics = string_locks_.get_metrics();
    ASSERT_EQ(metrics.total_keys, 0);
    ASSERT_EQ(metrics.active_locks, 0); // Active locks are also gone because entries are cleared
    ASSERT_EQ(metrics.unused_keys, 0);

    // Test acquiring after clear
    auto lock_after_clear = string_locks_.acquire("key3");
    ASSERT_TRUE(lock_after_clear.owns_lock());
    ASSERT_EQ(string_locks_.key_count(), 1);
}

TEST_F(NamedLockTest, TestStressConcurrentAccess) {
    const int num_threads = 8;
    const int operations_per_thread = 200;
    const int num_keys = 4; // Use a small number of keys to ensure contention
    std::atomic<int> completed_operations{0};
    NamedLock<int> stress_locks;

    auto worker = [&](int thread_id) {
        for (int i = 0; i < operations_per_thread; ++i) {
            int key = i % num_keys; // Cycle through keys
            
            if (i % 5 == 0) { // Occasionally try_acquire
                auto lock = stress_locks.try_acquire(key);
                if (lock) {
                    std::this_thread::sleep_for(std::chrono::microseconds(10 + (thread_id % 5))); // Minimal work
                    completed_operations++;
                }
            } else if (i % 5 == 1) { // Occasionally try_acquire_for
                auto lock = stress_locks.try_acquire_for(key, std::chrono::microseconds(50 + (i%10 * 10) ));
                 if (lock) {
                    std::this_thread::sleep_for(std::chrono::microseconds(10 + (thread_id % 5))); // Minimal work
                    completed_operations++;
                }
            }
            else { // Mostly blocking acquire
                auto lock = stress_locks.acquire(key);
                std::this_thread::sleep_for(std::chrono::microseconds(10 + (thread_id % 5))); // Minimal work
                completed_operations++;
            }
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    // We can't assert exact completed_operations if try_acquire/try_acquire_for are used often and fail.
    // The main point is that it doesn't crash and key counts are reasonable.
    EXPECT_GT(completed_operations.load(), (num_threads * operations_per_thread) / 2); // Expect at least half to complete

    auto metrics = stress_locks.get_metrics();
    EXPECT_LE(metrics.total_keys, num_keys); // Should not exceed num_keys
    EXPECT_EQ(metrics.active_locks, 0);    // All locks should be released
    
    stress_locks.cleanup_unused();
    EXPECT_EQ(stress_locks.key_count(), 0); // All keys should be cleaned up
}


// Here's the corrected version of the problematic test:

TEST_F(NamedLockTest, TestActiveLockCountCorrectness) {
    // Start with a clean slate
    string_locks_.clear();
    ASSERT_EQ(string_locks_.active_lock_count(), 0);

    // Test with different keys to avoid deadlock
    {
        auto guard1 = string_locks_.acquire("k1");
        ASSERT_TRUE(guard1.owns_lock());
        ASSERT_EQ(string_locks_.active_lock_count(), 1); // "k1" is active
        
        {
            // Acquire a lock on a *different* key to test total active_lock_count.
            // Acquiring on the same key ("k1") again in the same thread would deadlock
            // because std::timed_mutex is not recursive.
            auto guard2 = string_locks_.acquire("k2");
            ASSERT_TRUE(guard2.owns_lock());
            // Now "k1" and "k2" are active. Refcount for "k1" is 1, refcount for "k2" is 1.
            // Total active_lock_count = 1 (for k1) + 1 (for k2) = 2.
            ASSERT_EQ(string_locks_.active_lock_count(), 2);
            
            {
                auto guard3 = string_locks_.acquire("k3");
                ASSERT_TRUE(guard3.owns_lock());
                // "k1", "k2", "k3" active. Total active_lock_count = 1+1+1 = 3.
                ASSERT_EQ(string_locks_.active_lock_count(), 3);
            } // guard3 released ("k3" becomes inactive, refcount 0)
            ASSERT_EQ(string_locks_.active_lock_count(), 2); // "k1", "k2" still active
        } // guard2 released ("k2" becomes inactive, refcount 0)
        ASSERT_EQ(string_locks_.active_lock_count(), 1); // "k1" still active
    } // guard1 released ("k1" becomes inactive, refcount 0)
    
    // Final verification - all locks should be released
    ASSERT_EQ(string_locks_.active_lock_count(), 0);
}



// All locks acquired in this test scope should now be released.
// Add a final check.
// Need to call string_locks_.active_lock_count() outside the fixture's direct scope
// or ensure all guards are out of scope. The above structure handles guard scope.
// So, after guard1 is released, the count should be 0.
// This is implicitly tested if the fixture is fresh for each test.
// Let's add an explicit check for 0 after all local guards are gone.
// The above structure correctly tracks this.

TEST_F(NamedLockTest, EmptyKeyTest) {
    // Test with an empty string key
    {
        auto lock = string_locks_.acquire("");
        ASSERT_TRUE(lock.owns_lock());
        ASSERT_EQ(string_locks_.key_count(), 1);
        auto metrics = string_locks_.get_metrics();
        ASSERT_EQ(metrics.total_keys, 1);
        ASSERT_EQ(metrics.active_locks, 1);
    }
    string_locks_.cleanup_unused();
    ASSERT_EQ(string_locks_.key_count(), 0);
}

TEST_F(NamedLockTest, TryAcquireForImmediateTimeout) {
    auto lock1 = string_locks_.acquire("timeout_key_immediate");
    ASSERT_TRUE(lock1.owns_lock());

    // Try to acquire with zero timeout
    auto lock2 = string_locks_.try_acquire_for("timeout_key_immediate", std::chrono::milliseconds(0));
    ASSERT_FALSE(lock2.has_value()); // Should fail immediately if already locked

    // Try to acquire an available lock with zero timeout
    auto lock3 = string_locks_.try_acquire_for("available_key_immediate", std::chrono::milliseconds(0));
    ASSERT_TRUE(lock3.has_value());
    ASSERT_TRUE(lock3->owns_lock());
}

TEST_F(NamedLockTest, DestructorCorrectnessAfterMove) {
    // Ensure that the destructor of a moved-from Scoped object doesn't affect refcounts.
    auto* lock_ptr = new NamedLock<int>();
    {
        auto s1 = lock_ptr->acquire(1);
        ASSERT_EQ(lock_ptr->get_metrics().active_locks, 1);
        {
            NamedLock<int>::Scoped s2 = std::move(s1);
            ASSERT_FALSE(s1.owns_lock()); // s1 is now invalid
            ASSERT_TRUE(s2.owns_lock());
            ASSERT_EQ(lock_ptr->get_metrics().active_locks, 1); 
            // s1 destructor runs here - should be a no-op for refcount
        } // s2 destructor runs here - should decrement refcount
        ASSERT_EQ(lock_ptr->get_metrics().active_locks, 0);
    }
    ASSERT_EQ(lock_ptr->get_metrics().unused_keys, 1);
    lock_ptr->cleanup_unused();
    ASSERT_EQ(lock_ptr->get_metrics().total_keys, 0);
    delete lock_ptr;
}

TEST_F(NamedLockTest, MultipleNamedLockInstances) {
    NamedLock<std::string> locks_A;
    NamedLock<std::string> locks_B;

    std::string key = "shared_resource_name";

    // Lock the key in instance A
    auto guard_A = locks_A.acquire(key);
    ASSERT_TRUE(guard_A.owns_lock());
    ASSERT_EQ(locks_A.key_count(), 1);
    ASSERT_EQ(locks_B.key_count(), 0);

    // Try to lock the same key in instance B - should succeed as they are different managers
    auto guard_B = locks_B.acquire(key);
    ASSERT_TRUE(guard_B.owns_lock());
    ASSERT_EQ(locks_A.key_count(), 1);
    ASSERT_EQ(locks_B.key_count(), 1);

    // Try to lock the same key again in instance A (from a different "thread" perspective)
    // This will block if guard_A is still held, which it is.
    // For this test, just ensuring they don't interfere.
    std::thread t([&]() {
        auto guard_A2 = locks_A.try_acquire(key); // Will fail as guard_A holds it
        ASSERT_FALSE(guard_A2.has_value());
    });
    t.join();
}

TEST_F(NamedLockTest, RefcountOnFailedTryAcquire) {
    // Acquire a lock to make subsequent try_acquire fail
    auto main_lock = int_locks_.acquire(777);
    ASSERT_EQ(int_locks_.get_metrics().active_locks, 1);
    ASSERT_EQ(int_locks_.get_metrics().total_keys, 1);

    // Attempt a try_acquire that will fail
    auto opt_lock = int_locks_.try_acquire(777);
    ASSERT_FALSE(opt_lock.has_value());

    // Check that active_locks and total_keys are not erroneously incremented
    // The refcount should have been incremented and then decremented inside try_acquire.
    ASSERT_EQ(int_locks_.get_metrics().active_locks, 1); // Still held by main_lock
    ASSERT_EQ(int_locks_.get_metrics().total_keys, 1);
}

TEST_F(NamedLockTest, RefcountOnFailedTryAcquireFor) {
    // Acquire a lock to make subsequent try_acquire_for fail
    auto main_lock = int_locks_.acquire(888);
    ASSERT_EQ(int_locks_.get_metrics().active_locks, 1);
    ASSERT_EQ(int_locks_.get_metrics().total_keys, 1);

    // Attempt a try_acquire_for that will fail (timeout)
    auto opt_lock = int_locks_.try_acquire_for(888, std::chrono::milliseconds(1));
    ASSERT_FALSE(opt_lock.has_value());

    // Check that active_locks and total_keys are not erroneously incremented
    ASSERT_EQ(int_locks_.get_metrics().active_locks, 1); // Still held by main_lock
    ASSERT_EQ(int_locks_.get_metrics().total_keys, 1);
}

TEST_F(NamedLockTest, DefaultConstructedScopedLock) {
    NamedLock<std::string>::Scoped default_scoped;
    ASSERT_FALSE(default_scoped.owns_lock());
    ASSERT_FALSE(default_scoped); // operator bool
    default_scoped.reset(); // Should be safe to call reset

    NamedLock<std::string>::TimedScoped default_timed_scoped;
    ASSERT_FALSE(default_timed_scoped.owns_lock());
    ASSERT_FALSE(default_timed_scoped); // operator bool
    default_timed_scoped.reset(); // Should be safe to call reset
}

TEST_F(NamedLockTest, TestNonReentrantAcquireBehavior) {
    const std::string key = "reentrant_test_key";

    // 1. Acquire a lock using acquire()
    auto lock1 = string_locks_.acquire(key);
    ASSERT_TRUE(lock1.owns_lock()) << "Initial acquire should succeed";
    ASSERT_EQ(string_locks_.get_metrics().active_locks, 1);

    // 2. Attempt to acquire the same lock again using try_acquire() from the same thread
    auto lock2_opt = string_locks_.try_acquire(key);
    ASSERT_FALSE(lock2_opt.has_value()) << "try_acquire on already held lock by same thread should fail";
    
    // Verify active_locks count is still 1 (try_acquire should handle its internal refcounting)
    ASSERT_EQ(string_locks_.get_metrics().active_locks, 1);

    // 3. Attempt to acquire the same lock again using try_acquire_for() from the same thread
    auto lock3_opt = string_locks_.try_acquire_for(key, std::chrono::milliseconds(1));
    ASSERT_FALSE(lock3_opt.has_value()) << "try_acquire_for on already held lock by same thread should fail";

    // Verify active_locks count is still 1
    ASSERT_EQ(string_locks_.get_metrics().active_locks, 1);

    // lock1 goes out of scope and releases the lock
}

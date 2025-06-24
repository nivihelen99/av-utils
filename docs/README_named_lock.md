# NamedLock Utility

`NamedLock<T>` is a C++ utility that provides a mechanism for managing mutexes associated with specific names or keys of type `T`. This is particularly useful when you need to synchronize access to dynamically identified resources without pre-allocating a large number of mutexes.

## Features

-   **Templated Key Type**: Can be used with any key type `T` that is hashable (for `std::unordered_map`) and comparable. Common examples include `std::string` for resource names or `int` for IDs.
-   **RAII Scoped Locks**:
    -   `NamedLock::Scoped`: Returned by `acquire` and `try_acquire`. Automatically manages lock lifetime.
    -   `NamedLock::TimedScoped`: Returned by `try_acquire_for`. Automatically manages lock lifetime.
    These scoped lock objects ensure that mutexes are correctly released when they go out of scope, even in the presence of exceptions. They are move-only.
-   **Blocking Acquire**:
    -   `acquire(const T& key)`: Waits indefinitely until the lock for the given key can be acquired. Returns a `Scoped` lock.
-   **Non-Blocking Acquire**:
    -   `try_acquire(const T& key)`: Attempts to acquire the lock for the given key without blocking. Returns `std::optional<Scoped>`. If the lock is acquired, the optional contains a `Scoped` lock; otherwise, it's empty (`std::nullopt`).
-   **Timed Acquire**:
    -   `try_acquire_for(const T& key, const std::chrono::duration<Rep, Period>& timeout)`: Attempts to acquire the lock for the given key, waiting up to the specified timeout duration. Returns `std::optional<TimedScoped>`.
-   **Manual Cleanup**:
    -   `cleanup_unused()`: Removes mutex entries from the internal map if they are no longer actively locked (reference count is zero). This helps manage memory by pruning unused mutex objects.
-   **Metrics**:
    -   `get_metrics()`: Returns a `LockMetrics` struct containing `total_keys`, `active_locks`, and `unused_keys` for monitoring and debugging.

## Requirements for Key Type `T`

The type `T` used as the key for `NamedLock` must satisfy the following:
1.  Be hashable: An appropriate `std::hash<T>` specialization must exist.
2.  Be comparable: An `operator==` must be defined for comparing keys.

Standard types like `int`, `std::string`, etc., satisfy these requirements by default. For custom types, you may need to provide these yourself.

## Basic Usage

### Include Header
```cpp
#include "named_lock.h"
#include <string>
#include <iostream>
#include <thread>
#include <chrono> // For timeouts
```

### Blocking Acquire (`acquire`)
This is the most common way to use `NamedLock`. The thread will block until the lock is acquired.
```cpp
NamedLock<std::string> resourceLocks;

void process_resource(const std::string& resource_name) {
    auto lock_guard = resourceLocks.acquire(resource_name); // Blocks here if "resource_name" is locked
    if (lock_guard.owns_lock()) { // or simply: if (lock_guard)
        std::cout << "Lock acquired for: " << resource_name << std::endl;
        // Critical section: Access or modify the resource safely
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "Releasing lock for: " << resource_name << std::endl;
    }
    // Lock is automatically released when lock_guard goes out of scope
}
```

### Non-Blocking Acquire (`try_acquire`)
Use this when you want to attempt locking but do something else if the lock isn't immediately available.
```cpp
NamedLock<int> taskLocks;

void attempt_task(int task_id) {
    auto optional_lock_guard = taskLocks.try_acquire(task_id);
    if (optional_lock_guard) { // Check if the optional contains a lock
        std::cout << "Lock acquired for task: " << task_id << std::endl;
        // Critical section
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::cout << "Releasing lock for task: " << task_id << std::endl;
        // Lock automatically released
    } else {
        std::cout << "Could not acquire lock for task: " << task_id << ". It's busy." << std::endl;
        // Handle the case where the lock was not acquired
    }
}
```

### Timed Acquire (`try_acquire_for`)
Use this when you want to wait for a lock but give up after a certain duration.
```cpp
NamedLock<std::string> deviceLocks;

void access_device_with_timeout(const std::string& device_name) {
    auto optional_timed_lock = deviceLocks.try_acquire_for(device_name, std::chrono::milliseconds(50));
    if (optional_timed_lock) {
        std::cout << "Lock acquired for device: " << device_name << " within timeout." << std::endl;
        // Critical section
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "Releasing lock for device: " << device_name << std::endl;
    } else {
        std::cout << "Failed to acquire lock for device: " << device_name << " within 50ms." << std::endl;
    }
}
```

## Lock Re-entrancy

`NamedLock` uses `std::timed_mutex` internally, which is **not re-entrant**. This means if a thread acquires a lock for a key, and then attempts to acquire the *same lock* for the *same key* again (e.g., by calling `acquire(key)` again on the same `NamedLock` instance within the same critical section without releasing the first lock), it will result in a **deadlock**.

Design your critical sections to be acquired once and released. If re-entrant behavior is needed, it must be implemented externally or a different synchronization primitive should be considered.

## `cleanup_unused()`

When locks are acquired and released, `NamedLock` increments and decrements a reference count for the associated key's mutex. When a key's reference count drops to zero, its `LockEntry` (containing the mutex) is not immediately removed from the internal map. This is an optimization to avoid the overhead of map operations if the same key is locked again soon.

However, if keys are ephemeral or a large number of unique keys are used over time, the internal map can grow. The `cleanup_unused()` method should be called periodically (or at suitable points in your application's lifecycle) to iterate through the map and remove entries for keys that currently have no active locks (reference count is zero).

```cpp
NamedLock<int> sessionLocks;
// ... many lock acquire/release cycles ...

// At a suitable time, e.g., during low activity or shutdown:
sessionLocks.cleanup_unused();
std::cout << "Cleaned up unused session locks. Current key count: "
          << sessionLocks.key_count() << std::endl;
```
Failure to call `cleanup_unused()` might lead to unbounded growth of the `lock_map_` if new keys are continuously introduced.

## Monitoring with `get_metrics()`

You can get insights into the state of the `NamedLock` instance using `get_metrics()`:
```cpp
auto metrics = resourceLocks.get_metrics();
std::cout << "Total keys tracked: " << metrics.total_keys << std::endl;
std::cout << "Currently active locks (sum of refcounts): " << metrics.active_locks << std::endl;
std::cout << "Keys with refcount 0 (candidates for cleanup): " << metrics.unused_keys << std::endl;
```
This can be useful for debugging or monitoring the lock manager's behavior.

## `clear()`
The `clear()` method forcefully removes all entries from the `lock_map_`. This is a dangerous operation and should only be used if you are absolutely certain that no threads are currently holding or attempting to acquire locks managed by this `NamedLock` instance. Improper use can lead to undefined behavior or deadlocks.
```cpp
// Use with extreme caution, typically during application shutdown or reset.
// Ensure no locks are held.
resourceLocks.clear();
std::cout << "All locks cleared. Current key count: " << resourceLocks.key_count() << std::endl;
```

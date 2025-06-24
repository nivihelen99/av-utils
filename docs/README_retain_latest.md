# `RetainLatest` and `VersionedRetainLatest`

## Overview

The `retain_latest.h` header provides two thread-safe utility classes, `RetainLatest<T>` and `VersionedRetainLatest<T>`, designed to store and manage a single, most recent value. These classes are particularly useful in scenarios where intermediate updates can be safely discarded, ensuring that consumers always access the latest available data. Common use cases include managing application configurations, coalescing high-frequency telemetry data, or handling UI state updates.

Both classes use a mutex for synchronization, making them suitable for C++17 environments.

## `RetainLatest<T>`

`RetainLatest<T>` is a template class that holds an object of type `T`. It ensures that only the last value provided via `update()` or `emplace()` is retained.

### Features:
- **Thread-Safe**: All operations are internally synchronized.
- **Single Value**: Always holds at most one value. Older values are overwritten.
- **Clear-on-Read**: The `consume()` method retrieves the value and then clears the internal storage.
- **Non-Blocking Peek**: The `peek()` method allows reading the value without modifying it.
- **Update Callback**: An optional callback can be registered via `on_update()` to be notified synchronously when a new value is set.

### Basic Usage:

```cpp
#include "retain_latest.h" // Assuming the header is in the include path
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

int main() {
    retain_latest::RetainLatest<std::string> config_holder;

    // Producer thread updating the configuration
    std::thread producer([&config_holder]() {
        config_holder.update("Initial Config");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        config_holder.update("Updated Config V1");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        config_holder.update("Final Config V2"); // This will be the one consumed
    });

    // Consumer thread periodically checking for config
    std::thread consumer([&config_holder]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Wait for producer
        if (auto config_opt = config_holder.consume()) {
            std::cout << "Consumed config: " << *config_opt << std::endl;
            // Output: Consumed config: Final Config V2
        } else {
            std::cout << "No config available." << std::endl;
        }
    });

    producer.join();
    consumer.join();

    // After consume, the holder is empty
    if (!config_holder.has_value()) {
        std::cout << "Config holder is now empty." << std::endl;
        // Output: Config holder is now empty.
    }

    return 0;
}
```

### API:
- `void update(const T& value)`: Updates the stored value (copy).
- `void update(T&& value)`: Updates the stored value (move).
- `template<typename... Args> void emplace(Args&&... args)`: Constructs the value in-place.
- `std::optional<T> consume()`: Retrieves the value and clears storage. Returns `std::nullopt` if empty.
- `std::optional<T> peek() const`: Returns a copy of the value if available, else `std::nullopt`.
- `bool has_value() const`: Checks if a value is stored.
- `void on_update(std::function<void(const T&)> callback)`: Sets a callback for update events.
- `void clear()`: Clears any stored value.

## `VersionedRetainLatest<T>`

`VersionedRetainLatest<T>` extends `RetainLatest<T>` by associating a monotonically increasing version number with each stored value. This is useful for scenarios requiring optimistic concurrency control or staleness detection.

### Features:
- All features of `RetainLatest<T>`.
- **Automatic Versioning**: Each update automatically increments a version counter.
- **Versioned Value**: Values are stored as `Versioned<T>`, which contains `T value` and `uint64_t version`.
- **Compare-and-Update**: The `compare_and_update()` method allows updating the value only if the current version matches an expected version.
- **Staleness Detection**: `is_stale()` checks if a consumer's version is older than the current stored version.
- **Current Version**: `current_version()` retrieves the version number of the currently stored value.

### Basic Usage:

```cpp
#include "retain_latest.h" // Assuming the header is in the include path
#include <iostream>
#include <string>

int main() {
    retain_latest::VersionedRetainLatest<std::string> versioned_data;

    versioned_data.update("Data v0"); // Version 0
    versioned_data.update("Data v1"); // Version 1

    if (auto current_data = versioned_data.peek()) {
        std::cout << "Current: " << current_data->value
                  << " (Version: " << current_data->version << ")" << std::endl;
        // Output: Current: Data v1 (Version: 1)
    }

    uint64_t my_last_seen_version = 0;
    if (versioned_data.is_stale(my_last_seen_version)) {
        std::cout << "My data (version " << my_last_seen_version << ") is stale." << std::endl;
        // Output: My data (version 0) is stale.

        if (auto latest_data = versioned_data.consume()) {
            std::cout << "Consumed latest: " << latest_data->value
                      << " (Version: " << latest_data->version << ")" << std::endl;
            // Output: Consumed latest: Data v1 (Version: 1)
            my_last_seen_version = latest_data->version;
        }
    }

    // Attempt compare-and-update
    // This will fail because the current version is 1 (after Data v1), not 0.
    // Note: consume() above cleared the value, so we need to re-update for this example.
    versioned_data.update("Data v2 for CAS"); // Version 2 (assuming next_version_ starts from 0 and increments)
                                         // If consume() was called, next_version_ would be 2, and this value has version 2.
                                         // If not, it would be 2. Let's assume it was consumed and this is a new update.

    uint64_t version_before_cas = versioned_data.current_version().value_or(0); // Should be 2
    bool cas_success = versioned_data.compare_and_update("Data v3 CAS", version_before_cas);
    std::cout << "CAS update " << (cas_success ? "succeeded" : "failed") << std::endl;
    // Output: CAS update succeeded (if version_before_cas was indeed the latest)

    if (auto final_data = versioned_data.peek()) {
        std::cout << "After CAS: " << final_data->value
                  << " (Version: " << final_data->version << ")" << std::endl;
        // Output: After CAS: Data v3 CAS (Version: 3)
    }

    return 0;
}
```

### API (in addition to `RetainLatest` similar methods):
- `void update(const T& value)`: Updates value, increments version.
- `void update(T&& value)`: Updates value (move), increments version.
- `template<typename... Args> void emplace(Args&&... args)`: Constructs value in-place, increments version.
- `bool compare_and_update(const T& value, uint64_t expected_version)`: Updates if `expected_version` matches current. Returns `true` on success.
- `std::optional<Versioned<T>> consume()`: Retrieves `Versioned<T>` and clears.
- `std::optional<Versioned<T>> peek() const`: Returns a copy of `Versioned<T>`.
- `bool is_stale(uint64_t consumer_version) const`: Returns `true` if `consumer_version` is less than current.
- `std::optional<uint64_t> current_version() const`: Returns current version number.
- `void on_update(std::function<void(const Versioned<T>&)> callback)`: Sets callback for update events.

## Thread Safety Considerations

Both classes use `std::mutex` to protect shared data. Callbacks registered with `on_update` are invoked synchronously within the critical section of the `update` or `emplace` methods. Care should be taken to ensure that callbacks are lightweight and do not attempt to re-enter the `RetainLatest` or `VersionedRetainLatest` object in a way that could cause deadlock.
The `std::shared_ptr` is used to manage the lifetime of the stored object, and atomic operations on `std::shared_ptr` itself (like assignment or `std::move`) are thread-safe. The mutex primarily protects the sequence of operations (e.g., read-modify-write of the `value_ptr_`).
For `VersionedRetainLatest`, `next_version_` is an `std::atomic<uint64_t>` and is incremented using `fetch_add` with `std::memory_order_relaxed`, which is sufficient as the mutex ensures overall consistency of the update operation.

# ScopedFlag and FlagGuard Utilities

## Overview

The `scoped_flag.h` header provides RAII (Resource Acquisition Is Initialization) utility classes, `ScopedFlag` and `FlagGuard<T>`, designed to temporarily modify the value of a flag or variable. Upon exiting the scope where the guard object is declared, the original value of the flag is automatically restored. This is particularly useful for ensuring that flags are reset correctly, even in the presence of exceptions.

These utilities are inspired by Python's context managers and aim to provide a clean, safe, and convenient way to manage state that needs to be temporarily changed.

## Features

- **RAII Principle**: Automatic restoration of the original flag value when the guard object goes out of scope.
- **Exception Safety**: Guarantees restoration even if exceptions are thrown.
- **Type Support**:
    - `ScopedFlag`: Specialized for `bool` and `std::atomic<bool>`.
    - `FlagGuard<T>`: A template class that works with any assignable and copy-constructible type (e.g., `int`, `enum`, custom types).
- **Convenience Functions**: Helper functions like `temporarily_disable` and `temporarily_enable` for common boolean flag operations.
- **Non-Copyable/Non-Movable**: Prevents accidental misuse that could lead to incorrect flag restoration.
- **`previous()` Method**: Allows querying the original value of the flag before it was modified by the guard.
- **`FlagGuard<T>::set_if_not()`**: A static factory method to create a guard that only modifies the flag if its current value is different from the desired new value.

## `ScopedFlag`

`ScopedFlag` is designed specifically for boolean flags. It has overloaded constructors to handle both standard `bool` variables and `std::atomic<bool>` variables.

### API

```cpp
namespace utils {

class ScopedFlag {
public:
    // Constructor for regular bool
    explicit ScopedFlag(bool& flag, bool new_value);

    // Constructor for atomic bool
    explicit ScopedFlag(std::atomic_bool& flag, bool new_value);

    // Destructor (restores original value)
    ~ScopedFlag();

    // Returns the original value of the flag
    bool previous() const noexcept;

    // Deleted copy/move constructors and assignment operators
    ScopedFlag(const ScopedFlag&) = delete;
    ScopedFlag& operator=(const ScopedFlag&) = delete;
    ScopedFlag(ScopedFlag&&) = delete;
    ScopedFlag& operator=(ScopedFlag&&) = delete;
};

// Convenience functions
ScopedFlag temporarily_disable(bool& flag);
ScopedFlag temporarily_disable(std::atomic_bool& flag);
ScopedFlag temporarily_enable(bool& flag);
ScopedFlag temporarily_enable(std::atomic_bool& flag);

} // namespace utils
```

### Usage Example

```cpp
#include "scoped_flag.h"
#include <iostream>
#include <atomic>

bool g_enable_feature_x = false;
std::atomic<bool> g_critical_section_busy{false};

void process_data() {
    if (!g_enable_feature_x) {
        std::cout << "Feature X is disabled. Skipping process_data.\n";
        return;
    }
    std::cout << "Processing data with Feature X enabled...\n";
    // ... processing logic ...
}

void perform_critical_operation() {
    // Attempt to enter critical section
    utils::ScopedFlag busy_guard(g_critical_section_busy, true);
    if (busy_guard.previous()) { // Was it already busy?
        std::cout << "Critical section was already busy. Aborting.\n";
        // busy_guard goes out of scope, g_critical_section_busy is restored
        // (which means it remains true if it was already true, or becomes true if it was false)
        // This specific logic might need refinement based on desired behavior for already-set flags.
        // A better approach for mutex-like behavior might be to check first, then set.
        // For this example, we assume we set it regardless and check its prior state.
        return;
    }

    std::cout << "Entered critical section.\n";
    // ... critical operation logic ...
    std::cout << "Exiting critical section.\n";
    // g_critical_section_busy will be restored to false when busy_guard goes out of scope.
}


int main() {
    std::cout << "Initial: Feature X enabled: " << g_enable_feature_x << std::endl;
    process_data(); // Feature X is disabled

    {
        utils::ScopedFlag enable_x_guard(g_enable_feature_x, true);
        std::cout << "Scope 1: Feature X enabled: " << g_enable_feature_x << std::endl;
        process_data(); // Feature X is enabled

        {
            auto disable_x_guard = utils::temporarily_disable(g_enable_feature_x);
            std::cout << "Scope 2: Feature X enabled: " << g_enable_feature_x << std::endl;
            process_data(); // Feature X is temporarily disabled
            std::cout << "Scope 2: Previous value was: " << disable_x_guard.previous() << std::endl;
        } // disable_x_guard restores g_enable_feature_x to true

        std::cout << "Scope 1 (after Scope 2): Feature X enabled: " << g_enable_feature_x << std::endl;
        process_data(); // Feature X is enabled again
    } // enable_x_guard restores g_enable_feature_x to false

    std::cout << "Final: Feature X enabled: " << g_enable_feature_x << std::endl;
    process_data(); // Feature X is disabled again

    // Atomic flag example
    std::cout << "\nInitial: Critical section busy: " << g_critical_section_busy.load() << std::endl;
    perform_critical_operation();
    std::cout << "After op 1: Critical section busy: " << g_critical_section_busy.load() << std::endl;

    // Simulate it being busy
    g_critical_section_busy.store(true);
    std::cout << "Manually set: Critical section busy: " << g_critical_section_busy.load() << std::endl;
    perform_critical_operation(); // Will see it's busy
    std::cout << "After op 2: Critical section busy: " << g_critical_section_busy.load() << std::endl;
    // Note: If perform_critical_operation sets it to true and it was already true,
    // ScopedFlag will "restore" it to true. This is correct behavior for ScopedFlag.

    return 0;
}
```

## `FlagGuard<T>`

`FlagGuard<T>` is a template class that provides the same RAII functionality for flags or variables of any type `T`, provided `T` is assignable and copy-constructible.

### API

```cpp
namespace utils {

template <typename T>
class FlagGuard {
    static_assert(std::is_assignable_v<T&, T>, "Type T must be assignable");
    static_assert(std::is_copy_constructible_v<T>, "Type T must be copy constructible");
public:
    explicit FlagGuard(T& flag, T new_value);
    ~FlagGuard();

    const T& previous() const noexcept;

    // Conditionally sets the flag.
    // Creates a guard that modifies the flag to new_value only if flag != new_value.
    // If flag == new_value, the flag remains unchanged, and 'previous()' will return this value.
    static FlagGuard<T> set_if_not(T& flag, T new_value);

    // Deleted copy/move constructors and assignment operators
    FlagGuard(const FlagGuard&) = delete;
    FlagGuard& operator=(const FlagGuard&) = delete;
    FlagGuard(FlagGuard&&) = delete;
    FlagGuard& operator=(FlagGuard&&) = delete;
};

// Convenience type aliases
using BoolGuard = FlagGuard<bool>; // Equivalent to ScopedFlag for bool but using the generic template
using IntGuard = FlagGuard<int>;

} // namespace utils
```

### Usage Example

```cpp
#include "scoped_flag.h" // FlagGuard is in the same header
#include <iostream>

enum class LogLevel { Info, Warning, Error, Debug };
LogLevel g_current_log_level = LogLevel::Info;

std::ostream& operator<<(std::ostream& os, LogLevel level) {
    switch (level) {
        case LogLevel::Info: os << "Info"; break;
        case LogLevel::Warning: os << "Warning"; break;
        case LogLevel::Error: os << "Error"; break;
        case LogLevel::Debug: os << "Debug"; break;
        default: os << "Unknown"; break;
    }
    return os;
}

void log_message(LogLevel level, const std::string& message) {
    if (static_cast<int>(level) >= static_cast<int>(g_current_log_level)) {
        std::cout << "[" << level << "] " << message << std::endl;
    }
}

int main() {
    std::cout << "Initial log level: " << g_current_log_level << std::endl;
    log_message(LogLevel::Debug, "This is a debug message.");   // Won't print
    log_message(LogLevel::Info, "This is an info message.");    // Will print
    log_message(LogLevel::Error, "This is an error message.");  // Will print

    {
        utils::FlagGuard<LogLevel> level_guard(g_current_log_level, LogLevel::Debug);
        std::cout << "\nLog level temporarily set to: " << g_current_log_level
                  << " (previous was " << level_guard.previous() << ")" << std::endl;
        log_message(LogLevel::Debug, "Debug message visible now."); // Will print
        log_message(LogLevel::Info, "Info message still visible.");   // Will print
    } // g_current_log_level restored to LogLevel::Info

    std::cout << "\nLog level restored to: " << g_current_log_level << std::endl;
    log_message(LogLevel::Debug, "Debug message hidden again."); // Won't print

    // Example with set_if_not
    int g_value = 10;
    std::cout << "\nInitial g_value: " << g_value << std::endl;
    {
        // g_value is 10, new_value is 20. Flag will be changed.
        auto guard1 = utils::FlagGuard<int>::set_if_not(g_value, 20);
        std::cout << "g_value inside guard1: " << g_value << " (previous: " << guard1.previous() << ")" << std::endl; // 20 (previous: 10)
    } // g_value restored to 10
    std::cout << "g_value after guard1: " << g_value << std::endl; // 10

    {
        // g_value is 10, new_value is 10. Flag will NOT be changed by set_if_not logic.
        // The guard is still active and will "restore" it to its value at construction (10).
        auto guard2 = utils::FlagGuard<int>::set_if_not(g_value, 10);
        std::cout << "g_value inside guard2: " << g_value << " (previous: " << guard2.previous() << ")" << std::endl; // 10 (previous: 10)
    } // g_value restored to 10 (effectively no change)
    std::cout << "g_value after guard2: " << g_value << std::endl; // 10

    return 0;
}
```

## Exception Safety

Both `ScopedFlag` and `FlagGuard<T>` are exception-safe. If an exception is thrown within the scope where a guard object exists, the guard's destructor will still be called as part of stack unwinding, ensuring the original flag value is restored.

```cpp
bool g_critical_resource_locked = false;

void process_with_resource() {
    utils::ScopedFlag lock_guard(g_critical_resource_locked, true);
    std::cout << "Resource locked: " << g_critical_resource_locked << std::endl;

    if (/* some error condition */ true) {
        throw std::runtime_error("Failed during processing with resource!");
    }

    // ... more processing ...
    // lock_guard destructor called here if no exception
} // lock_guard destructor called here due to stack unwinding if exception occurs

int main() {
    try {
        process_with_resource();
    } catch (const std::exception& e) {
        std::cerr << "Caught exception: " << e.what() << std::endl;
    }
    std::cout << "After operation, resource locked: " << g_critical_resource_locked << std::endl; // Should be false
    return 0;
}
```

This mechanism helps prevent resource leaks or inconsistent states that might arise from flags not being reset properly after errors.

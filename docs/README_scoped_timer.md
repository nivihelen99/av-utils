# `ScopedTimer` - RAII-Based Automatic Timer

## Overview

The `ScopedTimer` class (`scoped_timer.h`) is a lightweight, header-only C++ utility that provides RAII-based automatic timing for code blocks. It starts measuring time upon its construction and automatically calculates and reports the elapsed time when it goes out of scope (i.e., upon its destruction).

This makes it very convenient for quick performance profiling of specific sections of code with minimal boilerplate. It is designed to be exception-safe, ensuring that timing information is reported even if an exception causes the scope to be exited prematurely.

## Features

-   **RAII Semantics:** Automatically starts and stops timing based on object lifetime.
-   **High-Resolution Timing:** Uses `std::chrono::steady_clock` for accurate time measurement.
-   **Microsecond Precision:** Reports durations in microseconds by default.
-   **Customizable Output:**
    -   Defaults to printing to `std::cout`.
    -   Can be configured to print to any `std::ostream`.
    -   Supports a custom callback function for flexible handling of timing results (e.g., logging to a file, sending to a metrics system).
-   **Descriptive Labels:** Timers can be given string labels for easy identification in output.
-   **Exception Safe:** The destructor ensures that timing information is reported even if an exception unwinds the stack, and it swallows exceptions thrown by the reporting callback itself.
-   **Manual Reset:** A `reset()` method allows restarting the timer within the same scope.
-   **Elapsed Time Query:** An `elapsed()` method allows querying the current duration without stopping the timer.
-   **Non-Copyable/Non-Movable:** To prevent accidental misuse or timing inaccuracies.
-   **Convenience Macros:** `SCOPED_TIMER("label")` and `SCOPED_TIMER_AUTO()` for quick and easy usage.

## Public Interface

### Constructors
-   **`ScopedTimer()`**:
    -   Creates an anonymous timer. Output (if default callback is used) will typically not have a specific label or use a default one.
-   **`explicit ScopedTimer(std::string label)`**:
    -   Creates a timer with a descriptive `label`. Default output to `std::cout`.
-   **`ScopedTimer(std::string label, std::ostream& out)`**:
    -   Creates a timer with a `label`, sending output to the specified `out` stream.
-   **`ScopedTimer(std::string label, Callback callback)`**:
    -   Creates a timer with a `label` and a custom `Callback` function.
    -   `Callback` type: `std::function<void(const std::string& label, std::chrono::microseconds duration)>`.

### Methods
-   **`void reset() noexcept`**:
    -   Restarts the timer, setting its start time to the current moment. The original label and callback remain.
-   **`std::chrono::microseconds elapsed() const noexcept`**:
    -   Returns the duration elapsed since the timer was constructed or last `reset()`, without stopping the timer or invoking the reporting callback.

### Macros for Convenience
-   **`SCOPED_TIMER(label_string_literal)`**:
    -   Example: `SCOPED_TIMER("My Network Call");`
    -   Creates a `ScopedTimer` instance with an automatically generated unique variable name and the provided string literal label.
-   **`SCOPED_TIMER_AUTO()`**:
    -   Example: `SCOPED_TIMER_AUTO();`
    -   Creates a `ScopedTimer` instance with an automatically generated unique variable name and an empty label (using the default constructor).

## Usage Examples

(Based on `examples/scoped_timer_example.cpp`)

### Basic Usage with Labels and `std::cout`

```cpp
#include "scoped_timer.h" // Adjust path as needed
#include <iostream>
#include <thread>         // For std::this_thread::sleep_for
#include <chrono>         // For duration literals

using namespace std::chrono_literals;

void some_long_operation() {
    std::this_thread::sleep_for(150ms);
}

int main() {
    std::cout << "Starting operations..." << std::endl;
    {
        ScopedTimer timer1("Overall Process"); // Times the whole block

        std::this_thread::sleep_for(50ms); // Part 1
        std::cout << "  Part 1 finished." << std::endl;

        {
            SCOPED_TIMER("Inner Critical Section"); // Macro usage
            some_long_operation();
        } // Inner timer reports here

        std::cout << "  Part 2 (after inner section) starting." << std::endl;
        std::this_thread::sleep_for(75ms); // Part 2

    } // timer1 (Overall Process) reports here
    std::cout << "Operations finished." << std::endl;

    // Example Output:
    // Starting operations...
    //   Part 1 finished.
    // [ScopedTimer] Inner Critical Section: 150XXX µs
    //   Part 2 (after inner section) starting.
    // [ScopedTimer] Overall Process: 275XXX µs
    // Operations finished.
}
```

### Custom Output Stream and Callback

```cpp
#include "scoped_timer.h"
#include <iostream>
#include <sstream> // For std::ostringstream
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

// Example of a custom logging function
void my_log_function(const std::string& operation_name, std::chrono::microseconds time_taken) {
    std::cerr << "[PERFORMANCE_LOG] Operation '" << operation_name
              << "' completed in " << time_taken.count() << " us." << std::endl;
}

int main() {
    // Using a custom output stream (e.g., a string stream)
    std::ostringstream log_buffer;
    {
        ScopedTimer timer_to_stream("File Write Op", log_buffer);
        std::this_thread::sleep_for(25ms);
    } // Output goes to log_buffer
    std::cout << "Captured in log_buffer: " << log_buffer.str();

    // Using a custom callback function
    {
        ScopedTimer timer_with_callback("API Call", my_log_function);
        std::this_thread::sleep_for(35ms);
    } // my_log_function will be called with "API Call" and the duration
}
```

### Using `reset()` and `elapsed()`

```cpp
#include "scoped_timer.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

int main() {
    ScopedTimer timer("Multi-Stage Op");

    std::cout << "Starting stage 1..." << std::endl;
    std::this_thread::sleep_for(20ms);
    std::cout << "Stage 1 elapsed: " << timer.elapsed().count() << " us" << std::endl;

    timer.reset(); // Reset timer for stage 2
    std::cout << "Starting stage 2 (after reset)..." << std::endl;
    std::this_thread::sleep_for(30ms);
    std::cout << "Stage 2 elapsed (since reset): " << timer.elapsed().count() << " us" << std::endl;

    // When timer goes out of scope, it will report the time for stage 2 only
} // Reports duration of stage 2
```

## Dependencies
- `<chrono>`
- `<functional>` (for `std::function` used in `Callback`)
- `<iostream>` (for default output and examples)
- `<string>`

`ScopedTimer` is a simple yet effective tool for ad-hoc performance measurements within C++ code.

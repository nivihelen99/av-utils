# IntervalCounter and IntervalCounterST

`IntervalCounter` and `IntervalCounterST` are C++ classes designed for efficient time-windowed event counting with sliding window support. They are useful for tracking event rates, monitoring, and implementing rate limiting.

- `IntervalCounter`: Thread-safe version using a mutex for synchronization.
- `IntervalCounterST`: Single-threaded version, offering higher performance by omitting locks. Only use this if you are certain that it will be accessed from a single thread.

Both classes are header-only and require C++17.

## Features

- **O(1) `record()` and `count()` operations (amortized):** Efficiently record events and query the current count within the window.
- **Sliding Window:** Automatically discards events older than the defined window duration.
- **Configurable Time Resolution:** Events are grouped into time buckets. The size of these buckets (resolution) can be configured.
- **Thread-Safety (IntervalCounter):** `IntervalCounter` can be safely used across multiple threads.
- **High Performance (IntervalCounterST):** `IntervalCounterST` provides a lock-free alternative for single-threaded scenarios where maximum performance is critical.

## API Overview

Both `IntervalCounter` and `IntervalCounterST` share a similar API.

### Constructor

```cpp
explicit IntervalCounter(
    std::chrono::seconds window,
    std::chrono::milliseconds resolution = std::chrono::milliseconds{1000}
);

explicit IntervalCounterST(
    std::chrono::seconds window,
    std::chrono::milliseconds resolution = std::chrono::milliseconds{1000}
);
```
- `window`: The total duration for which to track events (e.g., `60s`). Must be positive.
- `resolution`: The size of time buckets for quantizing events (e.g., `1s`, `100ms`). This determines the granularity of the counter. Must be positive.

### Core Methods

- `void record()`: Records a single event at the current time.
- `void record(int count)`: Records multiple events at the current time. Does nothing if `count` is zero or negative.
- `size_t count()`: Returns the total number of events currently within the sliding window. This operation also triggers the cleanup of expired buckets.
- `double rate_per_second()`: Calculates and returns the average number of events per second over the current window.
- `void clear()`: Removes all recorded events and resets the counter.

### Configuration and Diagnostics

- `std::chrono::seconds window_duration() const`: Returns the configured window duration.
- `std::chrono::milliseconds resolution() const`: Returns the configured resolution.
- `std::map<std::chrono::steady_clock::time_point, int> bucket_counts() const`: Returns a map where keys are the start timestamps of active buckets and values are the event counts within those buckets. This is useful for debugging or detailed statistics.
  - For `IntervalCounter`, this method is thread-safe.

### Type Aliases

For convenience, the following type aliases are provided:
```cpp
using RateTracker = util::IntervalCounter;
using RateTrackerST = util::IntervalCounterST;
```

## Usage Examples

### Basic Usage (Thread-Safe)

```cpp
#include "interval_counter.h" // Or "RateTracker.h" if using the alias
#include <iostream>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;
using namespace util; // Or your specific namespace

int main() {
    // Track events over the past 60 seconds, with 1-second resolution buckets
    IntervalCounter request_tracker(60s, 1s);

    // Record some events
    request_tracker.record();    // Record 1 event
    request_tracker.record(5);   // Record 5 events

    std::cout << "Events in the last 60s: " << request_tracker.count() << std::endl;
    std::cout << "Rate: " << request_tracker.rate_per_second() << " events/sec\n";

    // Simulate passage of time
    std::this_thread::sleep_for(30s);
    request_tracker.record(10); // Record 10 more events

    std::cout << "After 30s, events in the last 60s: " << request_tracker.count() << std::endl;
    std::cout << "New rate: " << request_tracker.rate_per_second() << " events/sec\n";

    // Wait for the initial events to expire
    std::this_thread::sleep_for(35s); // Total >60s passed since first events

    std::cout << "After >60s, events in the last 60s: " << request_tracker.count() << std::endl;
    std::cout << "Rate after expiration: " << request_tracker.rate_per_second() << " events/sec\n";
    // Expected count will be 10 (the events recorded 35s ago)

    return 0;
}
```

### High-Performance Single-Threaded Usage

```cpp
#include "interval_counter.h" // Or "RateTracker.h"
#include <iostream>
#include <chrono>
#include <vector>

using namespace std::chrono_literals;
using namespace util;

void process_high_frequency_data() {
    // Track events over 10 seconds, with 100ms resolution
    IntervalCounterST data_point_counter(10s, 100ms);

    // Simulate receiving data points rapidly
    for (int i = 0; i < 1000; ++i) {
        data_point_counter.record();
        // Potentially some very quick processing for each data point
        if (i % 100 == 0) {
            // Brief pause to simulate work and allow time to pass
            std::this_thread::sleep_for(50ms);
        }
    }

    std::cout << "Data points in last 10s: " << data_point_counter.count() << std::endl;
    std::cout << "Data rate: " << data_point_counter.rate_per_second() << " points/sec\n";

    // Display bucket distribution
    auto buckets = data_point_counter.bucket_counts();
    std::cout << "Number of active buckets: " << buckets.size() << std::endl;
    for (const auto& pair : buckets) {
        // Timestamps can be converted to relative times for easier reading if needed
        std::cout << "  Bucket at offset X: " << pair.second << " events\n";
    }
}

int main() {
    process_high_frequency_data();
    return 0;
}
```

### API Rate Limiting Example

```cpp
#include "interval_counter.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;
using namespace util;

class APIService {
private:
    IntervalCounter request_counter_;
    const size_t max_requests_per_minute_ = 100;

public:
    APIService() : request_counter_(60s, 1s) {} // 60-second window, 1s resolution

    bool handle_request(const std::string& user_id) {
        request_counter_.record(); // Record one request

        if (request_counter_.count() > max_requests_per_minute_) {
            std::cout << "User " << user_id << ": Rate limit exceeded! "
                      << request_counter_.count() << " requests in last minute. Rejecting.\n";
            return false; // Reject request
        }

        std::cout << "User " << user_id << ": Request accepted. Current rate: "
                  << request_counter_.count() << "/min.\n";
        // Process the request...
        return true;
    }
};

int main() {
    APIService service;

    // Simulate requests from multiple users (or a single user rapidly)
    for (int i = 0; i < 120; ++i) {
        service.handle_request("user123");
        std::this_thread::sleep_for(300ms); // Simulate requests coming in
    }
    return 0;
}
```

## When to Use

- **Rate Limiting:** Implementing API rate limiters, connection attempt limits, etc.
- **Monitoring:** Tracking error rates, request rates, message queue throughput.
- **Real-time Analytics:** Observing trends in event occurrences over time.
- **Performance Profiling:** Counting occurrences of specific operations within time windows.

Choose `IntervalCounter` for multi-threaded applications where shared access to the counter is needed. Opt for `IntervalCounterST` in performance-critical single-threaded loops or contexts where thread safety is handled externally or is not a concern.

# `ChronoRing<T, Clock>` – Time-Decaying Ring Buffer

## Overview

`ChronoRing<T, Clock>` is a C++ header-only utility that implements a fixed-size, time-aware ring buffer. It's designed for scenarios where you need to store a limited number of elements, each associated with a timestamp, and efficiently query or expire these elements based on time windows. This is particularly useful in systems dealing with streams of temporal data like logs, metrics, or real-time events, where recency and bounded memory usage are critical.

The default clock used is `std::chrono::steady_clock` for monotonic time tracking, but it can be customized via the `Clock` template parameter.

## Core Features

*   **Fixed Capacity**: The ring buffer holds a pre-defined maximum number of elements. When full, new elements overwrite the oldest ones.
*   **Timestamping**: Each element is stored alongside a timestamp. Timestamps are automatically recorded at the moment of insertion using `Clock::now()` unless an explicit timestamp is provided.
*   **Time-Based Queries**:
    *   `recent(duration)`: Retrieve elements inserted within a specified recent time duration.
    *   `entries_between(startTime, endTime)`: Fetch all entries whose timestamps fall within a given time interval.
*   **Expiration**:
    *   `expire_older_than(cutoffTime)`: Remove all entries older than a specified time point.
*   **Efficiency**:
    *   Insertions (`push`, `push_at`) are amortized O(1).
    *   Time-based queries and expirations are O(N), where N is the number of elements currently in the buffer.

## API Highlights

```cpp
#include "chrono_ring.h" // From the anomeda namespace
#include <chrono>
#include <string>
#include <vector>

// For brevity in examples
using namespace anomeda;
using namespace std::chrono_literals;

// Basic Usage
ChronoRing<std::string> message_log(100); // Capacity of 100

message_log.push("User logged in");
std::this_thread::sleep_for(50ms);
message_log.push_at("System alert", std::chrono::steady_clock::now() - 1s); // An event from the past

// Get messages from the last 200 milliseconds
std::vector<std::string> recent_messages = message_log.recent(200ms);

// Get all log entries
auto all_entries = message_log.entries_between(
    std::chrono::steady_clock::time_point::min(),
    std::chrono::steady_clock::time_point::max()
);

for(const auto& entry : all_entries) {
    // entry.value and entry.timestamp
}

// Remove entries older than 1 minute
message_log.expire_older_than(std::chrono::steady_clock::now() - 1min);

// Other utilities
size_t current_size = message_log.size();
size_t capacity = message_log.capacity();
bool is_empty = message_log.empty();
message_log.clear();
```

### Template Parameters

*   `typename T`: The type of the value to be stored.
*   `typename Clock = std::chrono::steady_clock`: The clock type to use for timestamping. Must meet C++ Clock requirements.

### Key Methods

*   `explicit ChronoRing(size_t capacity)`: Constructor.
*   `void push(const T& value)`: Adds an element with the current time.
*   `void push_at(const T& value, TimePoint time)`: Adds an element with a specific time.
*   `std::vector<T> recent(std::chrono::milliseconds duration) const`: Returns values within the recent duration from now.
*   `std::vector<Entry> entries_between(TimePoint start_time, TimePoint end_time) const`: Returns entries within the specified absolute time window.
*   `void expire_older_than(TimePoint cutoff)`: Removes entries older than the cutoff.
*   `size_t size() const`, `size_t capacity() const`, `bool empty() const`, `void clear()`

## Thread Safety

The `ChronoRing` class is **not thread-safe** for concurrent write operations, or concurrent read and write operations. If multiple threads need to access a `ChronoRing` instance concurrently, external synchronization mechanisms (e.g., `std::mutex`) must be used.

## Dependencies
* C++17
* Standard Library (vector, chrono, cstddef, stdexcept)

```

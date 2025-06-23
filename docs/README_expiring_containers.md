# Expiring Containers (`expiring_containers.h`)

## Overview

The `expiring_containers.h` header provides C++ container classes whose entries automatically expire after a specified Time-To-Live (TTL). These containers are useful for implementing caches, rate limiters, event deduplication systems, or any scenario where data should only be considered valid for a certain period.

The header currently includes:
-   `expiring::TimeStampedQueue<T>`: A FIFO queue with expiring entries.
-   `expiring::ExpiringDict<K, V>`: A key-value map with expiring entries.

Both containers use `std::chrono::steady_clock` for time measurements and allow TTLs to be specified with `std::chrono::duration` (defaulting to milliseconds).

## Namespace
All utilities are within the `expiring` namespace.

## `expiring::TimeStampedQueue<T>`

A First-In, First-Out (FIFO) queue where each element is timestamped upon insertion. Elements older than the configured TTL are automatically removed when methods like `pop()`, `front()`, `size()`, or `empty()` are called, or when `expire()` is explicitly invoked.

### Template Parameter
-   `T`: The type of elements stored in the queue.

### Public Interface Highlights
-   **Constructor**: `explicit TimeStampedQueue(std::chrono::milliseconds ttl)`
-   **`void push(const T& item)` / `void push(T&& item)`**: Adds an item to the back of thequeue with the current timestamp.
-   **`T pop()`**: Removes and returns the front (oldest) item. Expires old entries first. Throws `std::runtime_error` if empty after expiration.
-   **`const T& front()`**: Accesses the front item. Expires old entries first. Throws `std::runtime_error` if empty after expiration.
-   **`void expire()`**: Explicitly removes all expired entries.
-   **`size_t size()`**: Returns the number of non-expired elements (calls `expire()` internally).
-   **`bool empty()`**: Checks if the queue is empty after expiration (calls `expire()` internally).
-   **`void clear()`**: Removes all items immediately.
-   **`void set_ttl(std::chrono::milliseconds new_ttl)`**: Changes the TTL for future checks.
-   **`std::chrono::milliseconds get_ttl() const`**: Gets the current TTL.

## `expiring::ExpiringDict<K, V>`

A key-value map where each entry is timestamped upon insertion. Entries older than the configured TTL are considered expired. Expired entries may be removed during access operations (`find`, `contains`) or explicitly via the `expire()` method.

### Template Parameters
-   `K`: The key type (must be hashable and equality-comparable for the default underlying `std::unordered_map`).
-   `V`: The value type.

### Public Interface Highlights
-   **Constructor**: `explicit ExpiringDict(std::chrono::milliseconds ttl, bool access_renews = false)`
    -   `ttl`: Time-to-live for entries.
    -   `access_renews`: If `true`, successfully accessing an entry (via `find` or `contains`) will renew its timestamp, effectively resetting its TTL.
-   **`void insert(const K& key, const V& value)` / `void insert(const K& key, V&& value)`**: Inserts or overwrites a key-value pair, timestamping it with the current time.
-   **`V* find(const K& key)` / `const V* find(const K& key) const`**:
    -   Finds a value by key.
    -   If the entry is found but expired, it's removed (in non-const version) and `nullptr` is returned.
    -   If `access_renews` is true and the entry is found and live, its TTL is renewed.
    -   Returns a pointer to the value if found and live, otherwise `nullptr`.
-   **`bool contains(const K& key)`**: Checks if a key exists and its entry is not expired. Also handles expiration and TTL renewal like `find()`.
-   **`bool erase(const K& key)`**: Manually removes an entry.
-   **`void expire()`**: Explicitly removes all expired entries from the map.
-   **`size_t size()`**: Returns the number of non-expired entries (calls `expire()` internally).
-   **`bool empty()`**: Checks if the map is empty after expiration (calls `expire()` internally).
-   **`void clear()`**: Removes all entries immediately.
-   **`bool update(const K& key, const V& value)` / `bool update(const K& key, V&& value)`**: Updates the value for `key` and refreshes its timestamp (equivalent to `insert`). Returns `true` if the key already existed.
-   **`void set_ttl(std::chrono::milliseconds new_ttl)` / `std::chrono::milliseconds get_ttl() const`.**
-   **`void set_access_renews(bool renews)` / `bool get_access_renews() const`.**
-   **`template<typename Func> void for_each(Func&& func)`**: Calls `expire()` then applies `func` to each live (key, value) pair.

## Usage Examples

(Based on `examples/expiring_example.cpp`)

### `TimeStampedQueue` Example

```cpp
#include "expiring_containers.h"
#include <iostream>
#include <string>
#include <thread> // For std::this_thread::sleep_for
#include <chrono> // For duration literals

using namespace std::chrono_literals;

int main() {
    expiring::TimeStampedQueue<std::string> message_queue(100ms);

    message_queue.push("Message 1");
    std::this_thread::sleep_for(50ms);
    message_queue.push("Message 2");

    std::cout << "Queue size: " << message_queue.size() << std::endl; // Expected: 2

    std::this_thread::sleep_for(80ms); // Total time for Message 1: 50+80=130ms (expired)
                                      // Total time for Message 2: 80ms (live)

    std::cout << "Front after 130ms for M1, 80ms for M2: " << message_queue.front() << std::endl; // "Message 2"
    std::cout << "Queue size: " << message_queue.size() << std::endl; // Expected: 1

    std::this_thread::sleep_for(50ms); // Total time for Message 2: 80+50=130ms (expired)
    std::cout << "Queue empty after Message 2 expires? "
              << std::boolalpha << message_queue.empty() << std::endl; // Expected: true
}
```

### `ExpiringDict` Example

```cpp
#include "expiring_containers.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

int main() {
    // Cache with 200ms TTL, access renews TTL
    expiring::ExpiringDict<std::string, int> user_sessions(200ms, true);

    user_sessions.insert("user_alice", 12345);
    std::cout << "Alice's session active: "
              << std::boolalpha << user_sessions.contains("user_alice") << std::endl; // true

    std::this_thread::sleep_for(150ms);
    // Access Alice's session to renew TTL
    if (int* session_id = user_sessions.find("user_alice")) {
        std::cout << "Accessed Alice's session ID: " << *session_id << ". TTL renewed." << std::endl;
    }

    std::this_thread::sleep_for(150ms); // Total time since insert > 200ms, but < 200ms since last access
    std::cout << "Alice's session still active due to renewal: "
              << std::boolalpha << user_sessions.contains("user_alice") << std::endl; // true

    std::this_thread::sleep_for(250ms); // Now > 200ms since last access
    std::cout << "Alice's session expired after no renewal: "
              << std::boolalpha << user_sessions.contains("user_alice") << std::endl; // false
}
```

## Dependencies
- `<chrono>`
- `<deque>` (for `TimeStampedQueue`)
- `<unordered_map>` (for `ExpiringDict`)
- `<utility>`

These containers provide convenient mechanisms for managing time-sensitive data with automatic expiration.

# `RedisIDAllocator` (SWSS Distributed ID Allocator)

## Overview

The `RedisIDAllocator` class (`redis_id_allocator.h`) provides a robust solution for managing and allocating unique integer IDs in a distributed environment, leveraging a Redis backend. It is designed for use within the SWSS (Software for Open Networking in the Cloud) ecosystem, utilizing SWSS library components for Redis communication and logging.

This allocator supports a defined range of IDs, manages their allocation state using Redis bitmaps, and associates Time-To-Live (TTL) with each allocation, allowing for automatic cleanup of expired IDs. It features a distributed initialization mechanism where one instance becomes a "Creator" responsible for setup and cleanup, while other instances act as "Consumers."

## Key Features

-   **Distributed ID Allocation:** Ensures unique ID generation across multiple application instances or services sharing a Redis backend.
-   **Creator/Consumer Roles:** Uses a distributed lock on Redis for one instance to assume the Creator role, which handles initialization and periodic cleanup of expired IDs. Other instances act as Consumers.
-   **Range-Based Allocation:** Manages IDs within a configurable `[min_id, max_id]` range.
-   **Bitmap for Allocation Tracking:** Efficiently stores the allocation status of IDs using Redis bitmaps.
-   **TTL for IDs:** Allocated IDs have a Time-To-Live, after which they are considered expired and can be cleaned up by the Creator instance.
-   **Atomic Operations via Lua:** Core allocation, reservation, freeing, and cleanup logic is implemented using Lua scripts executed atomically on the Redis server.
-   **Specific ID Reservation:** Allows attempting to reserve specific IDs or ranges of IDs.
-   **Range Allocation/Freeing:** Supports allocating and freeing contiguous blocks of IDs.
-   **Statistics:** Provides methods to get statistics about the allocator's state and utilization.
-   **Thread Safety:** Operations on a `RedisIDAllocator` instance are protected by an internal mutex.
-   **SWSS Integration:** Uses `swss::DBConnector` for Redis communication and `swss::Logger` for logging.

## Core Concepts

-   **Configuration (`Config` struct):** Defines `min_id`, `max_id`, TTLs, cleanup intervals, Redis key prefixes, and parameters for distributed coordination.
-   **Redis Keys:**
    -   Bitmap Key (`<prefix>:<name>:bitmap`): Stores the allocation state of IDs.
    -   Meta Key (`<prefix>:<name>:meta`): A Redis Hash storing metadata for allocated IDs, primarily their expiration timestamps.
    -   Init Key (`<prefix>:<name>:init`): A flag indicating if the allocator structures have been initialized in Redis.
    -   Creator Key (`<prefix>:<name>:creator`): Used as a lease for the Creator instance, managed with TTLs and heartbeats.
    -   Config Key (`<prefix>:<name>:config`): Stores the serialized configuration of the allocator.
-   **Lua Scripts:** Embedded Lua scripts perform the actual Redis data manipulation for atomicity and efficiency.
-   **Heartbeats:** The Creator instance periodically renews its lease on the `creatorKey` to signal it's alive. Consumers can check this lease.

## Public Interface Highlights

### Factory Method (Primary Entry Point)
-   **`static std::unique_ptr<RedisIDAllocator> createOrAttach(swss::DBConnector* db, const std::string& name, const Config& config)`**:
    -   Attempts to become the Creator or attaches as a Consumer. This is the recommended way to get an instance.
    -   `db`: Pointer to a connected `swss::DBConnector` instance.
    -   `name`: A unique name for this allocator instance (used in Redis keys).
    -   `config`: The `Config` struct defining allocator parameters.

### Allocation & Reservation
-   **`std::optional<int> allocate(int ttl_seconds = 0)`**: Allocates the next available ID. Uses default TTL from `Config` if `ttl_seconds` is 0.
-   **`std::optional<int> allocateRange(int count, int ttl_seconds = 0)`**: Allocates a contiguous range of `count` IDs.
-   **`bool reserve(int id, int ttl_seconds = 0)`**: Attempts to reserve a specific `id`.
-   **`bool reserveRange(int start, int end, int ttl_seconds = 0)`**: Attempts to reserve a specific range of IDs.

### Freeing IDs
-   **`bool free(int id)`**: Marks a specific `id` as free.
-   **`bool freeRange(int start, int end)`**: Marks a range of IDs as free.

### Status & TTL Management
-   **`bool isAllocated(int id) const`**: Checks if an `id` is currently allocated.
-   **`bool extendTTL(int id, int additional_seconds)`**: Extends the TTL of an already allocated `id`.

### Creator-Specific Operations
-   **`int cleanupExpired()`**: Manually triggers cleanup of expired IDs (only if instance is Creator).
-   **`bool clearAll()`**: Removes all allocation data from Redis (dangerous, Creator only).

### Information & Statistics
-   **`Stats getStats() const`**: Returns a struct with allocation statistics, role, creator status, etc.
-   **`std::vector<int> getAllocatedIds() const`**: Retrieves a list of all currently allocated IDs.
-   **`Role getRole() const`**: Returns `Role::CREATOR` or `Role::CONSUMER`.

## Usage Example (Conceptual)

```cpp
#include "redis_id_allocator.h"
#include "swss/dbconnector.h" // For DBConnector
#include "swss/logger.h"      // For SWSS_LOG_NOTICE, etc.
#include <iostream>
#include <thread> // For simulating distributed instances

// Assume swss::DBConnector is initialized appropriately
// swss::DBConnector main_db("APPL_DB", 0);
// swss::DBConnector* db_ptr = &main_db; // Or from your application's context

void allocator_thread_func(swss::DBConnector* db, const std::string& allocator_name, int instance_id) {
    RedisIDAllocator::Config cfg;
    cfg.min_id = 1000;
    cfg.max_id = 1999;
    cfg.prefix = "myapp:ids";
    // Other config parameters can be set if needed

    SWSS_LOG_NOTICE("Instance %d: Attempting to create/attach to allocator '%s'", instance_id, allocator_name.c_str());
    std::unique_ptr<RedisIDAllocator> id_alloc = RedisIDAllocator::createOrAttach(db, allocator_name, cfg);

    if (!id_alloc) {
        SWSS_LOG_ERROR("Instance %d: Failed to create/attach allocator '%s'", instance_id, allocator_name.c_str());
        return;
    }
    SWSS_LOG_NOTICE("Instance %d: Role is %s", instance_id, (id_alloc->getRole() == RedisIDAllocator::Role::CREATOR ? "CREATOR" : "CONSUMER"));

    for (int i = 0; i < 5; ++i) {
        std::optional<int> new_id = id_alloc->allocate();
        if (new_id) {
            SWSS_LOG_INFO("Instance %d: Allocated ID: %d", instance_id, *new_id);
        } else {
            SWSS_LOG_WARN("Instance %d: Failed to allocate ID.", instance_id);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100 * instance_id + 50)); // Stagger requests
    }

    // Example: Creator might periodically call cleanup if auto_cleanup is disabled or for specific needs
    if (id_alloc->getRole() == RedisIDAllocator::Role::CREATOR && !cfg.enable_auto_cleanup) {
        SWSS_LOG_NOTICE("Instance %d (Creator): Performing manual cleanup.", instance_id);
        id_alloc->cleanupExpired();
    }

    RedisIDAllocator::Stats stats = id_alloc->getStats();
    SWSS_LOG_NOTICE("Instance %d: Stats - Allocated: %d, Available: %d, Utilization: %.2f%%, Creator Alive: %s",
        instance_id, stats.allocated_count, stats.available_count, stats.utilization_percent, (stats.creator_alive ? "Yes" : "No"));

    // id_alloc will release creator lease in its destructor if it's the creator
}

int main() {
    // Initialize SWSS logger (example)
    swss::Logger::getInstance().setMinPrio(swss::LoggerPrio::LOG_NOTICE);

    // This is a simplified example. In a real SWSS application, DBConnector instances
    // would be managed appropriately. For this demo, we'll use a single one.
    // Ensure Redis is running.
    swss::DBConnector appl_db("APPL_DB", 0, true); // Connect to APPL_DB

    std::string allocator_instance_name = "global_port_ids";

    // Simulate two instances trying to use the same allocator
    std::thread t1(allocator_thread_func, &appl_db, allocator_instance_name, 1);
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Give t1 a chance to become creator
    std::thread t2(allocator_thread_func, &appl_db, allocator_instance_name, 2);

    t1.join();
    t2.join();

    std::cout << "RedisIDAllocator example finished." << std::endl;
    return 0;
}
```

## Dependencies
-   **SWSS Libraries:**
    -   `swss/dbconnector.h` (for `swss::DBConnector`)
    -   `swss/rediscommand.h` (for `swss::RedisCommand`)
    -   `swss/logger.h` (for `SWSS_LOG_...`)
-   Standard C++ libraries: `<string>`, `<optional>`, `<vector>`, `<chrono>`, `<stdexcept>`, `<mutex>`, `<memory>`.

This `RedisIDAllocator` provides a robust, distributed ID management solution tailored for environments using Redis and the SWSS framework. Its use of Lua scripts ensures atomicity and consistency for ID operations across multiple clients.

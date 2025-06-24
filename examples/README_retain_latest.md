# RetainLatest<T> 🚀

A **lock-free, thread-safe** C++17 utility for retaining only the **most recent value** in high-frequency update scenarios. Perfect for event-driven systems, telemetry pipelines, UI updates, and asynchronous data streams where only the latest state matters.

## 🎯 Key Features

- **Latest-value-only retention** - Always holds exactly one value, no unbounded growth
- **Lock-free implementation** - Uses `std::atomic<std::shared_ptr<T>>` for thread safety
- **Zero-copy semantics** - Move semantics and in-place construction support
- **Optional versioning** - Monotonic version tracking for deduplication
- **Callback notifications** - Optional callbacks on value updates
- **Compare-and-swap operations** - Conflict resolution for concurrent updates
- **Header-only** - No external dependencies beyond C++17 standard library

## 🚀 Quick Start

```cpp
#include "retain_latest.hpp"
using namespace retain_latest;

// Basic usage
RetainLatest<std::string> config_buffer;

// Producer thread
config_buffer.update("config_v1");
config_buffer.update("config_v2");  // Overwrites v1
config_buffer.update("config_v3");  // Overwrites v2

// Consumer thread  
if (auto config = config_buffer.consume()) {
    std::cout << *config << std::endl;  // Prints "config_v3"
}
```

## 📋 Use Cases

| Scenario | Description |
|----------|-------------|
| **Telemetry Coalescing** | Sensor emits 10Hz, consumer reads 1Hz — discard stale samples |
| **Config Updates** | Apply only the latest configuration in async systems |
| **UI Redraw Scheduling** | Always render the most current state |
| **State Machine Events** | Replace prior events with more recent ones |
| **Network Reconciliation** | Apply latest control plane updates |

## 🔧 API Reference

### Basic RetainLatest<T>

```cpp
template <typename T>
class RetainLatest {
public:
    // Construction
    RetainLatest();
    
    // Update operations
    void update(const T& value);        // Copy version
    void update(T&& value);             // Move version
    template<typename... Args>
    void emplace(Args&&... args);       // In-place construction
    
    // Consumption
    std::optional<T> consume();         // Atomically take and clear
    std::optional<T> peek() const;      // Read without consuming
    
    // State checking
    bool has_value() const;             // Check if value available
    void clear();                       // Clear stored value
    
    // Notifications
    void on_update(std::function<void(const T&)> callback);
};
```

### Versioned RetainLatest<T>

```cpp
template <typename T>
class VersionedRetainLatest {
public:
    // All basic operations plus:
    
    // Versioned operations
    bool compare_and_update(const T& value, uint64_t expected_version);
    bool is_stale(uint64_t consumer_version) const;
    std::optional<uint64_t> current_version() const;
    
    // Returns Versioned<T> instead of T
    std::optional<Versioned<T>> consume();
    std::optional<Versioned<T>> peek() const;
};

// Versioned wrapper
template <typename T>
struct Versioned {
    T value;
    uint64_t version;
};
```

## 💡 Examples

### 1. High-Frequency Telemetry

```cpp
struct SensorReading {
    double temperature;
    double humidity;
    std::chrono::system_clock::time_point timestamp;
};

RetainLatest<SensorReading> sensor_buffer;

// High-frequency producer (10Hz)
auto sensor_thread = std::thread([&]() {
    while (running) {
        sensor_buffer.emplace(read_temperature(), read_humidity());
        std::this_thread::sleep_for(100ms);
    }
});

// Low-frequency consumer (1Hz)  
auto processor_thread = std::thread([&]() {
    while (running) {
        if (auto reading = sensor_buffer.consume()) {
            process_sensor_data(*reading);
        }
        std::this_thread::sleep_for(1s);
    }
});
```

### 2. Versioned State Management

```cpp
VersionedRetainLatest<GameState> state_buffer;

// Producer with conflict detection
auto current_version = state_buffer.current_version().value_or(0);
GameState new_state = calculate_new_state();

if (state_buffer.compare_and_update(new_state, current_version)) {
    std::cout << "State updated successfully" << std::endl;
} else {
    std::cout << "Conflict detected, retrying..." << std::endl;
}

// Consumer with staleness check
uint64_t my_version = 0;
if (state_buffer.is_stale(my_version)) {
    auto latest = state_buffer.consume();
    my_version = latest->version;
    apply_state(latest->value);
}
```

### 3. UI Update Coalescing

```cpp
RetainLatest<ViewState> ui_buffer;

// Set up notification callback
ui_buffer.on_update([](const ViewState& state) {
    schedule_redraw();  // Schedule async redraw
});

// Rapid state changes
ui_buffer.update(ViewState{items: {"

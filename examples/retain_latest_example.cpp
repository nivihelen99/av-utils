#include "retain_latest.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
#include <cassert>

using namespace retain_latest;
using namespace std::chrono_literals;

// Example 1: Basic usage with config updates
void example_basic_usage() {
    std::cout << "\n=== Example 1: Basic Config Updates ===\n";
    
    RetainLatest<std::string> config_buffer;
    
    // Producer updates config multiple times
    auto producer = std::thread([&config_buffer]() {
        config_buffer.update("config_v1");
        std::this_thread::sleep_for(10ms);
        
        config_buffer.update("config_v2");
        std::this_thread::sleep_for(10ms);
        
        config_buffer.update("config_v3");  // Only this should be consumed
        std::this_thread::sleep_for(10ms);
    });
    
    // Consumer polls for updates
    auto consumer = std::thread([&config_buffer]() {
        std::this_thread::sleep_for(50ms);  // Let producer finish
        
        if (auto config = config_buffer.consume()) {
            std::cout << "Consumer got: " << *config << std::endl;
            // Should print "config_v3" (v1 and v2 were dropped)
        }
    });
    
    producer.join();
    consumer.join();
}

// Example 2: Telemetry coalescing with callbacks
void example_telemetry_coalescing() {
    std::cout << "\n=== Example 2: Telemetry Coalescing ===\n";
    
    struct SensorReading {
        double temperature;
        double humidity;
        std::chrono::system_clock::time_point timestamp;
        
        SensorReading(double t, double h) 
            : temperature(t), humidity(h), timestamp(std::chrono::system_clock::now()) {}
    };
    
    RetainLatest<SensorReading> sensor_buffer;
    
    // Set up callback for monitoring
    sensor_buffer.on_update([](const SensorReading& reading) {
        std::cout << "New reading: " << reading.temperature << "°C, " 
                  << reading.humidity << "% humidity" << std::endl;
    });
    
    // High-frequency sensor updates (10Hz simulation)
    auto sensor = std::thread([&sensor_buffer]() {
        for (int i = 0; i < 10; ++i) {
            sensor_buffer.emplace(20.0 + i * 0.5, 45.0 + i * 0.2);
            std::this_thread::sleep_for(10ms);
        }
    });
    
    // Low-frequency consumer (1Hz simulation)
    auto processor = std::thread([&sensor_buffer]() {
        std::this_thread::sleep_for(150ms);  // Let sensor generate data
        
        if (auto reading = sensor_buffer.consume()) {
            std::cout << "Processor consumed final reading: " 
                      << reading->temperature << "°C" << std::endl;
        }
    });
    
    sensor.join();
    processor.join();
}

// Example 3: Versioned updates with staleness detection
void example_versioned_updates() {
    std::cout << "\n=== Example 3: Versioned Updates ===\n";
    
    VersionedRetainLatest<std::string> versioned_buffer;
    
    // Producer with versioned updates
    auto producer = std::thread([&versioned_buffer]() {
        versioned_buffer.update("state_1");
        std::this_thread::sleep_for(10ms);
        
        versioned_buffer.update("state_2");
        std::this_thread::sleep_for(10ms);
        
        versioned_buffer.update("state_3");
    });
    
    producer.join();
    
    // Consumer checks versions
    if (auto versioned = versioned_buffer.peek()) {
        std::cout << "Current state: " << versioned->value 
                  << " (version " << versioned->version << ")" << std::endl;
    }
    
    // Simulate stale consumer
    uint64_t consumer_version = 0;
    if (versioned_buffer.is_stale(consumer_version)) {
        std::cout << "Consumer is stale (version " << consumer_version << ")" << std::endl;
        
        if (auto latest = versioned_buffer.consume()) {
            std::cout << "Updated to: " << latest->value 
                      << " (version " << latest->version << ")" << std::endl;
        }
    }
}

// Example 4: Compare-and-update for conflict resolution
void example_compare_and_update() {
    std::cout << "\n=== Example 4: Compare-and-Update ===\n";
    
    VersionedRetainLatest<int> counter_buffer;
    
    // Initial value
    counter_buffer.update(100);
    
    auto current_version = counter_buffer.current_version();
    std::cout << "Initial value version: " << *current_version << std::endl;
    
    // Successful compare-and-update
    bool success = counter_buffer.compare_and_update(200, *current_version);
    std::cout << "Update with correct version: " << (success ? "SUCCESS" : "FAILED") << std::endl;
    
    // Failed compare-and-update (stale version)
    success = counter_buffer.compare_and_update(300, *current_version);  // Using old version
    std::cout << "Update with stale version: " << (success ? "SUCCESS" : "FAILED") << std::endl;
    
    if (auto final_value = counter_buffer.peek()) {
        std::cout << "Final value: " << final_value->value 
                  << " (version " << final_value->version << ")" << std::endl;
    }
}

// Example 5: UI update scheduling simulation
void example_ui_updates() {
    std::cout << "\n=== Example 5: UI Update Scheduling ===\n";
    
    struct ViewState {
        std::vector<std::string> items;
        bool dirty;
        
        ViewState(std::vector<std::string> i, bool d = true) 
            : items(std::move(i)), dirty(d) {}
    };
    
    RetainLatest<ViewState> ui_buffer;
    
    // Rapid UI state changes
    auto ui_updater = std::thread([&ui_buffer]() {
        ui_buffer.update(ViewState{{"item1"}});
        std::this_thread::sleep_for(5ms);
        
        ui_buffer.update(ViewState{{"item1", "item2"}});
        std::this_thread::sleep_for(5ms);
        
        ui_buffer.update(ViewState{{"item1", "item2", "item3"}});
        std::this_thread::sleep_for(5ms);
        
        ui_buffer.update(ViewState{{"item1", "item2", "item3", "item4"}});
    });
    
    // Render loop (slower than updates)
    auto renderer = std::thread([&ui_buffer]() {
        std::this_thread::sleep_for(30ms);  // Simulate frame time
        
        if (auto view_state = ui_buffer.consume()) {
            std::cout << "Rendering " << view_state->items.size() << " items: ";
            for (const auto& item : view_state->items) {
                std::cout << item << " ";
            }
            std::cout << std::endl;
        }
    });
    
    ui_updater.join();
    renderer.join();
}

// Basic correctness tests
void run_tests() {
    std::cout << "\n=== Running Tests ===\n";
    
    // Test 1: Basic functionality
    {
        RetainLatest<int> buffer;
        assert(!buffer.has_value());
        assert(!buffer.peek().has_value());
        assert(!buffer.consume().has_value());
        
        buffer.update(42);
        assert(buffer.has_value());
        assert(buffer.peek() == 42);
        
        auto consumed = buffer.consume();
        assert(consumed == 42);
        assert(!buffer.has_value());
        
        std::cout << "✓ Basic functionality test passed\n";
    }
    
    // Test 2: Overwrite behavior
    {
        RetainLatest<std::string> buffer;
        
        buffer.update("first");
        buffer.update("second");
        buffer.update("third");
        
        auto result = buffer.consume();
        assert(result == "third");  // Only latest should remain
        
        std::cout << "✓ Overwrite behavior test passed\n";
    }
    
    // Test 3: Versioned functionality
    {
        VersionedRetainLatest<int> buffer;
        
        buffer.update(100);
        buffer.update(200);
        
        auto versioned = buffer.peek();
        assert(versioned.has_value());
        assert(versioned->value == 200);
        assert(versioned->version == 1);  // Second update
        
        assert(buffer.is_stale(0));
        assert(!buffer.is_stale(1));
        assert(!buffer.is_stale(2));
        
        std::cout << "✓ Versioned functionality test passed\n";
    }
    
    // Test 4: Compare-and-update
    {
        VersionedRetainLatest<int> buffer;
        
        buffer.update(100);
        auto current_ver = buffer.current_version();
        
        // Should succeed
        assert(buffer.compare_and_update(200, *current_ver));
        
        // Should fail (stale version)
        assert(!buffer.compare_and_update(300, *current_ver));
        
        auto final = buffer.peek();
        assert(final->value == 200);
        
        std::cout << "✓ Compare-and-update test passed\n";
    }
    
    std::cout << "All tests passed! ✓\n";
}

int main() {
    std::cout << "RetainLatest<T> Utility Examples\n";
    std::cout << "=================================\n";
    
    run_tests();
    
    example_basic_usage();
    example_telemetry_coalescing();
    example_versioned_updates();
    example_compare_and_update();
    example_ui_updates();
    
    std::cout << "\nAll examples completed!\n";
    return 0;
}

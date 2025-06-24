#include "delta_map.h"
#include <iostream>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <cassert>
#include <chrono>

// Helper function to print map contents
template<typename MapType>
void print_map(const std::string& name, const MapType& m) {
    std::cout << name << ": {";
    bool first = true;
    for (const auto& [key, value] : m) {
        if (!first) std::cout << ", ";
        std::cout << "\"" << key << "\": " << value;
        first = false;
    }
    std::cout << "}\n";
}

// Example with basic integer values
void basic_example() {
    std::cout << "=== Basic Example ===\n";
    
    std::map<std::string, int> old_config = {
        {"timeout", 30},
        {"retries", 3},
        {"port", 8080}
    };
    
    std::map<std::string, int> new_config = {
        {"timeout", 60},      // changed
        {"retries", 3},       // unchanged
        {"host", 1234},       // added
        // port removed
    };
    
    deltamap::DeltaMap delta(old_config, new_config);
    
    print_map("Added", delta.added());
    print_map("Removed", delta.removed());  
    print_map("Changed", delta.changed());
    print_map("Unchanged", delta.unchanged());
    
    std::cout << "Total differences: " << delta.size() << "\n";
    std::cout << "Maps are identical: " << std::boolalpha << delta.empty() << "\n\n";
}

// Example with custom value comparator
struct Config {
    std::string value;
    int priority;
    
    bool operator==(const Config& other) const {
        return value == other.value && priority == other.priority;
    }
};

void custom_comparator_example() {
    std::cout << "=== Custom Comparator Example ===\n";
    
    std::map<std::string, Config> old_map = {
        {"service_a", {"http://old.com", 1}},
        {"service_b", {"http://stable.com", 2}}
    };
    
    std::map<std::string, Config> new_map = {
        {"service_a", {"http://new.com", 1}},  // value changed
        {"service_b", {"http://stable.com", 2}}, // unchanged
        {"service_c", {"http://added.com", 3}}   // added
    };
    
    // Custom comparator that only compares the value field
    auto value_only_equal = [](const Config& a, const Config& b) {
        return a.value == b.value;
    };
    
    deltamap::DeltaMap<std::string, Config, std::map<std::string, Config>, 
                      decltype(value_only_equal)> delta(old_map, new_map, value_only_equal);
    
    std::cout << "Added entries: " << delta.added().size() << "\n";
    std::cout << "Changed entries: " << delta.changed().size() << "\n";
    std::cout << "Unchanged entries: " << delta.unchanged().size() << "\n\n";
}

// Example with unordered_map
void unordered_map_example() {
    std::cout << "=== Unordered Map Example ===\n";
    
    std::unordered_map<int, std::string> route_table_old = {
        {1, "gateway_a"},
        {2, "gateway_b"},
        {3, "gateway_c"}
    };
    
    std::unordered_map<int, std::string> route_table_new = {
        {1, "gateway_a"},     // unchanged
        {2, "gateway_x"},     // changed
        {4, "gateway_d"}      // added
        // route 3 removed
    };
    
    deltamap::DeltaMap delta(route_table_old, route_table_new);
    
    std::cout << "Route changes detected:\n";
    for (const auto& [route, gateway] : delta.added()) {
        std::cout << "  Added route " << route << " -> " << gateway << "\n";
    }
    for (const auto& [route, gateway] : delta.removed()) {
        std::cout << "  Removed route " << route << " -> " << gateway << "\n";
    }
    for (const auto& [route, gateway] : delta.changed()) {
        std::cout << "  Changed route " << route << " -> " << gateway << "\n";
    }
    std::cout << "\n";
}

// Example showing delta application
void delta_application_example() {
    std::cout << "=== Delta Application Example ===\n";
    
    std::map<std::string, int> version1 = {
        {"feature_a", 1},
        {"feature_b", 2}
    };
    
    std::map<std::string, int> version2 = {
        {"feature_a", 1},
        {"feature_b", 3},
        {"feature_c", 1}
    };
    
    deltamap::DeltaMap delta(version1, version2);
    
    // Apply delta to version1 to get version2
    auto reconstructed = delta.apply_to(version1);
    
    std::cout << "Original version1 == reconstructed: " 
              << std::boolalpha << (version2 == reconstructed) << "\n";
    
    // Create inverse delta for rollback
    auto inverse_delta = delta.invert(version1, version2);
    auto rolled_back = inverse_delta.apply_to(version2);
    
    std::cout << "Rollback successful: " 
              << std::boolalpha << (version1 == rolled_back) << "\n\n";
}

// Performance test with larger datasets
void performance_example() {
    std::cout << "=== Performance Example ===\n";
    
    const size_t N = 10000;
    std::map<int, std::string> large_map1, large_map2;
    
    // Generate test data
    for (size_t i = 0; i < N; ++i) {
        large_map1[i] = "value_" + std::to_string(i);
        if (i % 2 == 0) {  // Keep even numbers, modify some
            large_map2[i] = (i % 4 == 0) ? "modified_" + std::to_string(i) : "value_" + std::to_string(i);
        }
        if (i >= N/2) {  // Add second half + some extra
            large_map2[i + N] = "new_value_" + std::to_string(i);
        }
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    deltamap::DeltaMap delta(large_map1, large_map2);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Processed " << N << " entries in " << duration.count() << "ms\n";
    std::cout << "Added: " << delta.added().size() << "\n";
    std::cout << "Removed: " << delta.removed().size() << "\n";
    std::cout << "Changed: " << delta.changed().size() << "\n";
    std::cout << "Unchanged: " << delta.unchanged().size() << "\n\n";
}

// Test individual key queries
void key_query_example() {
    std::cout << "=== Key Query Example ===\n";
    
    std::map<std::string, int> old_state = {{"a", 1}, {"b", 2}, {"c", 3}};
    std::map<std::string, int> new_state = {{"b", 2}, {"c", 30}, {"d", 4}};
    
    deltamap::DeltaMap delta(old_state, new_state);
    
    std::vector<std::string> test_keys = {"a", "b", "c", "d", "e"};
    
    for (const auto& key : test_keys) {
        std::cout << "Key '" << key << "': ";
        if (delta.was_added(key)) std::cout << "ADDED";
        else if (delta.was_removed(key)) std::cout << "REMOVED";
        else if (delta.was_changed(key)) std::cout << "CHANGED";
        else if (delta.was_unchanged(key)) std::cout << "UNCHANGED";
        else std::cout << "NOT_FOUND";
        std::cout << "\n";
    }
    std::cout << "\n";
}

int main() {
    basic_example();
    custom_comparator_example();
    unordered_map_example();
    delta_application_example();
    key_query_example();
    
    // Skip performance example in this demo to keep output manageable
    // performance_example();
    
    std::cout << "All examples completed successfully!\n";
    return 0;
}

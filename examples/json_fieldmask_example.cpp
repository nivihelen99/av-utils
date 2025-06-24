#include "../include/json_fieldmask.h" // Adjusted path
#include <iostream>
#include <iomanip>

// It's good practice to put helper functions and main within a namespace or keep them static
// For this example, we'll keep them as is, similar to the original structure.

// Forward declare FieldMask and json if not fully included by json_fieldmask.h header itself for examples
// However, json_fieldmask.h should provide these.
// using json = nlohmann::json; // This should come from json_fieldmask.h's inclusion of nlohmann/json.hpp
// namespace fieldmask { class FieldMask; } // This should also come from json_fieldmask.h

void print_json(const std::string& label, const json& j) {
    std::cout << label << ":\n" << std::setw(2) << j << "\n\n";
}

void print_mask(const std::string& label, const fieldmask::FieldMask& mask) {
    std::cout << label << ": " << mask.to_string() << "\n\n";
}

int main() {
    std::cout << "=== FieldMask / SparseUpdate Demo ===\n\n";

    // Example 1: Basic configuration diff
    std::cout << "--- Example 1: Basic Configuration Diff ---\n";

    json config_a = {
        {"config", {
            {"hostname", "router1"},
            {"mtu", 1500},
            {"enabled", true},
            {"location", "datacenter-1"}
        }}
    };

    json config_b = {
        {"config", {
            {"hostname", "router2"},        // changed
            {"mtu", 1500},                 // same
            {"enabled", false},            // changed
            {"location", "datacenter-1"},  // same
            {"description", "Main router"} // added
        }}
    };

    print_json("Original config (A)", config_a);
    print_json("Updated config (B)", config_b);

    fieldmask::FieldMask diff_mask = fieldmask::diff_fields(config_a, config_b);
    print_mask("Diff mask", diff_mask);

    // Extract only the changed fields
    json delta = fieldmask::extract_by_mask(config_b, diff_mask);
    print_json("Delta (changed fields only)", delta);

    // Apply the delta to original config
    json config_a_copy = config_a;
    fieldmask::apply_masked_update(config_a_copy, delta, diff_mask); // Note: original example applies delta with diff_mask, which is correct.
                                                               // If delta was sourced from config_b with diff_mask, then applying delta to config_a_copy
                                                               // using diff_mask is the way to transfer those specific changes.
    print_json("Config A after applying delta", config_a_copy);

    // A more direct way to update config_a_copy from config_b using the mask:
    // fieldmask::apply_masked_update(config_a_copy, config_b, diff_mask);
    // print_json("Config A after applying update from B using mask", config_a_copy);


    std::cout << "Configs match after update: " << (config_a_copy == config_b ? "YES" : "NO") << "\n\n";

    // Example 2: Network interfaces (arrays)
    std::cout << "--- Example 2: Network Interfaces (Arrays) ---\n";

    json network_a = {
        {"interfaces", json::array({
            {{"name", "eth0"}, {"enabled", true}, {"mtu", 1500}},
            {{"name", "eth1"}, {"enabled", false}, {"mtu", 1500}}
        })}
    };

    json network_b = {
        {"interfaces", json::array({
            {{"name", "eth0"}, {"enabled", true}, {"mtu", 9000}},    // mtu changed
            {{"name", "eth1"}, {"enabled", true}, {"mtu", 1500}},    // enabled changed
            {{"name", "eth2"}, {"enabled", true}, {"mtu", 1500}}     // new interface
        })}
    };

    print_json("Network A", network_a);
    print_json("Network B", network_b);

    fieldmask::FieldMask network_diff = fieldmask::diff_fields(network_a, network_b);
    print_mask("Network diff mask", network_diff);

    json network_delta = fieldmask::extract_by_mask(network_b, network_diff);
    print_json("Network delta", network_delta);

    // Example 3: Nested configuration with multiple levels
    std::cout << "--- Example 3: Deeply Nested Configuration ---\n";

    json deep_a = {
        {"system", {
            {"logging", {
                {"level", "info"},
                {"targets", {
                    {"console", {{"enabled", true}, {"format", "text"}}},
                    {"file", {{"enabled", false}, {"path", "/var/log/app.log"}}}
                }}
            }},
            {"security", {
                {"authentication", {
                    {"method", "ldap"},
                    {"timeout", 30}
                }}
            }}
        }}
    };

    json deep_b = {
        {"system", {
            {"logging", {
                {"level", "debug"},  // changed
                {"targets", {
                    {"console", {{"enabled", true}, {"format", "json"}}},    // format changed
                    {"file", {{"enabled", true}, {"path", "/var/log/app.log"}}}, // enabled changed
                    {"syslog", {{"enabled", true}, {"server", "log.example.com"}}} // new target
                }}
            }},
            {"security", {
                {"authentication", {
                    {"method", "ldap"},    // same
                    {"timeout", 60}       // changed
                }}
            }}
        }}
    };

    print_json("Deep config A", deep_a);
    print_json("Deep config B", deep_b);

    fieldmask::FieldMask deep_diff = fieldmask::diff_fields(deep_a, deep_b);
    print_mask("Deep diff mask", deep_diff);

    // Example 4: Manual mask creation and application
    std::cout << "--- Example 4: Manual Mask Creation ---\n";

    fieldmask::FieldMask manual_mask;
    manual_mask.add_path("/config/hostname");
    manual_mask.add_path("/config/enabled");

    print_mask("Manual mask", manual_mask);

    json selective_update = {
        {"config", {
            {"hostname", "router-updated"},
            {"enabled", true},
            {"mtu", 9000},  // This won't be applied due to mask
            {"extra", "ignored"}  // This won't be applied due to mask
        }}
    };

    json target = {
        {"config", {
            {"hostname", "old-router"},
            {"enabled", false},
            {"mtu", 1500},
            {"location", "datacenter-1"}
        }}
    };

    print_json("Target before selective update", target);
    print_json("Update source", selective_update);

    fieldmask::apply_masked_update(target, selective_update, manual_mask);
    print_json("Target after selective update", target);

    // Example 5: Pruning redundant paths
    std::cout << "--- Example 5: Pruning Redundant Paths ---\n";

    fieldmask::FieldMask redundant_mask;
    redundant_mask.add_path("/config");
    redundant_mask.add_path("/config/hostname");  // redundant - parent already included
    redundant_mask.add_path("/config/mtu");       // redundant - parent already included
    redundant_mask.add_path("/system/logging");
    redundant_mask.add_path("/system/logging/level"); // redundant - parent already included

    print_mask("Original mask with redundant paths", redundant_mask);

    fieldmask::FieldMask pruned_mask = fieldmask::prune_redundant_paths(redundant_mask);
    print_mask("Pruned mask", pruned_mask);

    // Example 6: Working with edge cases
    std::cout << "--- Example 6: Edge Cases ---\n";

    json edge_a = {
        {"data", json::array({1, 2, 3})},
        {"nullable", nullptr},
        {"boolean", false}
    };

    json edge_b = {
        {"data", json::array({1, 2, 3, 4, 5})},  // array extended
        {"nullable", "now-string"},               // type changed
        {"boolean", true}                        // value changed
    };

    print_json("Edge case A", edge_a);
    print_json("Edge case B", edge_b);

    fieldmask::FieldMask edge_diff = fieldmask::diff_fields(edge_a, edge_b);
    print_mask("Edge case diff", edge_diff);

    std::cout << "=== Demo Complete ===\n";

    return 0;
}

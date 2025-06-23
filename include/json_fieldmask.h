#pragma once

#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

using json = nlohmann::json;

namespace fieldmask {

/**
 * FieldMask represents a set of field paths that should be updated or examined.
 * Paths follow JSON Pointer syntax: /a/b/0/x
 */
class FieldMask {
public:
    std::set<std::string> paths;

    /**
     * Add a path to the mask
     */
    void add_path(const std::string& path) {
        paths.insert(path);
    }

    /**
     * Check if the mask contains a specific path
     */
    bool contains(const std::string& path) const {
        return paths.find(path) != paths.end();
    }

    /**
     * Check if the mask contains any path that starts with the given prefix
     */
    bool contains_prefix(const std::string& prefix) const {
        auto it = paths.lower_bound(prefix);
        return it != paths.end() && it->substr(0, prefix.length()) == prefix;
    }

    /**
     * Get all paths in the mask
     */
    const std::set<std::string>& get_paths() const {
        return paths;
    }

    /**
     * Check if mask is empty
     */
    bool empty() const {
        return paths.empty();
    }

    /**
     * Clear all paths
     */
    void clear() {
        paths.clear();
    }

    /**
     * Merge another mask into this one
     */
    void merge(const FieldMask& other) {
        paths.insert(other.paths.begin(), other.paths.end());
    }

    /**
     * Convert to string representation for debugging
     */
    std::string to_string() const {
        std::ostringstream oss;
        oss << "FieldMask{";
        bool first = true;
        for (const auto& path : paths) {
            if (!first) oss << ", ";
            oss << "\"" << path << "\"";
            first = false;
        }
        oss << "}";
        return oss.str();
    }
};

/**
 * Utility functions for path manipulation
 */
namespace path_utils {
    
    /**
     * Escape a single path component for JSON pointer
     */
    std::string escape_component(const std::string& component) {
        std::string result = component;
        // Replace ~ with ~0 first, then / with ~1 (order matters!)
        size_t pos = 0;
        while ((pos = result.find("~", pos)) != std::string::npos) {
            result.replace(pos, 1, "~0");
            pos += 2;
        }
        pos = 0;
        while ((pos = result.find("/", pos)) != std::string::npos) {
            result.replace(pos, 1, "~1");
            pos += 2;
        }
        return result;
    }

    /**
     * Build a JSON pointer path from components
     */
    std::string build_path(const std::vector<std::string>& components) {
        if (components.empty()) {
            return "";
        }
        
        std::ostringstream oss;
        for (const auto& component : components) {
            oss << "/" << escape_component(component);
        }
        return oss.str();
    }

    /**
     * Split a JSON pointer path into components
     */
    std::vector<std::string> split_path(const std::string& path) {
        std::vector<std::string> components;
        if (path.empty() || path == "/") {
            return components;
        }

        size_t start = 1; // Skip the leading '/'
        size_t pos = path.find('/', start);
        
        while (pos != std::string::npos) {
            components.push_back(path.substr(start, pos - start));
            start = pos + 1;
            pos = path.find('/', start);
        }
        
        if (start < path.length()) {
            components.push_back(path.substr(start));
        }
        
        return components;
    }

    /**
     * Get the parent path of a given path
     */
    std::string get_parent_path(const std::string& path) {
        if (path.empty() || path == "/") {
            return "";
        }
        
        size_t last_slash = path.find_last_of('/');
        if (last_slash == 0) {
            return "/";
        }
        if (last_slash == std::string::npos) {
            return "";
        }
        
        return path.substr(0, last_slash);
    }
}

/**
 * Compare two JSON values and collect differing paths
 */
void collect_diff_paths(const json& a, const json& b, 
                       const std::string& base_path, 
                       FieldMask& mask);

/**
 * Generate a FieldMask describing fields in b that differ from a
 */
FieldMask diff_fields(const json& a, const json& b) {
    FieldMask mask;
    collect_diff_paths(a, b, "", mask);
    return mask;
}

/**
 * Apply only the fields in the mask from src to target
 */
void apply_masked_update(json& target, const json& src, const FieldMask& mask) {
    for (const auto& path : mask.get_paths()) {
        try {
            // Get the value from source using JSON pointer
            json value = src.at(json::json_pointer(path));
            
            // Set the value in target using JSON pointer
            // This will create intermediate objects/arrays as needed
            target[json::json_pointer(path)] = value;
        } catch (const json::exception&) {
            // Path doesn't exist in source or target couldn't be updated
            // Skip this path
            continue;
        }
    }
}

/**
 * Extract a minimal JSON subtree with only fields from the mask
 */
json extract_by_mask(const json& src, const FieldMask& mask) {
    json result;
    
    for (const auto& path : mask.get_paths()) {
        try {
            json value = src.at(json::json_pointer(path));
            result[json::json_pointer(path)] = value;
        } catch (const json::exception&) {
            // Path doesn't exist in source, skip
            continue;
        }
    }
    
    return result;
}

/**
 * Implementation of diff collection (recursive)
 */
void collect_diff_paths(const json& a, const json& b, 
                       const std::string& base_path, 
                       FieldMask& mask) {
    
    // If values are not the same type, the whole path is different
    if (a.type() != b.type()) {
        mask.add_path(base_path.empty() ? "/" : base_path);
        return;
    }
    
    switch (a.type()) {
        case json::value_t::object: {
            // Collect all keys from both objects
            std::set<std::string> all_keys;
            for (auto it = a.begin(); it != a.end(); ++it) {
                all_keys.insert(it.key());
            }
            for (auto it = b.begin(); it != b.end(); ++it) {
                all_keys.insert(it.key());
            }
            
            // Compare each key
            for (const auto& key : all_keys) {
                std::string child_path = base_path + "/" + path_utils::escape_component(key);
                
                auto a_it = a.find(key);
                auto b_it = b.find(key);
                
                if (a_it == a.end()) {
                    // Key only exists in b (added)
                    mask.add_path(child_path);
                } else if (b_it == b.end()) {
                    // Key only exists in a (removed in b)
                    mask.add_path(child_path);
                } else {
                    // Key exists in both, recurse
                    collect_diff_paths(*a_it, *b_it, child_path, mask);
                }
            }
            break;
        }
        
        case json::value_t::array: {
            size_t max_size = std::max(a.size(), b.size());
            
            for (size_t i = 0; i < max_size; ++i) {
                std::string child_path = base_path + "/" + std::to_string(i);
                
                if (i >= a.size()) {
                    // Element only exists in b (added)
                    mask.add_path(child_path);
                } else if (i >= b.size()) {
                    // Element only exists in a (removed in b)
                    mask.add_path(child_path);
                } else {
                    // Element exists in both, recurse
                    collect_diff_paths(a[i], b[i], child_path, mask);
                }
            }
            break;
        }
        
        default: {
            // For primitive types, compare directly
            if (a != b) {
                mask.add_path(base_path.empty() ? "/" : base_path);
            }
            break;
        }
    }
}

/**
 * Prune redundant paths from a mask (remove child paths if parent is included)
 */
FieldMask prune_redundant_paths(const FieldMask& mask) {
    FieldMask result;
    
    for (const auto& path : mask.get_paths()) {
        bool is_redundant = false;
        
        // Check if any parent path is already in the result
        std::string current_path = path;
        while (!current_path.empty() && current_path != "/") {
            std::string parent = path_utils::get_parent_path(current_path);
            if (result.contains(parent)) {
                is_redundant = true;
                break;
            }
            current_path = parent;
        }
        
        if (!is_redundant) {
            result.add_path(path);
        }
    }
    
    return result;
}

/**
 * Create an inverted mask (fields that are the same between a and b)
 */
FieldMask invert_mask(const json& a, const json& b) {
    FieldMask diff = diff_fields(a, b);
    FieldMask all_paths;
    
    // Collect all possible paths from both objects
    std::function<void(const json&, const std::string&)> collect_all_paths = 
        [&](const json& obj, const std::string& base_path) {
            all_paths.add_path(base_path.empty() ? "/" : base_path);
            
            if (obj.is_object()) {
                for (auto it = obj.begin(); it != obj.end(); ++it) {
                    std::string child_path = base_path + "/" + path_utils::escape_component(it.key());
                    collect_all_paths(it.value(), child_path);
                }
            } else if (obj.is_array()) {
                for (size_t i = 0; i < obj.size(); ++i) {
                    std::string child_path = base_path + "/" + std::to_string(i);
                    collect_all_paths(obj[i], child_path);
                }
            }
        };
    
    collect_all_paths(a, "");
    collect_all_paths(b, "");
    
    // Create inverted mask
    FieldMask inverted;
    for (const auto& path : all_paths.get_paths()) {
        if (!diff.contains(path)) {
            inverted.add_path(path);
        }
    }
    
    return inverted;
}

} // namespace fieldmask


#ifdef EXAMPLES
#include "fieldmask.hpp"
#include <iostream>
#include <iomanip>

using namespace fieldmask;

void print_json(const std::string& label, const json& j) {
    std::cout << label << ":\n" << std::setw(2) << j << "\n\n";
}

void print_mask(const std::string& label, const FieldMask& mask) {
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
    
    FieldMask diff_mask = diff_fields(config_a, config_b);
    print_mask("Diff mask", diff_mask);
    
    // Extract only the changed fields
    json delta = extract_by_mask(config_b, diff_mask);
    print_json("Delta (changed fields only)", delta);
    
    // Apply the delta to original config
    json config_a_copy = config_a;
    apply_masked_update(config_a_copy, delta, diff_mask);
    print_json("Config A after applying delta", config_a_copy);
    
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
    
    FieldMask network_diff = diff_fields(network_a, network_b);
    print_mask("Network diff mask", network_diff);
    
    json network_delta = extract_by_mask(network_b, network_diff);
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
    
    FieldMask deep_diff = diff_fields(deep_a, deep_b);
    print_mask("Deep diff mask", deep_diff);
    
    // Example 4: Manual mask creation and application
    std::cout << "--- Example 4: Manual Mask Creation ---\n";
    
    FieldMask manual_mask;
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
    
    apply_masked_update(target, selective_update, manual_mask);
    print_json("Target after selective update", target);
    
    // Example 5: Pruning redundant paths
    std::cout << "--- Example 5: Pruning Redundant Paths ---\n";
    
    FieldMask redundant_mask;
    redundant_mask.add_path("/config");
    redundant_mask.add_path("/config/hostname");  // redundant - parent already included
    redundant_mask.add_path("/config/mtu");       // redundant - parent already included
    redundant_mask.add_path("/system/logging");
    redundant_mask.add_path("/system/logging/level"); // redundant - parent already included
    
    print_mask("Original mask with redundant paths", redundant_mask);
    
    FieldMask pruned_mask = prune_redundant_paths(redundant_mask);
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
    
    FieldMask edge_diff = diff_fields(edge_a, edge_b);
    print_mask("Edge case diff", edge_diff);
    
    std::cout << "=== Demo Complete ===\n";
    
    return 0;
}

#endif

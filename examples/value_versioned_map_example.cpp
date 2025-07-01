#include "value_versioned_map.h"
#include <iostream>
#include <string>
#include <vector>

// Helper function to print std::optional<std::reference_wrapper<const T>>
template <typename T>
std::ostream& operator<<(std::ostream& os, const std::optional<std::reference_wrapper<const T>>& opt_val) {
    if (opt_val) {
        os << opt_val.value().get();
    } else {
        os << "[not found]";
    }
    return os;
}

// Helper function to print std::optional<std::reference_wrapper<T>>
template <typename T>
std::ostream& operator<<(std::ostream& os, const std::optional<std::reference_wrapper<T>>& opt_val) {
    if (opt_val) {
        os << opt_val.value().get();
    } else {
        os << "[not found]";
    }
    return os;
}


int main() {
    // Create a ValueVersionedMap
    // Key: std::string, Value: std::string, Version: uint64_t (default)
    cpp_collections::ValueVersionedMap<std::string, std::string> config_map;

    std::cout << "--- Initializing Config Map ---" << std::endl;

    // Put some initial versions for "database_url"
    config_map.put("database_url", "postgres://user:pass@host1:5432/db", 100);
    config_map.put("database_url", "postgres://user:pass@host2:5432/db", 200); // Failover
    config_map.put("max_connections", "100", 150);

    std::cout << "Current size (number of keys): " << config_map.size() << std::endl;
    std::cout << "Total versions stored: " << config_map.total_versions() << std::endl;

    // --- Retrieving Values ---
    std::cout << "\n--- Retrieving Values ---" << std::endl;

    // Get latest value for "database_url"
    std::cout << "Latest 'database_url': " << config_map.get_latest("database_url") << std::endl;

    // Get "database_url" as of version 100
    std::cout << "'database_url' at version 100: " << config_map.get("database_url", 100) << std::endl;

    // Get "database_url" as of version 150 (should pick up version 100)
    std::cout << "'database_url' at version 150: " << config_map.get("database_url", 150) << std::endl;

    // Get "database_url" as of version 200
    std::cout << "'database_url' at version 200: " << config_map.get("database_url", 200) << std::endl;

    // Get "database_url" as of version 250 (should pick up version 200)
    std::cout << "'database_url' at version 250: " << config_map.get("database_url", 250) << std::endl;

    // Get "database_url" for a version that doesn't exist (too early)
    std::cout << "'database_url' at version 50: " << config_map.get("database_url", 50) << std::endl;

    // Get an exact version
    std::cout << "'database_url' exactly at version 100: " << config_map.get_exact("database_url", 100) << std::endl;
    std::cout << "'database_url' exactly at version 120: " << config_map.get_exact("database_url", 120) << std::endl;


    // Get value for "max_connections"
    std::cout << "Latest 'max_connections': " << config_map.get_latest("max_connections") << std::endl;
    std::cout << "'max_connections' at version 160: " << config_map.get("max_connections", 160) << std::endl;

    // Get value for a non-existent key
    std::cout << "Latest 'timeout_ms': " << config_map.get_latest("timeout_ms") << std::endl;

    // --- Modifying and Adding More Values ---
    std::cout << "\n--- Modifying and Adding More Values ---" << std::endl;
    config_map.put("database_url", "postgres://user:newpass@host2:5432/db", 220); // Password update
    config_map.put("timeout_ms", "5000", 210);

    std::cout << "Latest 'database_url' after update: " << config_map.get_latest("database_url") << std::endl;
    std::cout << "'database_url' at version 215 (before password update): " << config_map.get("database_url", 215) << std::endl;
    std::cout << "Latest 'timeout_ms': " << config_map.get_latest("timeout_ms") << std::endl;

    // --- Checking Existence ---
    std::cout << "\n--- Checking Existence ---" << std::endl;
    std::cout << "Contains key 'database_url'? " << (config_map.contains_key("database_url") ? "Yes" : "No") << std::endl;
    std::cout << "Contains key 'api_key'? " << (config_map.contains_key("api_key") ? "Yes" : "No") << std::endl;
    std::cout << "Contains version 200 for 'database_url'? " << (config_map.contains_version("database_url", 200) ? "Yes" : "No") << std::endl;
    std::cout << "Contains version 180 for 'database_url'? " << (config_map.contains_version("database_url", 180) ? "Yes" : "No") << std::endl;


    // --- Listing Keys and Versions ---
    std::cout << "\n--- Listing Keys and Versions ---" << std::endl;
    std::cout << "All keys in the map:" << std::endl;
    for (const auto& key : config_map.keys()) {
        std::cout << " - " << key << std::endl;
    }

    std::cout << "\nAll versions for 'database_url':" << std::endl;
    if (auto db_versions_opt = config_map.versions("database_url")) {
        for (const auto& version : db_versions_opt.value()) {
            std::cout << " - Version " << version << ": " << config_map.get_exact("database_url", version) << std::endl;
        }
    }

    std::cout << "\nIterating through all key-value (latest) pairs:" << std::endl;
    for(const auto& pair : config_map) { // Iterates key to map_of_versions
        const std::string& key = pair.first;
        // Get latest for this key to demonstrate typical iteration
        auto latest_val = config_map.get_latest(key);
        std::cout << " - Key: " << key << ", Latest Value: " << latest_val << std::endl;
    }


    // --- Removing Values ---
    std::cout << "\n--- Removing Values ---" << std::endl;
    std::cout << "Removing version 100 of 'database_url'..." << std::endl;
    config_map.remove_version("database_url", 100);
    std::cout << "'database_url' at version 100 after removal: " << config_map.get_exact("database_url", 100) << std::endl;
    std::cout << "'database_url' at version 150 after removal (should pick up next available or none): "
              << config_map.get("database_url", 150) << std::endl; // Should be nullopt as 100 was base
                                                                  // Oh, wait, if 200 exists, it should pick that up if 150 < 200.
                                                                  // The logic is correct: get(key, version) finds version <= requested.
                                                                  // If 100 is removed, upper_bound(150) might be 200. prev(200) is what?
                                                                  // If only {200:"v2", 220:"v3"} left. get("db", 150) -> upper_bound(150) is it_200. prev(it_200) is not valid. -> nullopt. Correct.

    std::cout << "Removing key 'max_connections'..." << std::endl;
    config_map.remove_key("max_connections");
    std::cout << "Contains key 'max_connections' after removal? " << (config_map.contains_key("max_connections") ? "Yes" : "No") << std::endl;
    std::cout << "Latest 'max_connections' after removal: " << config_map.get_latest("max_connections") << std::endl;

    // --- Clear the map ---
    std::cout << "\n--- Clearing the map ---" << std::endl;
    config_map.clear();
    std::cout << "Map empty after clear? " << (config_map.empty() ? "Yes" : "No") << std::endl;
    std::cout << "Size after clear: " << config_map.size() << std::endl;


    // Example with custom version type (e.g. struct)
    struct SemanticVersion {
        int major;
        int minor;
        int patch;

        bool operator<(const SemanticVersion& other) const {
            if (major != other.major) return major < other.major;
            if (minor != other.minor) return minor < other.minor;
            return patch < other.patch;
        }
        // Required for std::map if used as key without custom comparator,
        // and for std::hash if used in unordered_map key.
        // Not strictly needed for ValueVersionedMap's VersionT if CompareVersions is provided and works.
        bool operator==(const SemanticVersion& other) const {
            return major == other.major && minor == other.minor && patch == other.patch;
        }
    };
    // Need a hash for SemanticVersion if it were a Key in unordered_map
    // For VersionT, only CompareVersions is strictly needed by ValueVersionedMap's internal std::map
    // Let's make it printable for the example
    std::ostream& operator<<(std::ostream& os, const SemanticVersion& sv) {
        os << sv.major << "." << sv.minor << "." << sv.patch;
        return os;
    }

    std::cout << "\n--- Example with Custom Version Type (SemanticVersion) ---" << std::endl;
    cpp_collections::ValueVersionedMap<std::string, std::string, SemanticVersion> app_settings;

    app_settings.put("feature_flag_x", "enabled", {1, 0, 0});
    app_settings.put("feature_flag_x", "disabled_buggy", {1, 1, 0});
    app_settings.put("feature_flag_x", "enabled_fixed", {1, 1, 5});
    app_settings.put("api_endpoint", "/v1/api", {1, 0, 0});
    app_settings.put("api_endpoint", "/v2/api", {2, 0, 0});

    std::cout << "Latest 'feature_flag_x': " << app_settings.get_latest("feature_flag_x") << std::endl;
    std::cout << "'feature_flag_x' at version {1,0,5}: " << app_settings.get("feature_flag_x", {1,0,5}) << std::endl;
    std::cout << "'feature_flag_x' at version {1,1,2} (should be 'disabled_buggy' from 1.1.0): "
              << app_settings.get("feature_flag_x", {1,1,2}) << std::endl;
    std::cout << "'api_endpoint' at version {1,5,0} (should be '/v1/api' from 1.0.0): "
              << app_settings.get("api_endpoint", {1,5,0}) << std::endl;
    std::cout << "'api_endpoint' at version {0,9,0} (should be not found): "
              << app_settings.get("api_endpoint", {0,9,0}) << std::endl;


    return 0;
}

#include <iostream>
#include <string>
#include <vector> // For a more complex example in Config if desired
#include "../include/shadow_copy.h" // Adjust path as necessary

// A simple configuration structure
struct Config {
    int version;
    std::string user_name;
    std::vector<std::string> feature_flags;

    // For modified() check and printing
    bool operator==(const Config& other) const {
        return version == other.version &&
               user_name == other.user_name &&
               feature_flags == other.feature_flags;
    }

    bool operator!=(const Config& other) const {
        return !(*this == other);
    }

    // For easy printing
    friend std::ostream& operator<<(std::ostream& os, const Config& cfg) {
        os << "Config { version: " << cfg.version
           << ", user_name: \"" << cfg.user_name << "\""
           << ", flags: [";
        for (size_t i = 0; i < cfg.feature_flags.size(); ++i) {
            os << "\"" << cfg.feature_flags[i] << "\"";
            if (i < cfg.feature_flags.size() - 1) {
                os << ", ";
            }
        }
        os << "] }";
        return os;
    }
};

void print_status(const ShadowCopy<Config>& sc, const std::string& context) {
    std::cout << "\n--- " << context << " ---" << std::endl;
    std::cout << "Original: " << sc.original() << std::endl;
    if (sc.has_shadow()) {
        std::cout << "Shadow:   " << sc.current() << std::endl;
    } else {
        std::cout << "Shadow:   <none>" << std::endl;
    }
    std::cout << "Current:  " << sc.current() << std::endl;
    std::cout << "Has Shadow? " << (sc.has_shadow() ? "Yes" : "No") << std::endl;
    std::cout << "Modified?   " << (sc.modified() ? "Yes" : "No") << std::endl;
}

int main() {
    Config initial_config = {1, "default_user", {"flagA", "flagB"}};
    ShadowCopy<Config> shadow_cfg(initial_config);

    print_status(shadow_cfg, "Initial State");

    // 1. Check modified when unchanged
    if (!shadow_cfg.modified()) {
        std::cout << "\nAs expected, config is not modified initially." << std::endl;
    }

    // 2. Call get() and modify a member
    std::cout << "\nCalling get() and modifying user_name..." << std::endl;
    shadow_cfg.get().user_name = "test_user";
    print_status(shadow_cfg, "After modifying user_name");

    if (shadow_cfg.modified()) {
        std::cout << "\nConfig is now modified." << std::endl;
    }

    std::cout << "\nCalling get() again and modifying version and flags..." << std::endl;
    Config& mutable_config = shadow_cfg.get(); // get() returns a reference
    mutable_config.version = 2;
    mutable_config.feature_flags.push_back("flagC");
    print_status(shadow_cfg, "After modifying version and flags");

    // 3. Commit changes
    std::cout << "\nCommitting changes..." << std::endl;
    shadow_cfg.commit();
    print_status(shadow_cfg, "After commit");

    if (!shadow_cfg.modified()) {
        std::cout << "\nConfig is no longer modified after commit." << std::endl;
    }

    // 4. Demonstrate reset
    std::cout << "\nModifying again to demonstrate reset..." << std::endl;
    shadow_cfg.get().user_name = "another_user";
    shadow_cfg.get().feature_flags.clear();
    print_status(shadow_cfg, "After modifying for reset demo");

    std::cout << "\nResetting changes..." << std::endl;
    shadow_cfg.reset();
    print_status(shadow_cfg, "After reset");
    if (!shadow_cfg.modified()) {
        std::cout << "\nConfig is not modified after reset." << std::endl;
    }
    if (shadow_cfg.current().user_name == "test_user" && shadow_cfg.current().version == 2) {
         std::cout << "Config correctly reverted to state after last commit." << std::endl;
    }


    // 5. Demonstrate take
    std::cout << "\nModifying again to demonstrate take..." << std::endl;
    shadow_cfg.get().user_name = "user_for_take";
    shadow_cfg.get().version = 100;
    print_status(shadow_cfg, "After modifying for take demo");

    if (shadow_cfg.has_shadow() && shadow_cfg.modified()) {
        std::cout << "\nTaking the shadow value..." << std::endl;
        Config taken_config = shadow_cfg.take();
        std::cout << "Taken config: " << taken_config << std::endl;
        print_status(shadow_cfg, "After take");
        if (!shadow_cfg.has_shadow() && !shadow_cfg.modified()) {
            std::cout << "ShadowCopy is now clean and has no shadow." << std::endl;
        }
    }
    
    // Example of take when no shadow (should throw, or be guarded)
    std::cout << "\nAttempting to take when no shadow (should be handled by a try-catch if expected):" << std::endl;
    try {
        Config c = shadow_cfg.take(); // No shadow here
        std::cout << "Took value: " << c << " (This should not happen if take throws and no shadow exists)" << std::endl;
    } catch (const std::logic_error& e) {
        std::cout << "Caught expected exception: " << e.what() << std::endl;
    }

    // Demonstrate modified() by get() call only (no value change initially)
    std::cout << "\nDemonstrating modified() by get() call only..." << std::endl;
    Config base_state = { 7, "base", {"base_flag"} };
    ShadowCopy<Config> sc_get_only(base_state);
    print_status(sc_get_only, "Before get() call");
    sc_get_only.get(); // Call get() but don't change the value yet
    print_status(sc_get_only, "After get() call, no value change");
    if (sc_get_only.modified()) {
        std::cout << "Config is modified just by calling get(), as expected." << std::endl;
    } else {
        std::cout << "ERROR: Config should be modified after get() call." << std::endl;
    }
    // Now change value to ensure modified() stays true due to value difference too
    sc_get_only.get().version = 8;
    print_status(sc_get_only, "After get() call and value change");
     if (sc_get_only.modified()) {
        std::cout << "Config is still modified after value change, as expected." << std::endl;
    } else {
        std::cout << "ERROR: Config should be modified after value change." << std::endl;
    }


    std::cout << "\n--- Example Finished ---" << std::endl;

    return 0;
}

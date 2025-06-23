#include "scoped_flag.h"
#include <iostream>
#include <thread>
#include <functional> // Required for std::function

// Global flags for demonstration (defined in the example file)
bool g_logging_enabled = true;
std::atomic_bool g_debug_enabled{true};
thread_local bool g_in_progress = false;
int g_verbosity_level = 2;

namespace utils {
    // Forward declare to allow usage in main or example runner
    namespace examples {
        void run_all_examples();
    }
}

// Using directives for convenience inside the example functions
using utils::ScopedFlag;
template <typename T>
using FlagGuard = utils::FlagGuard<T>; // Alias for FlagGuard
using utils::temporarily_disable;
using utils::temporarily_enable;


namespace utils::examples {

void demonstrate_basic_usage() {
    std::cout << "=== Basic ScopedFlag Usage ===\n";

    std::cout << "Before: logging=" << g_logging_enabled
              << ", debug=" << g_debug_enabled.load() << "\n";

    {
        ScopedFlag log_guard(g_logging_enabled, false);
        ScopedFlag debug_guard(g_debug_enabled, false);

        std::cout << "Inside: logging=" << g_logging_enabled
                  << ", debug=" << g_debug_enabled.load() << "\n";
        std::cout << "Previous logging value was: " << log_guard.previous() << "\n";
    }

    std::cout << "After: logging=" << g_logging_enabled
              << ", debug=" << g_debug_enabled.load() << "\n\n";
}

void demonstrate_generic_guard() {
    std::cout << "=== Generic FlagGuard Usage ===\n";

    std::cout << "Before: verbosity=" << g_verbosity_level << "\n";

    {
        FlagGuard<int> guard(g_verbosity_level, 0);
        std::cout << "Inside: verbosity=" << g_verbosity_level << "\n";
        std::cout << "Previous verbosity was: " << guard.previous() << "\n";
    }

    std::cout << "After: verbosity=" << g_verbosity_level << "\n\n";
}

void demonstrate_convenience_functions() {
    std::cout << "=== Convenience Functions ===\n";

    std::cout << "Before: logging=" << g_logging_enabled << "\n";

    {
        auto guard = temporarily_disable(g_logging_enabled);
        std::cout << "Inside: logging=" << g_logging_enabled << "\n";
    }

    std::cout << "After: logging=" << g_logging_enabled << "\n\n";
}

void demonstrate_exception_safety() {
    std::cout << "=== Exception Safety ===\n";

    std::cout << "Before: logging=" << g_logging_enabled << "\n";

    try {
        ScopedFlag guard(g_logging_enabled, false);
        std::cout << "Inside try: logging=" << g_logging_enabled << "\n";
        throw std::runtime_error("Test exception");
    } catch (const std::exception& e) {
        std::cout << "Caught: " << e.what() << "\n";
    }

    std::cout << "After exception: logging=" << g_logging_enabled << "\n\n";
}

void demonstrate_recursion_protection() {
    std::cout << "=== Recursion Protection ===\n";

    std::function<void(int)> recursive_func = [&](int depth) {
        if (g_in_progress) {
            std::cout << "Recursion detected at depth " << depth << ", skipping\n";
            return;
        }

        // Need to use utils::ScopedFlag here if not bringing ScopedFlag into current namespace
        utils::ScopedFlag guard(g_in_progress, true);
        std::cout << "Processing at depth " << depth << "\n";

        if (depth < 3) {
            recursive_func(depth + 1);
        }
    };

    recursive_func(0);
    std::cout << "\n";
}

void run_all_examples() {
    demonstrate_basic_usage();
    demonstrate_generic_guard();
    demonstrate_convenience_functions();
    demonstrate_exception_safety();
    demonstrate_recursion_protection();
}

} // namespace utils::examples

int main() {
    utils::examples::run_all_examples();
    return 0;
}

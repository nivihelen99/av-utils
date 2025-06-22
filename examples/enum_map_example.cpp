#include "enum_map.h" // Should be corrected to #include <enum_map.h> if it's in an include path
#include <iostream>
#include <string>
#include <functional>
#include <vector> // For a more complex example

// Example 1: State machine
enum class State {
    INIT,
    RUNNING,
    ERROR,
    COUNT // COUNT must be the last enumerator for EnumMap size deduction
};

// Example 2: Opcode dispatch table
enum class Opcode {
    NOP,
    ACK,
    ERR,
    COUNT // COUNT must be the last enumerator
};

void handle_nop() { std::cout << "Handling NOP\n"; }
void handle_ack() { std::cout << "Handling ACK\n"; }
void handle_err() { std::cout << "Handling ERR\n"; }

// Example 3: Mode labels
enum class Mode {
    OFF,
    IDLE,
    ACTIVE,
    COUNT // COUNT must be the last enumerator
};

// Example 4: Using with non-default-constructible types (if EnumMap supports it)
// For this, TValue would need to be explicitly initialized for all enum keys.
// The current EnumMap default constructor now default-initializes, so this example
// would need to be adjusted or use a type that *can* be default-initialized.
struct NonDefaultConstructible {
    int val;
    NonDefaultConstructible(int v) : val(v) {}
    // NonDefaultConstructible() = delete; // To make it truly non-default-constructible
    // For now, let's assume it's default constructible for simplicity with current EnumMap.
    NonDefaultConstructible() : val(0) {}
    friend std::ostream& operator<<(std::ostream& os, const NonDefaultConstructible& ndc) {
        return os << "NDC(" << ndc.val << ")";
    }
};


void example_usage() {
    std::cout << "--- Basic Usage Example ---" << std::endl;
    // State machine example
    EnumMap<State, std::string> state_names = {
        {State::INIT, "Idle"},
        {State::RUNNING, "Running"},
        {State::ERROR, "Fault"}
    };
    // state_names[State::STOPPED] = "Stopped"; // This would fail if STOPPED is not in State or exceeds COUNT

    std::cout << "State at INIT: " << state_names[State::INIT] << "\n";
    state_names[State::INIT] = "Initializing";
    std::cout << "State at INIT (modified): " << state_names[State::INIT] << "\n";
    std::cout << "State at ERROR: " << state_names[State::ERROR] << "\n";

    // Dispatch table example
    std::cout << "\n--- Dispatch Table Example ---" << std::endl;
    EnumMap<Opcode, std::function<void()>> dispatch_table = {
        {Opcode::NOP, handle_nop},
        {Opcode::ACK, handle_ack},
        {Opcode::ERR, handle_err}
    };

    std::cout << "Dispatching ACK: ";
    dispatch_table[Opcode::ACK]();

    // Mode labels example
    std::cout << "\n--- Mode Labels & Iteration Example ---" << std::endl;
    EnumMap<Mode, std::string> mode_labels; // Default constructed
    mode_labels[Mode::OFF] = "Power Off";
    mode_labels[Mode::IDLE] = "Idle";
    mode_labels[Mode::ACTIVE] = "Running";

    // Iteration example (using new key-value iterators)
    std::cout << "All modes (key-value iteration):\n";
    for (const auto& pair : mode_labels) { // Uses new iterator
        // pair.first is TEnum, pair.second is TValue& (or const TValue&)
        // For older compilers or if structured bindings are not supported on proxy:
        // Mode m = pair.first;
        // const std::string& label = pair.second;
        // std::cout << "  Mode " << static_cast<int>(m) << ": " << label << "\n";
        std::cout << "  Mode " << static_cast<int>(pair.first) << ": " << pair.second << "\n";
    }

    std::cout << "All modes (value-only iteration - direct from data):\n";
    for (const auto& label : mode_labels.data()) { // Iterate over the underlying std::array
         std::cout << "  Label: " << label << "\n";
    }

    std::cout << "All modes (value-only iteration - explicit iterators):\n";
    for (auto it = mode_labels.value_begin(); it != mode_labels.value_end(); ++it) {
        const auto& label = *it;
        std::cout << "  Label: " << label << "\n";
    }

    // Bounds checking example
    std::cout << "\n--- Bounds Checking Example ---" << std::endl;
    EnumMap<State, int> counters; // Default values (e.g., 0 for int)
    try {
        counters.at(State::RUNNING) = 42;
        std::cout << "Counter at RUNNING: " << counters.at(State::RUNNING) << "\n";
        // Accessing an out-of-bounds enum (if State had an invalid value not up to COUNT)
        // std::cout << counters.at(static_cast<State>(100)) << "\n"; // Example of what would throw
    } catch (const std::out_of_range& e) {
        std::cout << "Error: " << e.what() << "\n";
    }

    // Fill example
    std::cout << "\n--- Fill Example ---" << std::endl;
    counters.fill(10);
    std::cout << "Counter at INIT after fill: " << counters[State::INIT] << std::endl;
    std::cout << "Counter at RUNNING after fill: " << counters[State::RUNNING] << std::endl;

    // Clear example
    std::cout << "\n--- Clear Example ---" << std::endl;
    counters.clear(); // Resets to default-constructed values (0 for int)
    std::cout << "Counter at INIT after clear: " << counters[State::INIT] << std::endl;
    std::cout << "Counter at RUNNING after clear: " << counters[State::RUNNING] << std::endl;

    state_names.clear(); // Resets to default-constructed strings ("")
    std::cout << "State name at INIT after clear: '" << state_names[State::INIT] << "'" << std::endl;


    // Erase example
    std::cout << "\n--- Erase Example ---" << std::endl;
    mode_labels[Mode::IDLE] = "Temporarily Idle";
    std::cout << "Mode IDLE before erase: " << mode_labels[Mode::IDLE] << std::endl;
    mode_labels.erase(Mode::IDLE); // Resets to default-constructed string ("")
    std::cout << "Mode IDLE after erase: '" << mode_labels[Mode::IDLE] << "'" << std::endl;
    bool erased = mode_labels.erase(static_cast<Mode>(99)); // Try to erase invalid key
    std::cout << "Erasing invalid key result: " << (erased ? "true" : "false") << std::endl;


    // Contains example
    std::cout << "\n--- Contains Example ---" << std::endl;
    if (mode_labels.contains(Mode::ACTIVE)) {
        std::cout << "Map contains Mode::ACTIVE" << std::endl;
    }
    if (!mode_labels.contains(static_cast<Mode>(99))) { // Example with an invalid enum value
        std::cout << "Map does not contain invalid Mode (e.g., 99)" << std::endl;
    }

    // Size and empty
    std::cout << "\n--- Size and Empty Example ---" << std::endl;
    std::cout << "Size of mode_labels: " << mode_labels.size() << std::endl;
    std::cout << "mode_labels is empty? " << (mode_labels.empty() ? "Yes" : "No") << std::endl;

    EnumMap<State, int, 0> empty_map; // Explicitly 0-sized map
    std::cout << "Size of empty_map: " << empty_map.size() << std::endl;
    std::cout << "empty_map is empty? " << (empty_map.empty() ? "Yes" : "No") << std::endl;


    // Example with NonDefaultConstructible (demonstrating default construction requirement for clear/erase)
    std::cout << "\n--- NonDefaultConstructible Example ---" << std::endl;
    EnumMap<Mode, NonDefaultConstructible> ndc_map = {
        {Mode::OFF, NonDefaultConstructible{1}},
        {Mode::IDLE, NonDefaultConstructible{2}}
        // Mode::ACTIVE will be default-constructed by EnumMap constructor if TValue is default constructible
    };
    // If NonDefaultConstructible was truly non-default, the line above would fail to compile
    // or the EnumMap constructor would need to ensure all elements are initialized.
    // Our current EnumMap default constructor initializes elements if TValue is default constructible.

    std::cout << "NDC at OFF: " << ndc_map[Mode::OFF].val << std::endl;
    std::cout << "NDC at IDLE: " << ndc_map[Mode::IDLE].val << std::endl;
    std::cout << "NDC at ACTIVE (defaulted): " << ndc_map[Mode::ACTIVE].val << std::endl;

    ndc_map.clear(); // This requires NonDefaultConstructible to be default constructible
    std::cout << "NDC at IDLE after clear: " << ndc_map[Mode::IDLE].val << std::endl;

    // Test deduction guide
    std::cout << "\n--- Deduction Guide Example ---" << std::endl;
    EnumMap state_colors = { // Uses CTAD
        std::pair{State::INIT, "Blue"},
        std::pair{State::RUNNING, "Green"},
        std::pair{State::ERROR, "Red"}
    };
    std::cout << "Color of State::RUNNING: " << state_colors[State::RUNNING] << std::endl;

    // Const EnumMap
    std::cout << "\n--- Const EnumMap Example ---" << std::endl;
    const EnumMap<Mode, std::string> const_mode_labels = {
        {Mode::OFF,    "Const Power Off"},
        {Mode::IDLE,   "Const Idle"},
        {Mode::ACTIVE, "Const Running"}
    };
    std::cout << "Const Mode IDLE: " << const_mode_labels[Mode::IDLE] << std::endl;
    for(const auto& pair : const_mode_labels) {
        std::cout << "  Const Mode " << static_cast<int>(pair.first) << ": " << pair.second << "\n";
    }
}

int main() {
    example_usage();
    return 0;
}

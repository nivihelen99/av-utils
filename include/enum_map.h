#pragma once

#include <array>
#include <initializer_list>
#include <type_traits>
#include <utility>
#include <stdexcept>

template <typename TEnum, typename TValue, std::size_t N = static_cast<std::size_t>(TEnum::COUNT)>
class EnumMap {
    static_assert(std::is_enum<TEnum>::value, "EnumMap requires an enum type");
    
private:
    std::array<TValue, N> data_;
    
    // Helper to validate enum is within bounds
    static constexpr std::size_t to_index(TEnum e) noexcept {
        return static_cast<std::size_t>(e);
    }
    
    static constexpr bool is_valid_enum(TEnum e) noexcept {
        return to_index(e) < N;
    }

public:
    // Type aliases for STL compatibility
    using key_type = TEnum;
    using mapped_type = TValue;
    using value_type = TValue;
    using size_type = std::size_t;
    using iterator = typename std::array<TValue, N>::iterator;
    using const_iterator = typename std::array<TValue, N>::const_iterator;
    using reference = TValue&;
    using const_reference = const TValue&;

    // Default constructor - default-initializes all values
    EnumMap() = default;
    
    // Initializer list constructor
    EnumMap(std::initializer_list<std::pair<TEnum, TValue>> init) {
        for (const auto& pair : init) {
            (*this)[pair.first] = pair.second;
        }
    }
    
    // Copy constructor
    EnumMap(const EnumMap& other) = default;
    
    // Move constructor
    EnumMap(EnumMap&& other) noexcept = default;
    
    // Copy assignment
    EnumMap& operator=(const EnumMap& other) = default;
    
    // Move assignment
    EnumMap& operator=(EnumMap&& other) noexcept = default;
    
    // Destructor
    ~EnumMap() = default;

    // Element access - non-const version
    TValue& operator[](TEnum key) {
        return data_[to_index(key)];
    }
    
    // Element access - const version
    const TValue& operator[](TEnum key) const {
        return data_[to_index(key)];
    }
    
    // Bounds-checked access - non-const version
    TValue& at(TEnum key) {
        if (!is_valid_enum(key)) {
            throw std::out_of_range("EnumMap::at: enum value out of range");
        }
        return data_[to_index(key)];
    }
    
    // Bounds-checked access - const version
    const TValue& at(TEnum key) const {
        if (!is_valid_enum(key)) {
            throw std::out_of_range("EnumMap::at: enum value out of range");
        }
        return data_[to_index(key)];
    }

    // Iterators
    iterator begin() noexcept {
        return data_.begin();
    }
    
    const_iterator begin() const noexcept {
        return data_.begin();
    }
    
    const_iterator cbegin() const noexcept {
        return data_.cbegin();
    }
    
    iterator end() noexcept {
        return data_.end();
    }
    
    const_iterator end() const noexcept {
        return data_.end();
    }
    
    const_iterator cend() const noexcept {
        return data_.cend();
    }

    // Capacity
    constexpr std::size_t size() const noexcept {
        return N;
    }
    
    constexpr bool empty() const noexcept {
        return N == 0;
    }
    
    constexpr std::size_t max_size() const noexcept {
        return N;
    }

    // Lookup
    bool contains(TEnum key) const noexcept {
        return is_valid_enum(key);
    }

    // Modifiers
    void fill(const TValue& value) {
        data_.fill(value);
    }
    
    void swap(EnumMap& other) noexcept {
        data_.swap(other.data_);
    }

    // Direct access to underlying array (for advanced use cases)
    std::array<TValue, N>& data() noexcept {
        return data_;
    }
    
    const std::array<TValue, N>& data() const noexcept {
        return data_;
    }

    // Equality comparison
    bool operator==(const EnumMap& other) const {
        return data_ == other.data_;
    }
    
    bool operator!=(const EnumMap& other) const {
        return data_ != other.data_;
    }
};

// Deduction guide for C++17 (helps with template argument deduction)
template<typename TEnum, typename TValue>
EnumMap(std::initializer_list<std::pair<TEnum, TValue>>) 
    -> EnumMap<TEnum, TValue, static_cast<std::size_t>(TEnum::COUNT)>;

// Convenience function for swapping
template<typename TEnum, typename TValue, std::size_t N>
void swap(EnumMap<TEnum, TValue, N>& lhs, EnumMap<TEnum, TValue, N>& rhs) noexcept {
    lhs.swap(rhs);
}

// Example usage and test code
#ifdef ENUMMAP_INCLUDE_EXAMPLES

#include <iostream>
#include <string>
#include <functional>

// Example 1: State machine
enum class State {
    INIT,
    RUNNING,
    ERROR,
    COUNT
};

// Example 2: Opcode dispatch table
enum class Opcode {
    NOP,
    ACK,
    ERR,
    COUNT
};

void handle_nop() { std::cout << "Handling NOP\n"; }
void handle_ack() { std::cout << "Handling ACK\n"; }
void handle_err() { std::cout << "Handling ERR\n"; }

// Example 3: Mode labels
enum class Mode { 
    OFF, 
    IDLE, 
    ACTIVE, 
    COUNT 
};

void example_usage() {
    // State machine example
    EnumMap<State, std::string> state_names = {
        {State::INIT, "Idle"},
        {State::RUNNING, "Running"},
        {State::ERROR, "Fault"}
    };
    
    std::cout << "State: " << state_names[State::ERROR] << "\n";
    
    // Dispatch table example
    EnumMap<Opcode, std::function<void()>> dispatch_table = {
        {Opcode::NOP, handle_nop},
        {Opcode::ACK, handle_ack},
        {Opcode::ERR, handle_err}
    };
    
    dispatch_table[Opcode::ACK]();
    
    // Mode labels example
    EnumMap<Mode, std::string> mode_labels = {
        {Mode::OFF,    "Power Off"},
        {Mode::IDLE,   "Idle"},
        {Mode::ACTIVE, "Running"}
    };
    
    // Iteration example
    std::cout << "All modes:\n";
    for (std::size_t i = 0; i < mode_labels.size(); ++i) {
        Mode mode = static_cast<Mode>(i);
        std::cout << "  " << mode_labels[mode] << "\n";
    }
    
    // Range-based for loop (iterates over values)
    std::cout << "State names:\n";
    for (const auto& name : state_names) {
        std::cout << "  " << name << "\n";
    }
    
    // Bounds checking example
    try {
        EnumMap<State, int> counters;
        counters.at(State::RUNNING) = 42;
        std::cout << "Counter: " << counters.at(State::RUNNING) << "\n";
    } catch (const std::out_of_range& e) {
        std::cout << "Error: " << e.what() << "\n";
    }
}

#endif // ENUMMAP_INCLUDE_EXAMPLES

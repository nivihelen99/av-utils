#pragma once

#include <iterator>
#include <optional>
#include <type_traits>
#include <vector>
#include <deque>
#include <iostream>
#include <sstream>
#include <string>
#include <cstdint>

namespace utils {

template <typename Iterator>
class Peekable {
public:
    using ValueType = typename std::iterator_traits<Iterator>::value_type;
    using difference_type = typename std::iterator_traits<Iterator>::difference_type;
    using iterator_category = typename std::iterator_traits<Iterator>::iterator_category;

private:
    Iterator current_;
    Iterator end_;
    mutable std::optional<ValueType> buffer_;
    mutable bool buffer_filled_ = false;

    void fill_buffer() const {
        if (!buffer_filled_ && current_ != end_) {
            buffer_ = *current_;
            buffer_filled_ = true;
        }
    }

public:
    // Constructor
    Peekable(Iterator begin, Iterator end) : current_(begin), end_(end) {}

    // Check if there are more elements
    bool has_next() const {
        fill_buffer();
        return buffer_filled_ && buffer_.has_value();
    }

    // Peek at next element without consuming
    std::optional<ValueType> peek() const {
        fill_buffer();
        return buffer_;
    }

    // Get next element and advance
    std::optional<ValueType> next() {
        fill_buffer();
        if (!buffer_filled_ || !buffer_.has_value()) {
            return std::nullopt;
        }
        
        auto result = std::move(buffer_);
        buffer_.reset();
        buffer_filled_ = false;
        ++current_;
        return result;
    }

    // Consume next element without returning it
    void consume() {
        next();
    }

    // Get underlying iterator position
    Iterator base() const {
        return current_;
    }

    // Iterator-style interface
    ValueType operator*() const {
        auto val = peek();
        return val ? *val : ValueType{};
    }

    Peekable& operator++() {
        consume();
        return *this;
    }

    Peekable operator++(int) {
        auto copy = *this;
        consume();
        return copy;
    }

    bool operator==(const Peekable& other) const {
        return current_ == other.current_;
    }

    bool operator!=(const Peekable& other) const {
        return !(*this == other);
    }

    // Extended features
    
    // Peek ahead by n steps (requires forward iterator or better)
    // This version uses SFINAE to only be available for appropriate iterator types
    template<typename Iter = Iterator>
    std::enable_if_t<
        std::is_base_of_v<std::forward_iterator_tag, 
                         typename std::iterator_traits<Iter>::iterator_category>,
        std::optional<ValueType>
    > peek_n(size_t n) const {
        auto temp_it = current_;
        
        // Skip buffered element if present
        if (buffer_filled_ && buffer_.has_value()) {
            if (n == 0) return buffer_;
            --n;
            ++temp_it;
        }
        
        // Advance n steps
        for (size_t i = 0; i < n && temp_it != end_; ++i, ++temp_it) {}
        
        return (temp_it != end_) ? std::optional<ValueType>(*temp_it) : std::nullopt;
    }
    
    // Check if peek_n is available for this iterator type
    static constexpr bool has_peek_n() {
        return std::is_base_of_v<std::forward_iterator_tag, iterator_category>;
    }
};

// Deduction guide for C++17
template<typename Iterator>
Peekable(Iterator, Iterator) -> Peekable<Iterator>;

// Convenience function to create Peekable from container
template<typename Container>
auto make_peekable(Container& container) {
    return Peekable(container.begin(), container.end());
}

template<typename Container>
auto make_peekable(const Container& container) {
    return Peekable(container.begin(), container.end());
}

// Range wrapper for range-based for loops
template<typename Iterator>
class PeekableRange {
private:
    Peekable<Iterator> peekable_;

public:
    PeekableRange(Iterator begin, Iterator end) : peekable_(begin, end) {}
    
    Peekable<Iterator>& begin() { return peekable_; }
    Peekable<Iterator> end() { 
        return Peekable<Iterator>(peekable_.base(), peekable_.base()); 
    }
};

template<typename Container>
auto peekable_range(Container& container) {
    return PeekableRange(container.begin(), container.end());
}

} // namespace utils

// ============================================================================
// EXAMPLES AND USAGE DEMONSTRATIONS
// ============================================================================

namespace examples {

void basic_usage_example() {
    std::cout << "=== Basic Usage Example ===\n";
    
    std::vector<int> data = {10, 20, 30};
    auto peekable = utils::make_peekable(data);

    while (peekable.has_next()) {
        auto peeked = peekable.peek();
        std::cout << "Next: " << *peeked << "\n";
        
        auto consumed = peekable.next();
        std::cout << "Consumed: " << *consumed << "\n";
    }
    std::cout << "\n";
}

void parser_example() {
    std::cout << "=== Parser Example ===\n";
    
    std::vector<std::string> tokens = {"if", "(", "condition", ")", "{", "body", "}"};
    auto peekable = utils::make_peekable(tokens);
    
    // Simple parser logic
    while (peekable.has_next()) {
        auto next_token = peekable.peek();
        
        if (*next_token == "if") {
            std::cout << "Found IF statement: ";
            peekable.consume(); // consume "if"
            
            if (peekable.has_next() && *peekable.peek() == "(") {
                peekable.consume(); // consume "("
                auto condition = peekable.next(); // get condition
                std::cout << "condition=" << *condition;
                
                if (peekable.has_next() && *peekable.peek() == ")") {
                    peekable.consume(); // consume ")"
                }
            }
            std::cout << "\n";
        } else {
            peekable.consume();
        }
    }
    std::cout << "\n";
}

void streaming_example() {
    std::cout << "=== Streaming Example ===\n";
    
    std::istringstream iss("hello world test");
    std::istream_iterator<std::string> begin(iss);
    std::istream_iterator<std::string> end;
    
    utils::Peekable stream_peekable(begin, end);
    
    while (stream_peekable.has_next()) {
        auto word = stream_peekable.peek();
        std::cout << "About to read: " << *word << "\n";
        
        auto consumed = stream_peekable.next();
        std::cout << "Read: " << *consumed << "\n";
    }
    std::cout << "\n";
}

void peek_ahead_example() {
    std::cout << "=== Peek Ahead Example ===\n";
    
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    auto peekable = utils::make_peekable(numbers);
    
    std::cout << "Iterator supports peek_n: " << peekable.has_peek_n() << "\n";
    
    while (peekable.has_next()) {
        auto current = peekable.peek();
        std::cout << "Current: " << *current;
        
        // Use peek_n only if supported
        if constexpr (decltype(peekable)::has_peek_n()) {
            auto next = peekable.peek_n(1);
            auto next_next = peekable.peek_n(2);
            
            if (next) std::cout << ", Next: " << *next;
            if (next_next) std::cout << ", Next+1: " << *next_next;
        }
        
        std::cout << "\n";
        peekable.consume();
    }
    std::cout << "\n";
}

void protocol_decoder_example() {
    std::cout << "=== Protocol Decoder Example ===\n";
    
    // Simulate protocol: [type_byte][length][data...]
    std::vector<uint8_t> protocol_data = {
        0x01, 0x04, 'H', 'e', 'l', 'l',  // Type 1, length 4, "Hell"
        0x02, 0x02, 'o', '!',            // Type 2, length 2, "o!"
        0x00                             // End marker
    };
    
    auto decoder = utils::make_peekable(protocol_data);
    
    while (decoder.has_next()) {
        auto type_byte = decoder.next();
        if (!type_byte || *type_byte == 0x00) break;
        
        std::cout << "Message type: " << static_cast<int>(*type_byte);
        
        if (decoder.has_next()) {
            auto length = decoder.next();
            std::cout << ", Length: " << static_cast<int>(*length);
            
            std::cout << ", Data: ";
            for (int i = 0; i < *length && decoder.has_next(); ++i) {
                auto data_byte = decoder.next();
                std::cout << static_cast<char>(*data_byte);
            }
            std::cout << "\n";
        }
    }
    std::cout << "\n";
}

void finite_state_machine_example() {
    std::cout << "=== Finite State Machine Example ===\n";
    
    std::string input = "aabbbcc";
    auto fsm = utils::make_peekable(input);
    
    enum State { EXPECT_A, EXPECT_B, EXPECT_C, DONE };
    State state = EXPECT_A;
    
    while (fsm.has_next() && state != DONE) {
        auto current_char = fsm.peek();
        
        switch (state) {
            case EXPECT_A:
                if (*current_char == 'a') {
                    std::cout << "Processing A: " << *current_char;
                    fsm.consume();
                    
                    // Check if we should transition (peek at next if available)
                    if (fsm.has_next()) {
                        auto next_char = fsm.peek();
                        if (*next_char != 'a') {
                            std::cout << " (transition to B state)";
                            state = EXPECT_B;
                        }
                    }
                    std::cout << "\n";
                } else {
                    std::cout << "Unexpected character in A state\n";
                    break;
                }
                break;
                
            case EXPECT_B:
                if (*current_char == 'b') {
                    std::cout << "Processing B: " << *current_char;
                    fsm.consume();
                    
                    // Check if we should transition (peek at next if available)
                    if (fsm.has_next()) {
                        auto next_char = fsm.peek();
                        if (*next_char != 'b') {
                            std::cout << " (transition to C state)";
                            state = EXPECT_C;
                        }
                    }
                    std::cout << "\n";
                } else {
                    std::cout << "Unexpected character in B state\n";
                    break;
                }
                break;
                
            case EXPECT_C:
                if (*current_char == 'c') {
                    std::cout << "Processing C: " << *current_char << "\n";
                    fsm.consume();
                    if (!fsm.has_next()) {
                        state = DONE;
                        std::cout << "FSM completed successfully!\n";
                    }
                } else {
                    std::cout << "Unexpected character in C state\n";
                    break;
                }
                break;
                
            case DONE:
                break;
        }
    }
    std::cout << "\n";
}

void iterator_style_example() {
    std::cout << "=== Iterator Style Example ===\n";
    
    std::vector<std::string> words = {"C++", "is", "awesome"};
    auto peekable = utils::make_peekable(words);
    
    // Using iterator-style operators
    while (peekable.has_next()) {
        std::cout << "Current word: " << *peekable << "\n";
        ++peekable;
    }
    std::cout << "\n";
}

void run_all_examples() {
    std::cout << "Running Peekable<T> Examples\n";
    std::cout << "============================\n\n";
    
    basic_usage_example();
    parser_example();
    streaming_example();
    peek_ahead_example();
    protocol_decoder_example();
    finite_state_machine_example();
    iterator_style_example();
    
    std::cout << "All examples completed!\n";
}

} // namespace examples

/*
// Usage in main.cpp:
int main() {
    examples::run_all_examples();
    return 0;
}
*/

#include "peekable.h" // Assuming peekable.h will be in include directory accessible via include paths
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iterator> // For std::istream_iterator
#include <cstdint>  // For uint8_t

// The examples namespace and its functions will be moved here
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

    // Check if peek_n is available for this iterator type
    // Note: has_peek_n() is a static member, so it's called on the type
    std::cout << "Iterator supports peek_n: "
              << std::boolalpha << decltype(peekable)::has_peek_n() << "\n";

    while (peekable.has_next()) {
        auto current = peekable.peek();
        std::cout << "Current: " << *current;

        // Use peek_n only if supported.
        // The if constexpr is important here for compile-time checking.
        if constexpr (decltype(peekable)::has_peek_n()) {
            auto next = peekable.peek_n(1); // Peek 1 step ahead from current (after buffer)
            auto next_next = peekable.peek_n(2); // Peek 2 steps ahead

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
        auto type_byte_opt = decoder.next(); // Consume type byte
        if (!type_byte_opt || *type_byte_opt == 0x00) break; // Check for end or empty optional
        uint8_t type_byte = *type_byte_opt;

        std::cout << "Message type: " << static_cast<int>(type_byte);

        if (decoder.has_next()) {
            auto length_opt = decoder.next(); // Consume length byte
            if (!length_opt) break; // Should not happen if type was present
            uint8_t length = *length_opt;

            std::cout << ", Length: " << static_cast<int>(length);

            std::cout << ", Data: ";
            for (int i = 0; i < length && decoder.has_next(); ++i) {
                auto data_byte_opt = decoder.next(); // Consume data byte
                if (!data_byte_opt) break; // Should not happen if length is correct
                std::cout << static_cast<char>(*data_byte_opt);
            }
            std::cout << "\n";
        } else {
            std::cout << " (incomplete message - missing length)\n";
            break;
        }
    }
    std::cout << "\n";
}

void finite_state_machine_example() {
    std::cout << "=== Finite State Machine Example ===\n";

    std::string input = "aabbbcc";
    auto fsm = utils::make_peekable(input);

    enum State { EXPECT_A, EXPECT_B, EXPECT_C, DONE, ERROR };
    State state = EXPECT_A;

    while (fsm.has_next() && state != DONE && state != ERROR) {
        auto current_char_opt = fsm.peek();
        if (!current_char_opt) { // Should not happen if fsm.has_next() is true
             state = ERROR;
             std::cout << "Error: has_next true but peek is empty\n";
             break;
        }
        char current_char = *current_char_opt;

        switch (state) {
            case EXPECT_A:
                if (current_char == 'a') {
                    std::cout << "Processing A: " << current_char;
                    fsm.consume();

                    if (fsm.has_next()) {
                        auto next_char_opt = fsm.peek();
                        if (next_char_opt && *next_char_opt != 'a') {
                            std::cout << " (transition to B state)";
                            state = EXPECT_B;
                        }
                    } else { // End of input after 'a's
                        std::cout << " (ends with A)";
                        state = DONE; // Or ERROR depending on FSM logic
                    }
                    std::cout << "\n";
                } else {
                    std::cout << "Unexpected character '" << current_char << "' in A state\n";
                    state = ERROR;
                }
                break;

            case EXPECT_B:
                if (current_char == 'b') {
                    std::cout << "Processing B: " << current_char;
                    fsm.consume();

                    if (fsm.has_next()) {
                        auto next_char_opt = fsm.peek();
                        if (next_char_opt && *next_char_opt != 'b') {
                            std::cout << " (transition to C state)";
                            state = EXPECT_C;
                        }
                    } else { // End of input after 'b's
                         std::cout << " (ends with B)";
                        state = DONE; // Or ERROR
                    }
                    std::cout << "\n";
                } else {
                    std::cout << "Unexpected character '" << current_char << "' in B state\n";
                    state = ERROR;
                }
                break;

            case EXPECT_C:
                if (current_char == 'c') {
                    std::cout << "Processing C: " << current_char << "\n";
                    fsm.consume();
                    if (!fsm.has_next()) {
                        state = DONE;
                        std::cout << "FSM completed successfully!\n";
                    }
                } else {
                    std::cout << "Unexpected character '" << current_char << "' in C state\n";
                    state = ERROR;
                }
                break;
            case DONE: // Should not be reached if loop condition is correct
            case ERROR:
                break;
        }
    }
    if (state == ERROR) {
        std::cout << "FSM ended in ERROR state.\n";
    } else if (state != DONE && !fsm.has_next()) {
        // FSM might end prematurely if input ends before reaching DONE state,
        // e.g. "aa" for FSM expecting "aabbcc"
        std::cout << "FSM ended: Input exhausted before reaching DONE state. Current state: " << state << "\n";
    }
    std::cout << "\n";
}

void iterator_style_example() {
    std::cout << "=== Iterator Style Example ===\n";

    std::vector<std::string> words = {"C++", "is", "awesome"};
    auto peekable = utils::make_peekable(words);

    // Using iterator-style operators
    while (peekable.has_next()) { // Important to check has_next with iterator style too
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

int main() {
    examples::run_all_examples();
    return 0;
}

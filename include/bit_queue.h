#pragma once

#include <deque>
#include <vector>
#include <cstdint>
#include <stdexcept>

namespace cpp_utils {

class BitQueue {
public:
    BitQueue() : num_bits_(0) {}

    // Enqueue a single bit
    void push(bool bit) {
        if (num_bits_ % 8 == 0) {
            buffer_.push_back(0);
        }
        if (bit) {
            buffer_.back() |= (1 << (7 - (num_bits_ % 8)));
        }
        num_bits_++;
    }

    // Enqueue a sequence of bits from a uint64_t value
    void push(uint64_t value, uint8_t count) {
        if (count > 64) {
            throw std::invalid_argument("Count cannot be greater than 64.");
        }
        for (int i = count - 1; i >= 0; --i) {
            push((value >> i) & 1);
        }
    }

    // Dequeue a single bit
    bool pop() {
        if (empty()) {
            throw std::out_of_range("BitQueue is empty.");
        }
        uint8_t byte = buffer_.front();
        bool bit = (byte >> (7 - (write_pos_ % 8))) & 1;
        num_bits_--;
        write_pos_++;
        if (write_pos_ % 8 == 0) {
            buffer_.pop_front();
        }
        return bit;
    }

    // Dequeue a sequence of bits into a uint64_t value
    uint64_t pop(uint8_t count) {
        if (count > 64) {
            throw std::invalid_argument("Count cannot be greater than 64.");
        }
        if (size() < count) {
            throw std::out_of_range("Not enough bits in BitQueue.");
        }
        uint64_t value = 0;
        for (int i = 0; i < count; ++i) {
            value = (value << 1) | pop();
        }
        return value;
    }

    // Peek at the next bit without dequeuing it
    bool front() const {
        if (empty()) {
            throw std::out_of_range("BitQueue is empty.");
        }
        uint8_t byte = buffer_.front();
        return (byte >> (7 - (write_pos_ % 8))) & 1;
    }

    // Returns the number of bits in the queue
    size_t size() const {
        return num_bits_;
    }

    // Checks if the queue is empty
    bool empty() const {
        return num_bits_ == 0;
    }

    // Clears the queue
    void clear() {
        buffer_.clear();
        num_bits_ = 0;
        write_pos_ = 0;
    }

private:
    std::deque<uint8_t> buffer_;
    size_t num_bits_;
    size_t write_pos_ = 0;
};

} // namespace cpp_utils

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
        // For end iterator, create a Peekable that is also at its end.
        // One way is to give it an empty range, e.g. (end, end)
        // Or more directly, if Peekable has a way to represent an "end" state.
        // The current Peekable equality relies on underlying iterators.
        // So an "end" peekable should have its 'current_' iterator at the original end_.
        // This requires a bit more thought if we want a generic end sentinel.
        // For now, this comparison `peekable_ != end_peekable` will work if end_peekable
        // is constructed such that its current_ points to where the iteration should stop.
        // A simple approach: an "end" Peekable is one where current_ == end_.
        // We can construct a Peekable that represents the end of the range.
        return Peekable<Iterator>(peekable_.base(), peekable_.base()); // This creates an empty/ended Peekable
    }
};

template<typename Container>
auto peekable_range(Container& container) {
    return PeekableRange(container.begin(), container.end());
}

} // namespace utils

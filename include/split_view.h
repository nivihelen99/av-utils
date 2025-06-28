#ifndef SPLIT_VIEW_H
#define SPLIT_VIEW_H

#include <string_view>
#include <iterator> // For std::iterator traits

class SplitView {
public:
    // Inner Iterator class
    class Iterator {
    public:
        // Iterator traits
        using iterator_category = std::input_iterator_tag;
        using value_type = std::string_view;
        using difference_type = std::ptrdiff_t;
        using pointer = const std::string_view*;
        using reference = const std::string_view&;

        // Constructor for begin()
        Iterator(std::string_view input, std::string_view delimiter, size_t initial_pos);
        Iterator(std::string_view input, char delimiter, size_t initial_pos);
        // Constructor for end()
        explicit Iterator(std::string_view input, bool is_end_marker);

        reference operator*() const;
        pointer operator->() const;
        Iterator& operator++();
        Iterator operator++(int); // Postfix increment

        bool operator==(const Iterator& other) const;
        bool operator!=(const Iterator& other) const;

    private:
        void advance_to_next_token(); // Renamed and declared

        std::string_view input_;
        std::string_view delimiter_;
        char single_char_delimiter_ = '\0'; // Used if delimiter is a single char
        bool use_single_char_delimiter_ = false;

        std::string_view current_token_;
        size_t current_pos_ = 0; // Current position in the input string_view
        bool is_end_ = false;
    };

    // Constructors for SplitView
    SplitView(std::string_view input, char delimiter);
    SplitView(std::string_view input, std::string_view delimiter);

    Iterator begin() const;
    Iterator end() const;

    std::string_view get_input() const { return input_; } // Public getter for input_

private:
    std::string_view input_;
    std::string_view delimiter_;
    char single_char_delimiter_ = '\0';
    bool use_single_char_delimiter_ = false;
};

// Implementation of Iterator methods

// Constructor for general use (called by begin())
SplitView::Iterator::Iterator(std::string_view input, std::string_view delimiter, size_t initial_pos)
    : input_(input), delimiter_(delimiter), current_pos_(initial_pos), is_end_(false) {
    use_single_char_delimiter_ = false;
    advance_to_next_token(); // Find the first token
}

// Constructor for char delimiter (called by begin())
SplitView::Iterator::Iterator(std::string_view input, char delimiter, size_t initial_pos)
    : input_(input), single_char_delimiter_(delimiter), current_pos_(initial_pos), is_end_(false) {
    use_single_char_delimiter_ = true;
    advance_to_next_token(); // Find the first token
}

// Constructor for the end() iterator
SplitView::Iterator::Iterator(std::string_view input, bool is_end_marker)
    : input_(input), current_pos_(input.length()), is_end_(is_end_marker) {
    // For end iterator, delimiter and use_single_char_delimiter_ don't matter for comparison
    // current_token_ should be empty for end iterator
    current_token_ = std::string_view();
}

const std::string_view& SplitView::Iterator::operator*() const {
    // TODO: Add assertion !is_end_ if desired, for safety
    return current_token_;
}

const std::string_view* SplitView::Iterator::operator->() const {
    // TODO: Add assertion !is_end_ if desired
    return &current_token_;
}

SplitView::Iterator& SplitView::Iterator::operator++() {
    if (!is_end_) { // Only advance if not already at end
        advance_to_next_token();
    }
    return *this;
}

SplitView::Iterator SplitView::Iterator::operator++(int) {
    Iterator temp = *this;
    ++(*this);
    return temp;
}

bool SplitView::Iterator::operator==(const Iterator& other) const {
    // Both iterators are end iterators
    if (is_end_ && other.is_end_) {
        // Optional: check if they originate from the same SplitView instance if that's a strict requirement
        // For now, if both are 'is_end_', they are considered equal for loop termination.
        return true;
    }
    // One is end, the other is not
    if (is_end_ || other.is_end_) {
        return false;
    }
    // Both are valid, non-end iterators. Compare their state.
    // They must point to the same input string, be at the same position,
    // and thus have the same current token.
    return input_.data() == other.input_.data() &&
           input_.length() == other.input_.length() &&
           current_pos_ == other.current_pos_ && // current_pos_ here refers to the start of the current_token_
           current_token_.data() == other.current_token_.data() && // Compare actual token pointers
           current_token_.length() == other.current_token_.length();
}

bool SplitView::Iterator::operator!=(const Iterator& other) const {
    return !(*this == other);
}

// Renamed from find_next_token and logic significantly revised
void SplitView::Iterator::advance_to_next_token() {
    // If current_pos_ has moved past the input string length, this iterator becomes an end iterator.
    if (current_pos_ > input_.length()) {
        is_end_ = true;
        current_token_ = std::string_view();
        return;
    }

    // Handle empty delimiter: As per Requirement.md, yields the whole string as one token,
    // or one empty token if input is empty.
    size_t delimiter_len = use_single_char_delimiter_ ? 1 : delimiter_.length();

    if (delimiter_len == 0) {
        // If current_pos_ is 0, it means we haven't yielded the "whole string" token yet.
        if (current_pos_ == 0) {
            current_token_ = input_; // The whole input string or empty if input is empty
            current_pos_ = input_.length() + 1; // Advance past end, next call to advance_to_next_token will set is_end_
            is_end_ = false; // We found a token.
        } else {
            // We've already yielded the token for an empty delimiter, so now it's the end.
            is_end_ = true;
            current_token_ = std::string_view();
        }
        return;
    }

    // Regular delimiter search
    size_t next_delimiter_pos;
    if (use_single_char_delimiter_) {
        next_delimiter_pos = input_.find(single_char_delimiter_, current_pos_);
    } else {
        next_delimiter_pos = input_.find(delimiter_, current_pos_);
    }

    if (next_delimiter_pos == std::string_view::npos) {
        // No more delimiters found.
        // The token is from current_pos_ to the end of the input string.
        if (current_pos_ <= input_.length()) {
            current_token_ = input_.substr(current_pos_);
            current_pos_ = input_.length() + 1; // Advance past the end for the next ++ call
            is_end_ = false; // We found a token (it might be empty).
        } else {
            // current_pos_ > input_.length(), already past the end.
            is_end_ = true;
            current_token_ = std::string_view();
        }
    } else {
        // Delimiter found. Token is from current_pos_ up to the delimiter.
        current_token_ = input_.substr(current_pos_, next_delimiter_pos - current_pos_);
        current_pos_ = next_delimiter_pos + delimiter_len; // Advance current_pos_ to start after the delimiter
        is_end_ = false; // We found a token.
    }
}

// Implementation of SplitView methods

SplitView::SplitView(std::string_view input, char delimiter)
    : input_(input), single_char_delimiter_(delimiter), use_single_char_delimiter_(true) {
}

SplitView::SplitView(std::string_view input, std::string_view delimiter)
    : input_(input), delimiter_(delimiter), use_single_char_delimiter_(false) {
}

SplitView::Iterator SplitView::begin() const {
    if (use_single_char_delimiter_) {
        return Iterator(input_, single_char_delimiter_, 0);
    } else {
        return Iterator(input_, delimiter_, 0);
    }
}

SplitView::Iterator SplitView::end() const {
    return Iterator(input_, true /* is_end_marker */);
}

#endif // SPLIT_VIEW_H

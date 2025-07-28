#pragma once

#include <vector>
#include <stdexcept>
#include <utility>

namespace cpp_collections {

template <typename T>
class History {
public:
    using value_type = T;
    using size_type = typename std::vector<T>::size_type;

private:
    std::vector<T> history_;

public:
    // Default constructor
    History() {
        history_.emplace_back();
    }

    // Constructor with an initial value
    explicit History(T initial_value) {
        history_.push_back(std::move(initial_value));
    }

    // Commit the current state, creating a new version
    void commit() {
        history_.push_back(history_.back());
    }

    // Get a const reference to a specific version
    const T& get(size_type version) const {
        if (version >= history_.size()) {
            throw std::out_of_range("Version not found");
        }
        return history_[version];
    }

    // Get a reference to the latest version for modification
    T& latest() {
        return history_.back();
    }

    // Get a const reference to the latest version
    const T& latest() const {
        return history_.back();
    }

    // Revert to a specific version, creating a new version with the old state
    void revert(size_type version) {
        if (version >= history_.size()) {
            throw std::out_of_range("Version not found");
        }
        history_.push_back(history_[version]);
    }

    // Get the number of versions
    size_type versions() const noexcept {
        return history_.size();
    }

    // Get the current version number
    size_type current_version() const noexcept {
        return history_.size() - 1;
    }

    // Clear the history, keeping only the latest version as the initial state
    void clear() {
        T latest_state = std::move(history_.back());
        history_.clear();
        history_.push_back(std::move(latest_state));
    }

    // Clear the history and reset to a default-constructed T
    void reset() {
        history_.clear();
        history_.emplace_back();
    }
};

} // namespace cpp_collections

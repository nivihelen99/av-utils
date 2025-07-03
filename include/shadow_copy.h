#ifndef SHADOW_COPY_H
#define SHADOW_COPY_H

#include <optional> // For std::optional
#include <utility>  // For std::move
#include <stdexcept> // For std::runtime_error (potentially for take())
#include <type_traits> // For std::is_copy_constructible_v, static_assert

template <typename T>
class ShadowCopy {
public:
    // Constructor from const reference
    ShadowCopy(const T& value)
        : original_(value), shadow_(), get_called_(false) {}

    // Constructor from rvalue reference
    ShadowCopy(T&& value)
        : original_(std::move(value)), shadow_(), get_called_(false) {}

    // Copy and Move Semantics for ShadowCopy<T>
    ShadowCopy(const ShadowCopy&) = default;
    ShadowCopy(ShadowCopy&&) = default;
    ShadowCopy& operator=(const ShadowCopy&) = default;
    ShadowCopy& operator=(ShadowCopy&&) = default;

public:
    // Core Accessors
    const T& original() const {
        return original_;
    }

    bool has_shadow() const {
        return shadow_.has_value();
    }

    const T& current() const {
        if (has_shadow()) {
            return *shadow_;
        }
        return original_;
    }

    // Mutator
    T& get() {
        get_called_ = true;
        if (!shadow_) {
            // To create a shadow from original_, T must be copy-constructible.
            static_assert(std::is_copy_constructible_v<T>, 
                          "ShadowCopy<T>::get() requires T to be copy-constructible to create a shadow from original.");
            shadow_.emplace(original_); // Create shadow by copying original
        }
        return *shadow_;
    }

    // State Management Methods
    bool modified() const {
        // Modified if get() was called, or if a shadow exists and differs from the original.
        // Assumes T supports operator!= or operator==.
        if (get_called_) {
            return true;
        }
        if (has_shadow()) {
            // If T doesn't have operator!=, this part might need SFINAE/concepts
            // or a specific trait to check for equality.
            // For now, we assume T is comparable.
            return original_ != *shadow_;
        }
        return false;
    }

    void reset() {
        shadow_.reset();
        get_called_ = false;
    }

    void commit() {
        if (has_shadow()) {
            original_ = std::move(*shadow_); // Move shadow content to original
                                           // If T is not movable, this will copy.
            shadow_.reset();               // Clear the shadow
        }
        get_called_ = false; // After commit, it's as if get() hasn't been called on the new state
    }

    // Moves out the shadow value, clearing the shadow.
    // Returns the value by value (moved).
    // Throws std::logic_error if there is no shadow.
    T take() {
        if (!has_shadow()) {
            throw std::logic_error("No shadow value to take from ShadowCopy.");
        }
        
        T value_to_return = std::move(*shadow_); // Move content from shadow
        shadow_.reset();                         // Clear the shadow
        get_called_ = false;
        return value_to_return; // Return by value (relies on RVO/move semantics of T)
    }

private:
    T original_;
    std::optional<T> shadow_;
    bool get_called_;
};

#endif // SHADOW_COPY_H

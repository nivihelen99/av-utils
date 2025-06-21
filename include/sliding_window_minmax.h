#pragma once

#include <deque>
#include <stdexcept>
#include <type_traits>
#include <chrono>

namespace sliding_window {

/**
 * @brief Sliding Window Minimum - Efficient O(1) minimum tracking over a fixed-size window
 * 
 * Uses a monotonic deque to maintain minimum values efficiently.
 * Supports push/pop operations with O(1) amortized time complexity.
 * 
 * @tparam T Element type - must be copyable/movable and comparable with <
 */
template<typename T>
class SlidingWindowMin {
    static_assert(std::is_copy_constructible_v<T> || std::is_move_constructible_v<T>, 
                  "T must be copyable or movable");
    static_assert(std::is_default_constructible_v<T>, 
                  "T must be default constructible");

private:
    size_t capacity_;
    std::deque<T> data_;        // Actual window elements (FIFO)
    std::deque<T> mono_;        // Monotonic deque for min tracking (non-decreasing)

public:
    /**
     * @brief Construct a sliding window with specified capacity
     * @param capacity Maximum number of elements in the window
     * @throws std::invalid_argument if capacity is 0
     */
    explicit SlidingWindowMin(size_t capacity) : capacity_(capacity) {
        if (capacity == 0) {
            throw std::invalid_argument("SlidingWindowMin capacity must be > 0");
        }
    }

    /**
     * @brief Add a new element to the window
     * 
     * If the window is at capacity, the oldest element is automatically removed.
     * Maintains the monotonic property of the internal deque.
     * 
     * @param value Element to add
     */
    void push(const T& value) {
        // If at capacity, remove oldest element
        if (data_.size() == capacity_) {
            pop();
        }

        // Add to main data
        data_.push_back(value);

        // Maintain monotonic property: remove elements from back that are >= value
        while (!mono_.empty() && mono_.back() >= value) {
            mono_.pop_back();
        }
        mono_.push_back(value);
    }

    /**
     * @brief Add a new element to the window (move version)
     */
    void push(T&& value) {
        if (data_.size() == capacity_) {
            pop();
        }

        // For monotonic deque, we need to compare before moving
        T value_copy = value;  // Make a copy for comparison
        data_.push_back(std::move(value));

        while (!mono_.empty() && mono_.back() >= value_copy) {
            mono_.pop_back();
        }
        mono_.push_back(std::move(value_copy));
    }

    /**
     * @brief Remove the oldest element from the window
     * @throws std::runtime_error if window is empty
     */
    void pop() {
        if (empty()) {
            throw std::runtime_error("Cannot pop from empty SlidingWindowMin");
        }

        T front_value = data_.front();
        data_.pop_front();

        // If the removed element was the current minimum, remove it from mono
        if (!mono_.empty() && mono_.front() == front_value) {
            mono_.pop_front();
        }
    }

    /**
     * @brief Get the current minimum value in the window
     * @return Reference to the minimum element
     * @throws std::runtime_error if window is empty
     */
    const T& min() const {
        if (empty()) {
            throw std::runtime_error("Cannot get min from empty SlidingWindowMin");
        }
        return mono_.front();
    }

    /**
     * @brief Get the number of elements currently in the window
     */
    size_t size() const noexcept {
        return data_.size();
    }

    /**
     * @brief Check if the window is empty
     */
    bool empty() const noexcept {
        return data_.empty();
    }

    /**
     * @brief Get the maximum capacity of the window
     */
    size_t capacity() const noexcept {
        return capacity_;
    }

    /**
     * @brief Clear all elements from the window
     */
    void clear() noexcept {
        data_.clear();
        mono_.clear();
    }

    /**
     * @brief Check if the window is at full capacity
     */
    bool full() const noexcept {
        return data_.size() == capacity_;
    }
};

/**
 * @brief Sliding Window Maximum - Efficient O(1) maximum tracking over a fixed-size window
 * 
 * Uses a monotonic deque to maintain maximum values efficiently.
 * Supports push/pop operations with O(1) amortized time complexity.
 * 
 * @tparam T Element type - must be copyable/movable and comparable with >
 */
template<typename T>
class SlidingWindowMax {
    static_assert(std::is_copy_constructible_v<T> || std::is_move_constructible_v<T>, 
                  "T must be copyable or movable");
    static_assert(std::is_default_constructible_v<T>, 
                  "T must be default constructible");

private:
    size_t capacity_;
    std::deque<T> data_;        // Actual window elements (FIFO)
    std::deque<T> mono_;        // Monotonic deque for max tracking (non-increasing)

public:
    /**
     * @brief Construct a sliding window with specified capacity
     * @param capacity Maximum number of elements in the window
     * @throws std::invalid_argument if capacity is 0
     */
    explicit SlidingWindowMax(size_t capacity) : capacity_(capacity) {
        if (capacity == 0) {
            throw std::invalid_argument("SlidingWindowMax capacity must be > 0");
        }
    }

    /**
     * @brief Add a new element to the window
     * 
     * If the window is at capacity, the oldest element is automatically removed.
     * Maintains the monotonic property of the internal deque.
     * 
     * @param value Element to add
     */
    void push(const T& value) {
        // If at capacity, remove oldest element
        if (data_.size() == capacity_) {
            pop();
        }

        // Add to main data
        data_.push_back(value);

        // Maintain monotonic property: remove elements from back that are <= value
        while (!mono_.empty() && mono_.back() <= value) {
            mono_.pop_back();
        }
        mono_.push_back(value);
    }

    /**
     * @brief Add a new element to the window (move version)
     */
    void push(T&& value) {
        if (data_.size() == capacity_) {
            pop();
        }

        // For monotonic deque, we need to compare before moving
        T value_copy = value;  // Make a copy for comparison
        data_.push_back(std::move(value));

        while (!mono_.empty() && mono_.back() <= value_copy) {
            mono_.pop_back();
        }
        mono_.push_back(std::move(value_copy));
    }

    /**
     * @brief Remove the oldest element from the window
     * @throws std::runtime_error if window is empty
     */
    void pop() {
        if (empty()) {
            throw std::runtime_error("Cannot pop from empty SlidingWindowMax");
        }

        T front_value = data_.front();
        data_.pop_front();

        // If the removed element was the current maximum, remove it from mono
        if (!mono_.empty() && mono_.front() == front_value) {
            mono_.pop_front();
        }
    }

    /**
     * @brief Get the current maximum value in the window
     * @return Reference to the maximum element
     * @throws std::runtime_error if window is empty
     */
    const T& max() const {
        if (empty()) {
            throw std::runtime_error("Cannot get max from empty SlidingWindowMax");
        }
        return mono_.front();
    }

    /**
     * @brief Get the number of elements currently in the window
     */
    size_t size() const noexcept {
        return data_.size();
    }

    /**
     * @brief Check if the window is empty
     */
    bool empty() const noexcept {
        return data_.empty();
    }

    /**
     * @brief Get the maximum capacity of the window
     */
    size_t capacity() const noexcept {
        return capacity_;
    }

    /**
     * @brief Clear all elements from the window
     */
    void clear() noexcept {
        data_.clear();
        mono_.clear();
    }

    /**
     * @brief Check if the window is at full capacity
     */
    bool full() const noexcept {
        return data_.size() == capacity_;
    }
};

/**
 * @brief Generic sliding window with custom comparator
 * 
 * Allows for custom comparison logic for specialized use cases.
 * 
 * @tparam T Element type
 * @tparam Compare Comparison function type
 */
template<typename T, typename Compare = std::less<T>>
class SlidingWindow {
    static_assert(std::is_copy_constructible_v<T> || std::is_move_constructible_v<T>, 
                  "T must be copyable or movable");

private:
    size_t capacity_;
    std::deque<T> data_;
    std::deque<T> mono_;
    Compare comp_;

public:
    explicit SlidingWindow(size_t capacity, Compare comp = Compare{}) 
        : capacity_(capacity), comp_(comp) {
        if (capacity == 0) {
            throw std::invalid_argument("SlidingWindow capacity must be > 0");
        }
    }

    void push(const T& value) {
        if (data_.size() == capacity_) {
            pop();
        }

        data_.push_back(value);

        // For min (std::less): remove elements >= value
        // For max (std::greater): remove elements <= value
        while (!mono_.empty() && !comp_(mono_.back(), value)) {
            mono_.pop_back();
        }
        mono_.push_back(value);
    }

    void pop() {
        if (empty()) {
            throw std::runtime_error("Cannot pop from empty SlidingWindow");
        }

        T front_value = data_.front();
        data_.pop_front();

        if (!mono_.empty() && mono_.front() == front_value) {
            mono_.pop_front();
        }
    }

    const T& extreme() const {
        if (empty()) {
            throw std::runtime_error("Cannot get extreme from empty SlidingWindow");
        }
        return mono_.front();
    }

    size_t size() const noexcept { return data_.size(); }
    bool empty() const noexcept { return data_.empty(); }
    size_t capacity() const noexcept { return capacity_; }
    void clear() noexcept { data_.clear(); mono_.clear(); }
    bool full() const noexcept { return data_.size() == capacity_; }
};

} // namespace sliding_window

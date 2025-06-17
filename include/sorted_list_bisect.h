#pragma once

#include <vector>
#include <algorithm>
#include <functional>
#include <stdexcept>

/**
 * @brief A sorted list container that maintains elements in sorted order
 * 
 * SortedList provides a dynamically sorted sequence container with efficient
 * binary search operations. Elements are automatically kept in sorted order
 * upon insertion, and duplicates are allowed.
 * 
 * @tparam T The element type
 * @tparam Compare The comparison function object type (defaults to std::less<T>)
 */
template <typename T, typename Compare = std::less<T>>
class SortedList {
public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = const T&;
    using const_reference = const T&;
    using iterator = typename std::vector<T>::const_iterator;
    using const_iterator = typename std::vector<T>::const_iterator;
    using reverse_iterator = typename std::vector<T>::const_reverse_iterator;
    using const_reverse_iterator = typename std::vector<T>::const_reverse_iterator;

    /**
     * @brief Default constructor
     */
    SortedList() = default;

    /**
     * @brief Constructor with custom comparator
     * @param comp The comparison function object
     */
    explicit SortedList(const Compare& comp) : comp_(comp) {}

    /**
     * @brief Constructor with initializer list
     * @param init Initializer list of elements
     * @param comp The comparison function object
     */
    SortedList(std::initializer_list<T> init, const Compare& comp = Compare()) 
        : comp_(comp) {
        for (const auto& item : init) {
            insert(item);
        }
    }

    // Capacity
    
    /**
     * @brief Returns the number of elements
     * @return Number of elements in the container
     */
    size_type size() const noexcept {
        return data_.size();
    }

    /**
     * @brief Checks whether the container is empty
     * @return true if the container is empty, false otherwise
     */
    bool empty() const noexcept {
        return data_.empty();
    }

    /**
     * @brief Clears the contents
     */
    void clear() noexcept {
        data_.clear();
    }

    // Element access

    /**
     * @brief Access element at specified position with bounds checking
     * @param index Position of the element
     * @return Reference to the element at specified position
     * @throws std::out_of_range if index >= size()
     */
    const_reference at(size_type index) const {
        if (index >= size()) {
            throw std::out_of_range("SortedList::at: index out of range");
        }
        return data_[index];
    }

    /**
     * @brief Access element at specified position
     * @param index Position of the element
     * @return Reference to the element at specified position
     * @note No bounds checking is performed
     */
    const_reference operator[](size_type index) const noexcept {
        return data_[index];
    }

    /**
     * @brief Find the index of the first occurrence of a value
     * @param value The value to search for
     * @return Index of the first occurrence
     * @throws std::runtime_error if value is not found
     */
    size_type index_of(const T& value) const {
        auto it = std::lower_bound(data_.begin(), data_.end(), value, comp_);
        if (it != data_.end() && !comp_(value, *it)) {
            return std::distance(data_.begin(), it);
        }
        throw std::runtime_error("SortedList::index_of: value not found");
    }

    // Modifiers

    /**
     * @brief Insert an element while maintaining sorted order
     * @param value The value to insert
     */
    void insert(const T& value) {
        auto it = std::lower_bound(data_.begin(), data_.end(), value, comp_);
        data_.insert(it, value);
    }

    /**
     * @brief Insert an element while maintaining sorted order (move version)
     * @param value The value to insert
     */
    void insert(T&& value) {
        auto it = std::lower_bound(data_.begin(), data_.end(), value, comp_);
        data_.insert(it, std::move(value));
    }

    /**
     * @brief Remove the first occurrence of a value
     * @param value The value to remove
     * @return true if an element was removed, false if not found
     */
    bool erase(const T& value) {
        auto it = std::lower_bound(data_.begin(), data_.end(), value, comp_);
        if (it != data_.end() && !comp_(value, *it)) {
            data_.erase(it);
            return true;
        }
        return false;
    }

    /**
     * @brief Remove element at specified position
     * @param index Position of the element to remove
     * @throws std::out_of_range if index >= size()
     */
    void erase_at(size_type index) {
        if (index >= size()) {
            throw std::out_of_range("SortedList::erase_at: index out of range");
        }
        data_.erase(data_.begin() + index);
    }

    template <typename... Args>
    iterator emplace(Args&&... args) {
        T value_to_insert(std::forward<Args>(args)...);
        auto it_pos_non_const = std::lower_bound(data_.begin(), data_.end(), value_to_insert, comp_);
        auto inserted_it_non_const = data_.insert(it_pos_non_const, std::move(value_to_insert));
        return std::next(cbegin(), std::distance(data_.begin(), inserted_it_non_const));
    }

    iterator erase(const_iterator pos) {
        // Precondition: pos must be a valid dereferenceable iterator.
        // pos != cend()
        auto dist = std::distance(cbegin(), pos);
        auto it_non_const = data_.begin() + dist;
        auto next_it_non_const = data_.erase(it_non_const);
        return std::next(cbegin(), std::distance(data_.begin(), next_it_non_const));
    }

    iterator erase(const_iterator first, const_iterator last) {
        // Precondition: [first, last) must be a valid range.
        auto dist_first = std::distance(cbegin(), first);
        auto it_non_const_first = data_.begin() + dist_first;
        auto dist_last = std::distance(cbegin(), last);
        auto it_non_const_last = data_.begin() + dist_last;

        auto next_it_non_const = data_.erase(it_non_const_first, it_non_const_last);
        return std::next(cbegin(), std::distance(data_.begin(), next_it_non_const));
    }

    void pop_front() {
        if (empty()) {
            throw std::logic_error("SortedList::pop_front: container is empty");
        }
        data_.erase(data_.begin());
    }

    void pop_back() {
        if (empty()) {
            throw std::logic_error("SortedList::pop_back: container is empty");
        }
        data_.pop_back();
    }

    // Search operations

    /**
     * @brief Find the index of the first element not less than the given value
     * @param value The value to compare against
     * @return Index of the first element not less than value
     */
    size_type lower_bound(const T& value) const {
        auto it = std::lower_bound(data_.begin(), data_.end(), value, comp_);
        return std::distance(data_.begin(), it);
    }

    /**
     * @brief Find the index of the first element greater than the given value
     * @param value The value to compare against
     * @return Index of the first element greater than value
     */
    size_type upper_bound(const T& value) const {
        auto it = std::upper_bound(data_.begin(), data_.end(), value, comp_);
        return std::distance(data_.begin(), it);
    }

    const_iterator find(const T& value) const {
        const_iterator it = std::lower_bound(data_.begin(), data_.end(), value, comp_);
        if (it != data_.end() && !comp_(value, *it)) {
            return it;
        }
        return data_.end();
    }

    /**
     * @brief Check if the container contains a specific value
     * @param value The value to search for
     * @return true if the value is found, false otherwise
     */
    bool contains(const T& value) const {
        auto it = std::lower_bound(data_.begin(), data_.end(), value, comp_);
        return it != data_.end() && !comp_(value, *it);
    }

    /**
     * @brief Count the number of elements equal to the given value
     * @param value The value to count
     * @return Number of elements equal to value
     */
    size_type count(const T& value) const {
        auto lower = std::lower_bound(data_.begin(), data_.end(), value, comp_);
        auto upper = std::upper_bound(data_.begin(), data_.end(), value, comp_);
        return std::distance(lower, upper);
    }

    // Range operations

    /**
     * @brief Get elements in the range [low, high)
     * @param low Lower bound (inclusive)
     * @param high Upper bound (exclusive)
     * @return Vector containing elements in the specified range
     */
    std::vector<T> range(const T& low, const T& high) const {
        auto lower_it = std::lower_bound(data_.begin(), data_.end(), low, comp_);
        auto upper_it = std::lower_bound(data_.begin(), data_.end(), high, comp_);
        return std::vector<T>(lower_it, upper_it);
    }

    /**
     * @brief Get a pair of indices representing the range [low, high)
     * @param low Lower bound (inclusive)
     * @param high Upper bound (exclusive)
     * @return Pair of (start_index, end_index) for the range
     */
    std::pair<size_type, size_type> range_indices(const T& low, const T& high) const {
        size_type start = lower_bound(low);
        size_type end = lower_bound(high);
        return {start, end};
    }

    // Iterators

    /**
     * @brief Returns an iterator to the beginning
     */
    const_iterator begin() const noexcept {
        return data_.begin();
    }

    /**
     * @brief Returns an iterator to the end
     */
    const_iterator end() const noexcept {
        return data_.end();
    }

    /**
     * @brief Returns a const iterator to the beginning
     */
    const_iterator cbegin() const noexcept {
        return data_.cbegin();
    }

    /**
     * @brief Returns a const iterator to the end
     */
    const_iterator cend() const noexcept {
        return data_.cend();
    }

    /**
     * @brief Returns a reverse iterator to the beginning of the reversed container
     */
    const_reverse_iterator rbegin() const noexcept {
        return data_.rbegin();
    }

    /**
     * @brief Returns a reverse iterator to the end of the reversed container
     */
    const_reverse_iterator rend() const noexcept {
        return data_.rend();
    }

    /**
     * @brief Returns a const reverse iterator to the beginning of the reversed container
     */
    const_reverse_iterator crbegin() const noexcept {
        return data_.crbegin();
    }

    /**
     * @brief Returns a const reverse iterator to the end of the reversed container
     */
    const_reverse_iterator crend() const noexcept {
        return data_.crend();
    }

    // Additional utility methods

    /**
     * @brief Get the front element
     * @return Reference to the first element
     * @throws std::runtime_error if the container is empty
     */
    const_reference front() const {
        if (empty()) {
            throw std::runtime_error("SortedList::front: container is empty");
        }
        return data_.front();
    }

    /**
     * @brief Get the back element
     * @return Reference to the last element
     * @throws std::runtime_error if the container is empty
     */
    const_reference back() const {
        if (empty()) {
            throw std::runtime_error("SortedList::back: container is empty");
        }
        return data_.back();
    }

    /**
     * @brief Reserve capacity for at least the specified number of elements
     * @param capacity The number of elements to reserve capacity for
     */
    void reserve(size_type capacity) {
        data_.reserve(capacity);
    }

    /**
     * @brief Get the current capacity
     * @return The current capacity of the underlying container
     */
    size_type capacity() const noexcept {
        return data_.capacity();
    }

    /**
     * @brief Shrink the capacity to fit the current size
     */
    void shrink_to_fit() {
        data_.shrink_to_fit();
    }

private:
    std::vector<T> data_;
    Compare comp_;
};

// Comparison operators

template <typename T, typename Compare>
bool operator==(const SortedList<T, Compare>& lhs, const SortedList<T, Compare>& rhs) {
    return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename T, typename Compare>
bool operator!=(const SortedList<T, Compare>& lhs, const SortedList<T, Compare>& rhs) {
    return !(lhs == rhs);
}

template <typename T, typename Compare>
bool operator<(const SortedList<T, Compare>& lhs, const SortedList<T, Compare>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename T, typename Compare>
bool operator<=(const SortedList<T, Compare>& lhs, const SortedList<T, Compare>& rhs) {
    return !(rhs < lhs);
}

template <typename T, typename Compare>
bool operator>(const SortedList<T, Compare>& lhs, const SortedList<T, Compare>& rhs) {
    return rhs < lhs;
}

template <typename T, typename Compare>
bool operator>=(const SortedList<T, Compare>& lhs, const SortedList<T, Compare>& rhs) {
    return !(lhs < rhs);
}

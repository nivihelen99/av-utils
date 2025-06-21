#pragma once

#include <list>
#include <unordered_map>
#include <vector>
#include <stdexcept>

/**
 * @brief A fixed-capacity, insertion-ordered set that evicts the oldest item when capacity is exceeded.
 * 
 * BoundedSet maintains insertion order and guarantees unique elements. When the maximum capacity
 * is reached, inserting a new element will automatically evict the oldest element (FIFO behavior).
 * 
 * @tparam T Element type. Must be hashable and equality comparable.
 */
template<typename T>
class BoundedSet {
public:
    using value_type = T;
    using size_type = std::size_t;
    using const_iterator = typename std::list<T>::const_iterator;

private:
    std::list<T> order_;  // Maintains insertion order (oldest at front)
    std::unordered_map<T, typename std::list<T>::iterator> index_;  // Fast lookup
    size_type max_size_;

public:
    /**
     * @brief Construct a BoundedSet with the given maximum capacity.
     * @param capacity Maximum number of elements the set can hold.
     * @throws std::invalid_argument if capacity is 0.
     */
    explicit BoundedSet(size_type capacity) : max_size_(capacity) {
        if (capacity == 0) {
            throw std::invalid_argument("BoundedSet capacity must be greater than 0");
        }
    }

    /**
     * @brief Insert an element into the set.
     * 
     * If the element already exists, this is a no-op and returns false.
     * If the element is new and the set is at capacity, the oldest element is evicted.
     * 
     * @param value The element to insert.
     * @return true if the element was inserted, false if it already existed.
     */
    bool insert(const T& value) {
        auto it = index_.find(value);
        if (it != index_.end()) {
            // Element already exists
            return false;
        }

        // Add new element to the back (newest)
        order_.push_back(value);
        auto list_it = std::prev(order_.end());
        index_[value] = list_it;

        // Evict oldest if over capacity
        if (order_.size() > max_size_) {
            auto oldest = order_.front();
            index_.erase(oldest);
            order_.pop_front();
        }

        return true;
    }

    /**
     * @brief Check if an element exists in the set.
     * @param value The element to check for.
     * @return true if the element exists, false otherwise.
     */
    bool contains(const T& value) const {
        return index_.find(value) != index_.end();
    }

    /**
     * @brief Remove an element from the set.
     * @param value The element to remove.
     * @return true if the element was removed, false if it didn't exist.
     */
    bool erase(const T& value) {
        auto it = index_.find(value);
        if (it == index_.end()) {
            return false;
        }

        order_.erase(it->second);
        index_.erase(it);
        return true;
    }

    /**
     * @brief Remove all elements from the set.
     */
    void clear() {
        order_.clear();
        index_.clear();
    }

    /**
     * @brief Get the current number of elements in the set.
     * @return The number of elements.
     */
    size_type size() const {
        return order_.size();
    }

    /**
     * @brief Get the maximum capacity of the set.
     * @return The maximum capacity.
     */
    size_type capacity() const {
        return max_size_;
    }

    /**
     * @brief Check if the set is empty.
     * @return true if empty, false otherwise.
     */
    bool empty() const {
        return order_.empty();
    }

    /**
     * @brief Get the oldest element in the set.
     * @return Reference to the oldest element.
     * @throws std::runtime_error if the set is empty.
     */
    const T& front() const {
        if (order_.empty()) {
            throw std::runtime_error("BoundedSet is empty");
        }
        return order_.front();
    }

    /**
     * @brief Get the newest element in the set.
     * @return Reference to the newest element.
     * @throws std::runtime_error if the set is empty.
     */
    const T& back() const {
        if (order_.empty()) {
            throw std::runtime_error("BoundedSet is empty");
        }
        return order_.back();
    }

    /**
     * @brief Get an iterator to the beginning (oldest element).
     * @return Const iterator to the beginning.
     */
    const_iterator begin() const {
        return order_.begin();
    }

    /**
     * @brief Get an iterator to the end.
     * @return Const iterator to the end.
     */
    const_iterator end() const {
        return order_.end();
    }

    /**
     * @brief Get a snapshot of current elements in insertion order.
     * @return Vector containing all elements from oldest to newest.
     */
    std::vector<T> as_vector() const {
        return std::vector<T>(order_.begin(), order_.end());
    }

    /**
     * @brief Change the capacity of the set.
     * 
     * If the new capacity is smaller than the current size, the oldest elements
     * are evicted to fit the new capacity.
     * 
     * @param new_capacity The new maximum capacity.
     * @throws std::invalid_argument if new_capacity is 0.
     */
    void reserve(size_type new_capacity) {
        if (new_capacity == 0) {
            throw std::invalid_argument("BoundedSet capacity must be greater than 0");
        }
        
        max_size_ = new_capacity;
        
        // Evict oldest elements if current size exceeds new capacity
        while (order_.size() > max_size_) {
            auto oldest = order_.front();
            index_.erase(oldest);
            order_.pop_front();
        }
    }

    /**
     * @brief Remove oldest entries if size exceeds capacity.
     * 
     * This method is useful if the capacity was changed externally or
     * if you want to ensure the set is within its capacity bounds.
     */
    void shrink_to_fit() {
        while (order_.size() > max_size_) {
            auto oldest = order_.front();
            index_.erase(oldest);
            order_.pop_front();
        }
    }
};

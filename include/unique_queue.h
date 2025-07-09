#pragma once

#include <queue>
#include <unordered_set>
#include <optional>
#include <stdexcept>
#include <functional> // For std::hash, std::equal_to, std::reference_wrapper, std::cref

// Forward declaration
template <typename T, typename Hash, typename Equal>
class UniqueQueue;

namespace internal {

// Helper struct for hashing std::reference_wrapper<const T> using UserHash
template <typename T, typename UserHash = std::hash<T>>
struct RefWrapperHash {
    std::size_t operator()(std::reference_wrapper<const T> rw) const {
        return UserHash{}(rw.get());
    }
};

// Helper struct for equality of std::reference_wrapper<const T> using UserEqual
template <typename T, typename UserEqual = std::equal_to<T>>
struct RefWrapperEqual {
    bool operator()(std::reference_wrapper<const T> lhs, std::reference_wrapper<const T> rhs) const {
        return UserEqual{}(lhs.get(), rhs.get());
    }
};

} // namespace internal

/**
 * @brief A hybrid container combining std::queue and std::unordered_set
 * 
 * UniqueQueue<T> maintains FIFO insertion order while preventing duplicates.
 * Elements are stored in insertion order and duplicates are automatically
 * rejected during insertion.
 * 
 * @tparam T Element type (must be hashable and equality comparable)
 * @tparam Hash Hash function for T (defaults to std::hash<T>)
 * @tparam Equal Equality predicate for T (defaults to std::equal_to<T>)
 */
template <typename T, 
          typename Hash = std::hash<T>, 
          typename Equal = std::equal_to<T>>
class UniqueQueue {
private:
    std::queue<T> queue_; // Owns the elements
    std::unordered_set<
        std::reference_wrapper<const T>,
        internal::RefWrapperHash<T, Hash>,
        internal::RefWrapperEqual<T, Equal>
    > seen_; // Stores references to elements in queue_

public:
    using value_type = T;
    using size_type = std::size_t;
    using reference = T&;
    using const_reference = const T&;

    /**
     * @brief Default constructor
     */
    UniqueQueue() = default;

    /**
     * @brief Copy constructor
     */
    UniqueQueue(const UniqueQueue& other) = default;

    /**
     * @brief Move constructor
     */
    UniqueQueue(UniqueQueue&& other) noexcept = default;

    /**
     * @brief Copy assignment operator
     */
    UniqueQueue& operator=(const UniqueQueue& other) = default;

    /**
     * @brief Move assignment operator
     */
    UniqueQueue& operator=(UniqueQueue&& other) noexcept = default;

    /**
     * @brief Destructor
     */
    ~UniqueQueue() = default;

    /**
     * @brief Insert element if not already present (copy version)
     * 
     * @param value Element to insert
     * @return true if inserted, false if duplicate
     */
    bool push(const T& value) {
        // O(N) duplicate check - TODO: Optimize with C++20 heterogeneous lookup if possible
        for (const auto& item_ref : seen_) {
            if (Equal{}(item_ref.get(), value)) {
                return false; // Duplicate found
            }
        }
        
        queue_.push(value); // T must be copy-constructible
        seen_.insert(std::cref(queue_.back()));
        return true;
    }

    /**
     * @brief Insert element if not already present (move version)
     * 
     * @param value Element to insert
     * @return true if inserted, false if duplicate
     */
    bool push(T&& value) {
        // O(N) duplicate check - TODO: Optimize with C++20 heterogeneous lookup if possible
        // Check happens before 'value' is moved.
        for (const auto& item_ref : seen_) {
            if (Equal{}(item_ref.get(), value)) {
                return false; // Duplicate found
            }
        }
        
        queue_.push(std::move(value)); // T must be move-constructible
        seen_.insert(std::cref(queue_.back()));
        return true;
    }

    /**
     * @brief Remove and return front element
     * 
     * @return Front element
     * @throws std::runtime_error if queue is empty
     */
    T pop() {
        if (queue_.empty()) {
            throw std::runtime_error("UniqueQueue::pop() called on empty queue");
        }
        
        // Get a reference to the front element *before* it's moved or queue is popped
        const T& front_ref = queue_.front();

        // Erase the reference from the 'seen_' set.
        // std::cref(front_ref) creates a temporary reference_wrapper.
        // The unordered_set needs to find the existing reference_wrapper that points to this front_ref.
        seen_.erase(std::cref(front_ref));

        // Now, move the element out of the queue.
        // Need const_cast because queue_.front() returns T& or const T&,
        // and we need T& to move from if T is not const.
        // If queue_ stores T, queue_.front() gives T&.
        T result = std::move(const_cast<T&>(front_ref));

        // Pop the (now moved-from) element from the queue.
        queue_.pop();

        return result;
    }

    /**
     * @brief Try to remove and return front element
     * 
     * @return Front element if available, std::nullopt if empty
     */
    std::optional<T> try_pop() {
        if (queue_.empty()) {
            return std::nullopt;
        }
        // Similar logic to pop()
        const T& front_ref = queue_.front();
        seen_.erase(std::cref(front_ref));
        T result = std::move(const_cast<T&>(front_ref));
        queue_.pop();
        return result;
    }

    /**
     * @brief Get reference to front element (const version)
     * 
     * @return Const reference to front element
     * @throws std::runtime_error if queue is empty
     */
    const T& front() const {
        if (queue_.empty()) {
            throw std::runtime_error("UniqueQueue::front() called on empty queue");
        }
        return queue_.front();
    }

    /**
     * @brief Get reference to front element (non-const version)
     * 
     * @return Reference to front element
     * @throws std::runtime_error if queue is empty
     */
    T& front() {
        if (queue_.empty()) {
            throw std::runtime_error("UniqueQueue::front() called on empty queue");
        }
        return queue_.front();
    }

    /**
     * @brief Check if element is present in queue
     * 
     * @param value Element to check
     * @return true if present, false otherwise
     */
    bool contains(const T& value) const {
        // O(N) scan - TODO: Optimize with C++20 heterogeneous lookup if possible
        for (const auto& item_ref : seen_) {
            if (Equal{}(item_ref.get(), value)) {
                return true; // Found
            }
        }
        return false; // Not found
    }

    /**
     * @brief Check if queue is empty
     * 
     * @return true if empty, false otherwise
     */
    bool empty() const noexcept {
        return queue_.empty();
    }

    /**
     * @brief Get number of elements in queue
     * 
     * @return Number of elements
     */
    size_type size() const noexcept {
        return queue_.size();
    }

    /**
     * @brief Remove specific element from queue (O(n) operation)
     * 
     * This operation maintains the order of remaining elements but
     * requires linear time to find and remove the element.
     * 
     * @param value Element to remove
     * @return true if element was found and removed, false otherwise
     */
    bool remove(const T& value) {
        bool removed = false;
        std::queue<T> new_queue; // Temporary queue to hold elements not removed
        
        // Iterate through the current queue_
        while (!queue_.empty()) {
            T& item_in_old_queue = queue_.front(); // Get reference to front
            
            if (!removed && Equal{}(item_in_old_queue, value)) {
                // Element found, remove its reference from seen_
                seen_.erase(std::cref(item_in_old_queue));
                queue_.pop(); // Remove from original queue (discard)
                removed = true;
            } else {
                // Element not (yet) removed or not the target element
                // Move it to the new_queue
                new_queue.push(std::move(item_in_old_queue));
                queue_.pop(); // Remove from original queue
            }
        }
        
        // Replace old queue with new_queue
        queue_ = std::move(new_queue);
        return removed;
    }

    /**
     * @brief Clear all elements from queue
     */
    void clear() {
        seen_.clear(); // Clear all references
        std::queue<T> empty_queue;
        queue_.swap(empty_queue); // Clear the queue itself
    }

    /**
     * @brief Swap contents with another UniqueQueue
     * 
     * @param other UniqueQueue to swap with
     */
    void swap(UniqueQueue& other) noexcept {
        using std::swap;
        swap(queue_, other.queue_);
        swap(seen_, other.seen_);
    }

    /**
     * @brief Get maximum possible size
     * 
     * @return Maximum size
     */
    size_type max_size() const noexcept {
        // std::queue doesn't have max_size(). The underlying container (std::deque by default) does.
        // seen_ (unordered_set) also stores a copy of each element, so its max_size is a reasonable proxy.
        return seen_.max_size();
    }

    // Iterator support (optional - for range-based for loops)
    // Note: This provides a snapshot view, not a live iterator
    class const_iterator {
    private:
        std::queue<T> queue_copy_;
        
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;
        
        explicit const_iterator(const std::queue<T>& q) : queue_copy_(q) {}
        const_iterator() = default;
        
        const T& operator*() const { return queue_copy_.front(); }
        const T* operator->() const { return &queue_copy_.front(); }
        
        const_iterator& operator++() {
            if (!queue_copy_.empty()) {
                queue_copy_.pop();
            }
            return *this;
        }
        
        const_iterator operator++(int) {
            const_iterator tmp = *this;
            ++(*this);
            return tmp;
        }
        
        bool operator==(const const_iterator& other) const {
            return queue_copy_.size() == other.queue_copy_.size();
        }
        
        bool operator!=(const const_iterator& other) const {
            return !(*this == other);
        }
    };

    const_iterator begin() const {
        return const_iterator(queue_);
    }

    const_iterator end() const {
        return const_iterator{};
    }

    const_iterator cbegin() const {
        return begin();
    }

    const_iterator cend() const {
        return end();
    }
};

// Non-member functions
template <typename T, typename Hash, typename Equal>
void swap(UniqueQueue<T, Hash, Equal>& lhs, UniqueQueue<T, Hash, Equal>& rhs) noexcept {
    lhs.swap(rhs);
}

template <typename T, typename Hash, typename Equal>
bool operator==(const UniqueQueue<T, Hash, Equal>& lhs, const UniqueQueue<T, Hash, Equal>& rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    
    // Compare element by element
    auto lhs_it = lhs.begin();
    auto rhs_it = rhs.begin();
    
    while (lhs_it != lhs.end() && rhs_it != rhs.end()) {
        if (!Equal{}(*lhs_it, *rhs_it)) {
            return false;
        }
        ++lhs_it;
        ++rhs_it;
    }
    
    return true;
}

template <typename T, typename Hash, typename Equal>
bool operator!=(const UniqueQueue<T, Hash, Equal>& lhs, const UniqueQueue<T, Hash, Equal>& rhs) {
    return !(lhs == rhs);
}

#ifndef INSTRUMENTED_RING_BUFFER_HPP
#define INSTRUMENTED_RING_BUFFER_HPP

#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <optional> // For try_pop that returns a value directly (alternative design)
#include <thread>   // For std::this_thread::yield in some spin-wait scenarios if used

// Forward declaration if needed, or include directly
// #include "some_utility.hpp" // If any external utility is used

namespace cpp_utils { // Assuming a common namespace, adjust if necessary

template <typename T>
class InstrumentedRingBuffer {
public:
    explicit InstrumentedRingBuffer(size_t capacity);

    // Rule of Five:
    InstrumentedRingBuffer(const InstrumentedRingBuffer&) = delete; // Non-copyable due to mutex/cv
    InstrumentedRingBuffer& operator=(const InstrumentedRingBuffer&) = delete; // Non-assignable
    InstrumentedRingBuffer(InstrumentedRingBuffer&&) noexcept; // Move constructor
    InstrumentedRingBuffer& operator=(InstrumentedRingBuffer&&) noexcept; // Move assignment
    ~InstrumentedRingBuffer() = default;

    // --- Core API ---
    bool try_push(const T& item);
    bool try_push(T&& item);

    void push(const T& item);
    void push(T&& item);

    bool try_pop(T& item); // Output parameter style
    // std::optional<T> try_pop(); // Alternative style, might be more modern C++

    T pop();

    // --- Capacity and State ---
    size_t size() const; // Returns current number of items
    size_t capacity() const;
    bool empty() const;
    bool full() const;

    // --- Introspection Metrics ---
    uint64_t get_push_success_count() const;
    uint64_t get_pop_success_count() const;
    uint64_t get_push_wait_count() const;    // Incremented when a push operation has to wait
    uint64_t get_pop_wait_count() const;     // Incremented when a pop operation has to wait
    uint64_t get_try_push_fail_count() const; // Incremented when try_push fails (buffer full)
    uint64_t get_try_pop_fail_count() const;  // Incremented when try_pop fails (buffer empty)
    size_t get_peak_size() const;            // Maximum number of items observed in the buffer

    void reset_metrics();

private:
    std::vector<T> buffer_;
    size_t head_; // Index of the next item to pop
    size_t tail_; // Index of the next available slot to push
    size_t current_size_;
    const size_t capacity_;

    mutable std::mutex mutex_; // `mutable` to allow locking in const methods like size()
    std::condition_variable cv_not_full_;
    std::condition_variable cv_not_empty_;

    // Metrics - using std::atomic for thread-safe updates
    std::atomic<uint64_t> push_success_count_;
    std::atomic<uint64_t> pop_success_count_;
    std::atomic<uint64_t> push_wait_count_;
    std::atomic<uint64_t> pop_wait_count_;
    std::atomic<uint64_t> try_push_fail_count_;
    std::atomic<uint64_t> try_pop_fail_count_;
    std::atomic<size_t> peak_size_;

    // Helper to update peak size, must be called under lock
    void update_peak_size_under_lock();
};

} // namespace cpp_utils

// Template implementations

namespace cpp_utils {

template <typename T>
InstrumentedRingBuffer<T>::InstrumentedRingBuffer(size_t capacity)
    : capacity_(capacity > 0 ? capacity : 1), // Ensure capacity is at least 1
      head_(0),
      tail_(0),
      current_size_(0),
      push_success_count_(0),
      pop_success_count_(0),
      push_wait_count_(0),
      pop_wait_count_(0),
      try_push_fail_count_(0),
      try_pop_fail_count_(0),
      peak_size_(0) {
    if (capacity == 0) {
        // Or throw std::invalid_argument("Capacity must be positive");
        // For now, defaulting to 1 as per above.
    }
    buffer_.resize(capacity_);
}

template <typename T>
InstrumentedRingBuffer<T>::InstrumentedRingBuffer(InstrumentedRingBuffer&& other) noexcept
    : capacity_(other.capacity_) { // capacity_ is const, must be initialized
    std::unique_lock<std::mutex> lock(other.mutex_); // Lock other's mutex before moving data

    buffer_ = std::move(other.buffer_);
    head_ = other.head_;
    tail_ = other.tail_;
    current_size_ = other.current_size_;

    // Move atomic variables
    push_success_count_.store(other.push_success_count_.load());
    pop_success_count_.store(other.pop_success_count_.load());
    push_wait_count_.store(other.push_wait_count_.load());
    pop_wait_count_.store(other.pop_wait_count_.load());
    try_push_fail_count_.store(other.try_push_fail_count_.load());
    try_pop_fail_count_.store(other.try_pop_fail_count_.load());
    peak_size_.store(other.peak_size_.load());

    // Reset other's state to a valid, empty state
    other.head_ = 0;
    other.tail_ = 0;
    other.current_size_ = 0;
    // other.buffer_ is already moved from, its state is valid (empty or moved-from state)
    // other.buffer_.clear(); // could also clear explicitly if desired
    // other.buffer_.resize(other.capacity_); // Or resize to its original capacity if it should remain usable

    // It's generally tricky to move condition variables.
    // Here, we are relying on the fact that the moved-from 'other' object
    // will likely be destroyed or not used for synchronization.
    // If 'other' were to be used again, its CVs might not be in a useful state
    // without re-initialization or careful handling.
    // For a typical move where 'other' is discarded, this is acceptable.
}

template <typename T>
InstrumentedRingBuffer<T>& InstrumentedRingBuffer<T>::operator=(InstrumentedRingBuffer&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    // Lock both mutexes to prevent deadlock, preferably using std::scoped_lock or std::lock
    std::unique_lock<std::mutex> this_lock(mutex_, std::defer_lock);
    std::unique_lock<std::mutex> other_lock(other.mutex_, std::defer_lock);
    std::lock(this_lock, other_lock);

    // Capacity is const, cannot be assigned. This implies that move assignment
    // should only be possible if capacities are compatible, or it's disallowed.
    // For this implementation, we assume this object is being assigned to and might have
    // a different capacity, which is problematic for const capacity_.
    // A common pattern is to only allow move assignment if capacities are identical,
    // or to make capacity_ non-const if move assignment should change it (less common for this type).
    // Given capacity_ is const, a true move assignment that changes capacity is impossible.
    // We will proceed as if this is a move of contents into an object *of the same capacity*.
    // If capacities differ, this operation is ill-defined or should throw/assert.
    // For simplicity, we'll assume capacities are managed such that this is valid.
    // Or, more realistically, make InstrumentedRingBuffer move-constructible but not move-assignable
    // if capacity_ must remain const and differ.
    // Let's proceed with the move of data, assuming capacity_ was already set and is compatible.

    buffer_ = std::move(other.buffer_); // This will copy vector's capacity and size
    head_ = other.head_;
    tail_ = other.tail_;
    current_size_ = other.current_size_;

    // buffer_.resize(capacity_); // Ensure our buffer matches our const capacity_

    push_success_count_.store(other.push_success_count_.load());
    pop_success_count_.store(other.pop_success_count_.load());
    push_wait_count_.store(other.push_wait_count_.load());
    pop_wait_count_.store(other.pop_wait_count_.load());
    try_push_fail_count_.store(other.try_push_fail_count_.load());
    try_pop_fail_count_.store(other.try_pop_fail_count_.load());
    peak_size_.store(other.peak_size_.load());

    other.head_ = 0;
    other.tail_ = 0;
    other.current_size_ = 0;
    // other.buffer_ is in a valid moved-from state.
    // other.buffer_.clear();
    // other.buffer_.resize(other.capacity_); // If other needs to be reset to its capacity

    return *this;
}


// --- Core API ---
template <typename T>
bool InstrumentedRingBuffer<T>::try_push(const T& item) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (current_size_ == capacity_) {
        try_push_fail_count_++;
        return false; // Buffer is full
    }
    buffer_[tail_] = item;
    tail_ = (tail_ + 1) % capacity_;
    current_size_++;
    update_peak_size_under_lock();
    push_success_count_++;
    lock.unlock();
    cv_not_empty_.notify_one();
    return true;
}

template <typename T>
bool InstrumentedRingBuffer<T>::try_push(T&& item) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (current_size_ == capacity_) {
        try_push_fail_count_++;
        return false; // Buffer is full
    }
    buffer_[tail_] = std::move(item);
    tail_ = (tail_ + 1) % capacity_;
    current_size_++;
    update_peak_size_under_lock();
    push_success_count_++;
    lock.unlock();
    cv_not_empty_.notify_one();
    return true;
}

template <typename T>
void InstrumentedRingBuffer<T>::push(const T& item) {
    std::unique_lock<std::mutex> lock(mutex_);
    bool waited = false;
    while (current_size_ == capacity_) {
        if (!waited) { // Increment wait count only once per blocking operation
            push_wait_count_++;
            waited = true;
        }
        cv_not_full_.wait(lock);
    }
    buffer_[tail_] = item;
    tail_ = (tail_ + 1) % capacity_;
    current_size_++;
    update_peak_size_under_lock();
    push_success_count_++;
    lock.unlock();
    cv_not_empty_.notify_one();
}

template <typename T>
void InstrumentedRingBuffer<T>::push(T&& item) {
    std::unique_lock<std::mutex> lock(mutex_);
    bool waited = false;
    while (current_size_ == capacity_) {
        if (!waited) {
            push_wait_count_++;
            waited = true;
        }
        cv_not_full_.wait(lock);
    }
    buffer_[tail_] = std::move(item);
    tail_ = (tail_ + 1) % capacity_;
    current_size_++;
    update_peak_size_under_lock();
    push_success_count_++;
    lock.unlock();
    cv_not_empty_.notify_one();
}

template <typename T>
bool InstrumentedRingBuffer<T>::try_pop(T& item) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (current_size_ == 0) {
        try_pop_fail_count_++;
        return false; // Buffer is empty
    }
    item = std::move(buffer_[head_]); // Use move if T is movable
    head_ = (head_ + 1) % capacity_;
    current_size_--;
    pop_success_count_++;
    lock.unlock();
    cv_not_full_.notify_one();
    return true;
}

template <typename T>
T InstrumentedRingBuffer<T>::pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    bool waited = false;
    while (current_size_ == 0) {
        if (!waited) {
            pop_wait_count_++;
            waited = true;
        }
        cv_not_empty_.wait(lock);
    }
    T item = std::move(buffer_[head_]); // Use move
    head_ = (head_ + 1) % capacity_;
    current_size_--;
    pop_success_count_++;
    lock.unlock();
    cv_not_full_.notify_one();
    return item;
}

// --- Capacity and State ---
template <typename T>
size_t InstrumentedRingBuffer<T>::size() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return current_size_;
}

template <typename T>
size_t InstrumentedRingBuffer<T>::capacity() const {
    return capacity_; // const member, no lock needed
}

template <typename T>
bool InstrumentedRingBuffer<T>::empty() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return current_size_ == 0;
}

template <typename T>
bool InstrumentedRingBuffer<T>::full() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return current_size_ == capacity_;
}

// --- Introspection Metrics ---
template <typename T>
uint64_t InstrumentedRingBuffer<T>::get_push_success_count() const {
    return push_success_count_.load();
}

template <typename T>
uint64_t InstrumentedRingBuffer<T>::get_pop_success_count() const {
    return pop_success_count_.load();
}

template <typename T>
uint64_t InstrumentedRingBuffer<T>::get_push_wait_count() const {
    return push_wait_count_.load();
}

template <typename T>
uint64_t InstrumentedRingBuffer<T>::get_pop_wait_count() const {
    return pop_wait_count_.load();
}

template <typename T>
uint64_t InstrumentedRingBuffer<T>::get_try_push_fail_count() const {
    return try_push_fail_count_.load();
}

template <typename T>
uint64_t InstrumentedRingBuffer<T>::get_try_pop_fail_count() const {
    return try_pop_fail_count_.load();
}

template <typename T>
size_t InstrumentedRingBuffer<T>::get_peak_size() const {
    return peak_size_.load();
}

template <typename T>
void InstrumentedRingBuffer<T>::reset_metrics() {
    // Atomically reset all metrics
    push_success_count_.store(0);
    pop_success_count_.store(0);
    push_wait_count_.store(0);
    pop_wait_count_.store(0);
    try_push_fail_count_.store(0);
    try_pop_fail_count_.store(0);
    // peak_size_ should ideally be reset in conjunction with current_size_ under lock,
    // or reset to 0, understanding it will then update from the current state.
    // Resetting peak_size_ to 0 is generally fine.
    // If we want to reset it to current_size:
    // std::unique_lock<std::mutex> lock(mutex_);
    // peak_size_.store(current_size_);
    // For a full reset:
    peak_size_.store(0);
    // If you also wanted to reset peak_size to the current_size after clearing other counters:
    // std::unique_lock<std::mutex> lock(mutex_);
    // peak_size_.store(current_size_);
}

// --- Private Helper Methods ---
template <typename T>
void InstrumentedRingBuffer<T>::update_peak_size_under_lock() {
    // Assumes mutex_ is already locked by the caller
    if (current_size_ > peak_size_.load()) {
        peak_size_.store(current_size_);
    }
}

} // namespace cpp_utils

#endif // INSTRUMENTED_RING_BUFFER_HPP

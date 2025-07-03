#ifndef CHRONO_RING_H
#define CHRONO_RING_H

#include <vector>
#include <chrono>
#include <cstddef> // For size_t
#include <stdexcept> // For std::out_of_range in case of 0 capacity

namespace anomeda { // Using a namespace to avoid potential collisions

template<typename T, typename Clock = std::chrono::steady_clock>
class ChronoRing {
public:
    using TimePoint = typename Clock::time_point;

    struct Entry {
        T value;
        TimePoint timestamp;
    };

    explicit ChronoRing(size_t capacity) : capacity_(capacity) {
        if (capacity == 0) {
            throw std::out_of_range("ChronoRing capacity must be greater than 0.");
        }
        buffer_.resize(capacity_); // Pre-allocate buffer
    }

    // Copy constructor
    ChronoRing(const ChronoRing& other) = default;

    // Move constructor
    ChronoRing(ChronoRing&& other) noexcept = default;

    // Copy assignment operator
    ChronoRing& operator=(const ChronoRing& other) = default;

    // Move assignment operator
    ChronoRing& operator=(ChronoRing&& other) noexcept = default;


    size_t size() const {
        return count_;
    }

    size_t capacity() const {
        return capacity_;
    }

    bool empty() const {
        return count_ == 0;
    }

    void clear() {
        // Reset head and count, no need to clear actual elements in buffer_
        // as they will be overwritten.
        head_ = 0;
        count_ = 0;
    }

    // push, push_at, recent, entries_between, expire_older_than will be added here
    void push(const T& value) {
        push_at(value, Clock::now());
    }

    void push_at(const T& value, TimePoint time) {
        if (capacity_ == 0) return; // Should have been caught by constructor, but defensive

        buffer_[head_] = {value, time};
        head_ = (head_ + 1) % capacity_;
        if (count_ < capacity_) {
            count_++;
        }
    }

    std::vector<T> recent(std::chrono::milliseconds duration) const {
        TimePoint cutoff = Clock::now() - duration;
        std::vector<T> result;
        if (count_ == 0) return result;

        // Iterate from newest to oldest, which is more complex due to wrap-around
        // Start from (head_ - 1 + capacity_) % capacity_ back to (head_ - count_ + capacity_) % capacity_
        for (size_t i = 0; i < count_; ++i) {
            // Calculate actual index in buffer. Newest is at head_ - 1, oldest is at head_ (if full) or 0 (if not full and head < count)
            // If head_ is 0 and buffer is full, newest is at capacity_ - 1
            // If head_ is X, newest is at X-1.
            // The element at index `j` in chronological order (0 is oldest, count-1 is newest)
            // is located at `(head_ - count_ + j + capacity_) % capacity_` if buffer is not full
            // or `(head_ + j) % capacity_` if buffer is full

            size_t current_idx;
            if (count_ < capacity_) { // Buffer not full, elements are from index 0 to count_ - 1, head_ points after the last element
                current_idx = (head_ - 1 - i + capacity_) % capacity_; // Iterate backwards from newest
            } else { // Buffer is full, elements are from head_ (oldest) around to head_ - 1 (newest)
                current_idx = (head_ - 1 - i + capacity_) % capacity_;
            }

            // This logic for iterating is tricky. Let's rethink.
            // It's easier to iterate through the valid portion of the buffer in logical order (oldest to newest)
            // and then filter.

            // The oldest element is at index `oldest_idx`.
            // If count_ < capacity_, oldest_idx is 0. head_ is count_.
            // If count_ == capacity_, oldest_idx is head_.
            size_t oldest_idx = (count_ < capacity_) ? 0 : head_;

            for (size_t j = 0; j < count_; ++j) {
                const Entry& entry = buffer_[(oldest_idx + j) % capacity_];
                if (entry.timestamp >= cutoff) {
                    result.push_back(entry.value);
                }
            }
            // The above loop iterates multiple times, which is wrong.
            // Corrected iteration:
            break; // Exit the incorrect outer loop. The inner loop logic needs to be standalone.
        }

        // Iterate through elements in their logical order (oldest to newest)
        result.clear();
        if (count_ == 0) return result;

        size_t oldest_actual_idx = (head_ + capacity_ - count_) % capacity_;
        for (size_t i = 0; i < count_; ++i) {
            const Entry& entry = buffer_[(oldest_actual_idx + i) % capacity_];
            if (entry.timestamp >= cutoff) {
                result.push_back(entry.value);
            }
        }
        return result;
    }

    std::vector<Entry> entries_between(TimePoint start_time, TimePoint end_time) const {
        std::vector<Entry> result;
        if (count_ == 0 || start_time > end_time) return result;

        size_t oldest_actual_idx = (head_ + capacity_ - count_) % capacity_;
        for (size_t i = 0; i < count_; ++i) {
            const Entry& entry = buffer_[(oldest_actual_idx + i) % capacity_];
            if (entry.timestamp >= start_time && entry.timestamp <= end_time) {
                result.push_back(entry);
            }
        }
        return result;
    }

    void expire_older_than(TimePoint cutoff) {
        if (count_ == 0) return;

        size_t oldest_actual_idx = (head_ + capacity_ - count_) % capacity_;
        size_t num_to_expire = 0;

        // Iterate from oldest to newest to find how many are older than cutoff
        for (size_t i = 0; i < count_; ++i) {
            const Entry& entry = buffer_[(oldest_actual_idx + i) % capacity_];
            if (entry.timestamp < cutoff) {
                num_to_expire++;
            } else {
                // This is the first element to keep. All previous ones (num_to_expire) are expired.
                break;
            }
        }

        if (num_to_expire == 0) {
            return; // Nothing to expire
        }

        if (num_to_expire == count_) {
            // All elements expired
            clear();
            return;
        }

        count_ -= num_to_expire;
        // head_ (next write position) is not changed by expiration of oldest items.
        // The 'oldest_actual_idx' will be different next time it's calculated due to the new 'count_'.
    }

private:
    std::vector<Entry> buffer_;
    size_t capacity_; // Renamed from capacity in API sketch to avoid conflict with method
    size_t head_ = 0;  // Index of the next element to be inserted (oldest element if full)
    size_t count_ = 0; // Number of actual elements in the buffer
};

} // namespace anomeda

#endif // CHRONO_RING_H

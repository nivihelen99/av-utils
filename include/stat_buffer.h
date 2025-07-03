#ifndef STAT_BUFFER_H
#define STAT_BUFFER_H

#include <deque>
#include <vector>
#include <numeric>
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <limits>

template <typename T, size_t N>
class StatBuffer {
public:
    static_assert(N > 0, "Capacity N must be greater than 0");

    StatBuffer() : current_sum(0), m2(0), current_min(std::numeric_limits<T>::max()), current_max(std::numeric_limits<T>::lowest()) {}

    void push(const T& value) {
        T old_value = (T)0; // Placeholder, only used if buffer is full
        bool was_full = full();

        if (was_full) {
            old_value = buffer_.front();
            buffer_.pop_front();

            // Welford: Downdate m2 if buffer is not empty after removal
            // This needs careful handling: if buffer becomes too small (0 or 1), m2 might not be well-defined or should be 0
            size_t count_before_removal = N; // Since it was full
            double mean_before_removal = static_cast<double>(current_sum) / count_before_removal;

            current_sum -= old_value; // sum is updated first

            if (!buffer_.empty()) { // buffer_.size() is N-1 here
                 double mean_after_removal = static_cast<double>(current_sum) / buffer_.size();
                 m2 -= (old_value - mean_before_removal) * (old_value - mean_after_removal);
                 if (m2 < 0) m2 = 0; // Correction for floating point inaccuracies
            } else { // Buffer became empty after removing old_value
                m2 = 0;
            }

            // Update min/max if the evicted value was the current min/max
            if (old_value == current_min) {
                current_min = std::numeric_limits<T>::max();
                if (!buffer_.empty()) {
                    current_min = buffer_.front();
                    for(const auto& val : buffer_) {
                        if (val < current_min) current_min = val;
                    }
                } else {
                    // current_max will also be reset below if buffer is empty
                }
            }
            if (old_value == current_max) {
                current_max = std::numeric_limits<T>::lowest();
                 if (!buffer_.empty()) {
                    current_max = buffer_.front();
                    for(const auto& val : buffer_) {
                        if (val > current_max) current_max = val;
                    }
                }
            }
        }

        buffer_.push_back(value);

        // Welford: Update m2 with the new value
        size_t count_after_addition = buffer_.size();
        double mean_before_addition = (count_after_addition == 1) ? 0.0 : static_cast<double>(current_sum) / (count_after_addition - 1);

        current_sum += value; // sum updated after getting mean_before_addition

        double mean_after_addition = static_cast<double>(current_sum) / count_after_addition;

        if (count_after_addition == 1) { // First element in buffer
             m2 = 0; // m2 is 0 for a single element
        } else {
             m2 += (value - mean_before_addition) * (value - mean_after_addition);
             if (m2 < 0) m2 = 0; // Correction for floating point inaccuracies
        }

        // Update min/max with the new value
        if (buffer_.size() == 1) { // First element overall, or after clear then push
            current_min = value;
            current_max = value;
        } else {
            if (value < current_min) {
                current_min = value;
            }
            if (value > current_max) {
                current_max = value;
            }
        }
    }

    size_t size() const {
        return buffer_.size();
    }

    size_t capacity() const {
        return N;
    }

    bool full() const {
        return buffer_.size() == N;
    }

    void clear() {
        buffer_.clear();
        current_sum = 0;
        m2 = 0;
        current_min = std::numeric_limits<T>::max();
        current_max = std::numeric_limits<T>::lowest();
    }

    T min() const {
        if (buffer_.empty()) {
            if constexpr (std::is_floating_point_v<T>) {
                return std::numeric_limits<T>::quiet_NaN();
            } else {
                throw std::runtime_error("min() called on empty buffer");
            }
        }
        return current_min;
    }

    T max() const {
        if (buffer_.empty()) {
             if constexpr (std::is_floating_point_v<T>) {
                return std::numeric_limits<T>::quiet_NaN();
            } else {
                throw std::runtime_error("max() called on empty buffer");
            }
        }
       return current_max;
    }

    T sum() const {
        if (buffer_.empty()) {
            return static_cast<T>(0);
        }
        return current_sum;
    }

    double mean() const {
        if (buffer_.empty()) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        return static_cast<double>(current_sum) / buffer_.size();
    }

    double variance() const { // Population variance
        if (buffer_.size() < 1) { // Changed from < 2 to < 1 as variance of 1 element is 0
            return std::numeric_limits<double>::quiet_NaN();
        }
        if (buffer_.size() == 1) return 0.0; // Variance of a single point is 0
        return m2 / buffer_.size();
    }

    double stddev() const { // Population standard deviation
        double var = variance();
        return std::isnan(var) ? var : std::sqrt(var);
    }

    // Optional: median and percentile can be added later
    // T median() const;
    // T percentile(double p) const;

private:
    std::deque<T> buffer_;
    T current_sum;
    double m2; // Sum of squares of differences from the current mean (for Welford's algorithm)

    // For more efficient min/max tracking without full scan on eviction
    T current_min;
    T current_max;

    // Welford's algorithm update for push (simplified, see full in push)
    // void update_welford(const T& newValue) {
    //     size_t count = buffer_.size();
    //     double old_mean = (count == 1) ? 0 : static_cast<double>(current_sum - newValue) / (count - 1);
    //     double new_mean = static_cast<double>(current_sum) / count;
    //     m2 += (newValue - old_mean) * (newValue - new_mean);
    // }

    // Welford's algorithm update for pop (more complex, involves previous mean)
    // void downdate_welford(const T& oldValue) {
    //    size_t count = buffer_.size() + 1; // count before removal
    //    double old_mean = static_cast<double>(current_sum + oldValue) / count; // mean before removal
    //    double new_mean = buffer_.empty() ? 0 : static_cast<double>(current_sum) / buffer_.size(); // mean after removal
    //    m2 -= (oldValue - old_mean) * (oldValue - new_mean);
    //    if (m2 < 0) m2 = 0; // prevent floating point inaccuracies from making m2 negative
    // }
};

#endif // STAT_BUFFER_H

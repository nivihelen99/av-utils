#ifndef COUNT_MIN_SKETCH_H
#define COUNT_MIN_SKETCH_H

#include <vector>
#include <string>
#include <cmath>      // For std::log, std::ceil, std::exp (for M_E)
#include <functional> // For std::hash (though we'll use custom hashing)
#include <limits>     // For std::numeric_limits
#include <stdexcept>  // For std::invalid_argument
#include <algorithm>  // For std::min

// Hashing utilities within detail namespace, adapted from bloom_filter.h
namespace detail {

// FNV-1a constants for size_t
#if defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__) || defined(__aarch64__)
constexpr size_t CMS_FNV_PRIME = 1099511628211ULL;
constexpr size_t CMS_FNV_OFFSET_BASIS = 14695981039346656037ULL;
#else // 32-bit
constexpr size_t CMS_FNV_PRIME = 16777619U;
constexpr size_t CMS_FNV_OFFSET_BASIS = 2166136261U;
#endif

// A secondary basis for generating a second hash function (h2)
// Derived by XORing with a constant pattern.
constexpr size_t CMS_FNV_OFFSET_BASIS_2 = CMS_FNV_OFFSET_BASIS ^
    (sizeof(size_t) == 8 ? 0x5A5A5A5A5A5A5A5AULL : 0x5A5A5A5AU);


// Helper to apply FNV-1a to a block of memory
inline size_t cms_fnv1a_hash_bytes(const unsigned char* data, size_t len, size_t basis, size_t prime) {
    size_t hash_val = basis;
    for (size_t i = 0; i < len; ++i) {
        hash_val ^= static_cast<size_t>(data[i]);
        hash_val *= prime;
    }
    return hash_val;
}

// Generic CountMinSketchHash for arithmetic types (integers, float, double, etc.)
// and any type T where taking its address and sizeof(T) is meaningful for hashing.
// This uses the h(x) = h1(x) + seed * h2(x) construction.
template <typename T>
struct CountMinSketchHash {
    size_t operator()(const T& item, size_t seed) const {
        // Ensure T is trivially copyable or that this approach is safe.
        static_assert(std::is_trivially_copyable<T>::value || std::is_standard_layout<T>::value,
                      "CountMinSketchHash<T> default implementation relies on byte representation. Ensure T is suitable.");

        const unsigned char* item_bytes = reinterpret_cast<const unsigned char*>(&item);
        size_t item_len = sizeof(T);

        size_t h1 = cms_fnv1a_hash_bytes(item_bytes, item_len, CMS_FNV_OFFSET_BASIS, CMS_FNV_PRIME);
        size_t h2 = cms_fnv1a_hash_bytes(item_bytes, item_len, CMS_FNV_OFFSET_BASIS_2, CMS_FNV_PRIME);

        return h1 + seed * h2;
    }
};

// Specialization for std::string
template <>
struct CountMinSketchHash<std::string> {
    size_t operator()(const std::string& item, size_t seed) const {
        const unsigned char* item_bytes = reinterpret_cast<const unsigned char*>(item.data());
        size_t item_len = item.length();

        size_t h1 = cms_fnv1a_hash_bytes(item_bytes, item_len, CMS_FNV_OFFSET_BASIS, CMS_FNV_PRIME);
        size_t h2 = cms_fnv1a_hash_bytes(item_bytes, item_len, CMS_FNV_OFFSET_BASIS_2, CMS_FNV_PRIME);

        return h1 + seed * h2;
    }
};

} // namespace detail

template <typename T>
class CountMinSketch {
public:
    /**
     * @brief Constructs a Count-Min Sketch.
     *
     * The sketch estimates item frequencies with errors bounded by `epsilon`
     * with a probability of `1 - delta`.
     *
     * @param epsilon The maximum additive error factor for estimates.
     *                Must be > 0.0 and < 1.0. Smaller epsilon means lower error but more memory.
     *                Error is `epsilon * total_items_count`.
     * @param delta The probability that the error guarantee is not met.
     *              Must be > 0.0 and < 1.0. Smaller delta means higher confidence but more hash functions (depth).
     */
    CountMinSketch(double epsilon, double delta)
        : epsilon_(epsilon), delta_(delta) {
        if (epsilon <= 0.0 || epsilon >= 1.0) {
            throw std::invalid_argument("Epsilon must be between 0.0 and 1.0 (exclusive).");
        }
        if (delta <= 0.0 || delta >= 1.0) {
            throw std::invalid_argument("Delta must be between 0.0 and 1.0 (exclusive).");
        }

        // Calculate width (w) and depth (d)
        // w = ceil(e / epsilon) where e is Euler's number
        width_ = static_cast<size_t>(std::ceil(std::exp(1.0) / epsilon_));
        // d = ceil(ln(1 / delta))
        depth_ = static_cast<size_t>(std::ceil(std::log(1.0 / delta_)));

        if (width_ == 0) width_ = 1; // Ensure at least 1 counter
        if (depth_ == 0) depth_ = 1; // Ensure at least 1 hash function

        // Initialize counters_ table
        counters_.resize(depth_, std::vector<unsigned int>(width_, 0));
    }

    /**
     * @brief Adds an item to the sketch, incrementing its count.
     *
     * @param item The item to add.
     * @param count The amount by which to increment the item's count (default is 1).
     */
    void add(const T& item, unsigned int count = 1) {
        if (count == 0) return; // Adding zero has no effect
        for (size_t i = 0; i < depth_; ++i) {
            size_t hash_val = hasher_(item, i);
            // Check for potential overflow before adding.
            // If counters_[i][hash_val % width_] + count > max_val, cap it.
            // This is a simple way to handle it; more sophisticated applications
            // might need to signal this or use larger counter types.
            unsigned int current_val = counters_[i][hash_val % width_];
            if (std::numeric_limits<unsigned int>::max() - count < current_val) {
                counters_[i][hash_val % width_] = std::numeric_limits<unsigned int>::max();
            } else {
                counters_[i][hash_val % width_] += count;
            }
        }
    }

    /**
     * @brief Estimates the frequency of an item.
     *
     * The estimate is guaranteed to be not less than the true frequency.
     * The estimate is `true_frequency + epsilon * N` with probability `1 - delta`,
     * where N is the sum of all counts inserted into the sketch.
     *
     * @param item The item to estimate.
     * @return The estimated frequency of the item.
     */
    unsigned int estimate(const T& item) const {
        if (width_ == 0 || depth_ == 0) return 0; // Should not happen with constructor guards

        unsigned int min_count = std::numeric_limits<unsigned int>::max();
        for (size_t i = 0; i < depth_; ++i) {
            size_t hash_val = hasher_(item, i);
            min_count = std::min(min_count, counters_[i][hash_val % width_]);
        }
        return min_count;
    }

    /**
     * @brief Returns the width (w) of the sketch's counter table.
     * This is the number of counters per hash function.
     */
    size_t get_width() const {
        return width_;
    }

    /**
     * @brief Returns the depth (d) of the sketch (number of hash functions).
     */
    size_t get_depth() const {
        return depth_;
    }

    /**
     * @brief Returns the configured error factor epsilon.
     */
    double get_error_factor_epsilon() const {
        return epsilon_;
    }

    /**
     * @brief Returns the configured error probability delta.
     */
    double get_error_probability_delta() const {
        return delta_;
    }

private:
    double epsilon_;
    double delta_;
    size_t width_;    // w
    size_t depth_;    // d

    std::vector<std::vector<unsigned int>> counters_;
    detail::CountMinSketchHash<T> hasher_;
};

#endif // COUNT_MIN_SKETCH_H

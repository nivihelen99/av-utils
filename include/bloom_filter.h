#ifndef BLOOM_FILTER_H
#define BLOOM_FILTER_H

#include <vector>
#include <string>
#include <cmath> // For std::log, std::pow, std::ceil
#include <functional> // For std::hash
#include <limits> // For std::numeric_limits
#include <stdexcept> // For std::invalid_argument

// Hashing utilities within detail namespace
namespace detail {

// FNV-1a constants for size_t
#if defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__) || defined(__aarch64__)
constexpr size_t FNV_PRIME = 1099511628211ULL;
constexpr size_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
#else // 32-bit
constexpr size_t FNV_PRIME = 16777619U;
constexpr size_t FNV_OFFSET_BASIS = 2166136261U;
#endif

// A secondary basis for generating a second hash function (h2)
// Derived by XORing with a constant pattern.
constexpr size_t FNV_OFFSET_BASIS_2 = FNV_OFFSET_BASIS ^
    (sizeof(size_t) == 8 ? 0x5A5A5A5A5A5A5A5AULL : 0x5A5A5A5AU);


// Helper to apply FNV-1a to a block of memory
inline size_t fnv1a_hash_bytes(const unsigned char* data, size_t len, size_t basis, size_t prime) {
    size_t hash_val = basis;
    for (size_t i = 0; i < len; ++i) {
        hash_val ^= static_cast<size_t>(data[i]);
        hash_val *= prime;
    }
    return hash_val;
}

// Generic BloomHash for arithmetic types (integers, float, double, etc.)
// and any type T where taking its address and sizeof(T) is meaningful for hashing.
// This uses the h(x) = h1(x) + seed * h2(x) construction.
// This is a primary template. It will be used if T is not std::string.
template <typename T>
struct BloomHash {
    size_t operator()(const T& item, size_t seed) const {
        // Ensure T is trivially copyable or that this approach is safe.
        // For fundamental types like int, float, etc., this is fine.
        // For user-defined structs, they should be simple data containers for this to work well.
        static_assert(std::is_trivially_copyable<T>::value || std::is_standard_layout<T>::value,
                      "BloomHash<T> default implementation relies on byte representation. Ensure T is suitable.");

        const unsigned char* item_bytes = reinterpret_cast<const unsigned char*>(&item);
        size_t item_len = sizeof(T);

        size_t h1 = fnv1a_hash_bytes(item_bytes, item_len, FNV_OFFSET_BASIS, FNV_PRIME);
        // Using a different basis for h2 to make it a different hash function.
        size_t h2 = fnv1a_hash_bytes(item_bytes, item_len, FNV_OFFSET_BASIS_2, FNV_PRIME);

        // Classic construction for multiple hashes: h1 + seed * h2
        // This should provide better distribution than perturbing a single hash.
        return h1 + seed * h2;
    }
};

// Specialization for std::string
// (To be refined in next step, for now using a basic FNV approach with seed for consistency)
template <>
struct BloomHash<std::string> {
    size_t operator()(const std::string& item, size_t seed) const {
        const unsigned char* item_bytes = reinterpret_cast<const unsigned char*>(item.data());
        size_t item_len = item.length();

        // For strings, we can use a similar h1 + seed * h2 approach.
        // h1 can be FNV on string. h2 can be FNV with different basis or a simple string hash.
        size_t h1 = fnv1a_hash_bytes(item_bytes, item_len, FNV_OFFSET_BASIS, FNV_PRIME);
        size_t h2 = fnv1a_hash_bytes(item_bytes, item_len, FNV_OFFSET_BASIS_2, FNV_PRIME);

        return h1 + seed * h2;
    }
};

} // namespace detail

template <typename T>
class BloomFilter {
public:
    /**
     * @brief Constructs a Bloom filter.
     *
     * @param expected_items The expected number of items to be inserted.
     * @param false_positive_probability The desired false positive probability (e.g., 0.01 for 1%).
     *                                   Must be > 0.0 and < 1.0.
     */
    BloomFilter(size_t expected_items, double false_positive_probability)
        : num_expected_items_(expected_items),
          fp_prob_(false_positive_probability) {
        if (expected_items == 0) {
            // Handle degenerate case: if no items are expected,
            // make a minimal filter (e.g. 1 bit, 1 hash func).
            // `might_contain` will always return false if nothing is added.
            // `add` will set the bit.
            num_bits_ = 1;
            num_hashes_ = 1;
        } else if (false_positive_probability <= 0.0 || false_positive_probability >= 1.0) {
            throw std::invalid_argument("False positive probability must be between 0.0 and 1.0 (exclusive).");
        } else {
            num_bits_ = calculate_optimal_m(expected_items, false_positive_probability);
            if (num_bits_ == 0) num_bits_ = 1; // Ensure at least 1 bit
            num_hashes_ = calculate_optimal_k(expected_items, num_bits_);
            if (num_hashes_ == 0) num_hashes_ = 1; // Ensure at least 1 hash function
        }
        bits_.resize(num_bits_, false);
    }

    /**
     * @brief Adds an item to the Bloom filter.
     *
     * @param item The item to add.
     */
    void add(const T& item) {
        for (size_t i = 0; i < num_hashes_; ++i) {
            bits_[get_hash(item, i) % num_bits_] = true;
        }
        item_count_++;
    }

    /**
     * @brief Checks if an item might be in the Bloom filter.
     *
     * @param item The item to check.
     * @return true if the item might be in the filter (could be a false positive).
     * @return false if the item is definitely not in the filter.
     */
    bool might_contain(const T& item) const {
        if (num_expected_items_ == 0 && item_count_ == 0) { // Empty filter created for 0 expected items
             return false;
        }
        if (bits_.empty()) return false; // Should not happen if constructor is correct

        for (size_t i = 0; i < num_hashes_; ++i) {
            if (!bits_[get_hash(item, i) % num_bits_]) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Returns the current number of items added to the filter.
     * Note: This is the number of `add` operations, not unique items.
     */
    size_t approximate_item_count() const {
        return item_count_;
    }

    /**
     * @brief Returns the size of the bit array.
     */
    size_t bit_array_size() const {
        return num_bits_;
    }

    /**
     * @brief Returns the number of hash functions used.
     */
    size_t number_of_hash_functions() const {
        return num_hashes_;
    }

    /**
     * @brief Returns the configured expected number of items.
     */
    size_t expected_items_capacity() const {
        return num_expected_items_;
    }

    /**
     * @brief Returns the configured false positive probability.
     */
    double configured_fp_probability() const {
        return fp_prob_;
    }


private:
    size_t num_expected_items_;
    double fp_prob_;
    size_t num_bits_;    // m
    size_t num_hashes_;  // k
    std::vector<bool> bits_;
    size_t item_count_ = 0; // Count of items added

    detail::BloomHash<T> hasher_;

    /**
     * @brief Calculates the optimal size of the bit array (m).
     * m = - (n * ln(p)) / (ln(2)^2)
     * where n is the number of items, p is the false positive probability.
     */
    static size_t calculate_optimal_m(size_t n, double p) {
        if (n == 0) return 1; // Avoid log(0) or division by zero if p is extreme
        double m_double = - (static_cast<double>(n) * std::log(p)) / (std::log(2.0) * std::log(2.0));
        // Ensure m_double is not NaN or infinity due to extreme p values.
        if (!std::isfinite(m_double) || m_double <= 0) {
            // Fallback for extreme p (e.g., very close to 0 or 1) or n=0
            // If p is very small, m can be very large. std::numeric_limits<size_t>::max() might be too large for vector.
            // Let's cap it or handle it. For now, if it's non-finite, it means p was probably bad.
            // Or if n is huge.
            // A practical upper limit might be needed if vector allocation is a concern.
            // For now, we assume inputs are reasonable.
            // If result is non-positive, means inputs were tricky. Default to a small size.
            if (m_double <=0) return 1;
        }
        return static_cast<size_t>(std::ceil(m_double));
    }

    /**
     * @brief Calculates the optimal number of hash functions (k).
     * k = (m / n) * ln(2)
     * where m is the size of the bit array, n is the number of items.
     */
    static size_t calculate_optimal_k(size_t n, size_t m) {
        if (n == 0) return 1; // If no items, 1 hash function is fine.
        if (m == 0) return 1; // If no bits, 1 hash function (though filter is useless).
        double k_double = (static_cast<double>(m) / static_cast<double>(n)) * std::log(2.0);
         // Ensure k_double is not NaN or infinity.
        if (!std::isfinite(k_double) || k_double <= 0) {
            return 1; // Default to 1 hash function if calculation is problematic.
        }
        size_t k = static_cast<size_t>(std::ceil(k_double));
        return (k == 0) ? 1 : k; // Ensure at least one hash function
    }

    /**
     * @brief Generates the i-th hash for the item.
     * Uses the seed parameter of BloomHash to generate different hashes.
     */
    size_t get_hash(const T& item, size_t seed_index) const {
        // seed_index is 0, 1, ..., num_hashes_ - 1
        return hasher_(item, seed_index);
    }
};

#endif // BLOOM_FILTER_H
